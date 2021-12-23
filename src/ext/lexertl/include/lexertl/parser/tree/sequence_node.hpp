// sequence_node.hpp
// Copyright (c) 2005-2020 Ben Hanson (http://www.benhanson.net/)
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
            using node = basic_node<id_type>;
            using bool_stack = typename node::bool_stack;
            using const_node_stack = typename node::const_node_stack;
            using node_ptr_vector = typename node::node_ptr_vector;
            using node_stack = typename node::node_stack;
            using node_type = typename node::node_type;
            using node_vector = typename node::node_vector;

            basic_sequence_node(observer_ptr<node> left_,
                observer_ptr<node> right_) :
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

                node_vector& lastpos_ = _left->lastpos();
                const node_vector& firstpos_ = _right->firstpos();

                for (observer_ptr<node> node_ : lastpos_)
                {
                    node_->append_followpos(firstpos_);
                }
            }

            virtual ~basic_sequence_node() override
            {
            }

            virtual node_type what_type() const override
            {
                return node::node_type::SEQUENCE;
            }

            virtual bool traverse(const_node_stack& node_stack_,
                bool_stack& perform_op_stack_) const override
            {
                perform_op_stack_.push(true);

                switch (_right->what_type())
                {
                case node::node_type::SEQUENCE:
                case node::node_type::SELECTION:
                case node::node_type::ITERATION:
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
            observer_ptr<node> _left;
            observer_ptr<node> _right;

            virtual void copy_node(node_ptr_vector& node_ptr_vector_,
                node_stack& new_node_stack_, bool_stack& perform_op_stack_,
                bool& down_) const override
            {
                if (perform_op_stack_.top())
                {
                    observer_ptr<node> rhs_ = new_node_stack_.top();

                    new_node_stack_.pop();

                    observer_ptr<node> lhs_ = new_node_stack_.top();

                    node_ptr_vector_.emplace_back
                    (std::make_unique<basic_sequence_node>(lhs_, rhs_));
                    new_node_stack_.top() = node_ptr_vector_.back().get();
                }
                else
                {
                    down_ = true;
                }

                perform_op_stack_.pop();
            }

            // No copy construction.
            basic_sequence_node(const basic_sequence_node&) = delete;
            // No assignment.
            const basic_sequence_node& operator =
                (const basic_sequence_node&) = delete;
        };
    }
}

#endif
