// stream_shared_iterator.hpp
// Copyright (c) 2010-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LEXERTL_STREAM_SHARED_ITERATOR_H
#define LEXERTL_STREAM_SHARED_ITERATOR_H

#include <algorithm>
// memcpy
#include <cstring>
#include <iostream>
#include <list>
#include <math.h>
#include "runtime_error.hpp"
#include "size_t.hpp"
#include <vector>

namespace lexertl
{
template<typename char_type>
class basic_stream_shared_iterator
{
public:
    typedef std::basic_istream<char_type> istream;
    typedef std::forward_iterator_tag iterator_category;
    typedef std::size_t difference_type;
    typedef char_type value_type;
    typedef char_type *pointer;
    typedef char_type &reference;

    basic_stream_shared_iterator() :
        _master(false),
        _live(false),
        _index(shared::npos()),
        _shared(0)
    {
    }

    basic_stream_shared_iterator(istream &stream_,
        const std::size_t buff_size_ = 1024,
        const std::size_t increment_ = 1024) :
        _master(true),
        _live(false),
        _index(shared::npos()),
        // For exception safety don't call new yet
        _shared(0)
    {
        // Safe to call potentially throwing new now.
        _shared = new shared(stream_, buff_size_, increment_);
        ++_shared->_ref_count;
        _iter = _shared->_clients.insert(_shared->_clients.end(), this);
    }

    basic_stream_shared_iterator(const basic_stream_shared_iterator &rhs_) :
        _master(false),
        _live(false),
        _index(rhs_._master ? rhs_._shared->lowest() : rhs_._index),
        _shared(rhs_._shared)
    {
        if (_shared)
        {
            // New copy of an iterator.
            // The assumption is that any copy must be live
            // even if the rhs is not (otherwise we will never
            // have a record of the start of the current range!)
            ++_shared->_ref_count;
            _iter = _shared->_clients.insert(_shared->_clients.end(), this);
            _live = true;
        }
    }

    ~basic_stream_shared_iterator()
    {
        if (_shared)
        {
            --_shared->_ref_count;
            _shared->erase(this);

            if (_shared->_ref_count == 0)
            {
                delete _shared;
                _shared = 0;
            }
        }
    }

    basic_stream_shared_iterator &operator =
        (const basic_stream_shared_iterator &rhs_)
    {
        if (this != &rhs_)
        {
            _master = false;
            _index  = rhs_._master ? rhs_._shared->lowest() : rhs_._index;

            if (_live && !rhs_._live)
            {
                _shared->erase(this);

                if (!rhs_._shared)
                {
                    --_shared->_ref_count;
                }
            }
            else if (!_live && rhs_._live)
            {
                rhs_._iter = rhs_._shared->_clients.insert(rhs_._shared->
                    _clients.end(), this);

                if (!_shared)
                {
                    ++rhs_._shared->_ref_count;
                }
            }

            _live = rhs_._live;
            _shared = rhs_._shared;
        }

        return *this;
    }

    bool operator ==(const basic_stream_shared_iterator &rhs_) const
    {
        return _index == rhs_._index &&
            (_shared == rhs_._shared ||
            (_index == shared::npos() || rhs_._index == shared::npos()) &&
            (!_shared || !rhs_._shared));
    }

    bool operator !=(const basic_stream_shared_iterator &rhs_) const
    {
        return !(*this == rhs_);
    }

    const char_type &operator *()
    {
        check_master();
        return _shared->_buffer[_index];
    }

    basic_stream_shared_iterator &operator ++()
    {
        check_master();
        ++_index;
        update_state();
        return *this;
    }

    basic_stream_shared_iterator operator ++(int)
    {
        basic_stream_shared_iterator iter_ = *this;

        check_master();
        ++_index;
        update_state();
        return iter_;
    }

private:
    class shared
    {
    public:
        std::size_t _ref_count;
        typedef std::vector<char_type> char_vector;
        typedef std::list<basic_stream_shared_iterator *> iter_list;
        istream &_stream;
        std::size_t _increment;
        std::size_t _len;
        char_vector _buffer;
        iter_list _clients;

        shared(istream &stream_, const std::size_t buff_size_,
            const std::size_t increment_) :
            _ref_count(0),
            _increment(increment_),
            _stream(stream_)
        {
            _buffer.resize(buff_size_);
            _stream.read(&_buffer.front(), _buffer.size());
            _len = static_cast<std::size_t>(_stream.gcount());
        }

        bool reload_buffer()
        {
            const std::size_t lowest_ = lowest();
            std::size_t read_ = 0;

            if (lowest_ == 0)
            {
                // Resize buffer
                const std::size_t old_size_ = _buffer.size();
                const std::size_t new_size_ = old_size_ + _increment;

                _buffer.resize(new_size_);
                _stream.read(&_buffer.front() + old_size_, _increment);
                read_ = static_cast<std::size_t>(_stream.gcount());

                if (read_)
                {
                    read_ += old_size_;
                    _len = read_;
                }
            }
            else
            {
                // Some systems have memcpy in namespace std
                using namespace std;
                const size_t start_ = _buffer.size() - lowest_;
                const size_t len_ = _buffer.size() - start_;

                memcpy(&_buffer.front(), &_buffer[lowest_], start_ *
                    sizeof(char_type));
                _stream.read(&_buffer.front() + start_, len_);
                read_ = static_cast<size_t>(_stream.gcount());
                subtract(lowest_);

                if (read_)
                {
                    read_ += start_;
                    _len = read_;
                }
                else
                {
                    _len = highest();
                }
            }

            return read_ != 0;
        }

        void erase(basic_stream_shared_iterator *ptr_)
        {
            if (ptr_->_iter != _clients.end())
            {
                _clients.erase(ptr_->_iter);
                ptr_->_iter = _clients.end();
            }
        }

        std::size_t lowest() const
        {
            std::size_t lowest_ = npos();
            typename iter_list::const_iterator iter_ = _clients.begin();
            typename iter_list::const_iterator end_ = _clients.end();

            for (; iter_ != end_; ++iter_)
            {
                const basic_stream_shared_iterator *ptr_ = *iter_;

                if (ptr_->_index < lowest_)
                {
                    lowest_ = ptr_->_index;
                }
            }

            if (lowest_ == npos())
            {
                lowest_ = 0;
            }

            return lowest_;
        }

        std::size_t highest() const
        {
            std::size_t highest_ = 0;
            typename iter_list::const_iterator iter_ = _clients.begin();
            typename iter_list::const_iterator end_ = _clients.end();

            for (; iter_ != end_; ++iter_)
            {
                const basic_stream_shared_iterator *ptr_ = *iter_;

                if (ptr_->_index != npos() && ptr_->_index > highest_)
                {
                    highest_ = ptr_->_index;
                }
            }

            return highest_;
        }

        void subtract(const std::size_t lowest_)
        {
            typename iter_list::iterator iter_ = _clients.begin();
            typename iter_list::iterator end_ = _clients.end();

            for (; iter_ != end_; ++iter_)
            {
                basic_stream_shared_iterator *ptr_ = *iter_;

                if (ptr_->_index != npos())
                {
                    ptr_->_index -= lowest_;
                }
            }
        }

        static std::size_t npos()
        {
            return ~static_cast<std::size_t>(0);
        }

    private:
        shared &operator =(const shared &rhs_);
    };

    bool _master;
    bool _live;
    std::size_t _index;
    shared *_shared;
    mutable typename shared::iter_list::iterator _iter;

    void check_master()
    {
        if (!_shared)
        {
            throw runtime_error("Cannot manipulate null (end) "
                "stream_shared_iterators.");
        }

        if (_master)
        {
            _master = false;
            _live = true;
            _index = _shared->lowest();
        }
    }

    void update_state()
    {
        if (_index >= _shared->_len)
        {
            if (!_shared->reload_buffer())
            {
                _shared->erase(this);
                _index = shared::npos();
                _live = false;
            }
        }
    }
};

typedef basic_stream_shared_iterator<char> stream_shared_iterator;
typedef basic_stream_shared_iterator<wchar_t> wstream_shared_iterator;
}

#endif
