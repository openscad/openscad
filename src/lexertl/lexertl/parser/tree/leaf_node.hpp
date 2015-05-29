// leaf_node.hpp
// Copyright (c) 2005-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_LEAF_NODE_HPP
#define LEXERTL_LEAF_NODE_HPP

#include "../../enums.hpp" // null_token
#include "node.hpp"
#include "../../size_t.hpp"

namespace lexertl
{
namespace detail
{
template<typename id_type>
class basic_leaf_node : public basic_node<id_type>
{
public:
    typedef basic_node<id_type> node;
    typedef typename node::bool_stack bool_stack;
    typedef typename node::const_node_stack const_node_stack;
    typedef typename node::node_ptr_vector node_ptr_vector;
    typedef typename node::node_stack node_stack;
    typedef typename node::node_type node_type;
    typedef typename node::node_vector node_vector;

    basic_leaf_node(const id_type token_, const bool greedy_) :
        node(token_ == node::null_token()),
        _token(token_),
        _set_greedy(!greedy_),
        _greedy(greedy_),
        _followpos()
    {
        if (!node::_nullable)
        {
            node::_firstpos.push_back(this);
            node::_lastpos.push_back(this);
        }
    }

    virtual ~basic_leaf_node()
    {
    }

    virtual void append_followpos(const node_vector &followpos_)
    {
        for (typename node_vector::const_iterator iter_ = followpos_.begin(),
            end_ = followpos_.end(); iter_ != end_; ++iter_)
        {
            _followpos.push_back(*iter_);
        }
    }

    virtual node_type what_type() const
    {
        return node::LEAF;
    }

    virtual bool traverse(const_node_stack &/*node_stack_*/,
        bool_stack &/*perform_op_stack_*/) const
    {
        return false;
    }

    virtual id_type token() const
    {
        return _token;
    }

    virtual void greedy(const bool greedy_)
    {
        if (!_set_greedy)
        {
            _greedy = greedy_;
            _set_greedy = true;
        }
    }

    virtual bool greedy() const
    {
        return _greedy;
    }

    virtual const node_vector &followpos() const
    {
        return _followpos;
    }

    virtual node_vector &followpos()
    {
        return _followpos;
    }

private:
    id_type _token;
    bool _set_greedy;
    bool _greedy;
    node_vector _followpos;

    virtual void copy_node(node_ptr_vector &node_ptr_vector_,
        node_stack &new_node_stack_, bool_stack &/*perform_op_stack_*/,
        bool &/*down_*/) const
    {
        node_ptr_vector_->push_back(static_cast<basic_leaf_node *>(0));
        node_ptr_vector_->back() = new basic_leaf_node(_token, _greedy);
        new_node_stack_.push(node_ptr_vector_->back());
    }
};
}
}

#endif
