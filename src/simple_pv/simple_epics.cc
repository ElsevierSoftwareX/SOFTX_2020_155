//
// Created by jonathan.hanks on 12/20/19.
//
#include "simple_epics.hh"
#include <stdexcept>
#include <algorithm>

namespace simple_epics
{
    gddAppFuncTable< simplePV > simplePV::func_table;

    namespace detail
    {
        class setup_pv_table
        {
        public:
            setup_pv_table( )
            {
                simplePV::setup_func_table( );
            }
        };

        setup_pv_table pv_table_setup_;
    } // namespace detail

    simplePV::~simplePV( )
    {
    }

    gddAppFuncTableStatus
    simplePV::read_status( gdd& g )
    {
        g.putConvert( val_->getStat( ) );
        return S_casApp_success;
    }

    gddAppFuncTableStatus
    simplePV::read_severity( gdd& g )
    {
        g.putConvert( val_->getSevr( ) );
        return S_casApp_success;
    }

    gddAppFuncTableStatus
    simplePV::read_precision( gdd& g )
    {
        g.putConvert( 0 );
        return S_casApp_success;
    }

    gddAppFuncTableStatus
    simplePV::read_alarm_high( gdd& g )
    {
        g.putConvert( attr_.alarm_high( ) );
        return S_casApp_success;
    }

    gddAppFuncTableStatus
    simplePV::read_alarm_low( gdd& g )
    {
        g.putConvert( attr_.alarm_low( ) );
        return S_casApp_success;
    }

    gddAppFuncTableStatus
    simplePV::read_warn_high( gdd& g )
    {
        g.putConvert( attr_.warn_high( ) );
        return S_casApp_success;
    }

    gddAppFuncTableStatus
    simplePV::read_warn_low( gdd& g )
    {
        g.putConvert( attr_.warn_low( ) );
        return S_casApp_success;
    }

    gddAppFuncTableStatus
    simplePV::read_value( gdd& g )
    {
        auto status =
            gddApplicationTypeTable::app_table.smartCopy( &g, val_.get( ) );
        return ( status ? S_cas_noConvert : S_casApp_success );
    }

    void
    simplePV::setup_func_table( )
    {
        auto install =
            []( const char* name,
                gddAppFuncTableStatus ( simplePV::*handler )( gdd& ) ) {
                gddAppFuncTableStatus status;

                //           char error_string[100];

                status = func_table.installReadFunc( name, handler );
                if ( status != S_gddAppFuncTable_Success )
                {
                    //                errSymLookup(status, error_string,
                    //                sizeof(error_string));
                    //               throw std::runtime_error(error_string);
                    throw std::runtime_error(
                        "Unable to initialize pv lookup table" );
                }
            };

        install( "units", &simplePV::read_attr_not_handled );
        install( "status", &simplePV::read_status );
        install( "severity", &simplePV::read_severity );
        install( "maxElements", &simplePV::read_attr_not_handled );
        install( "precision", &simplePV::read_precision );
        install( "alarmHigh", &simplePV::read_alarm_high );
        install( "alarmLow", &simplePV::read_alarm_low );
        install( "alarmHighWarning", &simplePV::read_warn_high );
        install( "alarmLowWarning", &simplePV::read_warn_low );
        install( "maxElements", &simplePV::read_attr_not_handled );
        install( "graphicHigh", &simplePV::read_attr_not_handled );
        install( "graphicLow", &simplePV::read_attr_not_handled );
        install( "controlHigh", &simplePV::read_attr_not_handled );
        install( "controlLow", &simplePV::read_attr_not_handled );
        install( "enums", &simplePV::read_attr_not_handled );
        install( "menuitem", &simplePV::read_attr_not_handled );
        install( "timestamp", &simplePV::read_attr_not_handled );
        install( "value", &simplePV::read_value );
    }

    caStatus
    simplePV::read( const casCtx& ctx, gdd& prototype )
    {
        return func_table.read( *this, prototype );
    }
    caStatus
    simplePV::write( const casCtx& ctx, const gdd& value )
    {
        return S_casApp_noSupport;
    }

    aitEnum
    simplePV::bestExternalType( ) const
    {
        return val_->primitiveType( );
    }

    caStatus
    simplePV::interestRegister( )
    {
        monitored_ = true;
        return S_casApp_success;
    }

    void
    simplePV::interestDelete( )
    {
        monitored_ = false;
    }

    void
    simplePV::update( )
    {
        set_value( *attr_.src( ) );
    }

    void
    simplePV::set_value( int value )
    {
        val_->putConvert( value );
        aitTimeStamp ts = aitTimeStamp( epicsTime::getCurrent( ) );
        val_->setTimeStamp( &ts );
        if ( value >= attr_.alarm_high( ) )
        {
            val_->setStat( epicsAlarmHiHi );
            val_->setSevr( epicsSevMajor );
        }
        else if ( value <= attr_.alarm_low( ) )
        {
            val_->setStat( epicsAlarmLoLo );
            val_->setSevr( epicsSevMajor );
        }
        else if ( value >= attr_.warn_high( ) )
        {
            val_->setStat( epicsAlarmHigh );
            val_->setSevr( epicsSevMinor );
        }
        else if ( value <= attr_.warn_low( ) )
        {
            val_->setStat( epicsAlarmLow );
            val_->setSevr( epicsSevMinor );
        }
        else
        {
            val_->setStat( epicsAlarmNone );
            val_->setSevr( epicsSevNone );
        }
        if ( monitored_ )
        {
            postEvent( casEventMask( server_.valueEventMask( ) ) |
                           server_.alarmEventMask( ),
                       *val_ );
        }
    }

    Server::~Server( ) = default;

    void
    Server::addPV( pvAttributes attr )
    {
        std::lock_guard< std::mutex > l_( m_ );

        auto it = attrs_.find( attr.name( ) );
        if ( it != attrs_.end( ) )
        {
            throw std::runtime_error(
                "Duplicate key insertion to the epics db" );
        }
        std::string name{ attr.name( ) };
        attrs_.insert( std::make_pair(
            std::move( name ),
            std::make_unique< simplePV >( *this, std::move( attr ) ) ) );
    }

    void
    Server::update( )
    {
        for ( auto& entry : attrs_ )
        {
            entry.second->update( );
        }
    }

    pvAttachReturn
    Server::pvAttach( const casCtx& ctx, const char* pPVAliasName )
    {
        std::lock_guard< std::mutex > l_( m_ );
        auto                          it = attrs_.find( pPVAliasName );
        if ( it == attrs_.end( ) )
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
        auto                          it = attrs_.find( pPVAliasName );
        if ( it == attrs_.end( ) )
        {
            return pverDoesNotExistHere;
        }
        return pverExistsHere;
    }

} // namespace simple_epics