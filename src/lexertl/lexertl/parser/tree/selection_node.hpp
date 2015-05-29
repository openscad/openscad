// selection_node.hpp
// Copyright (c) 2005-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_SELECTION_NODE_HPP
#define LEXERTL_SELECTION_NODE_HPP

#include "node.hpp"

namespace lexertl
{
namespace detail
{
template<typename id_type>
class basic_selection_node : public basic_node<id_type>
{
public:
    typedef basic_node<id_type> node;
    typedef typename node::bool_stack bool_stack;
    typedef typename node::const_node_stack const_node_stack;
    typedef typename node::node_ptr_vector node_ptr_vector;
    typedef typename node::node_stack node_stack;
    typedef typename node::node_type node_type;

    basic_selection_node(node *left_, node *right_) :
        node(left_->nullable() || right_->nullable()),
        _left(left_),
        _right(right_)
    {
        _left->append_firstpos(node::_firstpos);
        _right->append_firstpos(node::_firstpos);
        _left->append_lastpos(node::_lastpos);
        _right->append_lastpos(node::_lastpos);
    }

    virtual ~basic_selection_node()
    {
    }

    virtual node_type what_type() const
    {
        return node::SELECTION;
    }

    virtual bool traverse(const_node_stack &node_stack_,
        bool_stack &perform_op_stack_) const
    {
        perform_op_stack_.push(true);

        switch (_right->what_type())
        {
        case node::SEQUENCE:
        case node::SELECTION:
        case node::ITERATION:
            perform_op_stack_.push(false);
            break;
        default:
            break;
        }

        node_stack_.push(_right);
        node_stack_.push(_left);
        return true;
    }

private:
    // Not owner of these pointers...
    node *_left;
    node *_right;

    virtual void copy_node(node_ptr_vector &node_ptr_vector_,
        node_stack &new_node_stack_, bool_stack &perform_op_stack_,
        bool &down_) const
    {
        if (perform_op_stack_.top())
        {
            node *rhs_ = new_node_stack_.top();

            new_node_stack_.pop();

            node *lhs_ = new_node_stack_.top();

            node_ptr_vector_->push_back
                (static_cast<basic_selection_node *>(0));
            node_ptr_vector_->back() = new basic_selection_node(lhs_, rhs_);
            new_node_stack_.top() = node_ptr_vector_->back();
        }
        else
        {
            down_ = true;
        }

        perform_op_stack_.pop();
    }

    // No copy construction.
    basic_selection_node(const basic_selection_node &);
    // No assignment.
    const basic_selection_node &operator =(const basic_selection_node &);
};
}
}

#endif
