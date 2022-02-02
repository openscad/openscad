// re_token.hpp
// Copyright (c) 2005-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_RE_TOKEN_HPP
#define LEXERTL_RE_TOKEN_HPP

#include "../../string_token.hpp"

namespace lexertl
{
    namespace detail
    {
        // Note that tokens following END are never seen by parser.hpp.
        enum class token_type
        {
            BEGIN, REGEX, OREXP, SEQUENCE, SUB, EXPRESSION, REPEAT,
            DUP, OR, CHARSET, BOL, EOL, MACRO, OPENPAREN, CLOSEPAREN, OPT, AOPT,
            ZEROORMORE, AZEROORMORE, ONEORMORE, AONEORMORE, REPEATN, AREPEATN,
            END, DIFF
        };

        template<typename input_char_type, typename char_type>
        struct basic_re_token
        {
            using string_token = basic_string_token<char_type>;
            using string = std::basic_string<input_char_type>;

            token_type _type;
            string _extra;
            string_token _str;

            basic_re_token(const token_type type_ = token_type::BEGIN) :
                _type(type_),
                _extra(),
                _str()
            {
            }

            void clear()
            {
                _type = token_type::BEGIN;
                _extra.clear();
                _str.clear();
            }

            void swap(basic_re_token& rhs_)
            {
                std::swap(_type, rhs_._type);
                _extra.swap(rhs_._extra);
                _str.swap(rhs_._str);
            }

            char precedence(const token_type type_) const
            {
                // Moved in here for Solaris compiler.
                static const char precedence_table_
                    [static_cast<int>(token_type::END) + 1]
                    [static_cast<int>(token_type::END) + 1] =
                {
                    //        BEG, REG, ORE, SEQ, SUB, EXP, RPT, DUP,  | , CHR, BOL, EOL, MCR,  ( ,  ) ,  ? , ?? ,  * , *? ,  + , +?, {n}?, {n}, END
                    /*BEGIN*/{ ' ', '<', '<', '<', '<', '<', '<', ' ', ' ', '<', '<', '<', '<', '<', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>' },
                    /*REGEX*/{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '=', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>' },
                    /*OREXP*/{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '=', '>', '>', '>', '>', ' ', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>' },
                    /* SEQ */{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', ' ', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>' },
                    /* SUB */{ ' ', ' ', ' ', ' ', ' ', '=', '<', ' ', '>', '<', '<', '<', '<', '<', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>' },
                    /*EXPRE*/{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>' },
                    /* RPT */{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', '=', '>', '>', '>', '>', '>', '>', '>', '<', '<', '<', '<', '<', '<', '<', '<', '>' },
                    /*DUPLI*/{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>' },
                    /*  |  */{ ' ', ' ', ' ', '=', '<', '<', '<', ' ', ' ', '<', '<', '<', '<', '<', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
                    /*CHARA*/{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>' },
                    /* BOL */{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>' },
                    /* EOL */{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>' },
                    /*MACRO*/{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>' },
                    /*  (  */{ ' ', '=', '<', '<', '<', '<', '<', ' ', ' ', '<', '<', '<', '<', '<', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
                    /*  )  */{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>' },
                    /*  ?  */{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', '<', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>' },
                    /* ??  */{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>' },
                    /*  *  */{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', '<', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>' },
                    /* *?  */{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>' },
                    /*  +  */{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', '<', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>' },
                    /* +?  */{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>' },
                    /*{n,m}*/{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', '<', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>' },
                    /*{nm}?*/{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>' },
                    /* END */{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' }
                };

                return precedence_table_[static_cast<int>(_type)]
                    [static_cast<int>(type_)];
            }

            const char* precedence_string() const
            {
                // Moved in here for Solaris compiler.
                static const char* precedence_strings_
                    [static_cast<int>(token_type::END) + 1] =
                {
                    "BEGIN", "REGEX", "OREXP", "SEQUENCE", "SUB", "EXPRESSION",
                    "REPEAT", "DUPLICATE", "|", "CHARSET", "^", "$", "MACRO",
                    "(", ")", "?", "??", "*", "*?", "+", "+?", "{n[,[m]]}",
                    "{n[,[m]]}?", "END"
                };

                return precedence_strings_[static_cast<int>(_type)];
            }
        };
    }
}

#endif
