// state_machine.hpp
// Copyright (c) 2005-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_STATE_MACHINE_HPP
#define LEXERTL_STATE_MACHINE_HPP

#include "compile_assert.hpp"
// memcmp()
#include <cstring>
#include <deque>
#include "internals.hpp"
#include <map>
#include <set>
#include "sm_traits.hpp"
#include "string_token.hpp"

namespace lexertl
{
template<typename char_type, typename id_type = std::size_t>
class basic_state_machine
{
public:
    typedef basic_sm_traits<char_type, id_type,
        (sizeof(char_type) > 1), true, true> traits;
    typedef detail::basic_internals<id_type> internals;

    // If you get a compile error here you have
    // failed to define an unsigned id type.
    // g++ 4.4.7 won't compile this:
#if !defined(__GNUC__) || __GNUC__ > 4 || __GNUC__ == 4 && __GNUC__MINIOR__ > 4
    compile_assert<(~static_cast<id_type>(0) > 0)> _valid_id_type;
#endif

    basic_state_machine() :
#if !defined(__GNUC__) || __GNUC__ > 4 || __GNUC__ == 4 && __GNUC__MINIOR__ > 4
        _valid_id_type(),
#endif
        _internals()
    {
    }

    void clear()
    {
        _internals.clear();
    }

    internals &data()
    {
        return _internals;
    }

    const internals &data() const
    {
        return _internals;
    }

    bool empty() const
    {
        return _internals.empty();
    }

    id_type eoi() const
    {
        return _internals._eoi;
    }

    void minimise()
    {
        const id_type dfas_ = static_cast<id_type>(_internals.
            _dfa->size());

        for (id_type i_ = 0; i_ < dfas_; ++i_)
        {
            const id_type dfa_alphabet_ = _internals._dfa_alphabet[i_];
            id_type_vector *dfa_ = _internals._dfa[i_];

            if (dfa_alphabet_ != 0)
            {
                std::size_t size_ = 0;

                do
                {
                    size_ = dfa_->size();
                    minimise_dfa(dfa_alphabet_, *dfa_, size_);
                } while (dfa_->size() != size_);
            }
        }
    }

    static id_type npos()
    {
        return ~static_cast<id_type>(0);
    }

    static id_type skip()
    {
        return ~static_cast<id_type>(1);
    }

    void swap(basic_state_machine &rhs_)
    {
        _internals.swap(rhs_._internals);
    }

private:
    typedef typename internals::id_type_vector id_type_vector;
    typedef std::set<id_type> index_set;
    internals _internals;

    void minimise_dfa(const id_type dfa_alphabet_,
        id_type_vector &dfa_, std::size_t size_)
    {
        const id_type *first_ = &dfa_.front();
        const id_type *end_ = first_ + size_;
        id_type index_ = 1;
        id_type new_index_ = 1;
        id_type_vector lookup_(size_ / dfa_alphabet_, npos());
        id_type *lookup_ptr_ = &lookup_.front();
        index_set index_set_;
        const id_type bol_index_ = dfa_.front();

        *lookup_ptr_ = 0;
        // Only one 'jam' state, so skip it.
        first_ += dfa_alphabet_;

        for (; first_ < end_; first_ += dfa_alphabet_, ++index_)
        {
            const id_type *second_ = first_ + dfa_alphabet_;

            for (id_type curr_index_ = index_ + 1; second_ < end_;
                ++curr_index_, second_ += dfa_alphabet_)
            {
                if (index_set_.find(curr_index_) != index_set_.end())
                {
                    continue;
                }

                // Some systems have memcmp in namespace std.
                using namespace std;

                if (memcmp(first_, second_, sizeof(id_type) *
                    dfa_alphabet_) == 0)
                {
                    index_set_.insert(curr_index_);
                    lookup_ptr_[curr_index_] = new_index_;
                }
            }

            if (lookup_ptr_[index_] == npos())
            {
                lookup_ptr_[index_] = new_index_;
                ++new_index_;
            }
        }

        if (!index_set_.empty())
        {
            const id_type *front_ = &dfa_.front();
            id_type_vector new_dfa_(front_, front_ + dfa_alphabet_);
            typename index_set::const_iterator set_end_ = index_set_.end();
            const id_type *ptr_ = front_ + dfa_alphabet_;
            id_type *new_ptr_ = 0;

            new_dfa_.resize(size_ - index_set_.size() * dfa_alphabet_, 0);
            new_ptr_ = &new_dfa_.front() + dfa_alphabet_;
            size_ /= dfa_alphabet_;

            if (bol_index_)
            {
                new_dfa_.front() = lookup_ptr_[bol_index_];
            }

            for (index_ = 1; index_ < size_; ++index_)
            {
                if (index_set_.find(index_) != set_end_)
                {
                    ptr_ += dfa_alphabet_;
                    continue;
                }

                new_ptr_[end_state_index] = ptr_[end_state_index];
                new_ptr_[id_index] = ptr_[id_index];
                new_ptr_[user_id_index] = ptr_[user_id_index];
                new_ptr_[push_dfa_index] = ptr_[push_dfa_index];
                new_ptr_[next_dfa_index] = ptr_[next_dfa_index];
                new_ptr_[eol_index] = lookup_ptr_[ptr_[eol_index]];
                new_ptr_ += transitions_index;
                ptr_ += transitions_index;

                for (id_type i_ = transitions_index; i_ < dfa_alphabet_; ++i_)
                {
                    *new_ptr_++ = lookup_ptr_[*ptr_++];
                }
            }

            dfa_.swap(new_dfa_);
        }
    }
};

typedef basic_state_machine<char> state_machine;
typedef basic_state_machine<wchar_t> wstate_machine;

template<typename char_type, typename id_type = std::size_t,
    bool is_dfa = true>
struct basic_char_state_machine
{
    typedef basic_sm_traits<char_type, id_type, false, false, is_dfa> traits;
    typedef detail::basic_internals<id_type> internals;
    typedef typename internals::id_type_vector id_type_vector;

    struct state
    {
        typedef basic_string_token<char_type> string_token;
        typedef std::map<id_type, string_token> id_type_string_token_map;
        typedef std::pair<id_type, string_token> id_type_string_token_pair;
        enum push_pop_dfa {neither, push_dfa, pop_dfa};

        bool _end_state;
        push_pop_dfa _push_pop_dfa;
        id_type _id;
        id_type _user_id;
        id_type _push_dfa;
        id_type _next_dfa;
        id_type _eol_index;
        id_type_string_token_map _transitions;

        state() :
            _end_state(false),
            _push_pop_dfa(neither),
            _id(0),
            _user_id(traits::npos()),
            _push_dfa(traits::npos()),
            _next_dfa(0),
            _eol_index(traits::npos()),
            _transitions()
        {
        }

        bool operator ==(const state rhs_) const
        {
            return _end_state == rhs_._end_state &&
                _push_pop_dfa == rhs_._push_pop_dfa &&
                _id == rhs_._id &&
                _user_id == rhs_._user_id &&
                _push_dfa == rhs_._push_dfa &&
                _next_dfa == rhs_._next_dfa &&
                _eol_index == rhs_._eol_index &&
                _transitions == rhs_._transitions;
        }
    };

    typedef typename state::string_token string_token;
    typedef std::vector<state> state_vector;
    typedef std::vector<string_token> string_token_vector;
    typedef typename state::id_type_string_token_pair
        id_type_string_token_pair;

    struct dfa
    {
        id_type _bol_index;
        state_vector _states;

        dfa(const std::size_t size_) :
            _bol_index(traits::npos()),
            _states(state_vector(size_))
        {
        }

        std::size_t size() const
        {
            return _states.size();
        }

        void swap(dfa &rhs_)
        {
            std::swap(_bol_index, rhs_._bol_index);
            _states.swap(rhs_._states);
        }
    };

    typedef std::deque<dfa> dfa_deque;

    // g++ 4.4.7 won't compile this:
#if !defined(__GNUC__) || __GNUC__ > 4 || __GNUC__ == 4 && __GNUC__MINIOR__ > 4
    // If you get a compile error here you have
    // failed to define an unsigned id type.
    compile_assert<(~static_cast<id_type>(0) > 0)> _valid_id_type;
#endif
    dfa_deque _sm_deque;

    basic_char_state_machine() :
#if !defined(__GNUC__) || __GNUC__ > 4 || __GNUC__ == 4 && __GNUC__MINIOR__ > 4
        _valid_id_type(),
#endif
        _sm_deque()
    {
    }

    void append(const string_token_vector &token_vector_,
        const internals &internals_, const id_type dfa_index_)
    {
        const std::size_t dfa_alphabet_ = internals_._dfa_alphabet[dfa_index_];
        const std::size_t alphabet_ = dfa_alphabet_ - transitions_index;
        const id_type_vector &source_dfa_ = *internals_._dfa[dfa_index_];
        const id_type *ptr_ = &source_dfa_.front();
        const std::size_t size_ = (source_dfa_.size() - dfa_alphabet_) /
            dfa_alphabet_;
        typename state::id_type_string_token_map::iterator trans_iter_;

        _sm_deque.push_back(dfa(size_));

        dfa &dest_dfa_ = _sm_deque.back();

        if (*ptr_)
        {
            dest_dfa_._bol_index = *ptr_ - 1;
        }

        ptr_ += dfa_alphabet_;

        for (id_type i_ = 0; i_ < size_; ++i_)
        {
            state &state_ = dest_dfa_._states[i_];

            state_._end_state = ptr_[end_state_index] != 0;

            if (ptr_[push_dfa_index] != npos())
            {
                state_._push_pop_dfa = state::push_dfa;
            }
            else if (ptr_[end_state_index] & pop_dfa_bit)
            {
                state_._push_pop_dfa = state::pop_dfa;
            }

            state_._id = ptr_[id_index];
            state_._user_id = ptr_[user_id_index];
            state_._push_dfa = ptr_[push_dfa_index];
            state_._next_dfa = ptr_[next_dfa_index];

            if (ptr_[eol_index])
            {
                state_._eol_index = ptr_[eol_index] - 1;
            }

            ptr_ += transitions_index;

            for (id_type col_index_ = 0; col_index_ < alphabet_;
                ++col_index_, ++ptr_)
            {
                const id_type next_ = *ptr_;

                if (next_ > 0)
                {
                    trans_iter_ = state_._transitions.find(next_ - 1);

                    if (trans_iter_ == state_._transitions.end())
                    {
                        trans_iter_ = state_._transitions.insert
                            (id_type_string_token_pair(next_ - 1,
                            token_vector_[col_index_])).first;
                    }
                    else
                    {
                        trans_iter_->second.insert(token_vector_[col_index_]);
                    }
                }
            }
        }
    }

    void clear()
    {
        _sm_deque.clear();
    }

    bool empty() const
    {
        return _sm_deque.empty();
    }

    void minimise()
    {
        const id_type dfas_ = static_cast<id_type>(_sm_deque.size());

        for (id_type i_ = 0; i_ < dfas_; ++i_)
        {
            dfa *dfa_ = &_sm_deque[i_];

            if (dfa_->size() > 0)
            {
                std::size_t size_ = 0;

                do
                {
                    size_ = dfa_->size();
                    minimise_dfa(*dfa_, size_);
                } while (dfa_->size() != size_);
            }
        }
    }

    static id_type npos()
    {
        return traits::npos();
    }

    id_type size() const
    {
        return static_cast<id_type>(_sm_deque.size());
    }

    static id_type skip()
    {
        return ~static_cast<id_type>(1);
    }

    void swap(basic_char_state_machine &csm_)
    {
        _sm_deque.swap(csm_._sm_deque);
    }

private:
    typedef std::set<id_type> index_set;

    void minimise_dfa(dfa &dfa_, std::size_t size_)
    {
        const state *first_ = &dfa_._states.front();
        const state *end_ = first_ + size_;
        id_type index_ = 0;
        id_type new_index_ = 0;
        id_type_vector lookup_(size_, npos());
        id_type *lookup_ptr_ = &lookup_.front();
        index_set index_set_;

        for (; first_ != end_; ++first_, ++index_)
        {
            const state *second_ = first_ + 1;

            for (id_type curr_index_ = index_ + 1; second_ != end_;
                ++curr_index_, ++second_)
            {
                if (index_set_.find(curr_index_) != index_set_.end())
                {
                    continue;
                }

                if (*first_ == *second_)
                {
                    index_set_.insert(curr_index_);
                    lookup_ptr_[curr_index_] = new_index_;
                }
            }

            if (lookup_ptr_[index_] == npos())
            {
                lookup_ptr_[index_] = new_index_;
                ++new_index_;
            }
        }

        if (!index_set_.empty())
        {
            const state *front_ = &dfa_._states.front();
            dfa new_dfa_(new_index_);
            typename index_set::const_iterator set_end_ = index_set_.end();
            const state *ptr_ = front_;
            state *new_ptr_ = &new_dfa_._states.front();

            if (dfa_._bol_index != npos())
            {
                new_dfa_._bol_index = lookup_ptr_[dfa_._bol_index];
            }

            for (index_ = 0; index_ < size_; ++index_)
            {
                if (index_set_.find(index_) != set_end_)
                {
                    ++ptr_;
                    continue;
                }

                new_ptr_->_end_state = ptr_->_end_state;
                new_ptr_->_id = ptr_->_end_state;
                new_ptr_->_user_id = ptr_->_user_id;
                new_ptr_->_next_dfa = ptr_->_next_dfa;

                if (ptr_->_eol_index != npos())
                {
                    new_ptr_->_eol_index = lookup_ptr_[ptr_->_eol_index];
                }

                typename state::id_type_string_token_map::const_iterator
                    iter_ = ptr_->_transitions.begin();
                typename state::id_type_string_token_map::const_iterator end_ =
                    ptr_->_transitions.end();
                typename state::id_type_string_token_map::iterator find_;

                for (; iter_ != end_; ++iter_)
                {
                    find_ = new_ptr_->_transitions.find
                        (lookup_ptr_[iter_->first]);

                    if (find_ == new_ptr_->_transitions.end())
                    {
                        new_ptr_->_transitions.insert
                            (id_type_string_token_pair
                            (lookup_ptr_[iter_->first], iter_->second));
                    }
                    else
                    {
                        find_->second.insert(iter_->second);
                    }
                }

                ++ptr_;
                ++new_ptr_;
            }

            dfa_.swap(new_dfa_);
        }
    }
};

typedef basic_char_state_machine<char> char_state_machine;
typedef basic_char_state_machine<wchar_t> wchar_state_machine;
}

#endif
