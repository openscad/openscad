// tokeniser_state.hpp
// Copyright (c) 2005-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_RE_TOKENISER_STATE_HPP
#define LEXERTL_RE_TOKENISER_STATE_HPP

#include "../../char_traits.hpp"
#include "../../enums.hpp"
#include <locale>
#include "../../narrow.hpp"
#include <stack>

namespace lexertl
{
    namespace detail
    {
        template<typename ch_type, typename id_type>
        struct basic_re_tokeniser_state
        {
            using char_type = ch_type;
            using index_type =
                typename basic_char_traits<char_type>::index_type;

            const char_type* const _start;
            const char_type* const _end;
            const char_type* _curr;
            id_type _id;
            std::size_t _flags;
            std::stack<std::size_t> _flags_stack;
            std::locale _locale;
            const char_type* _macro_name;
            long _paren_count;
            bool _in_string;
            id_type _nl_id;

            basic_re_tokeniser_state(const char_type* start_,
                const char_type* const end_, id_type id_,
                const std::size_t flags_, const std::locale locale_,
                const char_type* macro_name_) :
                _start(start_),
                _end(end_),
                _curr(start_),
                _id(id_),
                _flags(flags_),
                _flags_stack(),
                _locale(locale_),
                _macro_name(macro_name_),
                _paren_count(0),
                _in_string(false),
                _nl_id(static_cast<id_type>(~0))
            {
            }

            basic_re_tokeniser_state(const basic_re_tokeniser_state& rhs_)
            {
                assign(rhs_);
            }

            // prevent VC++ 7.1 warning:
            const basic_re_tokeniser_state& operator =
                (const basic_re_tokeniser_state& rhs_)
            {
                return assign(rhs_);
            }

            basic_re_tokeniser_state&
                assign(const basic_re_tokeniser_state& rhs_)
            {
                _start = rhs_._start;
                _end = rhs_._end;
                _curr = rhs_._curr;
                _id = rhs_._id;
                _flags = rhs_._flags;
                _flags_stack = rhs_._flags_stack;
                _locale = rhs_._locale;
                _macro_name = rhs_._macro_name;
                _paren_count = rhs_._paren_count;
                _in_string = rhs_._in_string;
                _nl_id = rhs_._nl_id;
                return *this;
            }

            inline bool next(char_type& ch_)
            {
                if (_curr >= _end)
                {
                    ch_ = 0;
                    return true;
                }
                else
                {
                    ch_ = *_curr;
                    increment();
                    return false;
                }
            }

            inline void increment()
            {
                ++_curr;
            }

            inline std::size_t index()
            {
                return _curr - _start;
            }

            inline bool eos()
            {
                return _curr >= _end;
            }

            inline void unexpected_end(std::ostringstream& ss_)
            {
                ss_ << "Unexpected end of regex";
            }

            inline void error(std::ostringstream& ss_)
            {
                ss_ << " in ";

                if (_macro_name)
                {
                    ss_ << "MACRO '";
                    narrow(_macro_name, ss_);
                    ss_ << "'.";
                }
                else
                {
                    ss_ << "rule id " << _id << '.';
                }
            }
        };
    }
}

#endif
