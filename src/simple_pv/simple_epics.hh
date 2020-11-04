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
        class simplePVBase : public casPV
        {
        public:
            simplePVBase( ) : casPV( )
            {
            }
            ~simplePVBase( ) override = default;

            virtual void update( ) = 0;
        };
    } // namespace detail

    class Server;

    /*!
     * @brief A description of a PV, used to describe an int PV to the server.
     * @note this is given a pointer to the data.  This value is only read
     * when a Server object is told to update its data.
     */
    class pvIntAttributes
    {
    public:
        pvIntAttributes( std::string           pv_name,
                         int*                  value,
                         std::pair< int, int > alarm_range,
                         std::pair< int, int > warn_range )
            : name_{ std::move( pv_name ) },

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
        std::string name_;

        int alarm_high_;
        int alarm_low_;
        int warn_high_;
        int warn_low_;

        int* src_;
    };

    /*!
     * @brief A description of a PV, used to describe a string PV to the server.
     * @note this is given a pointer to the data.  This value is only read
     * when a Server object is told to update its data.
     */
    class pvStringAttributes
    {
    public:
        pvStringAttributes( std::string pv_name, const char* value )
            : name_{ std::move( pv_name ) }, src_{ value }
        {
        }

        const std::string&
        name( ) const
        {
            return name_;
        }

        const char*
        src( ) const
        {
            return src_;
        }

    private:
        std::string name_;

        const char* src_;
    };

    /*!
     * @brief A description of a PV, used to describe a double PV to the server.
     * @note this is given a pointer to the data.  This value is only read
     * when a Server object is told to update its data.
     */
    class pvDoubleAttributes
    {
    public:
        pvDoubleAttributes( std::string                 pv_name,
                            double*                     value,
                            std::pair< double, double > alarm_range,
                            std::pair< double, double > warn_range )
            : name_{ std::move( pv_name ) },

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

        double
        alarm_high( ) const
        {
            return alarm_high_;
        }
        double
        alarm_low( ) const
        {
            return alarm_low_;
        }
        double
        warn_high( ) const
        {
            return warn_high_;
        }
        double
        warn_low( ) const
        {
            return warn_low_;
        }

        const double*
        src( ) const
        {
            return src_;
        }

    private:
        std::string name_;

        double alarm_high_;
        double alarm_low_;
        double warn_high_;
        double warn_low_;

        double* src_;
    };

    /*!
     * @brief An R/O implementation of the Portable CA Server.
     */
    class Server : public caServer
    {
    public:
        Server( ) : caServer( ), pvs_{}
        {
        }
        ~Server( ) override;

        /*!
         * @brief Add a PV to the server.
         */
        void addPV( pvIntAttributes attr );
        void addPV( pvStringAttributes attr );
        void addPV( pvDoubleAttributes attr );

        /*!
         * @brief Reflect all changes in the data for each PV into the server
         */
        void update( );

        pvExistReturn pvExistTest( const casCtx&    ctx,
                                   const caNetAddr& clientAddress,
                                   const char*      pPVAliasName ) override;

        pvAttachReturn pvAttach( const casCtx& ctx,
                                 const char*   pPVAliasName ) override;

    private:
        std::mutex                                                       m_;
        std::map< std::string, std::unique_ptr< detail::simplePVBase > > pvs_;
    };

} // namespace simple_epics

#endif // DAQD_TRUNK_SIMPLE_EPICS_HH
