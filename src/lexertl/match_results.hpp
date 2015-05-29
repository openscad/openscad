// match_results.hpp
// Copyright (c) 2015 Ben Hanson (http://www.benhanson.net/)
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
template<typename iter, typename id_type = std::size_t,
    std::size_t flags = bol_bit | eol_bit | skip_bit | again_bit |
        multi_state_bit | advance_bit>
struct match_results
{
    typedef iter iter_type;
    typedef typename std::iterator_traits<iter_type>::value_type char_type;
    typedef typename basic_char_traits<char_type>::index_type index_type;
    typedef std::basic_string<char_type> string;

    id_type id;
    id_type user_id;
    iter_type start;
    iter_type end;
    iter_type eoi;
    bool bol;
    id_type state;

    match_results() :
        id(0),
        user_id(npos()),
        start(iter_type()),
        end(iter_type()),
        eoi(iter_type()),
        bol(true),
        state(0)
    {
    }

    match_results(const iter_type &start_, const iter_type &end_) :
        id(0),
        user_id(npos()),
        start(start_),
        end(start_),
        eoi(end_),
        bol(true),
        state(0)
    {
    }

    virtual ~match_results()
    {
    }

    string str() const
    {
        return string(start, end);
    }

    string substr(const std::size_t soffset_, const std::size_t eoffset_) const
    {
        return string(start + soffset_, end - eoffset_);
    }

    virtual void clear()
    {
        id  = 0;
        user_id = npos();
        start = eoi;
        end = eoi;
        bol = true;
        state = 0;
    }

    virtual void reset(const iter_type &start_, const iter_type &end_)
    {
        id  = 0;
        user_id = npos();
        start = start_;
        end  = start_;
        eoi = end_;
        bol = true;
        state = 0;
    }

    static id_type npos()
    {
        return ~static_cast<id_type>(0);
    }

    static id_type skip()
    {
        return ~static_cast<id_type>(1);
    }

    bool operator ==(const match_results &rhs_) const
    {
        return id == rhs_.id &&
            user_id == rhs_.user_id &&
            start == rhs_.start &&
            end == rhs_.end &&
            eoi == rhs_.eoi &&
            bol == rhs_.bol &&
            state == rhs_.state;
    }
};

template<typename iter, typename id_type = std::size_t,
    std::size_t flags = bol_bit | eol_bit | skip_bit | again_bit |
        multi_state_bit | recursive_bit | advance_bit>
struct recursive_match_results : public match_results<iter, id_type, flags>
{
    typedef std::pair<id_type, id_type> id_type_pair;
    std::stack<id_type_pair> stack;

    recursive_match_results() :
        match_results<iter, id_type, flags>(),
        stack()
    {
    }

    recursive_match_results(const iter &start_, const iter &end_) :
        match_results<iter, id_type, flags>(start_, end_),
        stack()
    {
    }

    virtual ~recursive_match_results()
    {
    }

    virtual void clear()
    {
        match_results<iter, id_type, flags>::clear();

        while (!stack.empty()) stack.pop();
    }

    virtual void reset(const iter &start_, const iter &end_)
    {
        match_results<iter, id_type, flags>::reset(start_, end_);

        while (!stack.empty()) stack.pop();
    }
};

typedef match_results<std::string::const_iterator> smatch;
typedef match_results<const char *> cmatch;
typedef match_results<std::wstring::const_iterator> wsmatch;
typedef match_results<const wchar_t *> wcmatch;

typedef recursive_match_results<std::string::const_iterator>
    srmatch;
typedef recursive_match_results<const char *> crmatch;
typedef recursive_match_results<std::wstring::const_iterator>
    wsrmatch;
typedef recursive_match_results<const wchar_t *> wcrmatch;
}

#endif
