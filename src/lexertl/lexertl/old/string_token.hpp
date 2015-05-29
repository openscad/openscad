// string_token.hpp
// Copyright (c) 2005-2010 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_STRING_TOKEN_HPP
#define LEXERTL_STRING_TOKEN_HPP

#include "../char_traits.hpp"
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace lexertl
{
template<typename char_type>
struct basic_string_token
{
    typedef std::basic_string<char_type> string;

    bool _negated;
    string _chars;

    basic_string_token() :
        _negated(false)
    {
    }

    basic_string_token(const bool negated_, const string &chars_) :
        _negated(negated_),
        _chars(chars_)
    {
    }

    void remove_duplicates()
    {
        const char_type *start_ = _chars.c_str();
        const char_type *end_ = start_ + _chars.size();

        // Optimisation for very large charsets:
        // sorting via pointers is much quicker than
        // via iterators...
        std::sort(const_cast<char_type *>(start_), const_cast<char_type *>
            (end_));
        _chars.erase(std::unique(_chars.begin(), _chars.end()),
            _chars.end());
    }

    void normalise()
    {
        const std::size_t max_chars_ = sizeof(char_type) == 1 ?
            num_chars : num_wchar_ts;

        if (_chars.length() == max_chars_)
        {
            _negated = !_negated;
            _chars.clear();
        }
        else if (_chars.length() > max_chars_ / 2)
        {
            negate();
        }
    }

    void negate()
    {
        const std::size_t max_chars_ = sizeof(char_type) == 1 ?
            num_chars : num_wchar_ts;
        char_type curr_char_ = std::numeric_limits<CharT>::min();
        string temp_;
        const char_type *curr_ = _chars.c_str();
        const char_type *chars_end_ = curr_ + _chars.size();

        _negated = !_negated;
        temp_.resize(max_chars_ - _chars.size());

        char_type *ptr_ = const_cast<char_type *>(temp_.c_str());
        std::size_t i_ = 0;

        while (curr_ < chars_end_)
        {
            while (*curr_ > curr_char_)
            {
                *ptr_ = curr_char_;
                ++ptr_;
                ++curr_char_;
                ++i_;
            }

            ++curr_char_;
            ++curr_;
            ++i_;
        }

        for (; i_ < max_chars_; ++i_)
        {
            *ptr_ = curr_char_;
            ++ptr_;
            ++curr_char_;
        }

        _chars = temp_;
    }

    bool operator <(const basic_string_token &rhs_) const
    {
        return _negated < rhs_._negated ||
            (_negated == rhs_._negated && _chars < rhs_._chars);
    }

    bool operator ==(const basic_string_token &rhs_) const
    {
        return _negated == rhs_._negated && _chars == rhs_._chars;
    }

    bool empty() const
    {
        return _chars.empty() && !_negated;
    }

    bool any() const
    {
        return _chars.empty() && _negated;
    }

    void clear()
    {
        _negated = false;
        _chars.clear();
    }

    void intersect(basic_string_token &rhs_, basic_string_token &overlap_)
    {
        if ((any() && rhs_.any()) || (_negated == rhs_._negated &&
            !any() && !rhs_.any()))
        {
            intersect_same_types(rhs_, overlap_);
        }
        else
        {
            intersect_diff_types(rhs_, overlap_);
        }
    }

    void merge(const basic_string_token &rhs_,
        basic_string_token &merged_) const
    {
        if ((any() && rhs_.any()) || (_negated == rhs_._negated &&
            !any() && !rhs_.any()))
        {
            merge_same_types(rhs_, merged_);
        }
        else
        {
            merge_diff_types(rhs_, merged_);
        }
    }

    static string escape_char(const char_type ch_)
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
                if (ch_ < 32)
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
    void intersect_same_types(basic_string_token &rhs_,
        basic_string_token &overlap_)
    {
        if (any())
        {
            clear();
            overlap_._negated = true;
            rhs_.clear();
        }
        else
        {
            typename string::iterator iter_ = _chars.begin();
            typename string::iterator end_ = _chars.end();
            typename string::iterator rhs_iter_ = rhs_._chars.begin();
            typename string::iterator rhs_end_ = rhs_._chars.end();

            overlap_._negated = _negated;

            while (iter_ != end_ && rhs_iter_ != rhs_end_)
            {
                if (*iter_ < *rhs_iter_)
                {
                    ++iter_;
                }
                else if (*iter_ > *rhs_iter_)
                {
                    ++rhs_iter_;
                }
                else
                {
                    overlap_._chars += *iter_;
                    iter_ = _chars.erase(iter_);
                    end_ = _chars.end();
                    rhs_iter_ = rhs_._chars.erase(rhs_iter_);
                    rhs_end_ = rhs_._chars.end();
                }
            }

            if (_negated)
            {
                // duplicates already merged, so safe to merge
                // using std lib.

                // src, dest
                merge(_chars, overlap_._chars);
                // duplicates already merged, so safe to merge
                // using std lib.

                // src, dest
                merge(rhs_._chars, overlap_._chars);
                _negated = false;
                rhs_._negated = false;
                std::swap(_chars, rhs_._chars);
                normalise();
                overlap_.normalise();
                rhs_.normalise();
            }
            else if (!overlap_._chars.empty())
            {
                normalise();
                overlap_.normalise();
                rhs_.normalise();
            }
        }
    }

    void intersect_diff_types(basic_string_token &rhs_,
        basic_string_token &overlap_)
    {
        if (any())
        {
            intersect_any(rhs_, overlap_);
        }
        else if (_negated)
        {
            intersect_negated(rhs_, overlap_);
        }
        else // _negated == false
        {
            intersect_charset(rhs_, overlap_);
        }
    }

    void intersect_any(basic_string_token &rhs_, basic_string_token &overlap_)
    {
        if (rhs_._negated)
        {
            rhs_.intersect_negated(*this, overlap_);
        }
        else // rhs._negated == false
        {
            rhs_.intersect_charset(*this, overlap_);
        }
    }

    void intersect_negated(basic_string_token &rhs_,
        basic_string_token &overlap_)
    {
        if (rhs_.any())
        {
            overlap_._negated = true;
            overlap_._chars = _chars;
            rhs_._negated = false;
            rhs_._chars = _chars;
            clear();
        }
        else // rhs._negated == false
        {
            rhs_.intersect_charset(*this, overlap_);
        }
    }

    void intersect_charset(basic_string_token &rhs_,
        basic_string_token &overlap_)
    {
        if (rhs_.any())
        {
            overlap_._chars = _chars;
            rhs_._negated = true;
            rhs_._chars = _chars;
            clear();
        }
        else // rhs_._negated == true
        {
            typename string::iterator iter_ = _chars.begin();
            typename string::iterator end_ = _chars.end();
            typename string::iterator rhs_iter_ = rhs_._chars.begin();
            typename string::iterator rhs_end_ = rhs_._chars.end();

            while (iter_ != end_ && rhs_iter_ != rhs_end_)
            {
                if (*iter_ < *rhs_iter_)
                {
                    overlap_._chars += *iter_;
                    rhs_iter_ = rhs_._chars.insert(rhs_iter_, *iter_);
                    ++rhs_iter_;
                    rhs_end_ = rhs_._chars.end();
                    iter_ = _chars.erase(iter_);
                    end_ = _chars.end();
                }
                else if (*iter_ > *rhs_iter_)
                {
                    ++rhs_iter_;
                }
                else
                {
                    ++iter_;
                    ++rhs_iter_;
                }
            }

            if (iter_ != end_)
            {
                // nothing bigger in rhs_ than iter_,
                // so safe to merge using std lib.
                string temp_(iter_, end_);

                // src, dest
                merge(temp_, overlap_._chars);
                _chars.erase(iter_, end_);
            }

            if (!overlap_._chars.empty())
            {
                merge(overlap_._chars, rhs_._chars);
                // possible duplicates, so check for any and erase.
                rhs_._chars.erase(std::unique(rhs_._chars.begin(),
                    rhs_._chars.end()), rhs_._chars.end());
                normalise();
                overlap_.normalise();
                rhs_.normalise();
            }
        }
    }

    void merge(string &src_, string &dest_)
    {
        string tmp_(src_.size() + dest_.size(), 0);

        std::merge(src_.begin(), src_.end(), dest_.begin(), dest_.end(),
            tmp_.begin());
        dest_ = tmp_;
    }

    void merge_same_types(const basic_string_token &rhs_,
        basic_string_token &merged_) const
    {
        if (any())
        {
            merged_._negated = true;
        }
        else if (_negated)
        {
            typename string::const_iterator iter_ = _chars.begin();
            typename string::const_iterator end_ = _chars.end();
            typename string::const_iterator rhs_iter_ = rhs_._chars.begin();
            typename string::const_iterator rhs_end_ = rhs_._chars.end();

            merged_._negated = _negated;

            while (iter_ != end_ && rhs_iter_ != rhs_end_)
            {
                if (*iter_ < *rhs_iter_)
                {
                    ++iter_;
                }
                else if (*iter_ > *rhs_iter_)
                {
                    ++rhs_iter_;
                }
                else
                {
                    merged_._chars += *iter_;
                    ++iter_;
                    ++rhs_iter_;
                }
            }

            merged_.normalise();
        }
        else
        {
            typename string::const_iterator iter_ = _chars.begin();
            typename string::const_iterator end_ = _chars.end();
            typename string::const_iterator rhs_iter_ = rhs_._chars.begin();
            typename string::const_iterator rhs_end_ = rhs_._chars.end();

            while (iter_ != end_ && rhs_iter_ != rhs_end_)
            {
                if (*iter_ < *rhs_iter_)
                {
                    merged_._chars += *iter_;
                    ++iter_;
                }
                else if (*iter_ > *rhs_iter_)
                {
                    merged_._chars += *rhs_iter_;
                    ++rhs_iter_;
                }
                else
                {
                    merged_._chars += *iter_;
                    ++iter_;
                    ++rhs_iter_;
                }
            }

            // Include any trailing chars
            if (iter_ != end_)
            {
                string temp_(iter_, end_);

                merged_._chars += temp_;
            }
            else if (rhs_iter_ != rhs_end_)
            {
                string temp_(rhs_iter_, rhs_end_);

                merged_._chars += temp_;
            }

            merged_.normalise();
        }
    }

    void merge_diff_types(const basic_string_token &rhs_,
        basic_string_token &merged_) const
    {
        if (_negated)
        {
            merge_negated(*this, rhs_, merged_);
        }
        else
        {
            merge_negated(rhs_, *this, merged_);
        }

        merged_.normalise();
    }

    void merge_negated(const basic_string_token &lhs_,
        const basic_string_token &rhs_, basic_string_token &merged_) const
    {
        typename string::const_iterator lhs_iter_ = lhs_._chars.begin();
        typename string::const_iterator lhs_end_ = lhs_._chars.end();
        typename string::const_iterator rhs_iter_ = rhs_._chars.begin();
        typename string::const_iterator rhs_end_ = rhs_._chars.end();

        merged_._negated = true;

        while (lhs_iter_ != lhs_end_ && rhs_iter_ != rhs_end_)
        {
            if (*lhs_iter_ < *rhs_iter_)
            {
                merged_._chars += *lhs_iter_;
                ++lhs_iter_;
            }
            else if (*lhs_iter_ > *rhs_iter_)
            {
                ++rhs_iter_;
            }
            else
            {
                ++lhs_iter_;
                ++rhs_iter_;
            }
        }

        // Only interested in any remaining 'negated' chars
        if (lhs_iter_ != lhs_end_)
        {
            string temp_(lhs_iter_, lhs_end_);

            merged_._chars += temp_;
        }
    }
};
}

#endif
