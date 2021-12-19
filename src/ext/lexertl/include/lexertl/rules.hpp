// rules.hpp
// Copyright (c) 2005-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_RULES_HPP
#define LEXERTL_RULES_HPP

#include "enums.hpp"
#include <locale>
#include <map>
#include "narrow.hpp"
#include "observer_ptr.hpp"
#include "parser/tokeniser/re_tokeniser.hpp"
#include "runtime_error.hpp"
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace lexertl
{
    template<typename r_ch_type, typename ch_type,
        typename id_ty = uint16_t>
        class basic_rules
    {
    public:
        using bool_vector = std::vector<bool>;
        using bool_vector_vector = std::vector<bool_vector>;
        using char_type = ch_type;
        using rules_char_type = r_ch_type;
        using id_type = id_ty;
        using id_vector = std::vector<id_type>;
        using id_vector_vector = std::vector<id_vector>;
        using re_state =
            detail::basic_re_tokeniser_state<rules_char_type, id_type>;
        using string = std::basic_string<rules_char_type>;
        using string_token = basic_string_token<char_type>;
        using string_vector = std::vector<string>;
        using string_set = std::set<string>;
        using string_pair = std::pair<string, string>;
        using string_id_type_map = std::map<string, id_type>;
        using string_id_type_pair = std::pair<string, id_type>;
        using token = detail::basic_re_token<rules_char_type, char_type>;
        using token_vector = std::vector<token>;
        using token_vector_vector = std::vector<token_vector>;
        using token_vector_vector_vector = std::vector<token_vector_vector>;
        using macro_map = std::map<string, token_vector>;
        using macro_pair = std::pair<string, token_vector>;
        using tokeniser =
            detail::basic_re_tokeniser<rules_char_type, char_type, id_type>;

        // If you get a compile error here you have
        // failed to define an unsigned id type.
        static_assert(std::is_unsigned<id_type>::value,
            "Your id type is signed");

#ifdef _WIN32
        basic_rules(const std::size_t flags_ = dot_not_cr_lf) :
#else
        basic_rules(const std::size_t flags_ = dot_not_newline) :
#endif
            _statemap(),
            _macro_map(),
            _regexes(),
            _features(),
            _ids(),
            _user_ids(),
            _next_dfas(),
            _pushes(),
            _pops(),
            _flags(flags_),
            _locale(),
            _lexer_state_names()
        {
            push_state(initial());
        }

        void clear()
        {
            _statemap.clear();
            _macro_map.clear();
            _regexes.clear();
            _features.clear();
            _ids.clear();
            _user_ids.clear();
            _next_dfas.clear();
            _pushes.clear();
            _pops.clear();
#ifdef _WIN32
            _flags = dot_not_cr_lf;
#else
            _flags = dot_not_newline;
#endif
            _locale = std::locale();
            _lexer_state_names.clear();
            push_state(initial());
        }

        void clear(const id_type dfa_)
        {
            if (_regexes.size() > dfa_)
            {
                _regexes[dfa_].clear();
                _features[dfa_] = 0;
                _ids[dfa_].clear();
                _user_ids[dfa_].clear();
                _next_dfas[dfa_].clear();
                _pushes[dfa_].clear();
                _pops[dfa_].clear();
            }
        }

        void flags(const std::size_t flags_)
        {
            _flags = flags_;
        }

        std::size_t flags() const
        {
            return _flags;
        }

        static id_type skip()
        {
            return static_cast<id_type>(~1);
        }

        id_type eoi() const
        {
            return 0;
        }

        static id_type npos()
        {
            return static_cast<id_type>(~0);
        }

        std::locale imbue(const std::locale& locale_)
        {
            std::locale loc_ = _locale;

            _locale = locale_;
            return loc_;
        }

        const std::locale& locale() const
        {
            return _locale;
        }

        const rules_char_type* state(const id_type index_) const
        {
            if (index_ == 0)
            {
                return initial();
            }
            else
            {
                const id_type i_ = index_ - 1;

                if (_lexer_state_names.size() > i_)
                {
                    return _lexer_state_names[i_].c_str();
                }
                else
                {
                    return 0;
                }
            }
        }

        id_type state(const rules_char_type* name_) const
        {
            typename string_id_type_map::const_iterator iter_ =
                _statemap.find(name_);

            if (iter_ == _statemap.end())
            {
                return npos();
            }
            else
            {
                return iter_->second;
            }
        }

        id_type push_state(const rules_char_type* name_)
        {
            validate(name_);

            if (_statemap.insert(string_id_type_pair(name_,
                static_cast<id_type>(_statemap.size()))).second)
            {
                _regexes.push_back(token_vector_vector());
                _features.push_back(0);
                _ids.push_back(id_vector());
                _user_ids.push_back(id_vector());
                _next_dfas.push_back(id_vector());
                _pushes.push_back(id_vector());
                _pops.push_back(bool_vector());

                if (string(name_) != initial())
                {
                    _lexer_state_names.push_back(name_);
                }
            }
            else
            {
                return _statemap.find(name_)->second;
            }

            if (_next_dfas.size() > npos())
            {
                // Overflow
                throw runtime_error("The data type you have chosen cannot hold "
                    "this many lexer start states.");
            }

            // Initial is not stored, so no need to - 1.
            return static_cast<id_type>(_lexer_state_names.size());
        }

        void insert_macro(const rules_char_type* name_,
            const rules_char_type* regex_)
        {
            insert_macro(name_, string(regex_));
        }

        void insert_macro(const rules_char_type* name_,
            const rules_char_type* regex_start_,
            const rules_char_type* regex_end_)
        {
            insert_macro(name_, string(regex_start_, regex_end_));
        }

        void insert_macro(const rules_char_type* name_, const string& regex_)
        {
            validate(name_);

            typename macro_map::const_iterator iter_ = _macro_map.find(name_);

            if (iter_ == _macro_map.end())
            {
                auto pair_ =
                    _macro_map.insert(macro_pair(name_, token_vector()));

                tokenise(regex_, pair_.first->second, npos(), name_);
            }
            else
            {
                std::ostringstream ss_;

                ss_ << "Attempt to redefine MACRO '";
                narrow(name_, ss_);
                ss_ << "'.";
                throw runtime_error(ss_.str());
            }
        }

        // Add rule to INITIAL
        void push(const rules_char_type* regex_, const id_type id_,
            const id_type user_id_ = npos())
        {
            push(string(regex_), id_, user_id_);
        }

        void push(const rules_char_type* regex_start_,
            const rules_char_type* regex_end_,
            const id_type id_, const id_type user_id_ = npos())
        {
            push(string(regex_start_, regex_end_), id_, user_id_);
        }

        void push(const string& regex_, const id_type id_,
            const id_type user_id_ = npos())
        {
            check_for_invalid_id(id_);
            _regexes.front().push_back(token_vector());
            tokenise(regex_, _regexes.front().back(), id_, nullptr);

            if (_regexes.front().back()[1]._type == detail::token_type::BOL)
            {
                _features.front() |= bol_bit;
            }

            if (_regexes.front().back()[_regexes.front().back().size() - 2].
                _type == detail::token_type::EOL)
            {
                _features.front() |= eol_bit;
            }

            if (id_ == skip())
            {
                _features.front() |= skip_bit;
            }
            else if (id_ == eoi())
            {
                _features.front() |= again_bit;
            }

            _ids.front().push_back(id_);
            _user_ids.front().push_back(user_id_);
            _next_dfas.front().push_back(0);
            _pushes.front().push_back(npos());
            _pops.front().push_back(false);
        }

        // Add rule with no id
        void push(const rules_char_type* curr_dfa_,
            const rules_char_type* regex_, const rules_char_type* new_dfa_)
        {
            push(curr_dfa_, string(regex_), new_dfa_);
        }

        void push(const rules_char_type* curr_dfa_,
            const rules_char_type* regex_start_,
            const rules_char_type* regex_end_, const rules_char_type* new_dfa_)
        {
            push(curr_dfa_, string(regex_start_, regex_end_), new_dfa_);
        }

        void push(const rules_char_type* curr_dfa_, const string& regex_,
            const rules_char_type* new_dfa_)
        {
            push(curr_dfa_, regex_, eoi(), new_dfa_, false);
        }

        // Add rule with id
        void push(const rules_char_type* curr_dfa_,
            const rules_char_type* regex_, const id_type id_,
            const rules_char_type* new_dfa_, const id_type user_id_ = npos())
        {
            push(curr_dfa_, string(regex_), id_, new_dfa_, user_id_);
        }

        void push(const rules_char_type* curr_dfa_,
            const rules_char_type* regex_start_,
            const rules_char_type* regex_end_, const id_type id_,
            const rules_char_type* new_dfa_, const id_type user_id_ = npos())
        {
            push(curr_dfa_, string(regex_start_, regex_end_),
                id_, new_dfa_, user_id_);
        }

        void push(const rules_char_type* curr_dfa_, const string& regex_,
            const id_type id_, const rules_char_type* new_dfa_,
            const id_type user_id_ = npos())
        {
            push(curr_dfa_, regex_, id_, new_dfa_, true, user_id_);
        }

        void reverse()
        {
            for (auto& state_ : _regexes)
            {
                for (auto& regex_ : state_)
                {
                    reverse(regex_);
                }
            }

            for (auto& pair_ : _macro_map)
            {
                reverse(pair_.second);
            }
        }

        const string_id_type_map& statemap() const
        {
            return _statemap;
        }

        const token_vector_vector_vector& regexes() const
        {
            return _regexes;
        }

        const id_vector& features() const
        {
            return _features;
        }

        const id_vector_vector& ids() const
        {
            return _ids;
        }

        const id_vector_vector& user_ids() const
        {
            return _user_ids;
        }

        const id_vector_vector& next_dfas() const
        {
            return _next_dfas;
        }

        const id_vector_vector& pushes() const
        {
            return _pushes;
        }

        const bool_vector_vector& pops() const
        {
            return _pops;
        }

        bool empty() const
        {
            bool empty_ = true;

            for (const auto& regex_ : _regexes)
            {
                if (!regex_.empty())
                {
                    empty_ = false;
                    break;
                }
            }

            return empty_;
        }

        static const rules_char_type* initial()
        {
            static const rules_char_type initial_[] =
            { 'I', 'N', 'I', 'T', 'I', 'A', 'L', 0 };

            return initial_;
        }

        static const rules_char_type* dot()
        {
            static const rules_char_type dot_[] = { '.', 0 };

            return dot_;
        }

        static const rules_char_type* all_states()
        {
            static const rules_char_type star_[] = { '*', 0 };

            return star_;
        }

    private:
        string_id_type_map _statemap;
        macro_map _macro_map;
        token_vector_vector_vector _regexes;
        id_vector _features;
        id_vector_vector _ids;
        id_vector_vector _user_ids;
        id_vector_vector _next_dfas;
        id_vector_vector _pushes;
        bool_vector_vector _pops;
        std::size_t _flags;
        std::locale _locale;
        string_vector _lexer_state_names;

        void tokenise(const string& regex_, token_vector& tokens_,
            const id_type id_, const rules_char_type* name_)
        {
            re_state state_(regex_.c_str(), regex_.c_str() + regex_.size(), id_,
                _flags, _locale, name_);
            string macro_;
            rules_char_type diff_ = 0;

            tokens_.push_back(token());

            do
            {
                observer_ptr<token> lhs_ = &tokens_.back();
                token rhs_;

                tokeniser::next(*lhs_, state_, rhs_);

                if (rhs_._type != detail::token_type::DIFF &&
                    lhs_->precedence(rhs_._type) == ' ')
                {
                    std::ostringstream ss_;

                    ss_ << "A syntax error occurred: '" <<
                        lhs_->precedence_string() <<
                        "' against '" << rhs_.precedence_string() <<
                        "' preceding index " << state_.index() <<
                        " in ";

                    if (name_ != nullptr)
                    {
                        ss_ << "macro ";
                        narrow(name_, ss_);
                    }
                    else
                    {
                        ss_ << "rule id " << state_._id;
                    }

                    ss_ << '.';
                    throw runtime_error(ss_.str());
                }

                if (rhs_._type == detail::token_type::MACRO)
                {
                    typename macro_map::const_iterator iter_ =
                        _macro_map.find(rhs_._extra);

                    macro_ = rhs_._extra;

                    if (iter_ == _macro_map.end())
                    {
                        const rules_char_type* rhs_name_ = rhs_._extra.c_str();
                        std::ostringstream ss_;

                        ss_ << "Unknown MACRO name '";
                        narrow(rhs_name_, ss_);
                        ss_ << "'.";
                        throw runtime_error(ss_.str());
                    }
                    else
                    {
                        const bool multiple_ = iter_->second.size() > 3;
                        const token& first_ = iter_->second[1];
                        const token& second_ =
                            iter_->second[iter_->second.size() - 2];
                        const bool bol_ = tokens_.size() == 1 &&
                            first_._type == detail::token_type::CHARSET &&
                            first_._str.size() == 1 &&
                            first_._str._ranges[0] ==
                            typename token::string_token::range('^', '^');
                        const bool eol_ = state_._end == regex_.c_str() +
                            regex_.size() &&
                            second_._type == detail::token_type::CHARSET &&
                            second_._str.size() == 1 &&
                            second_._str._ranges[0] ==
                            typename token::string_token::range('$', '$');

                        if (diff_)
                        {
                            if (multiple_)
                            {
                                std::ostringstream ss_;

                                ss_ << "Single CHARSET must "
                                    "follow {-} or {+} at index " <<
                                    state_.index() - 1 << " in ";

                                if (name_ != nullptr)
                                {
                                    ss_ << "macro ";
                                    narrow(name_, ss_);
                                }
                                else
                                {
                                    ss_ << "rule id " << state_._id;
                                }

                                ss_ << '.';
                                throw runtime_error(ss_.str());
                            }
                            else
                            {
                                rhs_ = iter_->second[1];
                            }
                        }

                        // Any macro with more than one charset (or quantifiers)
                        // requires bracketing.
                        if (multiple_ && !(bol_ || eol_))
                        {
                            token open_;

                            open_._type = detail::token_type::OPENPAREN;
                            open_._str.insert('(');
                            tokens_.push_back(open_);
                        }

                        // Don't need to store token if it is diff.
                        if (!diff_)
                        {
                            std::size_t start_offset_ = 1;
                            std::size_t end_offset_ = 1;

                            if (bol_)
                            {
                                token token_;

                                token_._type = detail::token_type::BOL;
                                tokens_.push_back(token_);
                                ++start_offset_;
                            }

                            if (eol_)
                            {
                                ++end_offset_;
                            }

                            // Don't insert BEGIN or END tokens
                            tokens_.insert(tokens_.end(),
                                iter_->second.begin() + start_offset_,
                                iter_->second.end() - end_offset_);

                            if (eol_)
                            {
                                token token_;

                                token_._type = detail::token_type::EOL;
                                tokens_.push_back(token_);
                            }

                            lhs_ = &tokens_.back();
                        }

                        if (multiple_ && !(bol_ || eol_))
                        {
                            token close_;

                            close_._type = detail::token_type::CLOSEPAREN;
                            close_._str.insert(')');
                            tokens_.push_back(close_);
                        }
                    }
                }
                else if (rhs_._type == detail::token_type::DIFF)
                {
                    if (!macro_.empty())
                    {
                        typename macro_map::const_iterator iter_ =
                            _macro_map.find(macro_);

                        if (iter_->second.size() > 3)
                        {
                            std::ostringstream ss_;

                            ss_ << "Single CHARSET must precede {-} or {+} at "
                                "index " << state_.index() - 1 << " in ";

                            if (name_ != nullptr)
                            {
                                ss_ << "macro ";
                                narrow(name_, ss_);
                            }
                            else
                            {
                                ss_ << "rule id " << state_._id;
                            }

                            ss_ << '.';
                            throw runtime_error(ss_.str());
                        }
                    }

                    diff_ = rhs_._extra[0];
                    macro_.clear();
                    continue;
                }
                else if (!diff_)
                {
                    tokens_.push_back(rhs_);
                    lhs_ = &tokens_.back();
                    macro_.clear();
                }

                // diff_ may have been set by previous conditional.
                if (diff_)
                {
                    if (rhs_._type != detail::token_type::CHARSET)
                    {
                        std::ostringstream ss_;

                        ss_ << "CHARSET must follow {-} or {+} at index " <<
                            state_.index() - 1 << " in ";

                        if (name_ != nullptr)
                        {
                            ss_ << "macro ";
                            narrow(name_, ss_);
                        }
                        else
                        {
                            ss_ << "rule id " << state_._id;
                        }

                        ss_ << '.';
                        throw runtime_error(ss_.str());
                    }

                    switch (diff_)
                    {
                    case '-':
                        lhs_->_str.remove(rhs_._str);

                        if (lhs_->_str.empty())
                        {
                            std::ostringstream ss_;

                            ss_ << "Empty charset created by {-} at index " <<
                                state_.index() - 1 << " in ";

                            if (name_ != nullptr)
                            {
                                ss_ << "macro ";
                                narrow(name_, ss_);
                            }
                            else
                            {
                                ss_ << "rule id " << state_._id;
                            }

                            ss_ << '.';
                            throw runtime_error(ss_.str());
                        }

                        break;
                    case '+':
                        lhs_->_str.insert(rhs_._str);
                        break;
                    }

                    diff_ = 0;
                }
            } while (tokens_.back()._type != detail::token_type::END);

            if (tokens_.size() == 2)
            {
                std::ostringstream ss_;

                ss_ << "Empty regex in ";

                if (name_ != nullptr)
                {
                    ss_ << "macro ";
                    narrow(name_, ss_);
                }
                else
                {
                    ss_ << "rule id " << state_._id;
                }

                ss_ << " is not allowed.";
                throw runtime_error(ss_.str());
            }
        }

        void reverse(token_vector& vector_)
        {
            token_vector new_vector_(vector_.size(), token());
            auto iter_ = vector_.rbegin();
            auto end_ = vector_.rend();
            auto dest_ = new_vector_.begin();
            std::stack<typename token_vector::reverse_iterator> stack_;

            for (; iter_ != end_; ++iter_, ++dest_)
            {
                switch (iter_->_type)
                {
                case detail::token_type::BEGIN:
                    iter_->swap(*dest_);
                    dest_->_type = detail::token_type::END;
                    break;
                case detail::token_type::BOL:
                    iter_->swap(*dest_);
                    dest_->_type = detail::token_type::EOL;
                    break;
                case detail::token_type::EOL:
                    iter_->swap(*dest_);
                    dest_->_type = detail::token_type::BOL;
                    break;
                case detail::token_type::OPENPAREN:
                    iter_->swap(*dest_);
                    dest_->_type = detail::token_type::CLOSEPAREN;

                    if (stack_.top() != end_)
                    {
                        ++dest_;
                        dest_->swap(*stack_.top());
                    }

                    stack_.pop();
                    break;
                case detail::token_type::CLOSEPAREN:
                    iter_->swap(*dest_);
                    dest_->_type = detail::token_type::OPENPAREN;
                    stack_.push(end_);
                    break;
                case detail::token_type::OPT:
                case detail::token_type::AOPT:
                case detail::token_type::ZEROORMORE:
                case detail::token_type::AZEROORMORE:
                case detail::token_type::ONEORMORE:
                case detail::token_type::AONEORMORE:
                case detail::token_type::REPEATN:
                case detail::token_type::AREPEATN:
                {
                    auto temp_ = iter_ + 1;

                    if (temp_->_type == detail::token_type::CLOSEPAREN)
                    {
                        stack_.push(iter_);
                        ++iter_;
                        iter_->swap(*dest_);
                        dest_->_type = detail::token_type::OPENPAREN;
                    }
                    else
                    {
                        dest_->swap(*temp_);
                        ++dest_;
                        dest_->swap(*iter_);
                        ++iter_;
                    }

                    break;
                }
                case detail::token_type::END:
                    iter_->swap(*dest_);
                    dest_->_type = detail::token_type::BEGIN;
                    break;
                default:
                    // detail::token_type::OR
                    // detail::token_type::CHARSET
                    iter_->swap(*dest_);
                    break;
                }
            }

            new_vector_.swap(vector_);
        }

        void push(const rules_char_type* curr_dfa_, const string& regex_,
            const id_type id_, const rules_char_type* new_dfa_,
            const bool check_, const id_type user_id_ = npos())
        {
            const bool star_ = *curr_dfa_ == '*' && *(curr_dfa_ + 1) == 0;
            const bool dot_ = *new_dfa_ == '.' && *(new_dfa_ + 1) == 0;
            const bool push_ = *new_dfa_ == '>';
            const rules_char_type* push_dfa_ = nullptr;
            const bool pop_ = *new_dfa_ == '<';

            if (push_ || pop_)
            {
                ++new_dfa_;
            }

            if (check_)
            {
                check_for_invalid_id(id_);
            }

            if (!dot_ && !pop_)
            {
                const rules_char_type* temp_ = new_dfa_;

                while (*temp_ && *temp_ != ':')
                {
                    ++temp_;
                }

                if (*temp_) push_dfa_ = temp_ + 1;

                validate(new_dfa_, *temp_ ? temp_ : 0);

                if (push_dfa_)
                {
                    validate(push_dfa_);
                }
            }

            // npos means pop here
            id_type new_dfa_id_ = npos();
            id_type push_dfa_id_ = npos();
            typename string_id_type_map::const_iterator iter_;
            auto end_ = _statemap.cend();
            id_vector next_dfas_;

            if (!dot_ && !pop_)
            {
                if (push_dfa_)
                {
                    iter_ = _statemap.find(string(new_dfa_, push_dfa_ - 1));
                }
                else
                {
                    iter_ = _statemap.find(new_dfa_);
                }

                if (iter_ == end_)
                {
                    std::ostringstream ss_;

                    ss_ << "Unknown state name '";
                    narrow(new_dfa_, ss_);
                    ss_ << "'.";
                    throw runtime_error(ss_.str());
                }

                new_dfa_id_ = iter_->second;

                if (push_dfa_)
                {
                    iter_ = _statemap.find(push_dfa_);

                    if (iter_ == end_)
                    {
                        std::ostringstream ss_;

                        ss_ << "Unknown state name '";
                        narrow(push_dfa_, ss_);
                        ss_ << "'.";
                        throw runtime_error(ss_.str());
                    }

                    push_dfa_id_ = iter_->second;
                }
            }

            if (star_)
            {
                const std::size_t size_ = _statemap.size();

                for (id_type i_ = 0; i_ < size_; ++i_)
                {
                    next_dfas_.push_back(i_);
                }
            }
            else
            {
                const rules_char_type* start_ = curr_dfa_;
                string next_dfa_;

                while (*curr_dfa_)
                {
                    while (*curr_dfa_ && *curr_dfa_ != ',')
                    {
                        ++curr_dfa_;
                    }

                    next_dfa_.assign(start_, curr_dfa_);

                    if (*curr_dfa_)
                    {
                        ++curr_dfa_;
                        start_ = curr_dfa_;
                    }

                    validate(next_dfa_.c_str());
                    iter_ = _statemap.find(next_dfa_.c_str());

                    if (iter_ == end_)
                    {
                        std::ostringstream ss_;

                        ss_ << "Unknown state name '";
                        curr_dfa_ = next_dfa_.c_str();
                        narrow(curr_dfa_, ss_);
                        ss_ << "'.";
                        throw runtime_error(ss_.str());
                    }

                    next_dfas_.push_back(iter_->second);
                }
            }

            for (std::size_t i_ = 0, size_ = next_dfas_.size();
                i_ < size_; ++i_)
            {
                const id_type curr_ = next_dfas_[i_];

                _regexes[curr_].push_back(token_vector());
                tokenise(regex_, _regexes[curr_].back(), id_, 0);

                if (_regexes[curr_].back()[1]._type == detail::token_type::BOL)
                {
                    _features[curr_] |= bol_bit;
                }

                if (_regexes[curr_].back()[_regexes[curr_].back().size() - 2].
                    _type == detail::token_type::EOL)
                {
                    _features[curr_] |= eol_bit;
                }

                if (id_ == skip())
                {
                    _features[curr_] |= skip_bit;
                }
                else if (id_ == eoi())
                {
                    _features[curr_] |= again_bit;
                }

                if (push_ || pop_)
                {
                    _features[curr_] |= recursive_bit;
                }

                _ids[curr_].push_back(id_);
                _user_ids[curr_].push_back(user_id_);
                _next_dfas[curr_].push_back(dot_ ? curr_ : new_dfa_id_);
                _pushes[curr_].push_back(push_ ? (push_dfa_ ?
                    push_dfa_id_ : curr_) : npos());
                _pops[curr_].push_back(pop_);
            }
        }

        void validate(const rules_char_type* name_,
            const rules_char_type* end_ = nullptr) const
        {
            const rules_char_type* start_ = name_;

            if (*name_ != '_' && !(*name_ >= 'A' && *name_ <= 'Z') &&
                !(*name_ >= 'a' && *name_ <= 'z'))
            {
                std::ostringstream ss_;

                ss_ << "Invalid name '";
                narrow(name_, ss_);
                ss_ << "'.";
                throw runtime_error(ss_.str());
            }
            else if (*name_)
            {
                ++name_;
            }

            while (*name_ && name_ != end_)
            {
                if (*name_ != '_' && *name_ != '-' &&
                    !(*name_ >= 'A' && *name_ <= 'Z') &&
                    !(*name_ >= 'a' && *name_ <= 'z') &&
                    !(*name_ >= '0' && *name_ <= '9'))
                {
                    std::ostringstream ss_;

                    ss_ << "Invalid name '";
                    name_ = start_;
                    narrow(name_, ss_);
                    ss_ << "'.";
                    throw runtime_error(ss_.str());
                }

                ++name_;
            }
        }

        void check_for_invalid_id(const id_type id_) const
        {
            if (id_ == eoi())
            {
                throw runtime_error("Cannot resuse the id for eoi.");
            }

            if (id_ == npos())
            {
                throw runtime_error("The id npos is reserved for the "
                    "UNKNOWN token.");
            }
        }
    };

    using rules = basic_rules<char, char>;
    using wrules = basic_rules<wchar_t, wchar_t>;
    using u32rules = basic_rules<char32_t, char32_t>;
}

#endif
