// utf_iterators.hpp
// Copyright (c) 2015-2020 Ben Hanson (http://www.benhanson.net/)
// Inspired by http://utfcpp.sourceforge.net/
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_UTF_ITERATORS_HPP
#define LEXERTL_UTF_ITERATORS_HPP

#include <iterator>

namespace lexertl
{
    template<typename char_iterator, typename char_type>
    class basic_utf8_in_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = char_type;
        using difference_type =
            typename std::iterator_traits<char_iterator>::difference_type;
        using pointer =
            typename std::iterator_traits<char_iterator>::pointer;
        using reference =
            typename std::iterator_traits<char_iterator>::reference;

        basic_utf8_in_iterator() :
            _it(char_iterator()),
            _end(char_iterator()),
            _char(0)
        {
        }

        explicit basic_utf8_in_iterator(const char_iterator& it_,
            const char_iterator& end_) :
            _it(it_),
            _end(it_),
            _char(0)
        {
            if (it_ != end_)
            {
                next();
            }
        }

        char_type operator *() const
        {
            return _char;
        }

        bool operator ==(const basic_utf8_in_iterator& rhs_) const
        {
            return _it == rhs_._it;
        }

        bool operator !=(const basic_utf8_in_iterator& rhs_) const
        {
            return _it != rhs_._it;
        }

        basic_utf8_in_iterator& operator ++()
        {
            _it = _end;
            next();
            return *this;
        }

        basic_utf8_in_iterator operator ++(int)
        {
            basic_utf8_in_iterator temp_ = *this;

            _it = _end;
            next();
            return temp_;
        }

        basic_utf8_in_iterator operator +(const std::size_t count_) const
        {
            basic_utf8_in_iterator temp_ = *this;

            for (std::size_t i_ = 0; i_ < count_; ++i_)
            {
                ++temp_;
            }

            return temp_;
        }

        basic_utf8_in_iterator operator -(const std::size_t count_) const
        {
            basic_utf8_in_iterator temp_ = *this;

            for (std::size_t i_ = 0; i_ < count_; ++i_)
            {
                temp_._end = temp_._it;
                --temp_._it;

                while ((*temp_._it & 0xc0) == 0x80) --temp_._it;
            }

            temp_.next();
            return temp_;
        }

        char_iterator get() const
        {
            return _it;
        }

    private:
        char_iterator _it;
        char_iterator _end;
        char_type _char;

        void next()
        {
            const char len_ = len(_it);
            char_type ch_ = *_it & 0xff;

            switch (len_)
            {
            case 1:
                _end = _it;
                ++_end;
                break;
            case 2:
                _end = _it;
                ++_end;

                if ((*_end & 0xc0) != 0x80) break;

                ch_ = (ch_ << 6 & 0x7ff) | (*_end & 0x3f);
                ++_end;
                break;
            case 3:
                _end = _it;
                ++_end;

                if ((*_end & 0xc0) != 0x80) break;

                ch_ = (ch_ << 12 & 0xffff) | ((*_end & 0xff) << 6 & 0xfff);
                ++_end;

                if ((*_end & 0xc0) != 0x80) break;

                ch_ |= *_end & 0x3f;
                ++_end;
                break;
            case 4:
                _end = _it;
                ++_end;

                if ((*_end & 0xc0) != 0x80) break;

                ch_ = (ch_ << 18 & 0x1fffff) | ((*_end & 0xff) << 12 & 0x3ffff);
                ++_end;

                if ((*_end & 0xc0) != 0x80) break;

                ch_ |= (*_end & 0xff) << 6 & 0xfff;
                ++_end;

                if ((*_end & 0xc0) != 0x80) break;

                ch_ |= *_end & 0x3f;
                ++_end;
                break;
            }

            _char = ch_;
        }

        char len(const char_iterator& it_) const
        {
            const unsigned char ch_ = *it_;

            return ch_ < 0x80 ? 1 :
                ch_ >> 5 == 0x06 ? 2 :
                ch_ >> 4 == 0x0e ? 3 :
                ch_ >> 3 == 0x1e ? 4 :
                1;
        }
    };

    template<typename char_iterator>
    class basic_utf8_out_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = char;
        using difference_type =
            typename std::iterator_traits<char_iterator>::difference_type;
        using pointer =
            typename std::iterator_traits<char_iterator>::pointer;
        using reference =
            typename std::iterator_traits<char_iterator>::reference;

        basic_utf8_out_iterator() :
            _count(0),
            _index(0)
        {
        }

        explicit basic_utf8_out_iterator(const char_iterator& it_,
            const char_iterator& end_) :
            _it(it_),
            _count(0),
            _index(0)
        {
            if (it_ != end_)
            {
                next();
            }
        }

        char operator *() const
        {
            return _bytes[_index];
        }

        bool operator ==(const basic_utf8_out_iterator& rhs_) const
        {
            return _it == rhs_._it;
        }

        bool operator !=(const basic_utf8_out_iterator& rhs_) const
        {
            return _it != rhs_._it;
        }

        basic_utf8_out_iterator& operator ++()
        {
            ++_index;

            if (_index >= _count)
            {
                ++_it;
                next();
            }

            return *this;
        }

        basic_utf8_out_iterator operator ++(int)
        {
            basic_utf8_out_iterator temp_ = *this;

            ++_index;

            if (_index >= _count)
            {
                ++_it;
                next();
            }

            return temp_;
        }

    private:
        char_iterator _it;
        char _bytes[4]{};
        unsigned char _count;
        unsigned char _index;

        void next()
        {
            const std::size_t ch_ = *_it;

            _count = len(ch_);
            _index = 0;

            switch (_count)
            {
            case 1:
                _bytes[0] = static_cast<char>(ch_);
                break;
            case 2:
                _bytes[0] = static_cast<char>((ch_ >> 6) | 0xc0);
                _bytes[1] = (ch_ & 0x3f) | 0x80;
                break;
            case 3:
                _bytes[0] = static_cast<char>((ch_ >> 12) | 0xe0);
                _bytes[1] = ((ch_ >> 6) & 0x3f) | 0x80;
                _bytes[2] = (ch_ & 0x3f) | 0x80;
                break;
            case 4:
                _bytes[0] = static_cast<char>((ch_ >> 18) | 0xf0);
                _bytes[1] = ((ch_ >> 12) & 0x3f) | 0x80;
                _bytes[2] = ((ch_ >> 6) & 0x3f) | 0x80;
                _bytes[3] = (ch_ & 0x3f) | 0x80;
                break;
            }
        }

        char len(const std::size_t ch_) const
        {
            return ch_ < 0x80 ? 1 :
                ch_ < 0x800 ? 2 :
                ch_ < 0x10000 ? 3 :
                4;
        }
    };

    template<typename char_iterator, typename char_type>
    class basic_utf16_in_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = char_type;
        using difference_type =
            typename std::iterator_traits<char_iterator>::difference_type;
        using pointer =
            typename std::iterator_traits<char_iterator>::pointer;
        using reference =
            typename std::iterator_traits<char_iterator>::reference;

        basic_utf16_in_iterator() :
            _it(char_iterator()),
            _end(char_iterator()),
            _char(0)
        {
        }

        explicit basic_utf16_in_iterator(const char_iterator& it_,
            const char_iterator& end_) :
            _it(it_),
            _end(it_),
            _char(0)
        {
            if (it_ != end_)
            {
                next();
            }
        }

        char_type operator *() const
        {
            return _char;
        }

        bool operator ==(const basic_utf16_in_iterator& rhs_) const
        {
            return _it == rhs_._it;
        }

        bool operator !=(const basic_utf16_in_iterator& rhs_) const
        {
            return _it != rhs_._it;
        }

        basic_utf16_in_iterator& operator ++()
        {
            _it = _end;
            next();
            return *this;
        }

        basic_utf16_in_iterator operator ++(int)
        {
            basic_utf16_in_iterator temp_ = *this;

            _it = _end;
            next();
            return temp_;
        }

        basic_utf16_in_iterator operator +(const std::size_t count_) const
        {
            basic_utf16_in_iterator temp_ = *this;

            for (std::size_t i_ = 0; i_ < count_; ++i_)
            {
                ++temp_;
            }

            return temp_;
        }

        basic_utf16_in_iterator operator -(const std::size_t count_) const
        {
            basic_utf16_in_iterator temp_ = *this;

            for (std::size_t i_ = 0; i_ < count_; ++i_)
            {
                temp_._end = temp_._it;
                --temp_._it;

                if (*temp_._it >= 0xdc00 && *temp_._it <= 0xdfff) --temp_._it;
            }

            temp_.next();
            return temp_;
        }

    private:
        char_iterator _it;
        char_iterator _end;
        char_type _char;

        void next()
        {
            char_type ch_ = *_it & 0xffff;

            _end = _it;

            if (ch_ >= 0xd800 && ch_ <= 0xdbff)
            {
                const char_type surrogate_ = *++_end & 0xffff;

                ch_ = (((ch_ - 0xd800) << 10) |
                    (surrogate_ - 0xdc00)) + 0x10000;
            }

            _char = ch_;
            ++_end;
        }
    };

    template<typename char_iterator, typename out_char>
    class basic_utf16_out_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = out_char;
        using difference_type =
            typename std::iterator_traits<char_iterator>::difference_type;
        using pointer =
            typename std::iterator_traits<char_iterator>::pointer;
        using reference =
            typename std::iterator_traits<char_iterator>::reference;

        basic_utf16_out_iterator() :
            _count(0),
            _index(0)
        {
        }

        explicit basic_utf16_out_iterator(const char_iterator& it_,
            const char_iterator& end_) :
            _it(it_),
            _count(0),
            _index(0)
        {
            if (it_ != end_)
            {
                next();
            }
        }

        out_char operator *() const
        {
            return _chars[_index];
        }

        bool operator ==(const basic_utf16_out_iterator& rhs_) const
        {
            return _it == rhs_._it;
        }

        bool operator !=(const basic_utf16_out_iterator& rhs_) const
        {
            return _it != rhs_._it;
        }

        basic_utf16_out_iterator& operator ++()
        {
            ++_index;

            if (_index >= _count)
            {
                ++_it;
                next();
            }

            return *this;
        }

        basic_utf16_out_iterator operator ++(int)
        {
            basic_utf16_out_iterator temp_ = *this;

            ++_index;

            if (_index >= _count)
            {
                ++_it;
                next();
            }

            return temp_;
        }

    private:
        char_iterator _it;
        out_char _chars[2]{};
        unsigned char _count;
        unsigned char _index;

        void next()
        {
            const std::size_t ch_ = *_it;

            _count = len(ch_);
            _index = 0;

            switch (_count)
            {
            case 1:
                _chars[0] = static_cast<out_char>(ch_);
                break;
            case 2:
                _chars[0] = static_cast<out_char>((ch_ >> 10) + 0xdc00u -
                    (0x10000 >> 10));
                _chars[1] = static_cast<out_char>((ch_ & 0x3ff) + 0xdc00u);
                break;
            }
        }

        char len(const std::size_t ch_) const
        {
            return ch_ > 0xffff ? 2 : 1;
        }
    };

    template<typename char_iterator>
    class basic_flip_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type =
            typename std::iterator_traits<char_iterator>::value_type;
        using difference_type =
            typename std::iterator_traits<char_iterator>::difference_type;
        using pointer =
            typename std::iterator_traits<char_iterator>::pointer;
        using reference =
            typename std::iterator_traits<char_iterator>::reference;

        basic_flip_iterator() :
            _it(char_iterator())
        {
        }

        explicit basic_flip_iterator(const char_iterator& it_) :
            _it(it_)
        {
        }

        value_type operator *() const
        {
            value_type val = *_it;
            char* first = reinterpret_cast<char*>(&val);
            char* second = reinterpret_cast<char*>(&val + 1) - 1;

            for (; first < second; ++first, --second)
            {
                std::swap(*first, *second);
            }

            return val;
        }

        bool operator ==(const basic_flip_iterator& rhs_) const
        {
            return _it == rhs_._it;
        }

        bool operator !=(const basic_flip_iterator& rhs_) const
        {
            return _it != rhs_._it;
        }

        basic_flip_iterator& operator ++()
        {
            ++_it;
            return *this;
        }

        basic_flip_iterator operator ++(int)
        {
            basic_flip_iterator temp_ = *this;

            ++_it;
            return temp_;
        }

    private:
        char_iterator _it;
    };
}

#endif
