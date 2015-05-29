// parser.hpp
// Copyright (c) 2005-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_PARSER_HPP
#define LEXERTL_PARSER_HPP

#include <assert.h>
#include <algorithm>
#include "../bool.hpp"
#include "tree/end_node.hpp"
#include "tree/iteration_node.hpp"
#include "tree/leaf_node.hpp"
#include <map>
#include "../containers/ptr_stack.hpp"
#include "tokeniser/re_tokeniser.hpp"
#include "../runtime_error.hpp"
#include "tree/selection_node.hpp"
#include "tree/sequence_node.hpp"
#include "../size_t.hpp"
#include <vector>

namespace lexertl
{
namespace detail
{
/*
    General principles of regex parsing:
    - Every regex is a sequence of sub-regexes.
    - Regexes consist of operands and operators
    - All operators decompose to sequence, selection ('|') and iteration ('*')
    - Regex tokens are stored on a stack.
    - When a complete sequence of regex tokens is on the stack it is processed.

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
    enum {char_24_bit = sm_traits::char_24_bit};
    typedef typename sm_traits::char_type char_type;
    typedef typename sm_traits::id_type id_type;
    typedef basic_end_node<id_type> end_node;
    typedef typename sm_traits::input_char_type input_char_type;
    typedef basic_string_token<input_char_type> input_string_token;
    typedef basic_iteration_node<id_type> iteration_node;
    typedef basic_leaf_node<id_type> leaf_node;
    typedef basic_re_tokeniser<rules_char_type, input_char_type, id_type>
        tokeniser;
    typedef basic_node<id_type> node;
    typedef typename node::node_ptr_vector node_ptr_vector;
    typedef std::basic_string<rules_char_type> string;
    typedef basic_string_token<char_type> string_token;
    typedef basic_selection_node<id_type> selection_node;
    typedef basic_sequence_node<id_type> sequence_node;
    typedef std::map<string_token, std::size_t> charset_map;
    typedef std::pair<string_token, std::size_t> charset_pair;
    typedef bool_<sm_traits::compressed> compressed;
    typedef basic_re_token<rules_char_type, input_char_type> token;
    typedef std::deque<token> token_deque;

    basic_parser(const std::locale &locale_,
        node_ptr_vector &node_ptr_vector_,
        charset_map &charset_map_, const id_type eoi_) :
        _locale(locale_),
        _node_ptr_vector(node_ptr_vector_),
        _charset_map(charset_map_),
        _eoi(eoi_),
        _token_stack(),
        _tree_node_stack()
    {
    }

    node *parse(const token_deque &regex_, const id_type id_,
        const id_type user_id_, const id_type next_dfa_,
        const id_type push_dfa_, const bool pop_dfa_,
        const std::size_t flags_, id_type &nl_id_, const bool seen_bol_)
    {
        typename token_deque::const_iterator iter_ = regex_.begin();
        typename token_deque::const_iterator end_ = regex_.end();
        node *root_ = 0;
        token *lhs_token_ = 0;
        // There cannot be less than 2 tokens
        std::auto_ptr<token> rhs_token_(new token(*iter_++));
        char action_ = 0;

        _token_stack->push(static_cast<token *>(0));
        _token_stack->top() = rhs_token_.release();
        rhs_token_.reset(new token(*iter_));

        if (iter_ + 1 != end_) ++iter_;

        do
        {
            lhs_token_ = _token_stack->top();
            action_ = lhs_token_->precedence(rhs_token_->_type);

            switch (action_)
            {
                case '<':
                case '=':
                    _token_stack->push(static_cast<token *>(0));
                    _token_stack->top() = rhs_token_.release();
                    rhs_token_.reset(new token(*iter_));

                    if (iter_ + 1 != end_) ++iter_;

                    break;
                case '>':
                    reduce(nl_id_);
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
        } while (!_token_stack->empty());

        if (_tree_node_stack.empty())
        {
            std::ostringstream ss_;

            ss_ << "Empty rules are not allowed in rule id " <<
                id_ << '.';
            throw runtime_error(ss_.str());
        }

        assert(_tree_node_stack.size() == 1);

        node *lhs_node_ = _tree_node_stack.top();

        _tree_node_stack.pop();
        _node_ptr_vector->push_back(static_cast<end_node *>(0));

        node *rhs_node_ = new end_node(id_, user_id_, next_dfa_,
            push_dfa_, pop_dfa_);

        _node_ptr_vector->back() = rhs_node_;
        _node_ptr_vector->push_back(static_cast<sequence_node *>(0));
        _node_ptr_vector->back() = new sequence_node
            (lhs_node_, rhs_node_);
        root_ = _node_ptr_vector->back();

        if (seen_bol_)
        {
            fixup_bol(root_);
        }

        if ((flags_ & match_zero_len) == 0)
        {
            const typename node::node_vector &firstpos_ = root_->firstpos();
            typename node::node_vector::const_iterator iter_ =
                firstpos_.begin();
            typename node::node_vector::const_iterator end_ =
                firstpos_.end();

            for (; iter_ != end_; ++iter_)
            {
                const node *node_ = *iter_;

                if (node_->end_state())
                {
                    std::ostringstream ss_;

                    ss_ << "Rules that match zero characters are not allowed "
                        "as this can cause an infinite loop in user code. The "
                        "match_zero_len flag overrides this check. Rule id " <<
                        id_ << '.';
                    throw runtime_error(ss_.str());
                }
            }
        }

        return root_;
    }

    static id_type bol_token()
    {
        return ~static_cast<id_type>(1);
    }

    static id_type eol_token()
    {
        return ~static_cast<id_type>(2);
    }

private:
    typedef typename input_string_token::range input_range;
    typedef typename string_token::range range;
    typedef ptr_vector<string_token> string_token_vector;
    typedef ptr_stack<token> token_stack;
    typedef typename node::node_stack tree_node_stack;

    const std::locale &_locale;
    node_ptr_vector &_node_ptr_vector;
    charset_map &_charset_map;
    id_type _eoi;
    token_stack _token_stack;
    tree_node_stack _tree_node_stack;

    struct find_functor
    {
        // Pointer to stop warning about cannot create assignment operator.
        const string_token *_token;

        find_functor(const string_token &token_) :
            _token(&token_)
        {
        }

        bool operator ()(const string_token *rhs_)
        {
            return *_token == *rhs_;
        }
    };

    void reduce(id_type &nl_id_)
    {
        token *lhs_ = 0;
        token *rhs_ = 0;
        token_stack handle_;
        char action_ = 0;

        do
        {
            rhs_ = _token_stack->top();
            handle_->push(static_cast<token *>(0));
            _token_stack->pop();
            handle_->top() = rhs_;

            if (!_token_stack->empty())
            {
                lhs_ = _token_stack->top();
                action_ = lhs_->precedence(rhs_->_type);
            }
        } while (!_token_stack->empty() && action_ == '=');

        assert(_token_stack->empty() || action_ == '<');

        switch (rhs_->_type)
        {
        case BEGIN:
            // finished processing so exit
            break;
        case REGEX:
            // finished parsing, nothing to do
            break;
        case OREXP:
            orexp(handle_);
            break;
        case SEQUENCE:
            _token_stack->push(static_cast<token *>(0));
            _token_stack->top() = new token(OREXP);
            break;
        case SUB:
            sub(handle_);
            break;
        case EXPRESSION:
            _token_stack->push(static_cast<token *>(0));
            _token_stack->top() = new token(SUB);
            break;
        case REPEAT:
            repeat(handle_);
            break;
        case BOL:
            bol(handle_);
            break;
        case EOL:
            eol(handle_, nl_id_);
            break;
        case CHARSET:
            charset(handle_, compressed());
            break;
        case OPENPAREN:
            openparen(handle_);
            break;
        case OPT:
        case AOPT:
            optional(rhs_->_type == OPT);
            _token_stack->push(static_cast<token *>(0));
            _token_stack->top() = new token(DUP);
            break;
        case ZEROORMORE:
        case AZEROORMORE:
            zero_or_more(rhs_->_type == ZEROORMORE);
            _token_stack->push(static_cast<token *>(0));
            _token_stack->top() = new token(DUP);
            break;
        case ONEORMORE:
        case AONEORMORE:
            one_or_more(rhs_->_type == ONEORMORE);
            _token_stack->push(static_cast<token *>(0));
            _token_stack->top() = new token(DUP);
            break;
        case REPEATN:
        case AREPEATN:
            repeatn(rhs_->_type == REPEATN, handle_->top());
            _token_stack->push(static_cast<token *>(0));
            _token_stack->top() = new token(DUP);
            break;
        default:
            throw runtime_error
                ("Internal error in regex_parser::reduce.");
            break;
        }
    }

    void orexp(token_stack &handle_)
    {
        assert(handle_->top()->_type == OREXP &&
            (handle_->size() == 1 || handle_->size() == 3));

        if (handle_->size() == 1)
        {
            std::auto_ptr<token> token_(new token(REGEX));

            _token_stack->push(static_cast<token *>(0));
            _token_stack->top() = token_.release();
        }
        else
        {
            token *token_ = handle_->top();

            handle_->pop();
            delete token_;
            token_ = 0;
            assert(handle_->top()->_type == OR);
            token_ = handle_->top();
            handle_->pop();
            delete token_;
            token_ = 0;
            assert(handle_->top()->_type == SEQUENCE);
            perform_or();
            _token_stack->push(static_cast<token *>(0));
            _token_stack->top() = new token(OREXP);
        }
    }

    void perform_or()
    {
        // perform or
        node *rhs_ = _tree_node_stack.top();

        _tree_node_stack.pop();

        node *lhs_ = _tree_node_stack.top();

        _node_ptr_vector->push_back(static_cast<selection_node *>(0));
        _node_ptr_vector->back() = new selection_node(lhs_, rhs_);
        _tree_node_stack.top() = _node_ptr_vector->back();
    }

    void sub(token_stack &handle_)
    {
        assert((handle_->top()->_type == SUB &&
            handle_->size() == 1) || handle_->size() == 2);

        if (handle_->size() == 1)
        {
            _token_stack->push(static_cast<token *>(0));
            _token_stack->top() = new token(SEQUENCE);
        }
        else
        {
            token *token_ = handle_->top();

            handle_->pop();
            delete token_;
            token_ = 0;
            assert(handle_->top()->_type == EXPRESSION);
            // perform join
            sequence();
            _token_stack->push(static_cast<token *>(0));
            _token_stack->top() = new token(SUB);
        }
    }

    void repeat(token_stack &handle_)
    {
        assert(handle_->top()->_type == REPEAT &&
            handle_->size() >= 1 && handle_->size() <= 3);

        if (handle_->size() == 1)
        {
            _token_stack->push(static_cast<token *>(0));
            _token_stack->top() = new token(EXPRESSION);
        }
        else
        {
            token *token_ = handle_->top();

            handle_->pop();
            delete token_;
            token_ = 0;
            assert(handle_->top()->_type == DUP);
            _token_stack->push(static_cast<token *>(0));
            _token_stack->top() = new token(REPEAT);
        }
    }

#ifndef NDEBUG
    void bol(token_stack &handle_)
#else
    void bol(token_stack &)
#endif
    {
        assert(handle_->top()->_type == BOL &&
            handle_->size() == 1);

        // store charset
        _node_ptr_vector->push_back(static_cast<leaf_node *>(0));
        _node_ptr_vector->back() = new leaf_node(bol_token(), true);
        _tree_node_stack.push(_node_ptr_vector->back());
        _token_stack->push(static_cast<token *>(0));
        _token_stack->top() = new token(REPEAT);
    }

#ifndef NDEBUG
    void eol(token_stack &handle_, id_type &nl_id_)
#else
    void eol(token_stack &, id_type &nl_id_)
#endif
    {
        const string_token nl_('\n');
        const id_type temp_nl_id_ = lookup(nl_);

        assert(handle_->top()->_type == EOL &&
            handle_->size() == 1);

        if (temp_nl_id_ != ~static_cast<id_type>(0))
        {
            nl_id_ = temp_nl_id_;
        }

        // store charset
        _node_ptr_vector->push_back(static_cast<leaf_node *>(0));
        _node_ptr_vector->back() = new leaf_node(eol_token(), true);
        _tree_node_stack.push(_node_ptr_vector->back());
        _token_stack->push(static_cast<token *>(0));
        _token_stack->top() = new token(REPEAT);
    }

    // Uncompressed
    void charset(token_stack &handle_, const false_ &)
    {
        assert(handle_->top()->_type == CHARSET &&
            handle_->size() == 1);

        const id_type id_ = lookup(handle_->top()->_str);

        // store charset
        _node_ptr_vector->push_back(static_cast<leaf_node *>(0));
        _node_ptr_vector->back() = new leaf_node(id_, true);
        _tree_node_stack.push(_node_ptr_vector->back());
        _token_stack->push(static_cast<token *>(0));
        _token_stack->top() = new token(REPEAT);
    }

    // Compressed
    void charset(token_stack &handle_, const true_ &)
    {
        assert(handle_->top()->_type == CHARSET &&
            handle_->size() == 1);

        std::auto_ptr<token> token_(handle_->top());

        handle_->pop();
        create_sequence(token_);
    }

    // Slice wchar_t into sequence of char.
    void create_sequence(std::auto_ptr<token> &token_)
    {
        typename token::string_token::range_vector::iterator iter_ =
            token_->_str._ranges.begin();
        typename token::string_token::range_vector::const_iterator end_ =
            token_->_str._ranges.end();

        string_token_vector data_[char_24_bit ? 3 : 2];

        for (; iter_ != end_; ++iter_)
        {
            slice_range(*iter_, data_, bool_<char_24_bit>());
        }

        push_ranges(data_, bool_<char_24_bit>());

        _token_stack->push(static_cast<token *>(0));
        _token_stack->top() = new token(OPENPAREN);
        _token_stack->push(static_cast<token *>(0));
        _token_stack->top() = new token(REGEX);
        _token_stack->push(static_cast<token *>(0));
        _token_stack->top() = new token(CLOSEPAREN);
    }

    // 16 bit unicode
    void slice_range(const input_range &range_, string_token_vector data_[2],
        const false_ &)
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
            insert_range(first_msb_, first_msb_, first_lsb_, 0xff, data_);

            if (second_msb_ > first_msb_ + 1)
            {
                insert_range(first_msb_ + 1, second_msb_ - 1, 0, 0xff, data_);
            }

            insert_range(second_msb_, second_msb_, 0, second_lsb_, data_);
        }
    }

    // 24 bit unicode
    void slice_range(const input_range &range_, string_token_vector data_[3],
        const true_ &)
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
            slice_range(range_, data2_, false_());

            for (std::size_t i_ = 0, size_ = data2_[0]->size();
                i_ < size_; ++i_)
            {
                insert_range(string_token(first_msb_, first_msb_),
                    *(*data2_[0])[i_], *(*data2_[1])[i_], data_);
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
    void insert_range(const unsigned char first_, const unsigned char second_,
        const unsigned char first2_, const unsigned char second2_,
        string_token_vector data_[2])
    {
        const string_token token_(first_ > second_ ? second_ : first_,
            first_ > second_ ? first_ : second_);
        const string_token token2_(first2_ > second2_ ? second2_ : first2_,
            first2_ > second2_ ? first2_ : second2_);

        insert_range(token_, token2_, data_);
    }

    void insert_range(const string_token &token_, const string_token &token2_,
        string_token_vector data_[2])
    {
        typename string_token_vector::vector::const_iterator iter_ =
            std::find_if(data_[0]->begin(), data_[0]->end(),
            find_functor(token_));

        if (iter_ == data_[0]->end())
        {
            data_[0]->push_back(0);
            data_[0]->back() = new string_token(token_);
            data_[1]->push_back(0);
            data_[1]->back() = new string_token(token2_);
        }
        else
        {
            const std::size_t index_ = iter_ - data_[0]->begin();

            (*data_[1])[index_]->insert(token2_);
        }
    }

    // 24 bit unicode
    void insert_range(const unsigned char first_, const unsigned char second_,
        const unsigned char first2_, const unsigned char second2_,
        const unsigned char first3_, const unsigned char second3_,
        string_token_vector data_[3])
    {
        const string_token token_(first_ > second_ ? second_ : first_,
            first_ > second_ ? first_ : second_);
        const string_token token2_(first2_ > second2_ ? second2_ : first2_,
            first2_ > second2_ ? first2_ : second2_);
        const string_token token3_(first3_ > second3_ ? second3_ : first3_,
            first3_ > second3_ ? first3_ : second3_);

        insert_range(token_, token2_, token3_, data_);
    }

    void insert_range(const string_token &token_, const string_token &token2_,
        const string_token &token3_, string_token_vector data_[3])
    {
        typename string_token_vector::vector::const_iterator iter_ =
            data_[0]->begin();
        typename string_token_vector::vector::const_iterator end_ =
            data_[0]->end();
        bool finished_ = false;

        do
        {
            iter_ = std::find_if(iter_, end_, find_functor(token_));

            if (iter_ == end_)
            {
                data_[0]->push_back(0);
                data_[0]->back() = new string_token(token_);
                data_[1]->push_back(0);
                data_[1]->back() = new string_token(token2_);
                data_[2]->push_back(0);
                data_[2]->back() = new string_token(token3_);
                finished_ = true;
            }
            else
            {
                const std::size_t index_ = iter_ - data_[0]->begin();

                if (*(*data_[1])[index_] == token2_)
                {
                    (*data_[2])[index_]->insert(token3_);
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
    void push_ranges(string_token_vector data_[2], const false_ &)
    {
        typename string_token_vector::vector::const_iterator viter_ =
            data_[0]->begin();
        typename string_token_vector::vector::const_iterator vend_ =
            data_[0]->end();
        typename string_token_vector::vector::const_iterator viter2_ =
            data_[1]->begin();

        push_range(*viter_++);
        push_range(*viter2_++);
        sequence();

        while (viter_ != vend_)
        {
            push_range(*viter_++);
            push_range(*viter2_++);
            sequence();
            perform_or();
        }
    }

    // 24 bit unicode
    void push_ranges(string_token_vector data_[3], const true_ &)
    {
        typename string_token_vector::vector::const_iterator viter_ =
            data_[0]->begin();
        typename string_token_vector::vector::const_iterator vend_ =
            data_[0]->end();
        typename string_token_vector::vector::const_iterator viter2_ =
            data_[1]->begin();
        typename string_token_vector::vector::const_iterator viter3_ =
            data_[2]->begin();

        push_range(*viter_++);
        push_range(*viter2_++);
        sequence();
        push_range(*viter3_++);
        sequence();

        while (viter_ != vend_)
        {
            push_range(*viter_++);
            push_range(*viter2_++);
            sequence();
            push_range(*viter3_++);
            sequence();
            perform_or();
        }
    }

    void push_range(const string_token *token_)
    {
        const id_type id_ = lookup(*token_);

        _node_ptr_vector->push_back(static_cast<leaf_node *>(0));
        _node_ptr_vector->back() = new leaf_node(id_, true);
        _tree_node_stack.push(_node_ptr_vector->back());
    }

    id_type lookup(const string_token &charset_)
    {
        // Converted to id_type below.
        std::size_t id_ = sm_traits::npos();
        typename charset_map::const_iterator iter_ =
            _charset_map.find(charset_);

        if (iter_ == _charset_map.end())
        {
            id_ = _charset_map.size();
            _charset_map.insert(charset_pair(charset_, id_));
        }
        else
        {
            id_ = iter_->second;
        }

        if (static_cast<id_type>(id_) < id_)
        {
            throw runtime_error("id_type is not large enough "
                "to hold all ids.");
        }

        return static_cast<id_type>(id_);
    }

    void openparen(token_stack &handle_)
    {
        token *token_ = handle_->top();

        assert(token_->_type == OPENPAREN &&
            handle_->size() == 3);

        handle_->pop();
        delete token_;
        token_ = handle_->top();
        assert(token_->_type == REGEX);
        handle_->pop();
        delete token_;
        token_ = 0;
        assert(handle_->top()->_type == CLOSEPAREN);
        _token_stack->push(static_cast<token *>(0));
        _token_stack->top() = new token(REPEAT);
    }

    void sequence()
    {
        node *rhs_ = _tree_node_stack.top();

        _tree_node_stack.pop();

        node *lhs_ = _tree_node_stack.top();

        _node_ptr_vector->push_back(static_cast<sequence_node *>(0));
        _node_ptr_vector->back() = new sequence_node(lhs_, rhs_);
        _tree_node_stack.top() = _node_ptr_vector->back();
    }

    void optional(const bool greedy_)
    {
        // perform ?
        node *lhs_ = _tree_node_stack.top();
        // Don't know if lhs_ is a leaf_node, so get firstpos.
        typename node::node_vector &firstpos_ = lhs_->firstpos();

        for (typename node::node_vector::iterator iter_ = firstpos_.begin(),
            end_ = firstpos_.end(); iter_ != end_; ++iter_)
        {
            // These are leaf_nodes!
            (*iter_)->greedy(greedy_);
        }

        _node_ptr_vector->push_back(static_cast<leaf_node *>(0));

        node *rhs_ = new leaf_node(node::null_token(), greedy_);

        _node_ptr_vector->back() = rhs_;
        _node_ptr_vector->push_back(static_cast<selection_node *>(0));
        _node_ptr_vector->back() = new selection_node(lhs_, rhs_);
        _tree_node_stack.top() = _node_ptr_vector->back();
    }

    void zero_or_more(const bool greedy_)
    {
        // perform *
        node *ptr_ = _tree_node_stack.top();

        _node_ptr_vector->push_back(static_cast<iteration_node *>(0));
        _node_ptr_vector->back() = new iteration_node(ptr_, greedy_);
        _tree_node_stack.top() = _node_ptr_vector->back();
    }

    void one_or_more(const bool greedy_)
    {
        // perform +
        node *lhs_ = _tree_node_stack.top();
        node *copy_ = lhs_->copy(_node_ptr_vector);

        _node_ptr_vector->push_back(static_cast<iteration_node *>(0));

        node *rhs_ = new iteration_node(copy_, greedy_);

        _node_ptr_vector->back() = rhs_;
        _node_ptr_vector->push_back(static_cast<sequence_node *>(0));
        _node_ptr_vector->back() = new sequence_node(lhs_, rhs_);
        _tree_node_stack.top() = _node_ptr_vector->back();
    }

    // perform {n[,[m]]}
    // Semantic checks have already been performed.
    // {0,}  = *
    // {0,1} = ?
    // {1,}  = +
    // therefore we do not check for these cases.
    void repeatn(const bool greedy_, const token *token_)
    {
        const rules_char_type *str_ = token_->_extra.c_str();
        std::size_t min_ = 0;
        bool comma_ = false;
        std::size_t max_ = 0;

        while (*str_>= '0' && *str_ <= '9')
        {
            min_ *= 10;
            min_ += *str_ - '0';
            ++str_;
        }

        comma_ = *str_ == ',';

        if (comma_) ++str_;

        while (*str_>= '0' && *str_ <= '9')
        {
            max_ *= 10;
            max_ += *str_ - '0';
            ++str_;
        }

        if (!(min_ == 1 && !comma_))
        {
            const std::size_t top_ = min_ > 0 ? min_ : max_;

            if (min_ == 0)
            {
                optional(greedy_);
            }

            node *prev_ = _tree_node_stack.top()->
                copy(_node_ptr_vector);
            node *curr_ = 0;

            for (std::size_t i_ = 2; i_ < top_; ++i_)
            {
                node *temp_ = prev_->copy(_node_ptr_vector);

                curr_ = temp_;
                _tree_node_stack.push(static_cast<node *>(0));
                _tree_node_stack.top() = prev_;
                sequence();
                prev_ = curr_;
            }

            if (comma_ && min_ > 0)
            {
                if (min_ > 1)
                {
                    node *temp_ = prev_->copy(_node_ptr_vector);

                    curr_ = temp_;
                    _tree_node_stack.push(static_cast<node *>(0));
                    _tree_node_stack.top() = prev_;
                    sequence();
                    prev_ = curr_;
                }

                if (comma_ && max_)
                {
                    _tree_node_stack.push(static_cast<node *>(0));
                    _tree_node_stack.top() = prev_;
                    optional(greedy_);

                    node *temp_ = _tree_node_stack.top();

                    _tree_node_stack.pop();
                    prev_ = temp_;

                    const std::size_t count_ = max_ - min_;

                    for (std::size_t i_ = 1; i_ < count_; ++i_)
                    {
                        node *temp_ = prev_->copy(_node_ptr_vector);

                        curr_ = temp_;
                        _tree_node_stack.push(static_cast<node *>(0));
                        _tree_node_stack.top() = prev_;
                        sequence();
                        prev_ = curr_;
                    }
                }
                else
                {
                    _tree_node_stack.push(static_cast<node *>(0));
                    _tree_node_stack.top() = prev_;
                    zero_or_more(greedy_);

                    node *temp_ = _tree_node_stack.top();

                    prev_ = temp_;
                    _tree_node_stack.pop();
                }
            }

            _tree_node_stack.push(static_cast<node *>(0));
            _tree_node_stack.top() = prev_;
            sequence();
        }
    }

    void fixup_bol(node * &root_)const
    {
        typename node::node_vector *first_ = &root_->firstpos();
        bool found_ = false;
        typename node::node_vector::const_iterator iter_ =
            first_->begin();
        typename node::node_vector::const_iterator end_ =
            first_->end();

        for (; iter_ != end_; ++iter_)
        {
            const node *node_ = *iter_;

            found_ = !node_->end_state() && node_->token() == bol_token();

            if (found_) break;
        }

        if (!found_)
        {
            _node_ptr_vector->push_back(static_cast<leaf_node *>(0));
            _node_ptr_vector->back() = new leaf_node(bol_token(), true);

            node *lhs_ = _node_ptr_vector->back();

            _node_ptr_vector->push_back(static_cast<leaf_node *>(0));
            _node_ptr_vector->back() = new leaf_node
                (node::null_token(), true);

            node *rhs_ = _node_ptr_vector->back();

            _node_ptr_vector->push_back(static_cast<selection_node *>(0));
            _node_ptr_vector->back() = new selection_node(lhs_, rhs_);
            lhs_ = _node_ptr_vector->back();

            _node_ptr_vector->push_back(static_cast<sequence_node *>(0));
            _node_ptr_vector->back() = new sequence_node(lhs_, root_);
            root_ = _node_ptr_vector->back();
        }
    }
};
}
}

#endif
