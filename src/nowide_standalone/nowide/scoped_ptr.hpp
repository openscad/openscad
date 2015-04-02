#ifndef NOWIDE_SCOPED_PTR_HPP
#define NOWIDE_SCOPED_PTR_HPP

//  (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
//  Copyright (c) 2001, 2002 Peter Dimov, 
//  Copyright (C) 2012 Artyom Beilis
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  http://www.boost.org/libs/smart_ptr/scoped_ptr.htm
//

#include <assert.h>

namespace nowide
{

//  scoped_ptr mimics a built-in pointer except that it guarantees deletion
//  of the object pointed to, either on destruction of the scoped_ptr or via
//  an explicit reset(). scoped_ptr is a simple solution for simple needs;
//  use shared_ptr or std::auto_ptr if your needs are more complex.

template<class T> class scoped_ptr // noncopyable
{
private:

    T * px;

    scoped_ptr(scoped_ptr const &);
    scoped_ptr & operator=(scoped_ptr const &);

    typedef scoped_ptr<T> this_type;

    void operator==( scoped_ptr const& ) const;
    void operator!=( scoped_ptr const& ) const;

public:

    typedef T element_type;

    explicit scoped_ptr( T * p = 0 ): px( p ) // never throws
    {
    }

    ~scoped_ptr() // never throws
    {
        delete px;
    }

    void reset(T * p = 0) // never throws
    {
        assert( p == 0 || p != px ); // catch self-reset errors
        this_type(p).swap(*this);
    }

    T & operator*() const // never throws
    {
        assert( px != 0 );
        return *px;
    }

    T * operator->() const // never throws
    {
        assert( px != 0 );
        return px;
    }

    T * get() const // never throws
    {
        return px;
    }

    operator bool() const
    {
        return px!=0;
    }

    void swap(scoped_ptr & b) // never throws
    {
        T * tmp = b.px;
        b.px = px;
        px = tmp;
    }
};


} // namespace nowide

#endif
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

