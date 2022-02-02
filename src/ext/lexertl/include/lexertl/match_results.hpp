// match_results.hpp
// Copyright (c) 2015-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_MATCH_RESULTS_HPP
#define LEXERTL_MATCH_RESULTS_HPP

#include "char_traits.hpp"
#include "enums.hpp"
#include <iterator>
#include <stack>
#include <string>

namespace lexertl
{
    template<typename iter, typename id_t = uint16_t,
        std::size_t flags = bol_bit | eol_bit | skip_bit | again_bit |
        multi_state_bit | advance_bit>
        struct match_results
    {
        using id_type = id_t;
        using iter_type = iter;
        using char_type = typename std::iterator_traits<iter_type>::value_type;
        using index_type = typename basic_char_traits<char_type>::index_type;
        using string = std::basic_string<char_type>;

        id_type id;
        id_type user_id;
        iter_type first;
        iter_type second;
        iter_type eoi;
        bool bol;
        id_type state;

        match_results() :
            id(0),
            user_id(npos()),
            first(iter_type()),
            second(iter_type()),
            eoi(iter_type()),
            bol(true),
            state(0)
        {
        }

        match_results(const iter_type& start_, const iter_type& end_,
            const bool bol_ = true, const id_type state_ = 0) :
            id(0),
            user_id(npos()),
            first(start_),
            second(start_),
            eoi(end_),
            bol(bol_),
            state(state_)
        {
        }

        virtual ~match_results() = default;

        string str() const
        {
            return string(first, second);
        }

        string substr(const std::size_t soffset_,
            const std::size_t eoffset_) const
        {
            return string(first + soffset_, second - eoffset_);
        }

        virtual void clear()
        {
            id = 0;
            user_id = npos();
            first = eoi;
            second = eoi;
            bol = true;
            state = 0;
        }

        virtual void reset(const iter_type& start_, const iter_type& end_)
        {
            id = 0;
            user_id = npos();
            first = start_;
            second = start_;
            eoi = end_;
            bol = true;
            state = 0;
        }

        std::size_t size() const
        {
            return second - first;
        }

        static id_type npos()
        {
            return static_cast<id_type>(~0);
        }

        static id_type skip()
        {
            return static_cast<id_type>(~1);
        }

        bool operator ==(const match_results& rhs_) const
        {
            return id == rhs_.id &&
                user_id == rhs_.user_id &&
                first == rhs_.first &&
                second == rhs_.second &&
                eoi == rhs_.eoi &&
                bol == rhs_.bol &&
                state == rhs_.state;
        }
    };

    template<typename iter, typename id_type = uint16_t,
        std::size_t flags = bol_bit | eol_bit | skip_bit | again_bit |
        multi_state_bit | recursive_bit | advance_bit>
        struct recursive_match_results :
        public match_results<iter, id_type, flags>
    {
        using id_type_pair = std::pair<id_type, id_type>;
        std::stack<id_type_pair> stack;

        recursive_match_results() :
            match_results<iter, id_type, flags>(),
            stack()
        {
        }

        recursive_match_results(const iter& start_, const iter& end_,
            const bool bol_ = true, const id_type state_ = 0) :
            match_results<iter, id_type, flags>(start_, end_, bol_, state_),
            stack()
        {
        }

        virtual ~recursive_match_results() override
        {
        }

        virtual void clear() override
        {
            match_results<iter, id_type, flags>::clear();

            while (!stack.empty()) stack.pop();
        }

        virtual void reset(const iter& start_, const iter& end_) override
        {
            match_results<iter, id_type, flags>::reset(start_, end_);

            while (!stack.empty()) stack.pop();
        }
    };

    using smatch = match_results<std::string::const_iterator>;
    using cmatch = match_results<const char*>;
    using wsmatch = match_results<std::wstring::const_iterator>;
    using wcmatch = match_results<const wchar_t*>;
    using u32smatch = match_results<std::u32string::const_iterator>;
    using u32cmatch = match_results<const char32_t*>;

    using srmatch =
        recursive_match_results<std::string::const_iterator>;
    using crmatch = recursive_match_results<const char*>;
    using wsrmatch =
        recursive_match_results<std::wstring::const_iterator>;
    using wcrmatch = recursive_match_results<const wchar_t*>;
    using u32srmatch =
        recursive_match_results<std::u32string::const_iterator>;
    using u32crmatch = recursive_match_results<const char32_t*>;
}

#endif
