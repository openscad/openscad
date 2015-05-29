// dot.hpp
// Copyright (c) 2005-2015 Ben Hanson (http://www.benhanson.net/)
// Copyright (c) 2013 Autodesk, Inc. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_DOT_HPP
#define LEXERTL_DOT_HPP

#include <ostream>
#include "rules.hpp"
#include "state_machine.hpp"
#include "sm_to_csm.hpp"

namespace lexertl
{
//! The class template basic_dot contains utility functions used to
//! dump a description of a finite state machine formatted in the
//! DOT language (http://www.graphviz.org/doc/info/lang.html). The
//! resulting directed graph can previewed by opening the ".dot" file
//! into the GraphViz application (http://www.graphviz.org).
template<typename sm, typename char_type, typename id_type = std::size_t,
    bool is_dfa = true>
class basic_dot
{
public:
    typedef lexertl::basic_char_state_machine<char_type, id_type, is_dfa>
        char_state_machine;
    typedef lexertl::basic_rules<char_type, id_type> rules;
    typedef std::basic_ostream<char_type> ostream;
    typedef std::basic_string<char_type> string;

    //! Dumps a description of the finite state machine expressed in
    //! the DOT language to the given output stream.
    static void dump(const sm &sm_, rules &rules_, ostream &stream_)
    {
        char_state_machine csm_;

        sm_to_csm(sm_, csm_);
        dump(csm_, rules_, stream_);
    }

    //! Dumps a description of the finite state machine expressed in
    //! the DOT language to the given output stream.
    static void dump(const char_state_machine &csm_, rules &rules_,
        ostream &stream_)
    {
        header(stream_);
        for (std::size_t dfa_ = 0, dfas_ = csm_.size(); dfa_ < dfas_; ++dfa_)
        {
            dump_ex(dfa_, csm_._sm_deque[dfa_], rules_, stream_);
        }
        trailer(stream_);
    }

protected:
    typedef typename char_state_machine::state dfa_state;
    typedef typename dfa_state::string_token string_token;
    typedef std::basic_stringstream<char_type> stringstream;

    // Naming of nodes used in the DOT diagram. The naming is of the
    // form: L<dfa_id>_S<state_id>.
    static string node_name(id_type dfa_id_, id_type state_id_)
    {
        stringstream namestream_;
        namestream_ << "L" << dfa_id_ << "_S" << state_id_;
        return namestream_.str();
    }

    // Escape control characters twice. This is necessary when
    // expressing character sets attached as to DOT nodes as
    // labels.
    static string double_escape_char(const id_type ch_)
    {
        stringstream out_;

        switch (ch_)
        {
            case '\0':
                out_ << '\\';
                out_ << '\\';
                out_ << '0';
                break;
            case '\a':
                out_ << '\\';
                out_ << '\\';
                out_ << 'a';
                break;
            case '\b':
                out_ << '\\';
                out_ << '\\';
                out_ << 'b';
                break;
            case '\f':
                out_ << '\\';
                out_ << '\\';
                out_ << 'f';
                break;
            case '\n':
                out_ << '\\';
                out_ << '\\';
                out_ << 'n';
                break;
            case '\r':
                out_ << '\\';
                out_ << '\\';
                out_ << 'r';
                break;
            case '\t':
                out_ << '\\';
                out_ << '\\';
                out_ << 't';
                break;
            case '\v':
                out_ << '\\';
                out_ << '\\';
                out_ << 'v';
                break;
            case '\\':
                out_ << '\\';
                out_ << '\\';
                break;
            case '"':
                out_ << '\\';
                out_ << '\\';
                out_ << '"';
                break;
            case '\'':
                out_ << '\\';
                out_ << '\\';
                out_ << '\'';
                break;
            default:
            {
                if (ch_ < 32 || ch_ > 126)
                {
                    out_ << '\\';
                    out_ << 'x';
                    out_ << std::hex <<
                        static_cast<std::size_t>(ch_);
                }
                else
                {
                    out_ << char_type(ch_);
                }

                break;
            }
        }

        return out_.str();
    }

    // Internal function actually performing the work of dumping the
    // state machine in DOT.
    static void dump_ex(id_type dfa_id_,
        const typename char_state_machine::dfa &dfa_,
        rules &rules_,
        ostream &stream_)
    {
        const std::size_t states_ = dfa_._states.size();
        typename dfa_state::id_type_string_token_map::const_iterator iter_;
        typename dfa_state::id_type_string_token_map::const_iterator end_;

        stream_ << std::endl;

        for (std::size_t i_ = 0; i_ < states_; ++i_)
        {
            const dfa_state &state_ = dfa_._states[i_];

            const string name = node_name(dfa_id_, i_);
            if (i_ == 0)
            {
                stream_ << "    " << name << " [shape = doublecircle, xlabel=\""
                        << rules_.state(dfa_id_) << "\"];" << std::endl;
            }
            else if (state_._end_state)
            {
                stream_ << "    " << name <<
                    " [shape = doublecircle, xlabel=\"id =" <<
                    static_cast<std::size_t>(state_._id) << "\"];" <<
                    std::endl;
            }
            else {
                stream_ << "    " << name << " [shape = circle];" << std::endl;
            }
        }

        stream_ << std::endl;

        for (std::size_t i_ = 0; i_ < states_; ++i_)
        {
            const dfa_state &state_ = dfa_._states[i_];

            iter_ = state_._transitions.begin();
            end_ = state_._transitions.end();

            const string src_name = node_name(dfa_id_, i_);

            for (; iter_ != end_; ++iter_)
            {
                const string dst_name = node_name(dfa_id_, iter_->first);
                stream_ << "    " << src_name << " -> " << dst_name <<
                " [label = \"";

                string_token token_ = iter_->second;

                open_bracket(stream_);

                if (!iter_->second.any() && iter_->second.negatable())
                {
                    token_.negate();
                    negated(stream_);
                }

                string chars_;
                typename string_token::range_vector::const_iterator
                    ranges_iter_ = token_._ranges.begin();
                typename string_token::range_vector::const_iterator
                    ranges_end_ = token_._ranges.end();

                for (; ranges_iter_ != ranges_end_; ++ranges_iter_)
                {
                    if (ranges_iter_->first == '^' ||
                        ranges_iter_->first == ']')
                    {
                        stream_ << "\\\\";
                    }

                    chars_ = double_escape_char(ranges_iter_->first);

                    if (ranges_iter_->first != ranges_iter_->second)
                    {
                        if (ranges_iter_->first + 1 < ranges_iter_->second)
                        {
                            chars_ += '-';
                        }

                        if (ranges_iter_->second == '^' ||
                            ranges_iter_->second == ']')
                        {
                            stream_ << "\\\\";
                        }

                        chars_ += double_escape_char(ranges_iter_->second);
                    }

                    stream_ << chars_;
                }

                close_bracket(stream_);
                stream_ << "\"];" << std::endl;
            }

            if (state_._end_state) {
                const string dst_name = node_name(state_._next_dfa, 0);
                stream_ << "    " << src_name << " -> " << dst_name
                        << " [style = \"dashed\"];" << std::endl;
            }
        }
    }

    static void header(ostream &stream_)
    {
        stream_ << "digraph DFAs {"    << std::endl;
        stream_ << "    rankdir = LR;" << std::endl;
    }

    static void trailer(ostream &stream_)
    {
        stream_ << "}" << std::endl;
    }

    static void open_bracket(ostream &stream_)
    {
        stream_ << "[";
    }

    static void negated(ostream &stream_)
    {
        stream_ << "^";
    }

    static void close_bracket(ostream &stream_)
    {
        stream_ << "]";
    }

};

typedef basic_dot<basic_state_machine<char>, char> dot;
typedef basic_dot<basic_state_machine<wchar_t>, wchar_t> wdot;
}

#endif
