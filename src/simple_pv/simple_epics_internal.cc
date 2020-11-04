//
// Created by jonathan.hanks on 1/6/20.
//
#include "simple_epics_internal.hh"
#include <cstring>

namespace simple_epics
{
    namespace detail
    {
        class setup_int_pv_table
        {
        public:
            setup_int_pv_table( )
            {
                simpleIntPV::setup_func_table( );
            }
        };

        setup_int_pv_table pv_int_table_setup_;

        class setup_string_pv_table
        {
        public:
            setup_string_pv_table( )
            {
                simpleStringPV::setup_func_table( );
            }
        };

        setup_string_pv_table pv_string_table_setup_;

        class setup_double_pv_table
        {
        public:
            setup_double_pv_table( )
            {
                simpleDoublePV::setup_func_table( );
            }
        };

        setup_double_pv_table pv_double_table_setup_;

        /*
         * Start of int PV
         */

        gddAppFuncTable< simpleIntPV >&
        simpleIntPV::get_func_table( )
        {
            static gddAppFuncTable< simpleIntPV > func_table;
            return func_table;
        }

        simpleIntPV::~simpleIntPV( ) = default;

        gddAppFuncTableStatus
        simpleIntPV::read_status( gdd& g )
        {
            g.putConvert( val_->getStat( ) );
            return S_casApp_success;
        }

        gddAppFuncTableStatus
        simpleIntPV::read_severity( gdd& g )
        {
            g.putConvert( val_->getSevr( ) );
            return S_casApp_success;
        }

        gddAppFuncTableStatus
        simpleIntPV::read_precision( gdd& g )
        {
            g.putConvert( 0 );
            return S_casApp_success;
        }

        gddAppFuncTableStatus
        simpleIntPV::read_alarm_high( gdd& g )
        {
            g.putConvert( attr_.alarm_high( ) );
            return S_casApp_success;
        }

        gddAppFuncTableStatus
        simpleIntPV::read_alarm_low( gdd& g )
        {
            g.putConvert( attr_.alarm_low( ) );
            return S_casApp_success;
        }

        gddAppFuncTableStatus
        simpleIntPV::read_warn_high( gdd& g )
        {
            g.putConvert( attr_.warn_high( ) );
            return S_casApp_success;
        }

        gddAppFuncTableStatus
        simpleIntPV::read_warn_low( gdd& g )
        {
            g.putConvert( attr_.warn_low( ) );
            return S_casApp_success;
        }

        gddAppFuncTableStatus
        simpleIntPV::read_value( gdd& g )
        {
            auto status =
                gddApplicationTypeTable::app_table.smartCopy( &g, val_.get( ) );
            return ( status ? S_cas_noConvert : S_casApp_success );
        }

        void
        simpleIntPV::setup_func_table( )
        {
            auto install =
                []( const char* name,
                    gddAppFuncTableStatus ( simpleIntPV::*handler )( gdd& ) ) {
                    gddAppFuncTableStatus status;

                    //           char error_string[100];

                    status = get_func_table( ).installReadFunc( name, handler );
                    if ( status != S_gddAppFuncTable_Success )
                    {
                        //                errSymLookup(status, error_string,
                        //                sizeof(error_string));
                        //               throw std::runtime_error(error_string);
                        throw std::runtime_error(
                            "Unable to initialize pv lookup table" );
                    }
                };

            install( "units", &simpleIntPV::read_attr_not_handled );
            install( "status", &simpleIntPV::read_status );
            install( "severity", &simpleIntPV::read_severity );
            install( "maxElements", &simpleIntPV::read_attr_not_handled );
            install( "precision", &simpleIntPV::read_precision );
            install( "alarmHigh", &simpleIntPV::read_alarm_high );
            install( "alarmLow", &simpleIntPV::read_alarm_low );
            install( "alarmHighWarning", &simpleIntPV::read_warn_high );
            install( "alarmLowWarning", &simpleIntPV::read_warn_low );
            install( "maxElements", &simpleIntPV::read_attr_not_handled );
            install( "graphicHigh", &simpleIntPV::read_attr_not_handled );
            install( "graphicLow", &simpleIntPV::read_attr_not_handled );
            install( "controlHigh", &simpleIntPV::read_attr_not_handled );
            install( "controlLow", &simpleIntPV::read_attr_not_handled );
            install( "enums", &simpleIntPV::read_attr_not_handled );
            install( "menuitem", &simpleIntPV::read_attr_not_handled );
            install( "timestamp", &simpleIntPV::read_attr_not_handled );
            install( "value", &simpleIntPV::read_value );
        }

        caStatus
        simpleIntPV::read( const casCtx& ctx, gdd& prototype )
        {
            return get_func_table( ).read( *this, prototype );
        }
        caStatus
        simpleIntPV::write( const casCtx& ctx, const gdd& value )
        {
            return S_casApp_noSupport;
        }

        aitEnum
        simpleIntPV::bestExternalType( ) const
        {
            return val_->primitiveType( );
        }

        caStatus
        simpleIntPV::interestRegister( )
        {
            monitored_ = true;
            return S_casApp_success;
        }

        void
        simpleIntPV::interestDelete( )
        {
            monitored_ = false;
        }

        void
        simpleIntPV::update( )
        {
            set_value( *attr_.src( ) );
        }

        void
        simpleIntPV::set_value( int value )
        {
            int current_value = 0;

            val_->getConvert( current_value );
            if ( current_value == value )
            {
                return;
            }

            val_->putConvert( value );
            aitTimeStamp ts = aitTimeStamp( epicsTime::getCurrent( ) );
            val_->setTimeStamp( &ts );

            aitUint16 stat = epicsAlarmNone;
            aitUint16 sevr = epicsSevNone;
            if ( value >= attr_.alarm_high( ) )
            {
                stat = epicsAlarmHiHi;
                sevr = epicsSevMajor;
            }
            else if ( value <= attr_.alarm_low( ) )
            {
                stat = epicsAlarmLoLo;
                sevr = epicsSevMajor;
            }
            else if ( value >= attr_.warn_high( ) )
            {
                stat = epicsAlarmHigh;
                sevr = epicsSevMinor;
            }
            else if ( value <= attr_.warn_low( ) )
            {
                stat = epicsAlarmLow;
                sevr = epicsSevMinor;
            }
            val_->setSevr( sevr );
            val_->setStat( stat );

            if ( monitored_ )
            {
                casEventMask mask = casEventMask( server_.valueEventMask( ) );
                bool         alarm_changed =
                    ( stat != val_->getStat( ) || sevr != val_->getSevr( ) );
                if ( alarm_changed )
                {
                    mask |= server_.alarmEventMask( );
                }
                postEvent( mask, *val_ );
            }
        }

        /*
         * Start of string PV
         */

        gddAppFuncTable< simpleStringPV >&
        simpleStringPV::get_func_table( )
        {
            static gddAppFuncTable< simpleStringPV > func_table;
            return func_table;
        }

        simpleStringPV::~simpleStringPV( ) = default;

        gddAppFuncTableStatus
        simpleStringPV::read_severity( gdd& g )
        {
            g.putConvert( val_->getSevr( ) );
            return S_casApp_success;
        }

        gddAppFuncTableStatus
        simpleStringPV::read_status( gdd& g )
        {
            g.putConvert( val_->getStat( ) );
            return S_casApp_success;
        }

        gddAppFuncTableStatus
        simpleStringPV::read_precision( gdd& g )
        {
            g.putConvert( 0 );
            return S_casApp_success;
        }

        gddAppFuncTableStatus
        simpleStringPV::read_value( gdd& g )
        {
            auto status =
                gddApplicationTypeTable::app_table.smartCopy( &g, val_.get( ) );
            return ( status ? S_cas_noConvert : S_casApp_success );
        }

        void
        simpleStringPV::setup_func_table( )
        {
            auto install = []( const char* name,
                               gddAppFuncTableStatus (
                                   simpleStringPV::*handler )( gdd& ) ) {
                gddAppFuncTableStatus status;

                //           char error_string[100];

                status = get_func_table( ).installReadFunc( name, handler );
                if ( status != S_gddAppFuncTable_Success )
                {
                    //                errSymLookup(status, error_string,
                    //                sizeof(error_string));
                    //               throw std::runtime_error(error_string);
                    throw std::runtime_error(
                        "Unable to initialize pv lookup table" );
                }
            };

            install( "units", &simpleStringPV::read_attr_not_handled );
            install( "status", &simpleStringPV::read_status );
            install( "severity", &simpleStringPV::read_severity );
            install( "maxElements", &simpleStringPV::read_attr_not_handled );
            install( "precision", &simpleStringPV::read_precision );
            install( "alarmHigh", &simpleStringPV::read_attr_not_handled );
            install( "alarmLow", &simpleStringPV::read_attr_not_handled );
            install( "alarmHighWarning",
                     &simpleStringPV::read_attr_not_handled );
            install( "alarmLowWarning",
                     &simpleStringPV::read_attr_not_handled );
            install( "maxElements", &simpleStringPV::read_attr_not_handled );
            install( "graphicHigh", &simpleStringPV::read_attr_not_handled );
            install( "graphicLow", &simpleStringPV::read_attr_not_handled );
            install( "controlHigh", &simpleStringPV::read_attr_not_handled );
            install( "controlLow", &simpleStringPV::read_attr_not_handled );
            install( "enums", &simpleStringPV::read_attr_not_handled );
            install( "menuitem", &simpleStringPV::read_attr_not_handled );
            install( "timestamp", &simpleStringPV::read_attr_not_handled );
            install( "value", &simpleStringPV::read_value );
        }

        caStatus
        simpleStringPV::read( const casCtx& ctx, gdd& prototype )
        {
            return get_func_table( ).read( *this, prototype );
        }
        caStatus
        simpleStringPV::write( const casCtx& ctx, const gdd& value )
        {
            return S_casApp_noSupport;
        }

        aitEnum
        simpleStringPV::bestExternalType( ) const
        {
            return val_->primitiveType( );
        }

        caStatus
        simpleStringPV::interestRegister( )
        {
            monitored_ = true;
            return S_casApp_success;
        }

        void
        simpleStringPV::interestDelete( )
        {
            monitored_ = false;
        }

        void
        simpleStringPV::update( )
        {
            set_value( attr_.src( ) );
        }

        void
        simpleStringPV::set_value( const char* value )
        {
            aitString current_value;

            val_->getConvert( current_value );
            if ( std::strcmp( current_value, value ) == 0 )
            {
                return;
            }

            val_->putConvert( value );
            aitTimeStamp ts = aitTimeStamp( epicsTime::getCurrent( ) );
            val_->setTimeStamp( &ts );
            val_->setSevr( epicsSevNone );
            val_->setStat( epicsAlarmNone );

            if ( monitored_ )
            {
                casEventMask mask = casEventMask( server_.valueEventMask( ) );
                postEvent( mask, *val_ );
            }
        }

        /*
         * Start of double PV
         */

        gddAppFuncTable< simpleDoublePV >&
        simpleDoublePV::get_func_table( )
        {
            static gddAppFuncTable< simpleDoublePV > func_table;
            return func_table;
        }

        simpleDoublePV::~simpleDoublePV( ) = default;

        gddAppFuncTableStatus
        simpleDoublePV::read_status( gdd& g )
        {
            g.putConvert( val_->getStat( ) );
            return S_casApp_success;
        }

        gddAppFuncTableStatus
        simpleDoublePV::read_severity( gdd& g )
        {
            g.putConvert( val_->getSevr( ) );
            return S_casApp_success;
        }

        gddAppFuncTableStatus
        simpleDoublePV::read_precision( gdd& g )
        {
            g.putConvert( 0 );
            return S_casApp_success;
        }

        gddAppFuncTableStatus
        simpleDoublePV::read_alarm_high( gdd& g )
        {
            g.putConvert( attr_.alarm_high( ) );
            return S_casApp_success;
        }

        gddAppFuncTableStatus
        simpleDoublePV::read_alarm_low( gdd& g )
        {
            g.putConvert( attr_.alarm_low( ) );
            return S_casApp_success;
        }

        gddAppFuncTableStatus
        simpleDoublePV::read_warn_high( gdd& g )
        {
            g.putConvert( attr_.warn_high( ) );
            return S_casApp_success;
        }

        gddAppFuncTableStatus
        simpleDoublePV::read_warn_low( gdd& g )
        {
            g.putConvert( attr_.warn_low( ) );
            return S_casApp_success;
        }

        gddAppFuncTableStatus
        simpleDoublePV::read_value( gdd& g )
        {
            auto status =
                gddApplicationTypeTable::app_table.smartCopy( &g, val_.get( ) );
            return ( status ? S_cas_noConvert : S_casApp_success );
        }

        void
        simpleDoublePV::setup_func_table( )
        {
            auto install = []( const char* name,
                               gddAppFuncTableStatus (
                                   simpleDoublePV::*handler )( gdd& ) ) {
                gddAppFuncTableStatus status;

                //           char error_string[100];

                status = get_func_table( ).installReadFunc( name, handler );
                if ( status != S_gddAppFuncTable_Success )
                {
                    //                errSymLookup(status, error_string,
                    //                sizeof(error_string));
                    //               throw std::runtime_error(error_string);
                    throw std::runtime_error(
                        "Unable to initialize pv lookup table" );
                }
            };

            install( "units", &simpleDoublePV::read_attr_not_handled );
            install( "status", &simpleDoublePV::read_status );
            install( "severity", &simpleDoublePV::read_severity );
            install( "maxElements", &simpleDoublePV::read_attr_not_handled );
            install( "precision", &simpleDoublePV::read_precision );
            install( "alarmHigh", &simpleDoublePV::read_alarm_high );
            install( "alarmLow", &simpleDoublePV::read_alarm_low );
            install( "alarmHighWarning", &simpleDoublePV::read_warn_high );
            install( "alarmLowWarning", &simpleDoublePV::read_warn_low );
            install( "maxElements", &simpleDoublePV::read_attr_not_handled );
            install( "graphicHigh", &simpleDoublePV::read_attr_not_handled );
            install( "graphicLow", &simpleDoublePV::read_attr_not_handled );
            install( "controlHigh", &simpleDoublePV::read_attr_not_handled );
            install( "controlLow", &simpleDoublePV::read_attr_not_handled );
            install( "enums", &simpleDoublePV::read_attr_not_handled );
            install( "menuitem", &simpleDoublePV::read_attr_not_handled );
            install( "timestamp", &simpleDoublePV::read_attr_not_handled );
            install( "value", &simpleDoublePV::read_value );
        }

        caStatus
        simpleDoublePV::read( const casCtx& ctx, gdd& prototype )
        {
            return get_func_table( ).read( *this, prototype );
        }
        caStatus
        simpleDoublePV::write( const casCtx& ctx, const gdd& value )
        {
            return S_casApp_noSupport;
        }

        aitEnum
        simpleDoublePV::bestExternalType( ) const
        {
            return val_->primitiveType( );
        }

        caStatus
        simpleDoublePV::interestRegister( )
        {
            monitored_ = true;
            return S_casApp_success;
        }

        void
        simpleDoublePV::interestDelete( )
        {
            monitored_ = false;
        }

        void
        simpleDoublePV::update( )
        {
            set_value( *attr_.src( ) );
        }

        void
        simpleDoublePV::set_value( double value )
        {
            double current_value = 0;

            val_->getConvert( current_value );
            if ( current_value == value )
            {
                return;
            }

            val_->putConvert( value );
            aitTimeStamp ts = aitTimeStamp( epicsTime::getCurrent( ) );
            val_->setTimeStamp( &ts );

            aitUint16 stat = epicsAlarmNone;
            aitUint16 sevr = epicsSevNone;
            if ( value >= attr_.alarm_high( ) )
            {
                stat = epicsAlarmHiHi;
                sevr = epicsSevMajor;
            }
            else if ( value <= attr_.alarm_low( ) )
            {
                stat = epicsAlarmLoLo;
                sevr = epicsSevMajor;
            }
            else if ( value >= attr_.warn_high( ) )
            {
                stat = epicsAlarmHigh;
                sevr = epicsSevMinor;
            }
            else if ( value <= attr_.warn_low( ) )
            {
                stat = epicsAlarmLow;
                sevr = epicsSevMinor;
            }
            val_->setSevr( sevr );
            val_->setStat( stat );

            if ( monitored_ )
            {
                casEventMask mask = casEventMask( server_.valueEventMask( ) );
                bool         alarm_changed =
                    ( stat != val_->getStat( ) || sevr != val_->getSevr( ) );
                if ( alarm_changed )
                {
                    mask |= server_.alarmEventMask( );
                }
                postEvent( mask, *val_ );
            }
        }

    } // namespace detail

} // namespace simple_epics