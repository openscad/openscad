// string_token.hpp
// Copyright (c) 2005-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_STRING_TOKEN_HPP
#define LEXERTL_STRING_TOKEN_HPP

#include "char_traits.hpp"
#include <ios> // Needed by GCC 4.4
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace lexertl
{
    template<typename ch_type>
    struct basic_string_token
    {
        using char_type = ch_type;
        using char_traits = basic_char_traits<char_type>;
        using index_type = typename char_traits::index_type;
        using range = std::pair<index_type, index_type>;
        using range_vector = std::vector<range>;
        using string = std::basic_string<char_type>;
        using string_token = basic_string_token<char_type>;

        range_vector _ranges;

        basic_string_token() :
            _ranges()
        {
        }

        basic_string_token(char_type ch_) :
            _ranges()
        {
            insert(range(ch_, ch_));
        }

        basic_string_token(char_type first_, char_type second_) :
            _ranges()
        {
            insert(range(first_, second_));
        }

        void clear()
        {
            _ranges.clear();
        }

        bool empty() const
        {
            return _ranges.empty();
        }

        std::size_t size() const
        {
            return _ranges.size();
        }

        bool any() const
        {
            return _ranges.size() == 1 && _ranges.front().first == 0 &&
                _ranges.front().second == char_traits::max_val();
        }

        bool operator <(const basic_string_token& rhs_) const
        {
            return _ranges < rhs_._ranges;
        }

        bool operator ==(const basic_string_token& rhs_) const
        {
            return _ranges == rhs_._ranges;
        }

        bool negatable() const
        {
            std::size_t size_ = 0;
            auto iter_ = _ranges.cbegin();
            auto end_ = _ranges.cend();

            for (; iter_ != end_; ++iter_)
            {
                size_ += static_cast<std::size_t>(iter_->second) + 1 -
                    static_cast<std::size_t>(iter_->first);
            }

            return size_ > static_cast<std::size_t>(char_traits::max_val()) / 2;
        }

        void swap(basic_string_token& rhs_)
        {
            _ranges.swap(rhs_._ranges);
        }

        void insert(const basic_string_token& rhs_)
        {
            auto iter_ = rhs_._ranges.cbegin();
            auto end_ = rhs_._ranges.cend();

            for (; iter_ != end_; ++iter_)
            {
                insert(*iter_);
            }
        }

        // Deliberately pass by value - may modify
        typename range_vector::iterator insert(range rhs_)
        {
            bool insert_ = true;
            auto iter_ = _ranges.begin();
            auto end_ = _ranges.end();
            auto erase_iter_ = end_;

            while (iter_ != end_)
            {
                // follows current item
                if (rhs_.first > iter_->second)
                {
                    if (rhs_.first == iter_->second + 1)
                    {
                        // Auto normalise
                        rhs_.first = iter_->first;
                    }
                    else
                    {
                        // No intersection, consider next
                        ++iter_;
                        continue;
                    }
                }
                // Precedes current item
                else if (rhs_.second < iter_->first)
                {
                    if (rhs_.second == iter_->first - 1)
                    {
                        // Auto normalise
                        rhs_.second = iter_->second;
                    }
                    else
                    {
                        // insert here
                        break;
                    }
                }
                else
                {
                    // overlap (under)
                    if (rhs_.first < iter_->first)
                    {
                        if (rhs_.second < iter_->second)
                        {
                            rhs_.second = iter_->second;
                        }
                    }
                    // overlap (over)
                    else if (rhs_.second > iter_->second)
                    {
                        if (rhs_.first > iter_->first)
                        {
                            rhs_.first = iter_->first;
                        }
                    }
                    // subset
                    else
                    {
                        insert_ = false;
                        iter_ = _ranges.end();
                        break;
                    }
                }

                // Code minimisation: this always applies unless we have already
                // exited the loop, or "continue" executed.
                if (erase_iter_ == end_)
                {
                    erase_iter_ = iter_;
                }

                ++iter_;
            }

            if (erase_iter_ != end_)
            {
                if (insert_)
                {
                    // Re-use obsolete location
                    *erase_iter_ = rhs_;
                    ++erase_iter_;
                }

                iter_ = _ranges.erase(erase_iter_, iter_);
            }
            else if (insert_)
            {
                iter_ = _ranges.insert(iter_, rhs_);
            }

            return iter_;
        }

        void negate()
        {
            index_type next_ = 0;
            const index_type max_ = char_traits::max_val();
            string_token temp_;
            auto iter_ = _ranges.cbegin();
            auto end_ = _ranges.cend();
            bool finished_ = false;

            for (; iter_ != end_; ++iter_)
            {
                if (next_ < iter_->first)
                {
                    temp_.insert(range(next_,
                        static_cast<index_type>(iter_->first - 1)));
                }

                if (iter_->second < max_)
                {
                    next_ = iter_->second + 1;
                }
                else
                {
                    finished_ = true;
                    break;
                }
            }

            if (!finished_)
            {
                temp_.insert(range(next_, max_));
            }

            swap(temp_);
        }

        void intersect(basic_string_token& rhs_, basic_string_token& overlap_)
        {
            auto lhs_iter_ = _ranges.begin();
            auto lhs_end_ = _ranges.end();
            auto rhs_iter_ = rhs_._ranges.begin();
            auto rhs_end_ = rhs_._ranges.end();

            while (lhs_iter_ != lhs_end_ && rhs_iter_ != rhs_end_)
            {
                if (rhs_iter_->first > lhs_iter_->second)
                {
                    ++lhs_iter_;
                }
                else if (rhs_iter_->second < lhs_iter_->first)
                {
                    ++rhs_iter_;
                }
                else
                {
                    range range_;

                    if (rhs_iter_->first > lhs_iter_->first)
                    {
                        range_.first = rhs_iter_->first;
                    }
                    else
                    {
                        range_.first = lhs_iter_->first;
                    }

                    if (rhs_iter_->second < lhs_iter_->second)
                    {
                        range_.second = rhs_iter_->second;
                    }
                    else
                    {
                        range_.second = lhs_iter_->second;
                    }

                    adjust(range_, *this, lhs_iter_, lhs_end_);
                    adjust(range_, rhs_, rhs_iter_, rhs_end_);
                    overlap_.insert(range_);
                }
            }
        }

        void remove(const basic_string_token& rhs_)
        {
            auto lhs_iter_ = _ranges.begin();
            auto lhs_end_ = _ranges.end();
            auto rhs_iter_ = rhs_._ranges.cbegin();
            auto rhs_end_ = rhs_._ranges.cend();

            while (lhs_iter_ != lhs_end_ && rhs_iter_ != rhs_end_)
            {
                if (rhs_iter_->first > lhs_iter_->second)
                {
                    ++lhs_iter_;
                }
                else if (rhs_iter_->second < lhs_iter_->first)
                {
                    ++rhs_iter_;
                }
                else
                {
                    range range_;

                    if (rhs_iter_->first > lhs_iter_->first)
                    {
                        range_.first = rhs_iter_->first;
                    }
                    else
                    {
                        range_.first = lhs_iter_->first;
                    }

                    if (rhs_iter_->second < lhs_iter_->second)
                    {
                        range_.second = rhs_iter_->second;
                    }
                    else
                    {
                        range_.second = lhs_iter_->second;
                    }

                    adjust(range_, *this, lhs_iter_, lhs_end_);
                }
            }
        }

        static string escape_char(const typename char_traits::index_type ch_)
        {
            string out_;

            switch (ch_)
            {
            case '\0':
                out_ += '\\';
                out_ += '0';
                break;
            case '\a':
                out_ += '\\';
                out_ += 'a';
                break;
            case '\b':
                out_ += '\\';
                out_ += 'b';
                break;
            case 27:
                out_ += '\\';
                out_ += 'x';
                out_ += '1';
                out_ += 'b';
                break;
            case '\f':
                out_ += '\\';
                out_ += 'f';
                break;
            case '\n':
                out_ += '\\';
                out_ += 'n';
                break;
            case '\r':
                out_ += '\\';
                out_ += 'r';
                break;
            case '\t':
                out_ += '\\';
                out_ += 't';
                break;
            case '\v':
                out_ += '\\';
                out_ += 'v';
                break;
            case '\\':
                out_ += '\\';
                out_ += '\\';
                break;
            case '"':
                out_ += '\\';
                out_ += '"';
                break;
            case '\'':
                out_ += '\\';
                out_ += '\'';
                break;
            default:
            {
                if (ch_ < 32 || ch_ > 126)
                {
                    std::basic_stringstream<char_type> ss_;

                    out_ += '\\';
                    out_ += 'x';
                    ss_ << std::hex <<
                        static_cast<std::size_t>(ch_);
                    out_ += ss_.str();
                }
                else
                {
                    out_ += ch_;
                }

                break;
            }
            }

            return out_;
        }

    private:
        void adjust(const range& range_, basic_string_token& token_,
            typename range_vector::iterator& iter_,
            typename range_vector::iterator& end_)
        {
            if (range_.first > iter_->first)
            {
                const index_type second_ = iter_->second;

                iter_->second = range_.first - 1;

                if (range_.second < second_)
                {
                    range new_range_(static_cast<index_type>(range_.second + 1),
                        second_);

                    iter_ = token_.insert(new_range_);
                    end_ = token_._ranges.end();
                }
            }
            else if (range_.second < iter_->second)
            {
                iter_->first = range_.second + 1;
            }
            else
            {
                iter_ = token_._ranges.erase(iter_);
                end_ = token_._ranges.end();
            }
        }
    };
}

#endif
