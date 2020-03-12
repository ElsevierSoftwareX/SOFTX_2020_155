//
// Created by jonathan.hanks on 3/2/20.
//

#ifndef DAQD_TRUNK_MAKE_UNIQUE_HH
#define DAQD_TRUNK_MAKE_UNIQUE_HH

#include <memory>

// std::make_unique didn't make it into C++11, so
// to allow this to work in a pre C++14 world, we
// provide a simple replacement.
//
// A make_unique<> for C++11.  Taken from
// "Effective Modern C++ by Scott Meyers (O'Reilly).
// Copyright 2015 Scott Meyers, 978-1-491-90399-5"
//
// Permission given in the book to reuse code segments.
//
// @tparam T The type of the object to be managed by the unique_ptr
// @tparam Ts The type of the arguments to T's constructor
// @param params The arguments to forward to the constructor
// @return a std::unique_ptr<T>
template < typename T, typename... Ts >
std::unique_ptr< T >
make_unique_ptr( Ts&&... params )
{
    return std::unique_ptr< T >( new T( std::forward< Ts >( params )... ) );
}

#endif // DAQD_TRUNK_MAKE_UNIQUE_HH
