// charset.hpp
// Copyright (c) 2005-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LEXERTL_CHARSET_HPP
#define LEXERTL_CHARSET_HPP

#include <algorithm>
#include <iterator>
#include <set>
#include "../string_token.hpp"

namespace lexertl
{
    namespace detail
    {
        template<typename char_type, typename id_type>
        struct basic_charset
        {
            using token = basic_string_token<char_type>;
            using index_set = std::set<id_type>;

            token _token;
            index_set _index_set;

            basic_charset() :
                _token(),
                _index_set()
            {
            }

            basic_charset(const token& token_, const id_type index_) :
                _token(token_),
                _index_set()
            {
                _index_set.insert(index_);
            }

            bool empty() const
            {
                return _token.empty() && _index_set.empty();
            }

            void intersect(basic_charset& rhs_, basic_charset& overlap_)
            {
                _token.intersect(rhs_._token, overlap_._token);

                if (!overlap_._token.empty())
                {
                    std::merge(_index_set.begin(), _index_set.end(),
                        rhs_._index_set.begin(), rhs_._index_set.end(),
                        std::inserter(overlap_._index_set,
                            overlap_._index_set.end()));

                    if (_token.empty())
                    {
                        _index_set.clear();
                    }

                    if (rhs_._token.empty())
                    {
                        rhs_._index_set.clear();
                    }
                }
            }
        };
    }
}

#endif
