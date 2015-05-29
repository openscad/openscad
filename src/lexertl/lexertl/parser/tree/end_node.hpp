// end_node.hpp
// Copyright (c) 2005-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_END_NODE_HPP
#define LEXERTL_END_NODE_HPP

#include "node.hpp"
#include "../../size_t.hpp"

namespace lexertl
{
namespace detail
{
template<typename id_type>
class basic_end_node : public basic_node<id_type>
{
public:
    typedef basic_node<id_type> node;
    typedef typename node::bool_stack bool_stack;
    typedef typename node::const_node_stack const_node_stack;
    typedef typename node::node_ptr_vector node_ptr_vector;
    typedef typename node::node_stack node_stack;
    typedef typename node::node_type node_type;
    typedef typename node::node_vector node_vector;

    basic_end_node(const id_type id_, const id_type user_id_,
        const id_type next_dfa_, const id_type push_dfa_,
        const bool pop_dfa_) :
        node(false),
        _id(id_),
        _user_id(user_id_),
        _next_dfa(next_dfa_),
        _push_dfa(push_dfa_),
        _pop_dfa(pop_dfa_),
        _followpos()
    {
        node::_firstpos.push_back(this);
        node::_lastpos.push_back(this);
    }

    virtual ~basic_end_node()
    {
    }

    virtual node_type what_type() const
    {
        return node::END;
    }

    virtual bool traverse(const_node_stack &/*node_stack_*/,
        bool_stack &/*perform_op_stack_*/) const
    {
        return false;
    }

    virtual const node_vector &followpos() const
    {
        // _followpos is always empty..!
        return _followpos;
    }

    virtual bool end_state() const
    {
        return true;
    }

    virtual id_type id() const
    {
        return _id;
    }

    virtual id_type user_id() const
    {
        return _user_id;
    }

    virtual id_type next_dfa() const
    {
        return _next_dfa;
    }

    virtual id_type push_dfa() const
    {
        return _push_dfa;
    }

    virtual bool pop_dfa() const
    {
        return _pop_dfa;
    }

private:
    id_type _id;
    id_type _user_id;
    id_type _next_dfa;
    id_type _push_dfa;
    bool _pop_dfa;
    node_vector _followpos;

    virtual void copy_node(node_ptr_vector &/*node_ptr_vector_*/,
        node_stack &/*new_node_stack_*/, bool_stack &/*perform_op_stack_*/,
        bool &/*down_*/) const
    {
        // Nothing to do, as end_nodes are not copied.
    }
};
}
}

#endif
