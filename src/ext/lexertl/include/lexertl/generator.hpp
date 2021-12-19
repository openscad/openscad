// generator.hpp
// Copyright (c) 2005-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_GENERATOR_HPP
#define LEXERTL_GENERATOR_HPP

#include <algorithm>
#include "partition/charset.hpp"
#include "char_traits.hpp"
#include "partition/equivset.hpp"
#include <list>
#include <memory>
#include "parser/parser.hpp"
#include "rules.hpp"
#include "state_machine.hpp"
#include <type_traits>

namespace lexertl
{
    template<typename rules, typename sm,
        typename char_traits = basic_char_traits
        <typename sm::traits::input_char_type> >
        class basic_generator
    {
    public:
        using id_type = typename rules::id_type;
        using rules_char_type = typename rules::rules_char_type;
        using sm_traits = typename sm::traits;
        using parser = detail::basic_parser<rules_char_type, sm_traits>;
        using charset_map = typename parser::charset_map;
        using node = typename parser::node;
        using node_ptr_vector = typename parser::node_ptr_vector;

        static void build(const rules& rules_, sm& sm_)
        {
            const std::size_t size_ = rules_.statemap().size();
            // Strong exception guarantee
            // http://www.boost.org/community/exception_safety.html
            internals internals_;
            sm temp_sm_;
            node_ptr_vector node_ptr_vector_;

            internals_._eoi = rules_.eoi();
            internals_.add_states(size_);

            for (id_type index_ = 0; index_ < size_; ++index_)
            {
                if (rules_.regexes()[index_].empty())
                {
                    std::ostringstream ss_;

                    ss_ << "Lexer states with no rules are not allowed "
                        "(lexer state " << index_ << ".)";
                    throw runtime_error(ss_.str());
                }
                else
                {
                    // Note that the following variables are per DFA.
                    // Map of regex charset tokens (strings) to index
                    charset_map charset_map_;
                    // Used to fix up $ and \n clashes.
                    id_type cr_id_ = sm_traits::npos();
                    id_type nl_id_ = sm_traits::npos();
                    // Regex syntax tree
                    observer_ptr<node> root_ = build_tree(rules_, index_,
                        node_ptr_vector_, charset_map_, cr_id_, nl_id_);

                    build_dfa(charset_map_, root_, internals_, temp_sm_, index_,
                        cr_id_, nl_id_);

                    if (internals_._dfa[index_].size() /
                        internals_._dfa_alphabet[index_] >= sm_traits::npos())
                    {
                        // Overflow
                        throw runtime_error("The data type you have chosen "
                            "cannot hold this many DFA rows.");
                    }
                }
            }

            // If you get a compile error here the id_type from rules and
            // state machine do no match.
            create(internals_, temp_sm_, rules_.features(), lookup());
            sm_.swap(temp_sm_);
        }

        static observer_ptr<node> build_tree(const rules& rules_,
            const std::size_t dfa_, node_ptr_vector& node_ptr_vector_,
            charset_map& charset_map_, id_type& cr_id_, id_type& nl_id_)
        {
            parser parser_(rules_.locale(), node_ptr_vector_, charset_map_,
                rules_.eoi());
            const auto& regexes_ = rules_.regexes();
            auto regex_iter_ = regexes_[dfa_].cbegin();
            auto regex_iter_end_ = regexes_[dfa_].cend();
            const auto& ids_ = rules_.ids();
            const auto& user_ids_ = rules_.user_ids();
            auto id_iter_ = ids_[dfa_].cbegin();
            auto user_id_iter_ = user_ids_[dfa_].cbegin();
            const auto& next_dfas_ = rules_.next_dfas();
            const auto& pushes_ = rules_.pushes();
            const auto& pops_ = rules_.pops();
            auto next_dfa_iter_ = next_dfas_[dfa_].cbegin();
            auto push_dfa_iter_ = pushes_[dfa_].cbegin();
            auto pop_dfa_iter_ = pops_[dfa_].cbegin();
            const bool seen_bol_ = (rules_.features()[dfa_] & bol_bit) != 0;
            observer_ptr<node> root_ = nullptr;

            root_ = parser_.parse(*regex_iter_, *id_iter_, *user_id_iter_,
                *next_dfa_iter_, *push_dfa_iter_, *pop_dfa_iter_,
                rules_.flags(), cr_id_, nl_id_, seen_bol_);
            ++regex_iter_;
            ++id_iter_;
            ++user_id_iter_;
            ++next_dfa_iter_;
            ++push_dfa_iter_;
            ++pop_dfa_iter_;

            // Build syntax trees
            while (regex_iter_ != regex_iter_end_)
            {
                observer_ptr<node> rhs_ = parser_.parse(*regex_iter_, *id_iter_,
                    *user_id_iter_, *next_dfa_iter_, *push_dfa_iter_,
                    *pop_dfa_iter_, rules_.flags(), cr_id_, nl_id_,
                    (rules_.features()[dfa_] & bol_bit) != 0);

                node_ptr_vector_.emplace_back
                (std::make_unique<selection_node>(root_, rhs_));
                root_ = node_ptr_vector_.back().get();

                ++regex_iter_;
                ++id_iter_;
                ++user_id_iter_;
                ++next_dfa_iter_;
                ++push_dfa_iter_;
                ++pop_dfa_iter_;
            }

            return root_;
        }

    protected:
        using compressed = std::integral_constant<bool, sm_traits::compressed>;
        using equivset = detail::basic_equivset<id_type>;
        using equivset_list = std::list<std::unique_ptr<equivset>>;
        using equivset_ptr = std::unique_ptr<equivset>;
        using sm_char_type = typename sm_traits::char_type;
        using charset = detail::basic_charset<sm_char_type, id_type>;
        using charset_ptr = std::unique_ptr<charset>;
        using charset_list = std::list<std::unique_ptr<charset>>;
        using internals = detail::basic_internals<id_type>;
        using id_type_set = typename std::set<id_type>;
        using id_type_vector = typename internals::id_type_vector;
        using index_set = typename charset::index_set;
        using index_set_vector = std::vector<index_set>;
        using is_dfa = std::integral_constant<bool, sm_traits::is_dfa>;
        using lookup = std::integral_constant<bool, sm_traits::lookup>;
        using node_set = std::set<observer_ptr<const node>>;
        using node_set_vector = std::vector<std::unique_ptr<node_set>>;
        using node_vector = typename node::node_vector;
        using node_vector_vector = std::vector<std::unique_ptr<node_vector>>;
        using selection_node = typename parser::selection_node;
        using size_t_vector = typename std::vector<std::size_t>;
        using string_token = typename parser::string_token;

        static void build_dfa(const charset_map& charset_map_,
            const observer_ptr<node> root_, internals& internals_, sm& sm_,
            const id_type dfa_index_, id_type& cr_id_, id_type& nl_id_)
        {
            // partitioned charset list
            charset_list charset_list_;
            // vector mapping token indexes to partitioned token index sets
            index_set_vector set_mapping_;
            auto& dfa_ = internals_._dfa[dfa_index_];
            std::size_t dfa_alphabet_ = 0;
            const node_vector& followpos_ = root_->firstpos();
            node_set_vector seen_sets_;
            node_vector_vector seen_vectors_;
            size_t_vector hash_vector_;
            id_type zero_id_ = sm_traits::npos();
            id_type_set eol_set_;

            set_mapping_.resize(charset_map_.size());
            partition_charsets(charset_map_, charset_list_, is_dfa());
            build_set_mapping(charset_list_, internals_, dfa_index_,
                set_mapping_);

            if (cr_id_ != sm_traits::npos() || nl_id_ != sm_traits::npos())
            {
                if (cr_id_ != sm_traits::npos())
                {
                    cr_id_ = *set_mapping_[cr_id_].begin();
                }

                if (nl_id_ != sm_traits::npos())
                {
                    nl_id_ = *set_mapping_[nl_id_].begin();
                }

                zero_id_ = sm_traits::compressed ?
                    *set_mapping_[charset_map_.find(string_token(0, 0))->
                    second].begin() : sm_traits::npos();
            }

            dfa_alphabet_ = charset_list_.size() + transitions_index +
                (cr_id_ == sm_traits::npos() &&
                    nl_id_ == sm_traits::npos() ? 0 : 1);

            if (dfa_alphabet_ > sm_traits::npos())
            {
                // Overflow
                throw runtime_error("The data type you have chosen cannot hold "
                    "the dfa alphabet.");
            }

            internals_._dfa_alphabet[dfa_index_] =
                static_cast<id_type>(dfa_alphabet_);
            // 'jam' state
            dfa_.resize(dfa_alphabet_, 0);
            closure(followpos_, seen_sets_, seen_vectors_, hash_vector_,
                static_cast<id_type>(dfa_alphabet_), dfa_);

            // Loop over states
            for (id_type index_ = 0; index_ < static_cast<id_type>
                (seen_vectors_.size()); ++index_)
            {
                equivset_list equiv_list_;

                // Intersect charsets
                build_equiv_list(*seen_vectors_[index_], set_mapping_,
                    equiv_list_, is_dfa());

                for (auto& equivset_ : equiv_list_)
                {
                    const id_type transition_ = closure
                    (equivset_->_followpos, seen_sets_, seen_vectors_,
                        hash_vector_, static_cast<id_type>(dfa_alphabet_),
                        dfa_);

                    if (transition_ != sm_traits::npos())
                    {
                        observer_ptr<id_type> ptr_ = &dfa_.front() +
                            ((static_cast<std::size_t>(index_) + 1) *
                                dfa_alphabet_);

                        // Prune abstemious transitions from end states.
                        if (*ptr_ && !equivset_->_greedy) continue;

                        set_transitions(transition_, equivset_.get(), dfa_,
                            ptr_, index_, eol_set_);
                    }
                }
            }

            fix_clashes(eol_set_, cr_id_, nl_id_, zero_id_, dfa_, dfa_alphabet_,
                compressed());
            append_dfa(charset_list_, internals_, sm_, dfa_index_, lookup());
        }

        static void set_transitions(const id_type transition_,
            equivset* equivset_, typename internals::id_type_vector& dfa_,
            id_type* ptr_, const id_type index_, id_type_set& eol_set_)
        {
            for (typename equivset::index_vector::const_iterator
                equiv_iter_ = equivset_->_index_vector.begin(),
                equiv_end_ = equivset_->_index_vector.end();
                equiv_iter_ != equiv_end_; ++equiv_iter_)
            {
                const id_type i_ = *equiv_iter_;

                if (i_ == parser::bol_token())
                {
                    dfa_.front() = transition_;
                }
                else if (i_ == parser::eol_token())
                {
                    ptr_[eol_index] = transition_;
                    eol_set_.insert(index_ + 1);
                }
                else
                {
                    ptr_[i_ + transitions_index] = transition_;
                }
            }
        }

        // Uncompressed
        static void fix_clashes(const id_type_set& eol_set_,
            const id_type cr_id_, const id_type nl_id_,
            const id_type /*zero_id_*/,
            typename internals::id_type_vector& dfa_,
            const std::size_t dfa_alphabet_, const std::false_type&)
        {
            for (const auto& eol_ : eol_set_)
            {
                observer_ptr<id_type> ptr_ = &dfa_.front() +
                    eol_ * dfa_alphabet_;
                const id_type eol_state_ = ptr_[eol_index];
                const id_type cr_state_ = ptr_[cr_id_ + transitions_index];
                const id_type nl_state_ = ptr_[nl_id_ + transitions_index];

                if (cr_state_)
                {
                    ptr_[transitions_index + cr_id_] = 0;
                    ptr_ = &dfa_.front() + eol_state_ * dfa_alphabet_;

                    if (ptr_[transitions_index + cr_id_] == 0)
                    {
                        ptr_[transitions_index + cr_id_] = cr_state_;
                    }
                }

                if (nl_state_)
                {
                    ptr_[transitions_index + nl_id_] = 0;
                    ptr_ = &dfa_.front() + eol_state_ * dfa_alphabet_;

                    if (ptr_[transitions_index + nl_id_] == 0)
                    {
                        ptr_[transitions_index + nl_id_] = nl_state_;
                    }
                }
            }
        }

        // Compressed
        static void fix_clashes(const id_type_set& eol_set_,
            const id_type cr_id_, const id_type nl_id_, const id_type zero_id_,
            typename internals::id_type_vector& dfa_,
            const std::size_t dfa_alphabet_, const std::true_type&)
        {
            std::size_t i_ = 0;

            for (const auto& eol_ : eol_set_)
            {
                observer_ptr<id_type> ptr_ = &dfa_.front() +
                    eol_ * dfa_alphabet_;
                const id_type eol_state_ = ptr_[eol_index];
                id_type cr_state_ = 0;
                id_type nl_state_ = 0;

                for (; i_ < (sm_traits::char_24_bit ? 2 : 1); ++i_)
                {
                    ptr_ = &dfa_.front() + ptr_[transitions_index + zero_id_] *
                        dfa_alphabet_;
                }

                cr_state_ = ptr_[transitions_index + cr_id_];

                if (cr_state_)
                {
                    ptr_ = &dfa_.front() + eol_state_ * dfa_alphabet_;

                    if (ptr_[transitions_index + zero_id_] != 0) continue;

                    ptr_[transitions_index + zero_id_] =
                        static_cast<id_type>(dfa_.size() / dfa_alphabet_);
                    dfa_.resize(dfa_.size() + dfa_alphabet_, 0);

                    for (i_ = 0; i_ < (sm_traits::char_24_bit ? 1 : 0); ++i_)
                    {
                        ptr_ = &dfa_.front() + dfa_.size() - dfa_alphabet_;
                        ptr_[transitions_index + zero_id_] =
                            static_cast<id_type>(dfa_.size() / dfa_alphabet_);
                        dfa_.resize(dfa_.size() + dfa_alphabet_, 0);
                    }

                    ptr_ = &dfa_.front() + dfa_.size() - dfa_alphabet_;
                    ptr_[transitions_index + cr_id_] = cr_state_;
                }

                nl_state_ = ptr_[transitions_index + nl_id_];

                if (nl_state_)
                {
                    ptr_ = &dfa_.front() + eol_state_ * dfa_alphabet_;

                    if (ptr_[transitions_index + zero_id_] != 0) continue;

                    ptr_[transitions_index + zero_id_] =
                        static_cast<id_type>(dfa_.size() / dfa_alphabet_);
                    dfa_.resize(dfa_.size() + dfa_alphabet_, 0);

                    for (i_ = 0; i_ < (sm_traits::char_24_bit ? 1 : 0); ++i_)
                    {
                        ptr_ = &dfa_.front() + dfa_.size() - dfa_alphabet_;
                        ptr_[transitions_index + zero_id_] =
                            static_cast<id_type>(dfa_.size() / dfa_alphabet_);
                        dfa_.resize(dfa_.size() + dfa_alphabet_, 0);
                    }

                    ptr_ = &dfa_.front() + dfa_.size() - dfa_alphabet_;
                    ptr_[transitions_index + nl_id_] = nl_state_;
                }
            }
        }

        // char_state_machine version
        static void append_dfa(const charset_list& charset_list_,
            const internals& internals_, sm& sm_, const id_type dfa_index_,
            const std::false_type&)
        {
            std::size_t size_ = charset_list_.size();
            typename sm::string_token_vector token_vector_;

            token_vector_.reserve(size_);

            for (const auto& charset_ : charset_list_)
            {
                token_vector_.push_back(charset_->_token);
            }

            sm_.append(token_vector_, internals_, dfa_index_);
        }

        // state_machine version
        static void append_dfa(const charset_list&, const internals&, sm&,
            const id_type, const std::true_type&)
        {
            // Nothing to do - will use create() instead
        }

        // char_state_machine version
        static void create(internals&, sm&, const id_type_vector&,
            const std::false_type&)
        {
            // Nothing to do - will use append_dfa() instead
        }

        // state_machine version
        static void create(internals& internals_, sm& sm_,
            const id_type_vector& features_, const std::true_type&)
        {
            for (std::size_t i_ = 0, size_ = internals_._dfa.size();
                i_ < size_; ++i_)
            {
                internals_._features |= features_[i_];
            }

            if (internals_._dfa.size() > 1)
            {
                internals_._features |= multi_state_bit;
            }

            sm_.data().swap(internals_);
        }

        // NFA version
        static void partition_charsets(const charset_map& map_,
            charset_list& lhs_, const std::false_type&)
        {
            fill_rhs_list(map_, lhs_);
        }

        // DFA version
        static void partition_charsets(const charset_map& map_,
            charset_list& lhs_, const std::true_type&)
        {
            charset_list rhs_;

            fill_rhs_list(map_, rhs_);

            if (!rhs_.empty())
            {
                typename charset_list::iterator iter_;
                typename charset_list::iterator end_;
                charset_ptr overlap_ = std::make_unique<charset>();

                lhs_.emplace_back(std::move(rhs_.front()));
                rhs_.pop_front();

                while (!rhs_.empty())
                {
                    charset_ptr r_(rhs_.front().release());

                    rhs_.pop_front();
                    iter_ = lhs_.begin();
                    end_ = lhs_.end();

                    while (!r_->empty() && iter_ != end_)
                    {
                        auto l_iter_ = iter_;

                        (*l_iter_)->intersect(*r_.get(), *overlap_.get());

                        if (overlap_->empty())
                        {
                            ++iter_;
                        }
                        else if ((*l_iter_)->empty())
                        {
                            l_iter_->reset(overlap_.release());
                            overlap_ = std::make_unique<charset>();
                            ++iter_;
                        }
                        else if (r_->empty())
                        {
                            r_.reset(overlap_.release());
                            overlap_ = std::make_unique<charset>();
                            break;
                        }
                        else
                        {
                            iter_ = lhs_.insert(++iter_, charset_ptr());
                            iter_->reset(overlap_.release());
                            overlap_ = std::make_unique<charset>();
                            ++iter_;
                            end_ = lhs_.end();
                        }
                    }

                    if (!r_->empty())
                    {
                        lhs_.emplace_back(std::move(r_));
                    }
                }
            }
        }

        static void fill_rhs_list(const charset_map& map_, charset_list& list_)
        {
            for (const auto& pair_ : map_)
            {
                list_.emplace_back(std::make_unique<charset>
                    (pair_.first, pair_.second));
            }
        }

        static void build_set_mapping(const charset_list& charset_list_,
            internals& internals_, const id_type dfa_index_,
            index_set_vector& set_mapping_)
        {
            auto iter_ = charset_list_.cbegin();
            auto end_ = charset_list_.cend();

            for (id_type index_ = 0; iter_ != end_; ++iter_, ++index_)
            {
                observer_ptr<const charset> cs_ = iter_->get();

                fill_lookup(cs_->_token, &internals_._lookup[dfa_index_],
                    index_, lookup());

                for (const id_type i_ : cs_->_index_set)
                {
                    set_mapping_[i_].insert(index_);
                }
            }
        }

        // char_state_machine version
        static void fill_lookup(const string_token&,
            observer_ptr<id_type_vector>,
            const id_type, const std::false_type&)
        {
            // Do nothing (lookup not used)
        }

        // state_machine version
        static void fill_lookup(const string_token& charset_,
            observer_ptr<id_type_vector> lookup_, const id_type index_,
            const std::true_type&)
        {
            observer_ptr<id_type> ptr_ = &lookup_->front();

            for (const auto& range_ : charset_._ranges)
            {
                for (typename char_traits::index_type char_ = range_.first;
                    char_ < range_.second; ++char_)
                {
                    // Note char_ must be unsigned
                    ptr_[char_] = index_ + transitions_index;
                }

                // Note range_.second must be unsigned
                ptr_[range_.second] = index_ + transitions_index;
            }
        }

        static id_type closure(const node_vector& followpos_,
            node_set_vector& seen_sets_, node_vector_vector& seen_vectors_,
            size_t_vector& hash_vector_, const id_type size_,
            id_type_vector& dfa_)
        {
            bool end_state_ = false;
            id_type id_ = 0;
            id_type user_id_ = sm_traits::npos();
            id_type next_dfa_ = 0;
            id_type push_dfa_ = sm_traits::npos();
            bool pop_dfa_ = false;
            std::size_t hash_ = 0;

            if (followpos_.empty()) return sm_traits::npos();

            id_type index_ = 0;
            std::unique_ptr<node_set> set_ptr_ = std::make_unique<node_set>();
            std::unique_ptr<node_vector> vector_ptr_ =
                std::make_unique<node_vector>();

            for (observer_ptr<node> node_ : followpos_)
            {
                closure_ex(node_, end_state_, id_, user_id_, next_dfa_,
                    push_dfa_, pop_dfa_, *set_ptr_, *vector_ptr_, hash_);
            }

            bool found_ = false;
            auto hash_iter_ = hash_vector_.cbegin();
            auto hash_end_ = hash_vector_.cend();
            auto set_iter_ = seen_sets_.cbegin();

            for (; hash_iter_ != hash_end_; ++hash_iter_, ++set_iter_)
            {
                found_ = *hash_iter_ == hash_ && *(*set_iter_) == *set_ptr_;
                ++index_;

                if (found_) break;
            }

            if (!found_)
            {
                seen_sets_.emplace_back(std::move(set_ptr_));
                seen_vectors_.emplace_back(std::move(vector_ptr_));
                hash_vector_.push_back(hash_);
                // State 0 is the jam state...
                index_ = static_cast<id_type>(seen_sets_.size());

                const std::size_t old_size_ = dfa_.size();

                dfa_.resize(old_size_ + size_, 0);

                if (end_state_)
                {
                    dfa_[old_size_] |= end_state_bit;

                    if (pop_dfa_)
                    {
                        dfa_[old_size_] |= pop_dfa_bit;
                    }

                    dfa_[old_size_ + id_index] = id_;
                    dfa_[old_size_ + user_id_index] = user_id_;
                    dfa_[old_size_ + push_dfa_index] = push_dfa_;
                    dfa_[old_size_ + next_dfa_index] = next_dfa_;
                }
            }

            return index_;
        }

        static void closure_ex(observer_ptr<node> node_, bool& end_state_,
            id_type& id_, id_type& user_id_, id_type& next_dfa_,
            id_type& push_dfa_, bool& pop_dfa_, node_set& set_ptr_,
            node_vector& vector_ptr_, std::size_t& hash_)
        {
            const bool temp_end_state_ = node_->end_state();

            if (temp_end_state_)
            {
                if (!end_state_)
                {
                    end_state_ = true;
                    id_ = node_->id();
                    user_id_ = node_->user_id();
                    next_dfa_ = node_->next_dfa();
                    push_dfa_ = node_->push_dfa();
                    pop_dfa_ = node_->pop_dfa();
                }
            }

            if (set_ptr_.insert(node_).second)
            {
                vector_ptr_.push_back(node_);
                hash_ += reinterpret_cast<std::size_t>(node_);
            }
        }

        // NFA version
        static void build_equiv_list(const node_vector& vector_,
            const index_set_vector& set_mapping_, equivset_list& lhs_,
            const std::false_type&)
        {
            fill_rhs_list(vector_, set_mapping_, lhs_);
        }

        // DFA version
        static void build_equiv_list(const node_vector& vector_,
            const index_set_vector& set_mapping_, equivset_list& lhs_,
            const std::true_type&)
        {
            equivset_list rhs_;

            fill_rhs_list(vector_, set_mapping_, rhs_);

            if (!rhs_.empty())
            {
                typename equivset_list::iterator iter_;
                typename equivset_list::iterator end_;
                equivset_ptr overlap_ = std::make_unique<equivset>();

                lhs_.emplace_back(std::move(rhs_.front()));
                rhs_.pop_front();

                while (!rhs_.empty())
                {
                    equivset_ptr r_(rhs_.front().release());

                    rhs_.pop_front();
                    iter_ = lhs_.begin();
                    end_ = lhs_.end();

                    while (!r_->empty() && iter_ != end_)
                    {
                        auto l_iter_ = iter_;

                        (*l_iter_)->intersect(*r_.get(), *overlap_.get());

                        if (overlap_->empty())
                        {
                            ++iter_;
                        }
                        else if ((*l_iter_)->empty())
                        {
                            l_iter_->reset(overlap_.release());
                            overlap_ = std::make_unique<equivset>();
                            ++iter_;
                        }
                        else if (r_->empty())
                        {
                            r_.reset(overlap_.release());
                            overlap_ = std::make_unique<equivset>();
                            break;
                        }
                        else
                        {
                            iter_ = lhs_.insert(++iter_, equivset_ptr());
                            iter_->reset(overlap_.release());
                            overlap_ = std::make_unique<equivset>();
                            ++iter_;
                            end_ = lhs_.end();
                        }
                    }

                    if (!r_->empty())
                    {
                        lhs_.emplace_back(std::move(r_));
                    }
                }
            }
        }

        static void fill_rhs_list(const node_vector& vector_,
            const index_set_vector& set_mapping_, equivset_list& list_)
        {
            for (observer_ptr<const node> node_ : vector_)
            {
                if (!node_->end_state())
                {
                    const id_type token_ = node_->token();

                    if (token_ != node::null_token())
                    {
                        if (token_ == parser::bol_token() ||
                            token_ == parser::eol_token())
                        {
                            std::set<id_type> index_set_;

                            index_set_.insert(token_);
                            list_.emplace_back
                            (std::make_unique<equivset>(index_set_,
                                token_, node_->greedy(), node_->followpos()));
                        }
                        else
                        {
                            list_.emplace_back(std::make_unique<equivset>
                                (set_mapping_[token_], token_, node_->greedy(),
                                    node_->followpos()));
                        }
                    }
                }
            }
        }
    };

    using generator = basic_generator<rules, state_machine>;
    using wgenerator = basic_generator<wrules, wstate_machine>;
    using u32generator = basic_generator<u32rules, u32state_machine>;
    using char_generator = basic_generator<rules, char_state_machine>;
    using wchar_generator = basic_generator<wrules, wchar_state_machine>;
    using u32char_generator = basic_generator<u32rules, u32char_state_machine>;
}

#endif
