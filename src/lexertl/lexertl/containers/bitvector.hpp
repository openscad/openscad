// bitvector.hpp
// Copyright (c) 2013-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LEXERTL_BITVECTOR_HPP
#define LEXERTL_BITVECTOR_HPP

#include <vector>

namespace lexertl
{
template<typename T>
class basic_bitvector
{
public:
    template<typename Ty>
    class reference
    {
    public:
        reference(Ty &block_, const std::size_t mask_) :
            _block(block_),
            _mask(mask_)
        {
        }

        operator bool() const
        {
            return (_block & _mask) != 0;
        }

        reference<Ty> &operator =(const bool bit_)
        {
            if (bit_)
            {
                _block |= _mask;
            }
            else
            {
                _block &= ~_mask;
            }

            return *this;
        }

        reference<Ty> &operator =(reference<Ty> &rhs_)
        {
            if (rhs_)
            {
                _block |= _mask;
            }
            else
            {
                _block &= ~_mask;
            }
        }

    private:
        Ty &_block;
        const std::size_t _mask;
    };

    basic_bitvector(const std::size_t size_) :
        _vec(block(size_) + (bit(size_) ? 1 : 0), 0)
    {
    }

    basic_bitvector(const basic_bitvector &rhs_) :
        _vec(rhs_._vec)
    {
    }

    basic_bitvector &operator =(const basic_bitvector &rhs_)
    {
        if (&rhs_ != this)
        {
            _vec = rhs_._vec;
        }

        return *this;
    }

    bool operator [](const std::size_t index_) const
    {
        return (_vec[block(index_)] & (1 << bit(index_))) != 0;
    }

    reference<T> operator [](const std::size_t index_)
    {
        return reference<T>(_vec[block(index_)], (1 << bit(index_)));
    }

    basic_bitvector<T> &operator |=(const basic_bitvector<T> &rhs_)
    {
        typename t_vector::iterator lhs_iter_ = _vec.begin();
        typename t_vector::iterator lhs_end_ = _vec.end();
        typename t_vector::const_iterator rhs_iter_ = rhs_._vec.begin();
        typename t_vector::const_iterator rhs_end_ = rhs_._vec.end();

        for (; lhs_iter_ != lhs_end_ && rhs_iter_ != rhs_end_;
            ++lhs_iter_, ++rhs_iter_)
        {
            *lhs_iter_ |= *rhs_iter_;
        }

        return *this;
    }

    basic_bitvector<T> &operator &=(const basic_bitvector<T> &rhs_)
    {
        typename t_vector::iterator lhs_iter_ = _vec.begin();
        typename t_vector::iterator lhs_end_ = _vec.end();
        typename t_vector::const_iterator rhs_iter_ = rhs_._vec.begin();
        typename t_vector::const_iterator rhs_end_ = rhs_._vec.end();

        for (; lhs_iter_ != lhs_end_ && rhs_iter_ != rhs_end_;
            ++lhs_iter_, ++rhs_iter_)
        {
            *lhs_iter_ &= *rhs_iter_;
        }

        return *this;
    }

    void clear()
    {
        typename t_vector::iterator iter_ = _vec.begin();
        typename t_vector::iterator end_ = _vec.end();

        for (; iter_ != end_; ++iter_)
        {
            *iter_ = 0;
        }
    }

    bool any() const
    {
        typename t_vector::const_iterator iter_ = _vec.begin();
        typename t_vector::const_iterator end_ = _vec.end();

        for (; iter_ != end_; ++iter_)
        {
            if (*iter_) break;
        }

        return iter_ != end_;
    }

    void negate()
    {
        typename t_vector::iterator iter_ = _vec.begin();
        typename t_vector::iterator end_ = _vec.end();

        for (; iter_ != end_; ++iter_)
        {
            *iter_ = ~*iter_;
        }
    }

    std::size_t find_first() const
    {
        return find_next(npos());
    }

    std::size_t find_next(const std::size_t index_) const
    {
        std::size_t ret_ = npos();
        const std::size_t block_ = index_ == npos() ? 0 : block(index_ + 1);
        std::size_t bit_ = index_ == npos() ? 0 : bit(index_ + 1);
        typename t_vector::const_iterator iter_ = _vec.begin() + block_;
        typename t_vector::const_iterator end_ = _vec.end();

        for (std::size_t i_ = block_; iter_ != end_; ++iter_, ++i_)
        {
            const bool bits_ = (*iter_ & (~static_cast<T>(0) << bit_)) != 0;

            if (bits_)
            {
                std::size_t j_ = bit_;
                std::size_t b_ = 1 << bit_;
                bool found_ = false;

                for (; j_ < sizeof(T) * 8; ++j_, b_ <<= 1)
                {
                    if (*iter_ & b_)
                    {
                        found_ = true;
                        break;
                    }
                }

                if (found_)
                {
                    ret_ = i_ * sizeof(T) * 8 + j_;
                    break;
                }
            }

            bit_ = 0;
        }

        return ret_;
    }

    std::size_t npos() const
    {
        return ~static_cast<std::size_t>(0);
    }

private:
    typedef std::vector<T> t_vector;

    t_vector _vec;

    std::size_t block(const std::size_t index_) const
    {
        return index_ / (sizeof(T) * 8);
    }

    std::size_t bit(const std::size_t index_) const
    {
        return index_ % (sizeof(T) * 8);
    }
};
}

#endif
