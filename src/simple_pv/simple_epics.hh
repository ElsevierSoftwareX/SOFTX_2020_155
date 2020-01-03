//
// Created by jonathan.hanks on 12/20/19.
//

#ifndef DAQD_TRUNK_SIMPLE_EPICS_HH
#define DAQD_TRUNK_SIMPLE_EPICS_HH

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <gddAppFuncTable.h>
#include <epicsTimer.h>

#include "casdef.h"
#include "gddApps.h"
#include "smartGDDPointer.h"

#include "simple_pv.h"

namespace simple_epics
{
    namespace detail
    {
        class setup_pv_table;
    }

    class Server;

    /*typedef struct SimplePV
{
    const char* name;
    int         pv_type; /// SIMPLE_PV_INT or SIMPLE_PV_STRING
    void*       data;

    // These values are only used for an int pv
    int alarm_high;
    int alarm_low;
    int warn_high;
    int warn_low;
} SimplePV;*/

    class pvAttributes
    {
    public:
        pvAttributes( std::string           pv_name,
                      int*                  value,
                      std::pair< int, int > alarm_range,
                      std::pair< int, int > warn_range )
            : pv_type_( SIMPLE_PV_INT ), name_{ std::move( pv_name ) },

              alarm_low_{ alarm_range.first },
              alarm_high_{ alarm_range.second }, warn_low_{ warn_range.first },
              warn_high_{ warn_range.second }, src_{ value }
        {
        }

        const std::string&
        name( ) const
        {
            return name_;
        }

        int
        alarm_high( ) const
        {
            return alarm_high_;
        }
        int
        alarm_low( ) const
        {
            return alarm_low_;
        }
        int
        warn_high( ) const
        {
            return warn_high_;
        }
        int
        warn_low( ) const
        {
            return warn_low_;
        }

        const int*
        src( ) const
        {
            return src_;
        }

    private:
        int         pv_type_;
        std::string name_;

        int alarm_high_;
        int alarm_low_;
        int warn_high_;
        int warn_low_;

        int* src_;
    };

    class simplePV : public casPV
    {
        friend class detail::setup_pv_table;

    public:
        simplePV( Server& server, pvAttributes attr )
            : casPV( ), server_{ server }, attr_{ std::move( attr ) },
              val_( ), monitored_{ false }
        {
            val_ = new gddScalar( gddAppType_value, aitEnumInt32 );
            val_->unreference( );
            set_value( *attr_.src( ) );
        }
        ~simplePV( ) override;

        caStatus read( const casCtx& ctx, gdd& prototype ) override;
        caStatus write( const casCtx& ctx, const gdd& value ) override;

        void destroy( ) override{};

        aitEnum bestExternalType( ) const override;

        const char*
        getName( ) const override
        {
            return attr_.name( ).c_str( );
        }

        caStatus interestRegister( ) override;

        void interestDelete( ) override;

        void update( );

    private:
        void set_value( int value );

        static void setup_func_table( );

        gddAppFuncTableStatus
        read_attr_not_handled( gdd& g )
        {
            return S_casApp_success;
        }

        gddAppFuncTableStatus read_status( gdd& g );

        gddAppFuncTableStatus read_severity( gdd& g );

        gddAppFuncTableStatus read_precision( gdd& g );

        gddAppFuncTableStatus read_alarm_high( gdd& g );

        gddAppFuncTableStatus read_alarm_low( gdd& g );

        gddAppFuncTableStatus read_warn_high( gdd& g );

        gddAppFuncTableStatus read_warn_low( gdd& g );

        gddAppFuncTableStatus read_value( gdd& g );

        Server&         server_;
        pvAttributes    attr_;
        smartGDDPointer val_;
        bool            monitored_;

        static gddAppFuncTable< simplePV > func_table;
    };

    class Server : public caServer
    {
    public:
        Server( ) : caServer( ), attrs_{}
        {
        }
        ~Server( ) override;

        void addPV( pvAttributes attr );
        void update( );

        pvExistReturn pvExistTest( const casCtx&    ctx,
                                   const caNetAddr& clientAddress,
                                   const char*      pPVAliasName ) override;

        pvAttachReturn pvAttach( const casCtx& ctx,
                                 const char*   pPVAliasName ) override;

    private:
        std::mutex                                           m_;
        std::map< std::string, std::unique_ptr< simplePV > > attrs_;
    };

} // namespace simple_epics

#endif // DAQD_TRUNK_SIMPLE_EPICS_HH
