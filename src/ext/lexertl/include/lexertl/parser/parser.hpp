// parser.hpp
// Copyright (c) 2005-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_PARSER_HPP
#define LEXERTL_PARSER_HPP

#include <cassert>
#include <algorithm>
#include "tree/end_node.hpp"
#include "tree/iteration_node.hpp"
#include "tree/leaf_node.hpp"
#include <map>
#include "tokeniser/re_tokeniser.hpp"
#include "../runtime_error.hpp"
#include "tree/selection_node.hpp"
#include "tree/sequence_node.hpp"
#include <type_traits>
#include <vector>

namespace lexertl
{
    namespace detail
    {
        /*
            General principles of regex parsing:
            - Every regex is a sequence of sub-regexes.
            - Regexes consist of operands and operators
            - All operators decompose to sequence,
              selection ('|') and iteration ('*')
            - Regex tokens are stored on a stack.
            - When a complete sequence of regex tokens
              is on the stack it is processed.

        Grammar:

        <REGEX>      -> <OREXP>
        <OREXP>      -> <SEQUENCE> | <OREXP>'|'<SEQUENCE>
        <SEQUENCE>   -> <SUB>
        <SUB>        -> <EXPRESSION> | <SUB><EXPRESSION>
        <EXPRESSION> -> <REPEAT>
        <REPEAT>     -> charset | macro | '('<REGEX>')' | <REPEAT><DUPLICATE>
        <DUPLICATE>  -> '?' | '??' | '*' | '*?' | '+' | '+?' | '{n[,[m]]}' |
                        '{n[,[m]]}?'
        */

        template<typename rules_char_type, typename sm_traits>
        class basic_parser
        {
        public:
            enum { char_24_bit = sm_traits::char_24_bit };
            using char_type = typename sm_traits::char_type;
            using id_type = typename sm_traits::id_type;
            using end_node = basic_end_node<id_type>;
            using input_char_type = typename sm_traits::input_char_type;
            using input_string_token = basic_string_token<input_char_type>;
            using iteration_node = basic_iteration_node<id_type>;
            using leaf_node = basic_leaf_node<id_type>;
            using tokeniser =
                basic_re_tokeniser<rules_char_type, input_char_type, id_type>;
            using node = basic_node<id_type>;
            using node_ptr_vector = typename node::node_ptr_vector;
            using string = std::basic_string<rules_char_type>;
            using string_token = basic_string_token<char_type>;
            using selection_node = basic_selection_node<id_type>;
            using sequence_node = basic_sequence_node<id_type>;
            using charset_map = std::map<string_token, id_type>;
            using charset_pair = std::pair<string_token, id_type>;
            using compressed =
                std::integral_constant<bool, sm_traits::compressed>;
            using token = basic_re_token<rules_char_type, input_char_type>;
            static_assert(std::is_move_assignable<token>::value&&
                std::is_move_constructible<token>::value,
                "token is not movable.");
            using token_vector = std::vector<token>;

            basic_parser(const std::locale& locale_,
                node_ptr_vector& node_ptr_vector_,
                charset_map& charset_map_, const id_type eoi_) :
                _locale(locale_),
                _node_ptr_vector(node_ptr_vector_),
                _charset_map(charset_map_),
                _eoi(eoi_),
                _token_stack(),
                _tree_node_stack()
            {
            }

            observer_ptr<node> parse(const token_vector& regex_,
                const id_type id_, const id_type user_id_,
                const id_type next_dfa_, const id_type push_dfa_,
                const bool pop_dfa_, const std::size_t flags_,
                id_type& cr_id_, id_type& nl_id_, const bool seen_bol_)
            {
                auto iter_ = regex_.cbegin();
                auto end_ = regex_.cend();
                observer_ptr<node> root_ = nullptr;
                observer_ptr<token> lhs_token_ = nullptr;
                // There cannot be less than 2 tokens
                auto rhs_token_ = std::make_unique<token>(*iter_++);
                char action_ = 0;

                _token_stack.emplace(std::move(rhs_token_));
                rhs_token_ = std::make_unique<token>(*iter_);

                if (iter_ + 1 != end_) ++iter_;

                do
                {
                    lhs_token_ = _token_stack.top().get();
                    action_ = lhs_token_->precedence(rhs_token_->_type);

                    switch (action_)
                    {
                    case '<':
                    case '=':
                        _token_stack.emplace(std::move(rhs_token_));
                        rhs_token_ = std::make_unique<token>(*iter_);

                        if (iter_ + 1 != end_) ++iter_;

                        break;
                    case '>':
                        reduce(cr_id_, nl_id_);
                        break;
                    default:
                    {
                        std::ostringstream ss_;

                        ss_ << "A syntax error occurred: '" <<
                            lhs_token_->precedence_string() <<
                            "' against '" << rhs_token_->precedence_string() <<
                            " in rule id " << id_ << '.';
                        throw runtime_error(ss_.str());
                        break;
                    }
                    }
                } while (!_token_stack.empty());

                if (_tree_node_stack.empty())
                {
                    std::ostringstream ss_;

                    ss_ << "Empty rules are not allowed in rule id " <<
                        id_ << '.';
                    throw runtime_error(ss_.str());
                }

                assert(_tree_node_stack.size() == 1);

                observer_ptr<node> lhs_node_ = _tree_node_stack.top();

                _tree_node_stack.pop();
                _node_ptr_vector.emplace_back(std::make_unique<end_node>
                    (id_, user_id_, next_dfa_, push_dfa_, pop_dfa_));

                observer_ptr<node> rhs_node_ = _node_ptr_vector.back().get();

                _node_ptr_vector.emplace_back(std::make_unique<sequence_node>
                    (lhs_node_, rhs_node_));
                root_ = _node_ptr_vector.back().get();

                if (seen_bol_)
                {
                    fixup_bol(root_);
                }

                if ((flags_ & match_zero_len) == 0)
                {
                    const auto& firstpos_ = root_->firstpos();

                    for (observer_ptr<const node> node_ : firstpos_)
                    {
                        if (node_->end_state())
                        {
                            std::ostringstream ss_;

                            ss_ << "Rules that match zero characters are not "
                                "allowed as this can cause an infinite loop "
                                "in user code. The match_zero_len flag "
                                "overrides this check. Rule id " <<
                                id_ << '.';
                            throw runtime_error(ss_.str());
                        }
                    }
                }

                return root_;
            }

            static id_type bol_token()
            {
                return static_cast<id_type>(~1);
            }

            static id_type eol_token()
            {
                return static_cast<id_type>(~2);
            }

        private:
            using input_range = typename input_string_token::range;
            using range = typename string_token::range;
            using string_token_vector =
                std::vector<std::unique_ptr<string_token>>;
            using token_stack = std::stack<std::unique_ptr<token>>;
            using tree_node_stack = typename node::node_stack;

            const std::locale& _locale;
            node_ptr_vector& _node_ptr_vector;
            charset_map& _charset_map;
            id_type _eoi;
            token_stack _token_stack;
            tree_node_stack _tree_node_stack;

            void reduce(id_type& cr_id_, id_type& nl_id_)
            {
                observer_ptr<token> lhs_ = nullptr;
                observer_ptr<token> rhs_ = nullptr;
                token_stack handle_;
                char action_ = 0;

                do
                {
                    handle_.emplace();
                    rhs_ = _token_stack.top().release();
                    handle_.top().reset(rhs_);
                    _token_stack.pop();

                    if (!_token_stack.empty())
                    {
                        lhs_ = _token_stack.top().get();
                        action_ = lhs_->precedence(rhs_->_type);
                    }
                } while (!_token_stack.empty() && action_ == '=');

                assert(_token_stack.empty() || action_ == '<');

                switch (rhs_->_type)
                {
                case token_type::BEGIN:
                    // finished processing so exit
                    break;
                case token_type::REGEX:
                    // finished parsing, nothing to do
                    break;
                case token_type::OREXP:
                    orexp(handle_);
                    break;
                case token_type::SEQUENCE:
                    _token_stack.emplace(std::make_unique<token>
                        (token_type::OREXP));
                    break;
                case token_type::SUB:
                    sub(handle_);
                    break;
                case token_type::EXPRESSION:
                    _token_stack.emplace(std::make_unique<token>
                        (token_type::SUB));
                    break;
                case token_type::REPEAT:
                    repeat(handle_);
                    break;
                case token_type::BOL:
                    bol(handle_);
                    break;
                case token_type::EOL:
                    eol(handle_, cr_id_, nl_id_);
                    break;
                case token_type::CHARSET:
                    charset(handle_, compressed());
                    break;
                case token_type::OPENPAREN:
                    openparen(handle_);
                    break;
                case token_type::OPT:
                case token_type::AOPT:
                    optional(rhs_->_type == token_type::OPT);
                    _token_stack.emplace(std::make_unique<token>
                        (token_type::DUP));
                    break;
                case token_type::ZEROORMORE:
                case token_type::AZEROORMORE:
                    zero_or_more(rhs_->_type == token_type::ZEROORMORE);
                    _token_stack.emplace(std::make_unique<token>
                        (token_type::DUP));
                    break;
                case token_type::ONEORMORE:
                case token_type::AONEORMORE:
                    one_or_more(rhs_->_type == token_type::ONEORMORE);
                    _token_stack.emplace(std::make_unique<token>
                        (token_type::DUP));
                    break;
                case token_type::REPEATN:
                case token_type::AREPEATN:
                    repeatn(rhs_->_type == token_type::REPEATN,
                        handle_.top().get());
                    _token_stack.emplace(std::make_unique<token>
                        (token_type::DUP));
                    break;
                default:
                    throw runtime_error
                    ("Internal error in regex_parser::reduce.");
                    break;
                }
            }

            void orexp(token_stack& handle_)
            {
                assert(handle_.top()->_type == token_type::OREXP &&
                    (handle_.size() == 1 || handle_.size() == 3));

                if (handle_.size() == 1)
                {
                    _token_stack.emplace(std::make_unique<token>
                        (token_type::REGEX));
                }
                else
                {
                    handle_.pop();
                    assert(handle_.top()->_type == token_type::OR);
                    handle_.pop();
                    assert(handle_.top()->_type == token_type::SEQUENCE);
                    perform_or();
                    _token_stack.emplace(std::make_unique<token>
                        (token_type::OREXP));
                }
            }

            void perform_or()
            {
                // perform or
                observer_ptr<node> rhs_ = _tree_node_stack.top();

                _tree_node_stack.pop();

                observer_ptr<node> lhs_ = _tree_node_stack.top();

                _node_ptr_vector.emplace_back
                (std::make_unique<selection_node>(lhs_, rhs_));
                _tree_node_stack.top() = _node_ptr_vector.back().get();
            }

            void sub(token_stack& handle_)
            {
                assert((handle_.top()->_type == token_type::SUB &&
                    handle_.size() == 1) || handle_.size() == 2);

                if (handle_.size() == 1)
                {
                    _token_stack.emplace(std::make_unique<token>
                        (token_type::SEQUENCE));
                }
                else
                {
                    handle_.pop();
                    assert(handle_.top()->_type == token_type::EXPRESSION);
                    // perform join
                    sequence();
                    _token_stack.emplace(std::make_unique<token>
                        (token_type::SUB));
                }
            }

            void repeat(token_stack& handle_)
            {
                assert(handle_.top()->_type == token_type::REPEAT &&
                    handle_.size() >= 1 && handle_.size() <= 3);

                if (handle_.size() == 1)
                {
                    _token_stack.emplace(std::make_unique<token>
                        (token_type::EXPRESSION));
                }
                else
                {
                    handle_.pop();
                    assert(handle_.top()->_type == token_type::DUP);
                    _token_stack.emplace(std::make_unique<token>
                        (token_type::REPEAT));
                }
            }

#ifndef NDEBUG
            void bol(token_stack& handle_)
#else
            void bol(token_stack&)
#endif
            {
                assert(handle_.top()->_type == token_type::BOL &&
                    handle_.size() == 1);

                // store charset
                _node_ptr_vector.emplace_back
                (std::make_unique<leaf_node>(bol_token(), true));
                _tree_node_stack.push(_node_ptr_vector.back().get());
                _token_stack.emplace(std::make_unique<token>
                    (token_type::REPEAT));
            }

#ifndef NDEBUG
            void eol(token_stack& handle_, id_type& cr_id_, id_type& nl_id_)
#else
            void eol(token_stack&, id_type& cr_id_, id_type& nl_id_)
#endif
            {
                const string_token cr_('\r');
                const string_token nl_('\n');
                const id_type temp_cr_id_ = lookup(cr_);
                const id_type temp_nl_id_ = lookup(nl_);

                assert(handle_.top()->_type == token_type::EOL &&
                    handle_.size() == 1);

                if (temp_cr_id_ != sm_traits::npos())
                {
                    cr_id_ = temp_cr_id_;
                }

                if (temp_nl_id_ != sm_traits::npos())
                {
                    nl_id_ = temp_nl_id_;
                }

                // store charset
                _node_ptr_vector.emplace_back
                (std::make_unique<leaf_node>(eol_token(), true));
                _tree_node_stack.push(_node_ptr_vector.back().get());
                _token_stack.emplace(std::make_unique<token>
                    (token_type::REPEAT));
            }

            // Uncompressed
            void charset(token_stack& handle_, const std::false_type&)
            {
                assert(handle_.top()->_type == token_type::CHARSET &&
                    handle_.size() == 1);

                const id_type id_ = lookup(handle_.top()->_str);

                // store charset
                _node_ptr_vector.
                    emplace_back(std::make_unique<leaf_node>(id_, true));
                _tree_node_stack.push(_node_ptr_vector.back().get());
                _token_stack.emplace(std::make_unique<token>
                    (token_type::REPEAT));
            }

            // Compressed
            void charset(token_stack& handle_, const std::true_type&)
            {
                assert(handle_.top()->_type == token_type::CHARSET &&
                    handle_.size() == 1);

                std::unique_ptr<token> token_(handle_.top().release());

                handle_.pop();
                create_sequence(token_);
            }

            // Slice wchar_t into sequence of char.
            void create_sequence(std::unique_ptr<token>& token_)
            {
                string_token_vector data_[char_24_bit ? 3 : 2];

                for (const input_range& range_ : token_->_str._ranges)
                {
                    slice_range(range_, data_,
                        std::integral_constant<bool, char_24_bit>());
                }

                push_ranges(data_, std::integral_constant<bool, char_24_bit>());

                _token_stack.emplace(std::make_unique<token>
                    (token_type::OPENPAREN));
                _token_stack.emplace(std::make_unique<token>
                    (token_type::REGEX));
                _token_stack.emplace(std::make_unique<token>
                    (token_type::CLOSEPAREN));
            }

            // 16 bit unicode
            void slice_range(const input_range& range_,
                string_token_vector data_[2], const std::false_type&)
            {
                const unsigned char first_msb_ = static_cast<unsigned char>
                    ((range_.first >> 8) & 0xff);
                const unsigned char first_lsb_ = static_cast<unsigned char>
                    (range_.first & 0xff);
                const unsigned char second_msb_ = static_cast<unsigned char>
                    ((range_.second >> 8) & 0xff);
                const unsigned char second_lsb_ = static_cast<unsigned char>
                    (range_.second & 0xff);

                if (first_msb_ == second_msb_)
                {
                    insert_range(first_msb_, first_msb_, first_lsb_,
                        second_lsb_, data_);
                }
                else
                {
                    insert_range(first_msb_, first_msb_, first_lsb_, 0xff,
                        data_);

                    if (second_msb_ > first_msb_ + 1)
                    {
                        insert_range(first_msb_ + 1, second_msb_ - 1, 0, 0xff,
                            data_);
                    }

                    insert_range(second_msb_, second_msb_, 0, second_lsb_,
                        data_);
                }
            }

            // 24 bit unicode
            void slice_range(const input_range& range_,
                string_token_vector data_[3], const std::true_type&)
            {
                const unsigned char first_msb_ = static_cast<unsigned char>
                    ((range_.first >> 16) & 0xff);
                const unsigned char first_mid_ = static_cast<unsigned char>
                    ((range_.first >> 8) & 0xff);
                const unsigned char first_lsb_ = static_cast<unsigned char>
                    (range_.first & 0xff);
                const unsigned char second_msb_ = static_cast<unsigned char>
                    ((range_.second >> 16) & 0xff);
                const unsigned char second_mid_ = static_cast<unsigned char>
                    ((range_.second >> 8) & 0xff);
                const unsigned char second_lsb_ = static_cast<unsigned char>
                    (range_.second & 0xff);

                if (first_msb_ == second_msb_)
                {
                    string_token_vector data2_[2];

                    // Re-use 16 bit slice function
                    slice_range(range_, data2_, std::false_type());

                    for (std::size_t i_ = 0, size_ = data2_[0].size();
                        i_ < size_; ++i_)
                    {
                        insert_range(string_token(first_msb_, first_msb_),
                            *data2_[0][i_], *data2_[1][i_], data_);
                    }
                }
                else
                {
                    insert_range(first_msb_, first_msb_,
                        first_mid_, first_mid_,
                        first_lsb_, 0xff, data_);

                    if (first_mid_ != 0xff)
                    {
                        insert_range(first_msb_, first_msb_,
                            first_mid_ + 1, 0xff,
                            0, 0xff, data_);
                    }

                    if (second_msb_ > first_msb_ + 1)
                    {
                        insert_range(first_mid_ + 1, second_mid_ - 1,
                            0, 0xff,
                            0, 0xff, data_);
                    }

                    if (second_mid_ != 0)
                    {
                        insert_range(second_msb_, second_msb_,
                            0, second_mid_ - 1,
                            0, 0xff, data_);
                        insert_range(second_msb_, second_msb_,
                            second_mid_, second_mid_,
                            0, second_lsb_, data_);
                    }
                    else
                    {
                        insert_range(second_msb_, second_msb_,
                            0, second_mid_,
                            0, second_lsb_, data_);
                    }
                }
            }

            // 16 bit unicode
            void insert_range(const unsigned char first_,
                const unsigned char second_, const unsigned char first2_,
                const unsigned char second2_, string_token_vector data_[2])
            {
                const string_token token_(first_ > second_ ? second_ : first_,
                    first_ > second_ ? first_ : second_);
                const string_token token2_(first2_ > second2_ ?
                    second2_ : first2_,
                    first2_ > second2_ ? first2_ : second2_);

                insert_range(token_, token2_, data_);
            }

            void insert_range(const string_token& token_,
                const string_token& token2_, string_token_vector data_[2])
            {
                typename string_token_vector::const_iterator iter_ =
                    std::find_if(data_[0].begin(), data_[0].end(),
                        [&token_](const std::unique_ptr<string_token>& rhs_)
                        {
                            return token_ == *rhs_.get();
                        });

                if (iter_ == data_[0].end())
                {
                    data_[0].emplace_back(std::make_unique
                        <string_token>(token_));
                    data_[1].emplace_back(std::make_unique
                        <string_token>(token2_));
                }
                else
                {
                    const std::size_t index_ = iter_ - data_[0].begin();

                    data_[1][index_]->insert(token2_);
                }
            }

            // 24 bit unicode
            void insert_range(const unsigned char first_,
                const unsigned char second_, const unsigned char first2_,
                const unsigned char second2_, const unsigned char first3_,
                const unsigned char second3_, string_token_vector data_[3])
            {
                const string_token token_(first_ > second_ ? second_ : first_,
                    first_ > second_ ? first_ : second_);
                const string_token token2_(first2_ > second2_ ?
                    second2_ : first2_,
                    first2_ > second2_ ? first2_ : second2_);
                const string_token token3_(first3_ > second3_ ?
                    second3_ : first3_,
                    first3_ > second3_ ? first3_ : second3_);

                insert_range(token_, token2_, token3_, data_);
            }

            void insert_range(const string_token& token_,
                const string_token& token2_, const string_token& token3_,
                string_token_vector data_[3])
            {
                auto iter_ = data_[0].cbegin();
                auto end_ = data_[0].cend();
                bool finished_ = false;

                do
                {
                    iter_ = std::find_if(iter_, end_,
                        [&token_](const std::unique_ptr<string_token>& rhs_)
                        {
                            return token_ == *rhs_.get();
                        });

                    if (iter_ == end_)
                    {
                        data_[0].emplace_back(std::make_unique
                            <string_token>(token_));
                        data_[1].emplace_back(std::make_unique
                            <string_token>(token2_));
                        data_[2].emplace_back(std::make_unique
                            <string_token>(token3_));
                        finished_ = true;
                    }
                    else
                    {
                        const std::size_t index_ = iter_ - data_[0].begin();

                        if (*data_[1][index_] == token2_)
                        {
                            data_[2][index_]->insert(token3_);
                            finished_ = true;
                        }
                        else
                        {
                            ++iter_;
                        }
                    }
                } while (!finished_);
            }

            // 16 bit unicode
            void push_ranges(string_token_vector data_[2],
                const std::false_type&)
            {
                auto viter_ = data_[0].cbegin();
                auto vend_ = data_[0].cend();
                auto viter2_ = data_[1].cbegin();

                push_range(viter_++->get());
                push_range(viter2_++->get());
                sequence();

                while (viter_ != vend_)
                {
                    push_range(viter_++->get());
                    push_range(viter2_++->get());
                    sequence();
                    perform_or();
                }
            }

            // 24 bit unicode
            void push_ranges(string_token_vector data_[3],
                const std::true_type&)
            {
                auto viter_ = data_[0].cbegin();
                auto vend_ = data_[0].cend();
                auto viter2_ = data_[1].cbegin();
                auto viter3_ = data_[2].cbegin();

                push_range(viter_++->get());
                push_range(viter2_++->get());
                sequence();
                push_range(viter3_++->get());
                sequence();

                while (viter_ != vend_)
                {
                    push_range(viter_++->get());
                    push_range(viter2_++->get());
                    sequence();
                    push_range(viter3_++->get());
                    sequence();
                    perform_or();
                }
            }

            void push_range(observer_ptr<const string_token> token_)
            {
                const id_type id_ = lookup(*token_);

                _node_ptr_vector.emplace_back(std::make_unique
                    <leaf_node>(id_, true));
                _tree_node_stack.push(_node_ptr_vector.back().get());
            }

            id_type lookup(const string_token& charset_)
            {
                // Converted to id_type below.
                std::size_t id_ = sm_traits::npos();

                if (static_cast<id_type>(id_) < id_)
                {
                    throw runtime_error("id_type is not large enough "
                        "to hold all ids.");
                }

                typename charset_map::const_iterator iter_ =
                    _charset_map.find(charset_);

                if (iter_ == _charset_map.end())
                {
                    id_ = _charset_map.size();
                    _charset_map.insert(charset_pair(charset_,
                        static_cast<id_type>(id_)));
                }
                else
                {
                    id_ = iter_->second;
                }

                return static_cast<id_type>(id_);
            }

            void openparen(token_stack& handle_)
            {
                assert(handle_.top()->_type == token_type::OPENPAREN &&
                    handle_.size() == 3);

                handle_.pop();
                assert(handle_.top()->_type == token_type::REGEX);
                handle_.pop();
                assert(handle_.top()->_type == token_type::CLOSEPAREN);
                _token_stack.emplace(std::make_unique<token>
                    (token_type::REPEAT));
            }

            void sequence()
            {
                observer_ptr<node> rhs_ = _tree_node_stack.top();

                _tree_node_stack.pop();

                observer_ptr<node> lhs_ = _tree_node_stack.top();

                _node_ptr_vector.emplace_back
                (std::make_unique<sequence_node>(lhs_, rhs_));
                _tree_node_stack.top() = _node_ptr_vector.back().get();
            }

            void optional(const bool greedy_)
            {
                // perform ?
                observer_ptr<node> lhs_ = _tree_node_stack.top();
                // Don't know if lhs_ is a leaf_node, so get firstpos.
                auto& firstpos_ = lhs_->firstpos();

                for (observer_ptr<node> node_ : firstpos_)
                {
                    // These are leaf_nodes!
                    node_->greedy(greedy_);
                }

                _node_ptr_vector.emplace_back(std::make_unique<leaf_node>
                    (node::null_token(), greedy_));

                observer_ptr<node> rhs_ = _node_ptr_vector.back().get();

                _node_ptr_vector.emplace_back
                (std::make_unique<selection_node>(lhs_, rhs_));
                _tree_node_stack.top() = _node_ptr_vector.back().get();
            }

            void zero_or_more(const bool greedy_)
            {
                // perform *
                observer_ptr<node> ptr_ = _tree_node_stack.top();

                _node_ptr_vector.emplace_back
                (std::make_unique<iteration_node>(ptr_, greedy_));
                _tree_node_stack.top() = _node_ptr_vector.back().get();
            }

            void one_or_more(const bool greedy_)
            {
                // perform +
                observer_ptr<node> lhs_ = _tree_node_stack.top();
                observer_ptr<node> copy_ = lhs_->copy(_node_ptr_vector);

                _node_ptr_vector.emplace_back(std::make_unique<iteration_node>
                    (copy_, greedy_));

                observer_ptr<node> rhs_ = _node_ptr_vector.back().get();

                _node_ptr_vector.emplace_back
                (std::make_unique<sequence_node>(lhs_, rhs_));
                _tree_node_stack.top() = _node_ptr_vector.back().get();
            }

            // perform {n[,[m]]}
            // Semantic checks have already been performed.
            // {0,}  = *
            // {0,1} = ?
            // {1,}  = +
            // therefore we do not check for these cases.
            void repeatn(const bool greedy_, observer_ptr<const token> token_)
            {
                const rules_char_type* str_ = token_->_extra.c_str();
                std::size_t min_ = 0;
                bool comma_ = false;
                std::size_t max_ = 0;

                while (*str_ >= '0' && *str_ <= '9')
                {
                    min_ *= 10;
                    min_ += static_cast<std::size_t>(*str_) - '0';
                    ++str_;
                }

                comma_ = *str_ == ',';

                if (comma_) ++str_;

                while (*str_ >= '0' && *str_ <= '9')
                {
                    max_ *= 10;
                    max_ += static_cast<std::size_t>(*str_) - '0';
                    ++str_;
                }

                if (!(min_ == 1 && !comma_))
                {
                    const std::size_t top_ = min_ > 0 ? min_ : max_;

                    if (min_ == 0)
                    {
                        optional(greedy_);
                    }

                    observer_ptr<node> prev_ = _tree_node_stack.top()->
                        copy(_node_ptr_vector);
                    observer_ptr<node> curr_ = nullptr;

                    for (std::size_t i_ = 2; i_ < top_; ++i_)
                    {
                        curr_ = prev_->copy(_node_ptr_vector);
                        _tree_node_stack.push(prev_);
                        sequence();
                        prev_ = curr_;
                    }

                    if (comma_ && min_ > 0)
                    {
                        if (min_ > 1)
                        {
                            curr_ = prev_->copy(_node_ptr_vector);
                            _tree_node_stack.push(prev_);
                            sequence();
                            prev_ = curr_;
                        }

                        if (comma_ && max_)
                        {
                            _tree_node_stack.push(prev_);
                            optional(greedy_);
                            prev_ = _tree_node_stack.top();
                            _tree_node_stack.pop();

                            const std::size_t count_ = max_ - min_;

                            for (std::size_t i_ = 1; i_ < count_; ++i_)
                            {
                                curr_ = prev_->copy(_node_ptr_vector);
                                _tree_node_stack.push(prev_);
                                sequence();
                                prev_ = curr_;
                            }
                        }
                        else
                        {
                            _tree_node_stack.push(prev_);
                            zero_or_more(greedy_);
                            prev_ = _tree_node_stack.top();
                            _tree_node_stack.pop();
                        }
                    }

                    _tree_node_stack.push(prev_);
                    sequence();
                }
            }

            void fixup_bol(observer_ptr<node>& root_)const
            {
                const auto& first_ = root_->firstpos();
                bool found_ = false;

                for (observer_ptr<const node> node_ : first_)
                {
                    found_ = !node_->end_state() && node_->token() ==
                        bol_token();

                    if (found_) break;
                }

                if (!found_)
                {
                    _node_ptr_vector.emplace_back
                    (std::make_unique<leaf_node>(bol_token(), true));

                    observer_ptr<node> lhs_ = _node_ptr_vector.back().get();

                    _node_ptr_vector.emplace_back
                    (std::make_unique<leaf_node>(node::null_token(), true));

                    observer_ptr<node> rhs_ = _node_ptr_vector.back().get();

                    _node_ptr_vector.emplace_back
                    (std::make_unique<selection_node>(lhs_, rhs_));
                    lhs_ = _node_ptr_vector.back().get();

                    _node_ptr_vector.emplace_back
                    (std::make_unique<sequence_node>(lhs_, root_));
                    root_ = _node_ptr_vector.back().get();
                }
            }
        };
    }
}

#endif
