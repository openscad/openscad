// equivset.hpp
// Copyright (c) 2005-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_EQUIVSET_HPP
#define LEXERTL_EQUIVSET_HPP

#include <algorithm>
#include "../parser/tree/node.hpp"
#include <set>

namespace lexertl
{
namespace detail
{
template<typename id_type>
struct basic_equivset
{
    typedef std::set<id_type> index_set;
    typedef std::vector<id_type> index_vector;
    // Not owner of nodes:
    typedef basic_node<id_type> node;
    typedef std::vector<node *> node_vector;

    index_vector _index_vector;
    id_type _id;
    bool _greedy;
    node_vector _followpos;

    basic_equivset() :
        _index_vector(),
        _id(0),
        _greedy(true),
        _followpos()
    {
    }

    basic_equivset(const index_set &index_set_, const id_type id_,
        const bool greedy_, const node_vector &followpos_) :
        _index_vector(index_set_.begin(), index_set_.end()),
        _id(id_),
        _greedy(greedy_),
        _followpos(followpos_)
    {
    }

    bool empty() const
    {
        return _index_vector.empty() && _followpos.empty();
    }

    void intersect(basic_equivset &rhs_, basic_equivset &overlap_)
    {
        intersect_indexes(rhs_._index_vector, overlap_._index_vector);

        if (!overlap_._index_vector.empty())
        {
            // Note that the LHS takes priority in order to
            // respect rule ordering priority in the lex spec.
            overlap_._id = _id;
            overlap_._greedy = _greedy;
            overlap_._followpos = _followpos;

            typename node_vector::const_iterator overlap_begin_ =
                overlap_._followpos.begin();
            typename node_vector::const_iterator overlap_end_ =
                overlap_._followpos.end();
            typename node_vector::const_iterator rhs_iter_ =
                rhs_._followpos.begin();
            typename node_vector::const_iterator rhs_end_ =
                rhs_._followpos.end();

            for (; rhs_iter_ != rhs_end_; ++rhs_iter_)
            {
                node *node_ = *rhs_iter_;

                if (std::find(overlap_begin_, overlap_end_, node_) ==
                    overlap_end_)
                {
                    overlap_._followpos.push_back(node_);
                    overlap_begin_ = overlap_._followpos.begin();
                    overlap_end_ = overlap_._followpos.end();
                }
            }

            if (_index_vector.empty())
            {
                _followpos.clear();
            }

            if (rhs_._index_vector.empty())
            {
                rhs_._followpos.clear();
            }
        }
    }

private:
    void intersect_indexes(index_vector &rhs_, index_vector &overlap_)
    {
        typename index_vector::iterator iter_ = _index_vector.begin();
        typename index_vector::iterator end_ = _index_vector.end();
        typename index_vector::iterator rhs_iter_ = rhs_.begin();
        typename index_vector::iterator rhs_end_ = rhs_.end();

        while (iter_ != end_ && rhs_iter_ != rhs_end_)
        {
            const id_type index_ = *iter_;
            const id_type rhs_index_ = *rhs_iter_;

            if (index_ < rhs_index_)
            {
                ++iter_;
            }
            else if (index_ > rhs_index_)
            {
                ++rhs_iter_;
            }
            else
            {
                overlap_.push_back(index_);
                iter_ = _index_vector.erase(iter_);
                end_ = _index_vector.end();
                rhs_iter_ = rhs_.erase(rhs_iter_);
                rhs_end_ = rhs_.end();
            }
        }
    }
};
}
}

#endif
