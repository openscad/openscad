// debug.hpp
// Copyright (c) 2005-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_DEBUG_HPP
#define LEXERTL_DEBUG_HPP

#include <map>
#include <ostream>
#include "rules.hpp"
#include "size_t.hpp"
#include "sm_to_csm.hpp"
#include "state_machine.hpp"
#include "string_token.hpp"
#include <vector>

namespace lexertl
{
template<typename sm, typename char_type, typename id_type = std::size_t,
    bool is_dfa = true>
class basic_debug
{
public:
    typedef lexertl::basic_char_state_machine<char_type, id_type, is_dfa>
        char_state_machine;
    typedef std::basic_ostream<char_type> ostream;
    typedef lexertl::basic_rules<char_type, id_type> rules;
    typedef std::basic_string<char_type> string;

    static void dump(const sm &sm_, rules &rules_, ostream &stream_)
    {
        char_state_machine csm_;

        sm_to_csm(sm_, csm_);
        dump(csm_, rules_, stream_);
    }

    static void dump(const sm &sm_, ostream &stream_)
    {
        char_state_machine csm_;

        sm_to_csm(sm_, csm_);
        dump(csm_, stream_);
    }

    static void dump(const char_state_machine &csm_, rules &rules_,
        ostream &stream_)
    {
        for (std::size_t dfa_ = 0, dfas_ = csm_.size(); dfa_ < dfas_; ++dfa_)
        {
            lexer_state(stream_);
            stream_ << rules_.state(dfa_) << std::endl << std::endl;

            dump_ex(csm_._sm_deque[dfa_], stream_);
        }
    }

    static void dump(const char_state_machine &csm_, ostream &stream_)
    {
        for (std::size_t dfa_ = 0, dfas_ = csm_.size(); dfa_ < dfas_; ++dfa_)
        {
            lexer_state(stream_);
            stream_ << dfa_ << std::endl << std::endl;

            dump_ex(csm_._sm_deque[dfa_], stream_);
        }
    }

protected:
    typedef typename char_state_machine::state dfa_state;
    typedef typename dfa_state::string_token string_token;
    typedef std::basic_stringstream<char_type> stringstream;

    static void dump_ex(const typename char_state_machine::dfa &dfa_,
        ostream &stream_)
    {
        const std::size_t states_ = dfa_._states.size();
        const id_type bol_index_ = dfa_._bol_index;
        typename dfa_state::id_type_string_token_map::const_iterator iter_;
        typename dfa_state::id_type_string_token_map::const_iterator end_;

        for (std::size_t i_ = 0; i_ < states_; ++i_)
        {
            const dfa_state &state_ = dfa_._states[i_];

            state(stream_);
            stream_ << i_ << std::endl;

            if (state_._end_state)
            {
                end_state(stream_);

                if (state_._push_pop_dfa == dfa_state::push_dfa)
                {
                    push(stream_);
                    stream_ << state_._push_dfa;
                }
                else if (state_._push_pop_dfa == dfa_state::pop_dfa)
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
                stream_ << static_cast<std::size_t>(bol_index_) << std::endl;
            }

            if (state_._eol_index != char_state_machine::npos())
            {
                eol(stream_);
                stream_ << static_cast<std::size_t>(state_._eol_index) <<
                    std::endl;
            }

            iter_ = state_._transitions.begin();
            end_ = state_._transitions.end();

            for (; iter_ != end_; ++iter_)
            {
                string_token token_ = iter_->second;

                open_bracket(stream_);

                if (!iter_->second.any() && iter_->second.negatable())
                {
                    token_.negate();
                    negated(stream_);
                }

                string chars_;
                typename string_token::range_vector::const_iterator
                    ranges_iter_ = token_._ranges.begin();
                typename string_token::range_vector::const_iterator
                    ranges_end_ = token_._ranges.end();

                for (; ranges_iter_ != ranges_end_; ++ranges_iter_)
                {
                    if (ranges_iter_->first == '-' ||
                        ranges_iter_->first == '^' ||
                        ranges_iter_->first == ']')
                    {
                        stream_ << '\\';
                    }

                    chars_ = string_token::escape_char
                        (ranges_iter_->first);

                    if (ranges_iter_->first != ranges_iter_->second)
                    {
                        if (ranges_iter_->first + 1 < ranges_iter_->second)
                        {
                            chars_ += '-';
                        }

                        if (ranges_iter_->second == '-' ||
                            ranges_iter_->second == '^' ||
                            ranges_iter_->second == ']')
                        {
                            stream_ << '\\';
                        }

                        chars_ += string_token::escape_char
                            (ranges_iter_->second);
                    }

                    stream_ << chars_;
                }

                close_bracket(stream_);
                stream_ << static_cast<std::size_t>(iter_->first) <<
                    std::endl;
            }

            stream_ << std::endl;
        }
    }

    static void lexer_state(std::ostream &stream_)
    {
        stream_ << "Lexer state: ";
    }

    static void lexer_state(std::wostream &stream_)
    {
        stream_ << L"Lexer state: ";
    }

    static void state(std::ostream &stream_)
    {
        stream_ << "State: ";
    }

    static void state(std::wostream &stream_)
    {
        stream_ << L"State: ";
    }

    static void bol(std::ostream &stream_)
    {
        stream_ << "  BOL -> ";
    }

    static void bol(std::wostream &stream_)
    {
        stream_ << L"  BOL -> ";
    }

    static void eol(std::ostream &stream_)
    {
        stream_ << "  EOL -> ";
    }

    static void eol(std::wostream &stream_)
    {
        stream_ << L"  EOL -> ";
    }

    static void end_state(std::ostream &stream_)
    {
        stream_ << "  END STATE";
    }

    static void end_state(std::wostream &stream_)
    {
        stream_ << L"  END STATE";
    }

    static void id(std::ostream &stream_)
    {
        stream_ << ", Id = ";
    }

    static void id(std::wostream &stream_)
    {
        stream_ << L", Id = ";
    }

    static void push(std::ostream &stream_)
    {
        stream_ << ", PUSH ";
    }

    static void push(std::wostream &stream_)
    {
        stream_ << L", PUSH ";
    }

    static void pop(std::ostream &stream_)
    {
        stream_ << ", POP";
    }

    static void pop(std::wostream &stream_)
    {
        stream_ << L", POP";
    }

    static void user_id(std::ostream &stream_)
    {
        stream_ << ", User Id = ";
    }

    static void user_id(std::wostream &stream_)
    {
        stream_ << L", User Id = ";
    }

    static void open_bracket(std::ostream &stream_)
    {
        stream_ << "  [";
    }

    static void open_bracket(std::wostream &stream_)
    {
        stream_ << L"  [";
    }

    static void negated(std::ostream &stream_)
    {
        stream_ << "^";
    }

    static void negated(std::wostream &stream_)
    {
        stream_ << L"^";
    }

    static void close_bracket(std::ostream &stream_)
    {
        stream_ << "] -> ";
    }

    static void close_bracket(std::wostream &stream_)
    {
        stream_ << L"] -> ";
    }

    static void dfa(std::ostream &stream_)
    {
        stream_ << ", dfa = ";
    }

    static void dfa(std::wostream &stream_)
    {
        stream_ << L", dfa = ";
    }
};

typedef basic_debug<basic_state_machine<char>, char> debug;
typedef basic_debug<basic_state_machine<wchar_t>, wchar_t> wdebug;
}

#endif
