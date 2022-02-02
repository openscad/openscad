// node.hpp
// Copyright (c) 2005-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_NODE_HPP
#define LEXERTL_NODE_HPP

#include <assert.h>
#include <memory>
#include "../../observer_ptr.hpp"
#include "../../runtime_error.hpp"
#include <stack>
#include <vector>

namespace lexertl
{
    namespace detail
    {
        template<typename id_type>
        class basic_node
        {
        public:
            enum class node_type { LEAF, SEQUENCE, SELECTION, ITERATION, END };

            using bool_stack = std::stack<bool>;
            using node_stack = std::stack<observer_ptr<basic_node>>;
            using const_node_stack = std::stack<observer_ptr<const basic_node>>;
            using node_vector = std::vector<observer_ptr<basic_node>>;
            using node_ptr_vector = std::vector<std::unique_ptr<basic_node>>;

            basic_node() :
                _nullable(false),
                _firstpos(),
                _lastpos()
            {
            }

            basic_node(const bool nullable_) :
                _nullable(nullable_),
                _firstpos(),
                _lastpos()
            {
            }

            virtual ~basic_node()
            {
            }

            static id_type null_token()
            {
                return static_cast<id_type>(~0);
            }

            bool nullable() const
            {
                return _nullable;
            }

            void append_firstpos(node_vector& firstpos_) const
            {
                firstpos_.insert(firstpos_.end(),
                    _firstpos.begin(), _firstpos.end());
            }

            void append_lastpos(node_vector& lastpos_) const
            {
                lastpos_.insert(lastpos_.end(),
                    _lastpos.begin(), _lastpos.end());
            }

            virtual void append_followpos(const node_vector&/*followpos_*/)
            {
                throw runtime_error("Internal error node::append_followpos().");
            }

            observer_ptr<basic_node> copy
                (node_ptr_vector& node_ptr_vector_) const
            {
                observer_ptr<basic_node> new_root_ = nullptr;
                const_node_stack node_stack_;
                bool_stack perform_op_stack_;
                bool down_ = true;
                node_stack new_node_stack_;

                node_stack_.push(this);

                while (!node_stack_.empty())
                {
                    while (down_)
                    {
                        down_ = node_stack_.top()->traverse(node_stack_,
                            perform_op_stack_);
                    }

                    while (!down_ && !node_stack_.empty())
                    {
                        observer_ptr<const basic_node> top_ = node_stack_.top();

                        top_->copy_node(node_ptr_vector_, new_node_stack_,
                            perform_op_stack_, down_);

                        if (!down_) node_stack_.pop();
                    }
                }

                assert(new_node_stack_.size() == 1);
                new_root_ = new_node_stack_.top();
                new_node_stack_.pop();
                return new_root_;
            }

            virtual node_type what_type() const = 0;

            virtual bool traverse(const_node_stack& node_stack_,
                bool_stack& perform_op_stack_) const = 0;

            node_vector& firstpos()
            {
                return _firstpos;
            }

            const node_vector& firstpos() const
            {
                return _firstpos;
            }

            // _lastpos modified externally, so not const &
            node_vector& lastpos()
            {
                return _lastpos;
            }

            virtual bool end_state() const
            {
                return false;
            }

            virtual id_type id() const
            {
                throw runtime_error("Internal error node::id().");
#ifdef __SUNPRO_CC
                // Stop bogus Solaris compiler warning
                return id_type();
#endif
            }

            virtual id_type user_id() const
            {
                throw runtime_error("Internal error node::user_id().");
#ifdef __SUNPRO_CC
                // Stop bogus Solaris compiler warning
                return id_type();
#endif
            }

            virtual id_type next_dfa() const
            {
                throw runtime_error("Internal error node::next_dfa().");
#ifdef __SUNPRO_CC
                // Stop bogus Solaris compiler warning
                return id_type();
#endif
            }

            virtual id_type push_dfa() const
            {
                throw runtime_error("Internal error node::push_dfa().");
#ifdef __SUNPRO_CC
                // Stop bogus Solaris compiler warning
                return id_type();
#endif
            }

            virtual bool pop_dfa() const
            {
                throw runtime_error("Internal error node::pop_dfa().");
#ifdef __SUNPRO_CC
                // Stop bogus Solaris compiler warning
                return false;
#endif
            }

            virtual id_type token() const
            {
                throw runtime_error("Internal error node::token().");
#ifdef __SUNPRO_CC
                // Stop bogus Solaris compiler warning
                return id_type();
#endif
            }

            virtual void greedy(const bool /*greedy_*/)
            {
                throw runtime_error("Internal error node::greedy(bool).");
            }

            virtual bool greedy() const
            {
                throw runtime_error("Internal error node::greedy().");
#ifdef __SUNPRO_CC
                // Stop bogus Solaris compiler warning
                return false;
#endif
            }

            virtual const node_vector& followpos() const
            {
                throw runtime_error("Internal error node::followpos().");
#ifdef __SUNPRO_CC
                // Stop bogus Solaris compiler warning
                return firstpos;
#endif
            }

            virtual node_vector& followpos()
            {
                throw runtime_error("Internal error node::followpos().");
#ifdef __SUNPRO_CC
                // Stop bogus Solaris compiler warning
                return firstpos;
#endif
            }

        protected:
            const bool _nullable;
            node_vector _firstpos;
            node_vector _lastpos;

            virtual void copy_node(node_ptr_vector& node_ptr_vector_,
                node_stack& new_node_stack_, bool_stack& perform_op_stack_,
                bool& down_) const = 0;

        private:
            // No copy construction.
            basic_node(const basic_node&) = delete;
            // No assignment.
            const basic_node& operator =(const basic_node&) = delete;
        };
    }
}

#endif
