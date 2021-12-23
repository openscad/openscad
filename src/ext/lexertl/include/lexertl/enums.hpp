// enums.hpp
// Copyright (c) 2005-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LEXERTL_ENUMS_HPP
#define LEXERTL_ENUMS_HPP

namespace lexertl
{
    enum regex_flags
    {
        icase = 1, dot_not_newline = 2, dot_not_cr_lf = 4,
        skip_ws = 8, match_zero_len = 16
    };
    // 0 = end state, 1 = id, 2 = user id, 3 = push_dfa_index
    // 4 = next dfa, 5 = dead state, 6 = dfa_start
    enum
    {
        end_state_index, id_index, user_id_index, push_dfa_index,
        next_dfa_index, eol_index, dead_state_index, transitions_index
    };
    // Rule flags:
    enum feature_flags
    {
        bol_bit = 1, eol_bit = 2, skip_bit = 4, again_bit = 8,
        multi_state_bit = 16, recursive_bit = 32, advance_bit = 64
    };
    // End state flags:
    enum { end_state_bit = 1, pop_dfa_bit = 2 };
}

#endif
