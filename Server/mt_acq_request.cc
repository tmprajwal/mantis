/*
 * mt_acq_request.cc
 *
 *  Created on: Mar 20, 2014
 *      Author: nsoblath
 */

#define MANTIS_API_EXPORTS

#include "mt_acq_request.hh"
#include "mt_version.hh"

#include<string>
using std::string;

#include "logger.hh"

namespace mantis
{
    LOGGER(mtlog, "acq_request");

    std::string acq_request::interpret_status( status a_status )
    {
        switch( a_status )
        {
            case created:
                return string( "created" );
                break;
            case acknowledged:
                return string( "acknowledged" );
                break;
            case waiting:
                return string( "waiting (in queue)" );
                break;
            case started:
                return string( "started" );
                break;
            case running:
                return string( "running" );
                break;
            case stopped:
                return string( "stopped (started and then stopped normally)" );
                break;
            case error:
                return string( "error" );
                break;
            case canceled:
                return string( "canceled (started but stopped abnormally)" );
                break;
            case revoked:
                return string( "revoked (will not be performed)" );
                break;
            default:
                return string( "unknown" );
        }
    }

    acq_request::acq_request( uuid_t a_id ) :
            param_node(),
            f_id( a_id )
    {
        // default description
        add( "id", param_value( boost::uuids::to_string( f_id ) ) );
        add( "status", param_value( 0 ) );
        add( "client", new param_node() );
        add( "mantis", new param_node() );
        add( "monarch", new param_node() );
        add( "acquisition", new param_node() );
        add( "response", new param_node() );
        add( "file", new param_value( "scrambled.egg" ) );
        add( "description", new param_value( "" ) );
    }

    acq_request::acq_request( const acq_request& orig ) :
        param_node( orig ),
        f_id( orig.f_id )
    {
    }

    acq_request::~acq_request()
    {
    }

    acq_request& acq_request::operator=( const acq_request& rhs )
    {
        f_id = rhs.f_id;
        this->param_node::operator=( rhs );
        return *this;
    }

    void acq_request::set_id( uuid_t a_id )
    {
        f_id = a_id;
        this->replace( "id", param_value( boost::uuids::to_string( f_id ) ) );
        return;
    }

    uuid_t acq_request::get_id() const
    {
        return f_id;
    }

    std::string acq_request::get_id_string() const
    {
        return this->get_value( "id" );
    }

    void acq_request::set_status( status a_status )
    {
        this->replace( "status", param_value( (unsigned)a_status ) );
        return;
    }

    acq_request::status acq_request::get_status() const
    {
        return (status)(this->get_value< unsigned >( "status" ));
    }

    void acq_request::set_client_exe( const std::string& a_exe )
    {
        this->node_at( "client" )->replace( "exe", param_value( a_exe ) );
        return;
    }

    void acq_request::set_client_version( const std::string& a_ver )
    {
        this->node_at( "client" )->replace( "version", param_value( a_ver ) );
        return;
    }

    void acq_request::set_client_commit( const std::string& a_commit )
    {
        this->node_at( "client" )->replace( "commit", param_value( a_commit ) );
        return;
    }

    void acq_request::set_client_package( const std::string& a_pkg )
    {
        this->node_at( "client" )->replace( "package", param_value( a_pkg ) );
        return;
    }

    void acq_request::set_mantis_server_exe( const std::string& a_exe )
    {
        this->node_at( "mantis" )->replace( "exe", param_value( a_exe ) );
        return;
    }

    void acq_request::set_mantis_server_version( const std::string& a_ver )
    {
        this->node_at( "mantis" )->replace( "version", param_value( a_ver ) );
        return;
    }

    void acq_request::set_mantis_server_commit( const std::string& a_commit )
    {
        this->node_at( "mantis" )->replace( "commit", param_value( a_commit ) );
        return;
    }

    void acq_request::set_monarch_version( const std::string& a_ver )
    {
        this->node_at( "monarch" )->replace( "version", param_value( a_ver ) );
        return;
    }

    void acq_request::set_monarch_commit( const std::string& a_ver )
    {
        this->node_at( "monarch" )->replace( "commit", param_value( a_ver ) );
        return;
    }

    void acq_request::set_file_config( const param_value& a_config )
    {
        this->replace( "file", a_config );
        return;
    }

    void acq_request::set_description_config( const param_value& a_config )
    {
        this->replace( "description", a_config );
        return;
    }

    void acq_request::set_acquisition_config( const param_node& a_config )
    {
        this->replace( "acquisition", a_config );
        return;
    }

    void acq_request::set_response( const param_node& a_response )
    {
        this->replace( "response", a_response );
        return;
    }

} /* namespace mantis */
