// lookup.hpp
// Copyright (c) 2009-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_LOOKUP_HPP
#define LEXERTL_LOOKUP_HPP

#include <cassert>
#include "match_results.hpp"
#include <type_traits>

namespace lexertl
{
    namespace detail
    {
        template<bool>
        struct bol_state
        {
            bol_state(const bool)
            {
            }
        };

        template<>
        struct bol_state<true>
        {
            bool _bol;
            bool _end_bol;

            bol_state(const bool bol_) :
                _bol(bol_),
                _end_bol(bol_)
            {
            }
        };

        template<typename id_type, bool>
        struct eol_state
        {
        };

        template<typename id_type>
        struct eol_state<id_type, true>
        {
            id_type _EOL_state;

            eol_state() :
                _EOL_state(0)
            {
            }
        };

        template<typename id_type, bool>
        struct multi_state_state
        {
            multi_state_state(const id_type)
            {
            }
        };

        template<typename id_type>
        struct multi_state_state<id_type, true>
        {
            id_type _start_state;

            multi_state_state(const id_type state_) :
                _start_state(state_)
            {
            }
        };

        template<typename id_type, bool>
        struct recursive_state
        {
            recursive_state(const id_type*)
            {
            }
        };

        template<typename id_type>
        struct recursive_state<id_type, true>
        {
            bool _pop;
            id_type _push_dfa;

            recursive_state(const id_type* ptr_) :
                _pop((*ptr_& pop_dfa_bit) != 0),
                _push_dfa(*(ptr_ + push_dfa_index))
            {
            }
        };

        template<typename internals, typename id_type, typename index_type,
            std::size_t flags>
            struct lookup_state
        {
            const id_type* _lookup;
            id_type _dfa_alphabet;
            const id_type* _dfa;
            const id_type* _ptr;
            bool _end_state;
            id_type _id;
            id_type _uid;
            bol_state<(flags& bol_bit) != 0> _bol_state;
            eol_state<id_type, (flags& eol_bit) != 0> _eol_state;
            multi_state_state<id_type, (flags& multi_state_bit) != 0>
                _multi_state_state;
            recursive_state<id_type, (flags& recursive_bit) != 0>
                _recursive_state;

            lookup_state(const internals& internals_, const bool bol_,
                const id_type state_) :
                _lookup(&internals_._lookup[state_][0]),
                _dfa_alphabet(internals_._dfa_alphabet[state_]),
                _dfa(&internals_._dfa[state_][0]),
                _ptr(_dfa + _dfa_alphabet),
                _end_state(*_ptr != 0),
                _id(*(_ptr + id_index)),
                _uid(*(_ptr + user_id_index)),
                _bol_state(bol_),
                _eol_state(),
                _multi_state_state(state_),
                _recursive_state(_ptr)
            {
            }

            void reset_recursive(const std::false_type&)
            {
                // Do nothing
            }

            void reset_recursive(const std::true_type&)
            {
                _recursive_state._pop = (*_ptr & pop_dfa_bit) != 0;
                _recursive_state._push_dfa = *(_ptr + push_dfa_index);
            }

            void bol_start_state(const std::false_type&)
            {
                // Do nothing
            }

            void bol_start_state(const std::true_type&)
            {
                if (_bol_state._bol)
                {
                    const id_type state_ = *_dfa;

                    if (state_)
                    {
                        _ptr = &_dfa[state_ * _dfa_alphabet];
                    }
                }
            }

            template<typename char_type>
            bool is_eol(const char_type, const std::false_type&)
            {
                return false;
            }

            template<typename char_type>
            bool is_eol(const char_type curr_, const std::true_type&)
            {
                bool ret_ = false;

                _eol_state._EOL_state = _ptr[eol_index];
                ret_ = _eol_state._EOL_state &&
                    (curr_ == '\r' || curr_ == '\n');

                if (ret_)
                {
                    _ptr = &_dfa[_eol_state._EOL_state * _dfa_alphabet];
                }

                return ret_;
            }

            template<typename char_type>
            id_type next_char(const char_type prev_char_,
                const std::false_type&)
            {
                const id_type state_ = _ptr[_lookup
                    [static_cast<index_type>(prev_char_)]];

                if (state_ != 0)
                {
                    _ptr = &_dfa[state_ * _dfa_alphabet];
                }

                return state_;
            }

            template<typename char_type>
            id_type next_char(const char_type prev_char_, const std::true_type&)
            {
                const std::size_t bytes_ = sizeof(char_type) < 3 ?
                    sizeof(char_type) : 3;
                const std::size_t shift_[] = { 0, 8, 16 };
                id_type state_ = 0;

                for (std::size_t i_ = 0; i_ < bytes_; ++i_)
                {
                    state_ = _ptr[_lookup[static_cast<unsigned char>
                        ((prev_char_ >> shift_[bytes_ - 1 - i_]) & 0xff)]];

                    if (state_ == 0)
                    {
                        break;
                    }

                    _ptr = &_dfa[state_ * _dfa_alphabet];
                }

                return state_;
            }

            template<typename char_type>
            void bol(const char_type, const std::false_type&)
            {
                // Do nothing
            }

            template<typename char_type>
            void bol(const char_type prev_char_, const std::true_type&)
            {
                _bol_state._bol = prev_char_ == '\n';
            }

            void eol(const id_type, const std::false_type&)
            {
                // Do nothing
            }

            void eol(const id_type err_val_, const std::true_type&)
            {
                _eol_state._EOL_state = err_val_;
            }

            void reset_start_state(const std::false_type&)
            {
                // Do nothing
            }

            void reset_start_state(const std::true_type&)
            {
                _multi_state_state._start_state = *(_ptr + next_dfa_index);
            }

            void reset_end_bol(const std::false_type&)
            {
                // Do nothing
            }

            void reset_end_bol(const std::true_type&)
            {
                _bol_state._end_bol = _bol_state._bol;
            }

            template<typename iter_type>
            void end_state(iter_type& end_token_, iter_type& curr_)
            {
                if (*_ptr)
                {
                    _end_state = true;
                    reset_end_bol
                    (std::integral_constant<bool, (flags& bol_bit) != 0>());
                    _id = *(_ptr + id_index);
                    _uid = *(_ptr + user_id_index);
                    reset_recursive(std::integral_constant<bool,
                        (flags& recursive_bit) != 0>());
                    reset_start_state(std::integral_constant<bool,
                        (flags& multi_state_bit) != 0>());
                    end_token_ = curr_;
                }
            }

            template<typename iter_type, typename char_type>
            void check_eol(iter_type&, iter_type&, const id_type,
                const char_type, const std::false_type&)
            {
                // Do nothing
            }

            template<typename iter_type, typename char_type>
            void check_eol(iter_type& end_token_, iter_type& curr_,
                const id_type npos_, const char_type eoi_,
                const std::true_type&)
            {
                if (_eol_state._EOL_state != npos_ && curr_ == eoi_)
                {
                    _eol_state._EOL_state = _ptr[eol_index];

                    if (_eol_state._EOL_state)
                    {
                        _ptr = &_dfa[_eol_state._EOL_state * _dfa_alphabet];
                        end_state(end_token_, curr_);
                    }
                }
            }

            template<typename results>
            void pop(results&, const std::false_type&)
            {
                // Nothing to do
            }

            template<typename results>
            void pop(results& results_, const std::true_type&)
            {
                if (_recursive_state._pop)
                {
                    _multi_state_state._start_state =
                        results_.stack.top().first;
                    results_.stack.pop();
                }
                else if (_recursive_state._push_dfa != results::npos())
                {
                    results_.stack.push(typename results::id_type_pair
                    (_recursive_state._push_dfa, _id));
                }
            }

            template<typename results>
            bool is_id_eoi(const id_type eoi_, const results&,
                const std::false_type&)
            {
                return _id == eoi_;
            }

            template<typename results>
            bool is_id_eoi(const id_type eoi_, const results& results_,
                const std::true_type&)
            {
                return _id == eoi_ || (_recursive_state._pop &&
                    !results_.stack.empty() &&
                    results_.stack.top().second == eoi_);
            }

            void start_state(id_type&, const std::false_type&)
            {
                // Do nothing
            }

            void start_state(id_type& start_state_, const std::true_type&)
            {
                start_state_ = _multi_state_state._start_state;
            }

            void bol(bool&, const std::false_type&)
            {
                // Do nothing
            }

            void bol(bool& end_bol_, const std::true_type&)
            {
                end_bol_ = _bol_state._end_bol;
            }
        };

        template<typename results>
        void inc_end(results&, const std::false_type&)
        {
            // Do nothing
        }

        template<typename results>
        void inc_end(results& results_, const std::true_type&)
        {
            ++results_.second;
        }

        template<typename sm_type, std::size_t flags, typename results,
            bool compressed, bool recursive>
            void next(const sm_type& sm_, results& results_,
                const std::integral_constant<bool, compressed>& compressed_,
                const std::integral_constant<bool, recursive>& recursive_,
                const std::forward_iterator_tag&)
        {
            using id_type = typename sm_type::id_type;
            const auto& internals_ = sm_.data();
            auto end_token_ = results_.second;
        skip:
            auto curr_ = results_.second;

            results_.first = curr_;

        again:
            if (curr_ == results_.eoi)
            {
                results_.id = internals_._eoi;
                results_.user_id = results::npos();
                return;
            }

            lookup_state<typename sm_type::internals, id_type,
                typename results::index_type, flags> lu_state_
                (internals_, results_.bol, results_.state);
            lu_state_.bol_start_state
            (std::integral_constant<bool, (flags& bol_bit) != 0>());

            while (curr_ != results_.eoi)
            {
                if (!lu_state_.is_eol(*curr_,
                    std::integral_constant<bool, (flags & eol_bit) != 0>()))
                {
                    const auto prev_char_ = *curr_;
                    const id_type state_ = lu_state_.next_char(prev_char_,
                        compressed_);

                    lu_state_.bol(prev_char_,
                        std::integral_constant<bool, (flags& bol_bit) != 0>());

                    if (state_ == 0)
                    {
                        lu_state_.is_eol(results::npos(),
                            std::integral_constant<bool,
                            (flags& eol_bit) != 0>());
                        break;
                    }

                    ++curr_;
                }

                lu_state_.end_state(end_token_, curr_);
            }

            lu_state_.check_eol(end_token_, curr_,
                results::npos(), results_.eoi,
                std::integral_constant<bool, (flags& eol_bit) != 0>());

            if (lu_state_._end_state)
            {
                // Return longest match
                lu_state_.pop(results_, recursive_);

                lu_state_.start_state(results_.state,
                    std::integral_constant<bool,
                    (flags& multi_state_bit) != 0>());
                lu_state_.bol(results_.bol,
                    std::integral_constant<bool, (flags& bol_bit) != 0>());
                results_.second = end_token_;

                if (lu_state_._id == sm_.skip()) goto skip;

                if (lu_state_.is_id_eoi(internals_._eoi, results_, recursive_))
                {
                    curr_ = end_token_;
                    goto again;
                }
            }
            else
            {
                results_.second = end_token_;
                results_.bol = *results_.second == '\n';
                results_.first = results_.second;
                // No match causes char to be skipped
                inc_end(results_,
                    std::integral_constant<bool, (flags& advance_bit) != 0>());
                lu_state_._id = results::npos();
                lu_state_._uid = results::npos();
            }

            results_.id = lu_state_._id;
            results_.user_id = lu_state_._uid;
        }
    }

    template<typename iter_type, typename sm_type, std::size_t flags>
    void lookup(const sm_type& sm_, match_results<iter_type,
        typename sm_type::id_type, flags>& results_)
    {
        using value_type = typename std::iterator_traits<iter_type>::value_type;
        using cat = typename std::iterator_traits<iter_type>::iterator_category;

        // If this asserts, you have either not defined all the correct
        // flags, or you should be using recursive_match_results instead
        // of match_results.
        assert((sm_.data()._features & flags) == sm_.data()._features);
        detail::next<sm_type, flags>(sm_, results_,
            std::integral_constant<bool, (sizeof(value_type) > 1)>(),
            std::false_type(), cat());
    }

    template<typename iter_type, typename sm_type, std::size_t flags>
    void lookup(const sm_type& sm_, recursive_match_results<iter_type,
        typename sm_type::id_type, flags>& results_)
    {
        using value_type = typename std::iterator_traits<iter_type>::value_type;
        using cat = typename std::iterator_traits<iter_type>::iterator_category;

        // If this asserts, you have not defined all the correct flags
        assert((sm_.data()._features & flags) == sm_.data()._features);
        detail::next<sm_type, flags | recursive_bit>(sm_, results_,
            std::integral_constant<bool, (sizeof(value_type) > 1)>(),
            std::true_type(), cat());
    }
}

#endif
