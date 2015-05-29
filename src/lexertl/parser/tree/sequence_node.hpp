// sequence_node.hpp
// Copyright (c) 2005-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_SEQUENCE_NODE_HPP
#define LEXERTL_SEQUENCE_NODE_HPP

#include "node.hpp"

namespace lexertl
{
namespace detail
{
template<typename id_type>
class basic_sequence_node : public basic_node<id_type>
{
public:
    typedef basic_node<id_type> node;
    typedef typename node::bool_stack bool_stack;
    typedef typename node::const_node_stack const_node_stack;
    typedef typename node::node_ptr_vector node_ptr_vector;
    typedef typename node::node_stack node_stack;
    typedef typename node::node_type node_type;
    typedef typename node::node_vector node_vector;

    basic_sequence_node(node *left_,
        node *right_) :
        node(left_->nullable() && right_->nullable()),
        _left(left_),
        _right(right_)
    {
        _left->append_firstpos(node::_firstpos);

        if (_left->nullable())
        {
            _right->append_firstpos(node::_firstpos);
        }

        if (_right->nullable())
        {
            _left->append_lastpos(node::_lastpos);
        }

        _right->append_lastpos(node::_lastpos);

        node_vector &lastpos_ = _left->lastpos();
        const node_vector &firstpos_ = _right->firstpos();

        for (typename node_vector::iterator iter_ = lastpos_.begin(),
            end_ = lastpos_.end(); iter_ != end_; ++iter_)
        {
            (*iter_)->append_followpos(firstpos_);
        }
    }

    virtual ~basic_sequence_node()
    {
    }

    virtual node_type what_type() const
    {
        return node::SEQUENCE;
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
                (static_cast<basic_sequence_node<id_type> *>(0));
            node_ptr_vector_->back() = new basic_sequence_node<id_type>
                (lhs_, rhs_);
            new_node_stack_.top() = node_ptr_vector_->back();
        }
        else
        {
            down_ = true;
        }

        perform_op_stack_.pop();
    }

    // No copy construction.
    basic_sequence_node(const basic_sequence_node &);
    // No assignment.
    const basic_sequence_node &operator =(const basic_sequence_node &);
};
}
}

#endif
