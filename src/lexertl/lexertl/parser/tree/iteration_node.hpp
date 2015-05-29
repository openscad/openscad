// iteration_node.hpp
// Copyright (c) 2005-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_ITERATION_NODE_HPP
#define LEXERTL_ITERATION_NODE_HPP

#include "node.hpp"

namespace lexertl
{
namespace detail
{
template<typename id_type>
class basic_iteration_node : public basic_node<id_type>
{
public:
    typedef basic_node<id_type> node;
    typedef typename node::bool_stack bool_stack;
    typedef typename node::const_node_stack const_node_stack;
    typedef typename node::node_ptr_vector node_ptr_vector;
    typedef typename node::node_stack node_stack;
    typedef typename node::node_type node_type;
    typedef typename node::node_vector node_vector;

    basic_iteration_node(node *next_, const bool greedy_) :
        node(true),
        _next(next_),
        _greedy(greedy_)
    {
        typename node_vector::iterator iter_;
        typename node_vector::iterator end_;

        _next->append_firstpos(node::_firstpos);
        _next->append_lastpos(node::_lastpos);

        for (iter_ = node::_lastpos.begin(), end_ = node::_lastpos.end();
            iter_ != end_; ++iter_)
        {
            (*iter_)->append_followpos(node::_firstpos);
        }

        for (iter_ = node::_firstpos.begin(), end_ = node::_firstpos.end();
            iter_ != end_; ++iter_)
        {
            (*iter_)->greedy(greedy_);
        }
    }

    virtual ~basic_iteration_node()
    {
    }

    virtual node_type what_type() const
    {
        return node::ITERATION;
    }

    virtual bool traverse(const_node_stack &node_stack_,
        bool_stack &perform_op_stack_) const
    {
        perform_op_stack_.push(true);
        node_stack_.push(_next);
        return true;
    }

private:
    // Not owner of this pointer...
    node *_next;
    bool _greedy;

    virtual void copy_node(node_ptr_vector &node_ptr_vector_,
        node_stack &new_node_stack_, bool_stack &perform_op_stack_,
        bool &down_) const
    {
        if (perform_op_stack_.top())
        {
            node *ptr_ = new_node_stack_.top();

            node_ptr_vector_->push_back
                (static_cast<basic_iteration_node<id_type> *>(0));
            node_ptr_vector_->back() = new basic_iteration_node
                (ptr_, _greedy);
            new_node_stack_.top() = node_ptr_vector_->back();
        }
        else
        {
            down_ = true;
        }

        perform_op_stack_.pop();
    }

    // No copy construction.
    basic_iteration_node(const basic_iteration_node &);
    // No assignment.
    const basic_iteration_node &operator =(const basic_iteration_node &);
};
}
}

#endif
