// tokeniser.hpp
// Copyright (c) 2005-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_RE_TOKENISER_HPP
#define LEXERTL_RE_TOKENISER_HPP

#include <cstring>
#include "re_token.hpp"
#include "../../runtime_error.hpp"
#include <sstream>
#include "../../string_token.hpp"
#include "re_tokeniser_helper.hpp"

namespace lexertl
{
    namespace detail
    {
        template<typename rules_char_type, typename char_type, typename id_type>
        class basic_re_tokeniser
        {
        public:
            using re_token = basic_re_token<rules_char_type, char_type>;
            using tokeniser_helper =
                basic_re_tokeniser_helper<rules_char_type, char_type, id_type>;
            using char_state = typename tokeniser_helper::char_state;
            using state = typename tokeniser_helper::state;
            using string_token = basic_string_token<char_type>;

            static void next(re_token& lhs_, state& state_, re_token& token_)
            {
                rules_char_type ch_ = 0;
                bool eos_ = state_.next(ch_);
                bool skipped_ = false;

                token_.clear();

                do
                {
                    // string begin/end
                    while (!eos_ && ch_ == '"')
                    {
                        state_._in_string ^= 1;
                        eos_ = state_.next(ch_);
                    }

                    if (eos_) break;

                    // (?# ...)
                    skipped_ = comment(eos_, ch_, state_);

                    if (eos_) break;

                    // skip_ws set
                    skipped_ |= skip(eos_, ch_, state_);
                } while (!eos_ && skipped_);

                if (eos_)
                {
                    if (state_._in_string)
                    {
                        std::ostringstream ss_;

                        // Pointless returning index if at end of string
                        state_.unexpected_end(ss_);
                        ss_ << " (missing '\"')";
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }

                    if (state_._paren_count)
                    {
                        std::ostringstream ss_;

                        // Pointless returning index if at end of string
                        state_.unexpected_end(ss_);
                        ss_ << " (missing ')')";
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }

                    token_._type = token_type::END;
                }
                else
                {
                    if (ch_ == '\\')
                    {
                        // Even if we are in a string,
                        // respect escape sequences...
                        token_._type = token_type::CHARSET;
                        escape(state_, token_._str);
                    }
                    else if (state_._in_string)
                    {
                        // All other meta characters lose their special meaning
                        // inside a string.
                        token_._type = token_type::CHARSET;
                        add_char(ch_, state_, token_._str);
                    }
                    else
                    {
                        // Not an escape sequence and not inside a string, so
                        // check for meta characters.
                        switch (ch_)
                        {
                        case '(':
                            token_._type = token_type::OPENPAREN;
                            ++state_._paren_count;
                            read_options(state_);
                            break;
                        case ')':
                            --state_._paren_count;

                            if (state_._paren_count < 0)
                            {
                                std::ostringstream ss_;

                                ss_ << "Number of open parenthesis < 0 "
                                    "at index " << state_.index() - 1;
                                state_.error(ss_);
                                throw runtime_error(ss_.str());
                            }

                            token_._type = token_type::CLOSEPAREN;

                            if (!state_._flags_stack.empty())
                            {
                                state_._flags = state_._flags_stack.top();
                                state_._flags_stack.pop();
                            }

                            break;
                        case '?':
                            if (!state_.eos() && *state_._curr == '?')
                            {
                                token_._type = token_type::AOPT;
                                state_.increment();
                            }
                            else
                            {
                                token_._type = token_type::OPT;
                            }

                            break;
                        case '*':
                            if (!state_.eos() && *state_._curr == '?')
                            {
                                token_._type = token_type::AZEROORMORE;
                                state_.increment();
                            }
                            else
                            {
                                token_._type = token_type::ZEROORMORE;
                            }

                            break;
                        case '+':
                            if (!state_.eos() && *state_._curr == '?')
                            {
                                token_._type = token_type::AONEORMORE;
                                state_.increment();
                            }
                            else
                            {
                                token_._type = token_type::ONEORMORE;
                            }

                            break;
                        case '{':
                            open_curly(lhs_, state_, token_);
                            break;
                        case '|':
                            token_._type = token_type::OR;
                            break;
                        case '^':
                            if (!state_._macro_name &&
                                state_._curr - 1 == state_._start)
                            {
                                token_._type = token_type::BOL;
                            }
                            else
                            {
                                token_._type = token_type::CHARSET;
                                token_._str.insert(range(ch_, ch_));
                            }

                            break;
                        case '$':
                            if (!state_._macro_name && state_._curr ==
                                state_._end)
                            {
                                token_._type = token_type::EOL;
                            }
                            else
                            {
                                token_._type = token_type::CHARSET;
                                token_._str.insert(range(ch_, ch_));
                            }

                            break;
                        case '.':
                        {
                            token_._type = token_type::CHARSET;

                            if (state_._flags & dot_not_newline)
                            {
                                token_._str.insert(range('\n', '\n'));
                            }
                            else if (state_._flags & dot_not_cr_lf)
                            {
                                token_._str.insert(range('\n', '\n'));
                                token_._str.insert(range('\r', '\r'));
                            }

                            token_._str.negate();
                            break;
                        }
                        case '[':
                        {
                            token_._type = token_type::CHARSET;
                            tokeniser_helper::charset(state_, token_._str);
                            break;
                        }
                        case '/':
                        {
                            std::ostringstream ss_;

                            ss_ << "Lookahead ('/') is not supported yet";
                            state_.error(ss_);
                            throw runtime_error(ss_.str());
                            break;
                        }
                        default:
                            token_._type = token_type::CHARSET;
                            add_char(ch_, state_, token_._str);
                            break;
                        }
                    }
                }
            }

        private:
            using range = typename string_token::range;

            static bool comment(bool& eos_, rules_char_type& ch_, state& state_)
            {
                bool skipped_ = false;

                if (!state_._in_string && ch_ == '(' && !state_.eos() &&
                    *state_._curr == '?' && state_._curr + 1 < state_._end &&
                    *(state_._curr + 1) == '#')
                {
                    std::size_t paren_count_ = 1;

                    state_.increment();
                    state_.increment();

                    do
                    {
                        eos_ = state_.next(ch_);

                        if (ch_ == '(')
                        {
                            ++paren_count_;
                        }
                        else if (ch_ == ')')
                        {
                            --paren_count_;
                        }
                    } while (!eos_ && !(ch_ == ')' && paren_count_ == 0));

                    if (eos_)
                    {
                        std::ostringstream ss_;

                        // Pointless returning index if at end of string
                        state_.unexpected_end(ss_);
                        ss_ << " (unterminated comment)";
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }
                    else
                    {
                        eos_ = state_.next(ch_);
                    }

                    skipped_ = true;
                }

                return skipped_;
            }

            static bool skip(bool& eos_, rules_char_type& ch_, state& state_)
            {
                bool skipped_ = false;

                if ((state_._flags & skip_ws) && !state_._in_string)
                {
                    bool c_comment_ = false;
                    bool skip_ws_ = false;

                    do
                    {
                        c_comment_ = ch_ == '/' && !state_.eos() &&
                            *state_._curr == '*';
                        skip_ws_ = !c_comment_ && (ch_ == ' ' || ch_ == '\t' ||
                            ch_ == '\n' || ch_ == '\r' || ch_ == '\f' ||
                            ch_ == '\v');

                        if (c_comment_)
                        {
                            state_.increment();
                            eos_ = state_.next(ch_);

                            while (!eos_ && !(ch_ == '*' && !state_.eos() &&
                                *state_._curr == '/'))
                            {
                                eos_ = state_.next(ch_);
                            }

                            if (eos_)
                            {
                                std::ostringstream ss_;

                                // Pointless returning index if at end of string
                                state_.unexpected_end(ss_);
                                ss_ << " (unterminated C style comment)";
                                state_.error(ss_);
                                throw runtime_error(ss_.str());
                            }
                            else
                            {
                                state_.increment();
                                eos_ = state_.next(ch_);
                            }

                            skipped_ = true;
                        }
                        else if (skip_ws_)
                        {
                            eos_ = state_.next(ch_);
                            skipped_ = true;
                        }
                    } while (!eos_ && (c_comment_ || skip_ws_));
                }

                return skipped_;
            }

            static void read_options(state& state_)
            {
                if (!state_.eos() && *state_._curr == '?')
                {
                    rules_char_type ch_ = 0;
                    bool eos_ = false;
                    bool negate_ = false;

                    state_.increment();
                    eos_ = state_.next(ch_);
                    state_._flags_stack.push(state_._flags);

                    while (!eos_ && ch_ != ':')
                    {
                        switch (ch_)
                        {
                        case '-':
                            negate_ ^= 1;
                            break;
                        case 'i':
                            if (negate_)
                            {
                                state_._flags = state_._flags & ~icase;
                            }
                            else
                            {
                                state_._flags = state_._flags | icase;
                            }

                            negate_ = false;
                            break;
                        case 's':
                            if (negate_)
                            {
#ifdef _WIN32
                                state_._flags = state_._flags | dot_not_cr_lf;
#else
                                state_._flags = state_._flags | dot_not_newline;
#endif
                            }
                            else
                            {
#ifdef _WIN32
                                state_._flags = state_._flags & ~dot_not_cr_lf;
#else
                                state_._flags =
                                    state_._flags & ~dot_not_newline;
#endif
                            }

                            negate_ = false;
                            break;
                        case 'x':
                            if (negate_)
                            {
                                state_._flags = state_._flags & ~skip_ws;
                            }
                            else
                            {
                                state_._flags = state_._flags | skip_ws;
                            }

                            negate_ = false;
                            break;
                        default:
                        {
                            std::ostringstream ss_;

                            ss_ << "Unknown option at index " <<
                                state_.index() - 1;
                            state_.error(ss_);
                            throw runtime_error(ss_.str());
                        }
                        }

                        eos_ = state_.next(ch_);
                    }

                    // End of string handler will handle early termination
                }
                else if (!state_._flags_stack.empty())
                {
                    state_._flags_stack.push(state_._flags);
                }
            }

            static void escape(state& state_, string_token& token_)
            {
                char_type ch_ = 0;
                std::size_t str_len_ = 0;
                const char* str_ = tokeniser_helper::escape_sequence(state_,
                    ch_, str_len_);

                if (str_)
                {
                    char_state state2_(str_ + 1, str_ + str_len_, state_._id,
                        state_._flags, state_._locale, nullptr);

                    tokeniser_helper::charset(state2_, token_);
                }
                else
                {
                    add_char(ch_, state_, token_);
                }
            }

            static void add_char(const char_type ch_, const state& state_,
                string_token& token_)
            {
                range range_(ch_, ch_);

                token_.insert(range_);

                if (state_._flags & icase)
                {
                    string_token folded_;

                    tokeniser_helper::fold(range_, state_._locale,
                        folded_, typename tokeniser_helper::template
                        size<sizeof(char_type)>());

                    if (!folded_.empty())
                    {
                        token_.insert(folded_);
                    }
                }
            }

            static void open_curly(re_token& lhs_, state& state_,
                re_token& token_)
            {
                if (state_.eos())
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " (missing '}')";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }
                else if (*state_._curr == '-' || *state_._curr == '+')
                {
                    rules_char_type ch_ = 0;

                    if (lhs_._type != token_type::CHARSET)
                    {
                        std::ostringstream ss_;

                        ss_ << "CHARSET must precede {";
                        narrow(state_._curr, ss_);
                        ss_ << "} at index " << state_.index() - 1;
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }

                    state_.next(ch_);
                    token_._type = token_type::DIFF;
                    token_._extra = ch_;

                    if (state_.next(ch_))
                    {
                        std::ostringstream ss_;

                        // Pointless returning index if at end of string
                        state_.unexpected_end(ss_);
                        ss_ << " (missing '}')";
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }

                    if (ch_ != '}')
                    {
                        std::ostringstream ss_;

                        ss_ << "Missing '}' at index " << state_.index() - 1;
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }
                }
                else if (*state_._curr >= '0' && *state_._curr <= '9')
                {
                    repeat_n(state_, token_);
                }
                else
                {
                    macro(state_, token_);
                }
            }

            // SYNTAX:
            //   {n[,[n]]}
            // SEMANTIC RULES:
            //   {0} - INVALID (throw exception)
            //   {0,} = *
            //   {0,0} - INVALID (throw exception)
            //   {0,1} = ?
            //   {1,} = +
            //   {min,max} where min == max - {min}
            //   {min,max} where max < min - INVALID (throw exception)
            static void repeat_n(state& state_, re_token& token_)
            {
                rules_char_type ch_ = 0;
                bool eos_ = state_.next(ch_);
                std::size_t min_ = 0;
                std::size_t max_ = 0;

                while (!eos_ && ch_ >= '0' && ch_ <= '9')
                {
                    min_ *= 10;
                    min_ += static_cast<std::size_t>(ch_) - '0';
                    token_._extra += ch_;
                    eos_ = state_.next(ch_);
                }

                if (eos_)
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " (missing repeat terminator '}')";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                bool min_max_ = false;
                bool repeatn_ = true;

                if (ch_ == ',')
                {
                    token_._extra += ch_;
                    eos_ = state_.next(ch_);

                    if (eos_)
                    {
                        std::ostringstream ss_;

                        // Pointless returning index if at end of string
                        state_.unexpected_end(ss_);
                        ss_ << " (missing repeat terminator '}')";
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }

                    if (ch_ == '}')
                    {
                        // Small optimisation: Check for '*' equivalency.
                        if (min_ == 0)
                        {
                            token_._type = token_type::ZEROORMORE;
                            repeatn_ = false;
                        }
                        // Small optimisation: Check for '+' equivalency.
                        else if (min_ == 1)
                        {
                            token_._type = token_type::ONEORMORE;
                            repeatn_ = false;
                        }
                    }
                    else
                    {
                        if (ch_ < '0' || ch_ > '9')
                        {
                            std::ostringstream ss_;

                            ss_ << "Missing repeat terminator '}' at index " <<
                                state_.index() - 1;
                            state_.error(ss_);
                            throw runtime_error(ss_.str());
                        }

                        min_max_ = true;

                        do
                        {
                            max_ *= 10;
                            max_ += static_cast<std::size_t>(ch_) - '0';
                            token_._extra += ch_;
                            eos_ = state_.next(ch_);
                        } while (!eos_ && ch_ >= '0' && ch_ <= '9');

                        if (eos_)
                        {
                            std::ostringstream ss_;

                            // Pointless returning index if at end of string
                            state_.unexpected_end(ss_);
                            ss_ << " (missing repeat terminator '}')";
                            state_.error(ss_);
                            throw runtime_error(ss_.str());
                        }

                        // Small optimisation: Check for '?' equivalency.
                        if (min_ == 0 && max_ == 1)
                        {
                            token_._type = token_type::OPT;
                            repeatn_ = false;
                        }
                        // Small optimisation: if min == max, then min.
                        else if (min_ == max_)
                        {
                            token_._extra.erase(token_._extra.find(','));
                            min_max_ = false;
                            max_ = 0;
                        }
                    }
                }

                if (ch_ != '}')
                {
                    std::ostringstream ss_;

                    ss_ << "Missing repeat terminator '}' at index " <<
                        state_.index() - 1;
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                if (repeatn_)
                {
                    // SEMANTIC VALIDATION follows:
                    // NOTE: {0,} has already become *
                    // therefore we don't check for a comma.
                    if (min_ == 0 && max_ == 0)
                    {
                        std::ostringstream ss_;

                        ss_ << "Cannot have exactly zero "
                            "repeats preceding index " <<
                            state_.index();
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }

                    if (min_max_ && max_ < min_)
                    {
                        std::ostringstream ss_;

                        ss_ << "Max less than min preceding index " <<
                            state_.index();
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }

                    if (!state_.eos() && *state_._curr == '?')
                    {
                        token_._type = token_type::AREPEATN;
                        state_.increment();
                    }
                    else
                    {
                        token_._type = token_type::REPEATN;
                    }
                }
                else if (token_._type == token_type::ZEROORMORE)
                {
                    if (!state_.eos() && *state_._curr == '?')
                    {
                        token_._type = token_type::AZEROORMORE;
                        state_.increment();
                    }
                }
                else if (token_._type == token_type::ONEORMORE)
                {
                    if (!state_.eos() && *state_._curr == '?')
                    {
                        token_._type = token_type::AONEORMORE;
                        state_.increment();
                    }
                }
                else if (token_._type == token_type::OPT)
                {
                    if (!state_.eos() && *state_._curr == '?')
                    {
                        token_._type = token_type::AOPT;
                        state_.increment();
                    }
                }
            }

            static void macro(state& state_, re_token& token_)
            {
                rules_char_type ch_ = 0;
                bool eos_ = false;

                state_.next(ch_);

                if (ch_ != '_' && !(ch_ >= 'A' && ch_ <= 'Z') &&
                    !(ch_ >= 'a' && ch_ <= 'z'))
                {
                    std::ostringstream ss_;

                    ss_ << "Invalid MACRO name at index " << state_.index() - 1;
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                do
                {
                    token_._extra += ch_;
                    eos_ = state_.next(ch_);

                    if (eos_)
                    {
                        std::ostringstream ss_;

                        // Pointless returning index if at end of string
                        state_.unexpected_end(ss_);
                        ss_ << " (missing MACRO name terminator '}')";
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }
                } while (ch_ == '_' || ch_ == '-' ||
                    (ch_ >= 'A' && ch_ <= 'Z') ||
                    (ch_ >= 'a' && ch_ <= 'z') ||
                    (ch_ >= '0' && ch_ <= '9'));

                if (ch_ != '}')
                {
                    std::ostringstream ss_;

                    ss_ << "Missing MACRO name terminator '}' at index " <<
                        state_.index() - 1;
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                token_._type = token_type::MACRO;
            }
        };
    }
}

#endif
