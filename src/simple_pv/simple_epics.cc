//
// Created by jonathan.hanks on 12/20/19.
//
#include "simple_epics.hh"
#include "simple_epics_internal.hh"

#include <stdexcept>
#include <algorithm>

namespace simple_epics
{
    Server::~Server( ) = default;

    void
    Server::addPV( pvIntAttributes attr )
    {
        std::lock_guard< std::mutex > l_( m_ );

        auto it = pvs_.find( attr.name( ) );
        if ( it != pvs_.end( ) )
        {
            throw std::runtime_error(
                "Duplicate key insertion to the epics db" );
        }
        std::string name{ attr.name( ) };
        pvs_.insert( std::make_pair( std::move( name ),
                                     std::make_unique< detail::simpleIntPV >(
                                         *this, std::move( attr ) ) ) );
    }

    void
    Server::addPV( pvStringAttributes attr )
    {
        std::lock_guard< std::mutex > l_( m_ );

        auto it = pvs_.find( attr.name( ) );
        if ( it != pvs_.end( ) )
        {
            throw std::runtime_error(
                "Duplicate key insertion to the epics db" );
        }
        std::string name{ attr.name( ) };
        pvs_.insert( std::make_pair( std::move( name ),
                                     std::make_unique< detail::simpleStringPV >(
                                         *this, std::move( attr ) ) ) );
    }

    void
    Server::update( )
    {
        for ( auto& entry : pvs_ )
        {
            entry.second->update( );
        }
    }

    pvAttachReturn
    Server::pvAttach( const casCtx& ctx, const char* pPVAliasName )
    {
        std::lock_guard< std::mutex > l_( m_ );
        auto                          it = pvs_.find( pPVAliasName );
        if ( it == pvs_.end( ) )
        {
            return S_casApp_pvNotFound;
        }
        return pvCreateReturn( *( it->second ) );
    }

    pvExistReturn
    Server::pvExistTest( const casCtx&    ctx,
                         const caNetAddr& clientAddress,
                         const char*      pPVAliasName )
    {
        std::lock_guard< std::mutex > l_( m_ );
        auto                          it = pvs_.find( pPVAliasName );
        if ( it == pvs_.end( ) )
        {
            return pverDoesNotExistHere;
        }
        return pverExistsHere;
    }

} // namespace simple_epics