// debug.hpp
// Copyright (c) 2005-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_DEBUG_HPP
#define LEXERTL_DEBUG_HPP

#include <map>
#include <ostream>
#include "rules.hpp"
#include "sm_to_csm.hpp"
#include "state_machine.hpp"
#include "string_token.hpp"
#include <vector>

namespace lexertl
{
    template<typename sm, typename char_type, typename id_type = uint16_t,
        bool is_dfa = true>
        class basic_debug
    {
    public:
        using char_state_machine =
            basic_char_state_machine<char_type, id_type, is_dfa>;
        using ostream = std::basic_ostream<char_type>;
        using rules = basic_rules<char_type, char_type, id_type>;
        using string = std::basic_string<char_type>;

        static void dump(const sm& sm_, rules& rules_, ostream& stream_)
        {
            char_state_machine csm_;

            sm_to_csm(sm_, csm_);
            dump(csm_, rules_, stream_);
        }

        static void dump(const sm& sm_, ostream& stream_)
        {
            char_state_machine csm_;

            sm_to_csm(sm_, csm_);
            dump(csm_, stream_);
        }

        static void dump(const char_state_machine& csm_, rules& rules_,
            ostream& stream_)
        {
            for (std::size_t dfa_ = 0, dfas_ = csm_.size();
                dfa_ < dfas_; ++dfa_)
            {
                lexer_state(stream_);
                stream_ << rules_.state(dfa_) << std::endl << std::endl;

                dump_ex(csm_._sm_vector[dfa_], stream_);
            }
        }

        static void dump(const char_state_machine& csm_, ostream& stream_)
        {
            for (std::size_t dfa_ = 0, dfas_ = csm_.size();
                dfa_ < dfas_; ++dfa_)
            {
                lexer_state(stream_);
                stream_ << dfa_ << std::endl << std::endl;

                dump_ex(csm_._sm_vector[dfa_], stream_);
            }
        }

    protected:
        using dfa_state = typename char_state_machine::state;
        using string_token = typename dfa_state::string_token;
        using stringstream = std::basic_stringstream<char_type>;

        static void dump_ex(const typename char_state_machine::dfa& dfa_,
            ostream& stream_)
        {
            const std::size_t states_ = dfa_._states.size();
            const id_type bol_index_ = dfa_._bol_index;

            for (std::size_t i_ = 0; i_ < states_; ++i_)
            {
                const dfa_state& state_ = dfa_._states[i_];

                state(stream_);
                stream_ << i_ << std::endl;

                if (state_._end_state)
                {
                    end_state(stream_);

                    if (state_._push_pop_dfa ==
                        dfa_state::push_pop_dfa::push_dfa)
                    {
                        push(stream_);
                        stream_ << state_._push_dfa;
                    }
                    else if (state_._push_pop_dfa ==
                        dfa_state::push_pop_dfa::pop_dfa)
                    {
                        pop(stream_);
                    }

                    id(stream_);
                    stream_ << static_cast<std::size_t>(state_._id);
                    user_id(stream_);
                    stream_ << static_cast<std::size_t>(state_._user_id);
                    dfa(stream_);
                    stream_ << static_cast<std::size_t>(state_._next_dfa);
                    stream_ << std::endl;
                }

                if (i_ == 0 && bol_index_ != char_state_machine::npos())
                {
                    bol(stream_);
                    stream_ << static_cast<std::size_t>(bol_index_) <<
                        std::endl;
                }

                if (state_._eol_index != char_state_machine::npos())
                {
                    eol(stream_);
                    stream_ << static_cast<std::size_t>(state_._eol_index) <<
                        std::endl;
                }

                for (const auto& tran_ : state_._transitions)
                {
                    string_token token_ = tran_.second;

                    open_bracket(stream_);

                    if (!tran_.second.any() && tran_.second.negatable())
                    {
                        token_.negate();
                        negated(stream_);
                    }

                    string chars_;

                    for (const auto& range_ : token_._ranges)
                    {
                        if (range_.first == '-' || range_.first == '^' ||
                            range_.first == ']')
                        {
                            stream_ << '\\';
                        }

                        chars_ = string_token::escape_char
                        (range_.first);

                        if (range_.first != range_.second)
                        {
                            if (range_.first + 1 < range_.second)
                            {
                                chars_ += '-';
                            }

                            if (range_.second == '-' || range_.second == '^' ||
                                range_.second == ']')
                            {
                                stream_ << '\\';
                            }

                            chars_ += string_token::escape_char(range_.second);
                        }

                        stream_ << chars_;
                    }

                    close_bracket(stream_);
                    stream_ << static_cast<std::size_t>(tran_.first) <<
                        std::endl;
                }

                stream_ << std::endl;
            }
        }

        static void lexer_state(std::ostream& stream_)
        {
            stream_ << "Lexer state: ";
        }

        static void lexer_state(std::wostream& stream_)
        {
            stream_ << L"Lexer state: ";
        }

        static void state(std::ostream& stream_)
        {
            stream_ << "State: ";
        }

        static void state(std::wostream& stream_)
        {
            stream_ << L"State: ";
        }

        static void bol(std::ostream& stream_)
        {
            stream_ << "  BOL -> ";
        }

        static void bol(std::wostream& stream_)
        {
            stream_ << L"  BOL -> ";
        }

        static void eol(std::ostream& stream_)
        {
            stream_ << "  EOL -> ";
        }

        static void eol(std::wostream& stream_)
        {
            stream_ << L"  EOL -> ";
        }

        static void end_state(std::ostream& stream_)
        {
            stream_ << "  END STATE";
        }

        static void end_state(std::wostream& stream_)
        {
            stream_ << L"  END STATE";
        }

        static void id(std::ostream& stream_)
        {
            stream_ << ", Id = ";
        }

        static void id(std::wostream& stream_)
        {
            stream_ << L", Id = ";
        }

        static void push(std::ostream& stream_)
        {
            stream_ << ", PUSH ";
        }

        static void push(std::wostream& stream_)
        {
            stream_ << L", PUSH ";
        }

        static void pop(std::ostream& stream_)
        {
            stream_ << ", POP";
        }

        static void pop(std::wostream& stream_)
        {
            stream_ << L", POP";
        }

        static void user_id(std::ostream& stream_)
        {
            stream_ << ", User Id = ";
        }

        static void user_id(std::wostream& stream_)
        {
            stream_ << L", User Id = ";
        }

        static void open_bracket(std::ostream& stream_)
        {
            stream_ << "  [";
        }

        static void open_bracket(std::wostream& stream_)
        {
            stream_ << L"  [";
        }

        static void negated(std::ostream& stream_)
        {
            stream_ << "^";
        }

        static void negated(std::wostream& stream_)
        {
            stream_ << L"^";
        }

        static void close_bracket(std::ostream& stream_)
        {
            stream_ << "] -> ";
        }

        static void close_bracket(std::wostream& stream_)
        {
            stream_ << L"] -> ";
        }

        static void dfa(std::ostream& stream_)
        {
            stream_ << ", dfa = ";
        }

        static void dfa(std::wostream& stream_)
        {
            stream_ << L", dfa = ";
        }
    };

    using debug = basic_debug<state_machine, char>;
    using wdebug = basic_debug<wstate_machine, wchar_t>;
}

#endif
