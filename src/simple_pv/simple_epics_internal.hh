//
// Created by jonathan.hanks on 1/6/20.
//

#ifndef DAQD_TRUNK_SIMPLE_PV_INTERNAL_HH
#define DAQD_TRUNK_SIMPLE_PV_INTERNAL_HH

#include "simple_epics.hh"

namespace simple_epics
{

    namespace detail
    {
        class setup_int_pv_table;

        /*!
         * @brief A representation of a R/O integer in a PV
         */
        class simpleIntPV : public simplePVBase
        {
            friend class setup_int_pv_table;

        public:
            simpleIntPV( caServer& server, pvIntAttributes attr )
                : simplePVBase( ), server_{ server }, attr_{ std::move(
                                                          attr ) },
                  val_( ), monitored_{ false }
            {
                val_ = new gddScalar( gddAppType_value, aitEnumInt32 );
                val_->unreference( );
                set_value( *attr_.src( ) );
            }
            ~simpleIntPV( ) override;

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

            void update( ) override;

        private:
            void set_value( int value );

            static void                            setup_func_table( );
            static gddAppFuncTable< simpleIntPV >& get_func_table( );

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

            caServer&       server_;
            pvIntAttributes attr_;
            smartGDDPointer val_;
            bool            monitored_;
        };

        class setup_string_pv_table;
        /*!
         * @brief A representation of a R/O integer in a PV
         */
        class simpleStringPV : public simplePVBase
        {
            friend class setup_string_pv_table;

        public:
            simpleStringPV( caServer& server, pvStringAttributes attr )
                : simplePVBase( ), server_{ server }, attr_{ std::move(
                                                          attr ) },
                  val_( ), monitored_{ false }
            {
                val_ = new gddScalar( gddAppType_value, aitEnumString );
                val_->unreference( );
                set_value( attr_.src( ) );
            }
            ~simpleStringPV( ) override;

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

            void update( ) override;

        private:
            void set_value( const char* value );

            static void                               setup_func_table( );
            static gddAppFuncTable< simpleStringPV >& get_func_table( );

            gddAppFuncTableStatus
            read_attr_not_handled( gdd& g )
            {
                return S_casApp_success;
            }

            gddAppFuncTableStatus read_severity( gdd& g );

            gddAppFuncTableStatus read_status( gdd& g );

            gddAppFuncTableStatus read_precision( gdd& g );

            gddAppFuncTableStatus read_value( gdd& g );

            caServer&          server_;
            pvStringAttributes attr_;
            smartGDDPointer    val_;
            bool               monitored_;
        };

    } // namespace detail

} // namespace simple_epics

#endif // DAQD_TRUNK_SIMPLE_PV_INTERNAL_HH
