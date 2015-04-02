//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef NOWIDE_ENCODING_ERRORS_HPP_INCLUDED
#define NOWIDE_ENCODING_ERRORS_HPP_INCLUDED

#include <nowide/config.hpp>
#ifdef NOWIDE_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4275 4251 4231 4660)
#endif
#include <stdexcept>



namespace nowide {
    namespace conv {
        ///
        /// \addtogroup codepage 
        ///
        /// @{

        ///
        /// \brief The excepton that is thrown in case of conversion error
        ///
        class conversion_error : public std::runtime_error {
        public:
            conversion_error() : std::runtime_error("Conversion failed") {}
        };

        ///
        /// enum that defines conversion policy
        ///
        typedef enum {
            skip            = 0,    ///< Skip illegal/unconvertable characters
            stop            = 1,    ///< Stop conversion and throw conversion_error
            default_method  = skip  ///< Default method - skip
        } method_type;


        /// @}

    } // conv

} // nowide

#ifdef NOWIDE_MSVC
#pragma warning(pop)
#endif

#endif

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

