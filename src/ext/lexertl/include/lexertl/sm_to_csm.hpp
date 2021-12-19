// sm_to_csm.hpp
// Copyright (c) 2015-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_SM_TO_CSM_HPP
#define LEXERTL_SM_TO_CSM_HPP

#include "enums.hpp"
#include "observer_ptr.hpp"
#include <cstddef>

namespace lexertl
{
    template<typename sm, typename char_state_machine>
    void sm_to_csm(const sm& sm_, char_state_machine& csm_)
    {
        using id_type = typename sm::traits::id_type;
        using internals = typename sm::internals;
        using string_token = typename char_state_machine::state::string_token;
        using index_type = typename string_token::index_type;
        using string_token_vector =
            typename char_state_machine::string_token_vector;
        const internals& internals_ = sm_.data();
        const std::size_t dfas_ = internals_._dfa.size();

        for (id_type i_ = 0; i_ < dfas_; ++i_)
        {
            if (internals_._dfa_alphabet[i_] == 0) continue;

            const std::size_t alphabet_ = internals_._dfa_alphabet[i_] -
                transitions_index;
            string_token_vector token_vector_(alphabet_, string_token());
            observer_ptr<const id_type> ptr_ = &internals_._lookup[i_].front();

            for (std::size_t c_ = 0; c_ < 256; ++c_, ++ptr_)
            {
                if (*ptr_ >= transitions_index)
                {
                    string_token& token_ = token_vector_
                        [*ptr_ - transitions_index];

                    token_.insert(typename string_token::range
                    (index_type(c_), index_type(c_)));
                }
            }

            csm_.append(token_vector_, internals_, i_);
        }
    }
}

#endif
