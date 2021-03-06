#define MANTIS_API_EXPORTS
#define M3_API_EXPORTS

#include "mt_acq_request_db.hh"

#include "mt_acq_request.hh"
#include "mt_config_manager.hh"
#include "mt_exception.hh"
#include "mt_message.hh"
#include "mt_request_receiver.hh"
#include "mt_version.hh"

#include "M3Version.hh"

#include "logger.hh"
#include "param.hh"

using std::string;

using scarab::param;
using scarab::param_array;
using scarab::param_node;
using scarab::param_value;

namespace mantis
{
    LOGGER( mtlog, "acq_request_db" );

    acq_request_db::acq_request_db( config_manager* a_conf_mgr, const std::string& a_exe_name ) :
            f_db_mutex(),
            f_acq_request_db(),
            f_queue_mutex(),
            f_acq_request_queue(),
            f_request_in_queue_condition(),
            f_queue_is_active( true ),
            f_queue_active_condition(),
            f_config_mgr( a_conf_mgr ),
            f_exe_name( a_exe_name )
    {
    }

    acq_request_db::~acq_request_db()
    {
        clear();
        return;
    }


    //***************
    // DB Commands
    //***************

    bool acq_request_db::empty()
    {
        bool t_empty;
        f_db_mutex.lock();
        t_empty = f_acq_request_db.empty();
        f_db_mutex.unlock();
        return t_empty;
    }

    acq_request* acq_request_db::get_acq_request( uuid_t a_id )
    {
        acq_request* t_desc = NULL;
        f_db_mutex.lock();
        acq_request_db_data::iterator t_acq_request_it;
        t_acq_request_it = f_acq_request_db.find( a_id );
        if( t_acq_request_it != f_acq_request_db.end()) t_desc = t_acq_request_it->second;
        f_db_mutex.unlock();
        return t_desc;
    }

    const acq_request* acq_request_db::get_acq_request( uuid_t a_id ) const
    {
        const acq_request* t_desc = NULL;
        f_db_mutex.lock();
        acq_request_db_data::const_iterator t_acq_request_it;
        t_acq_request_it = f_acq_request_db.find( a_id );
        if( t_acq_request_it != f_acq_request_db.end()) t_desc = t_acq_request_it->second;
        f_db_mutex.unlock();
        return t_desc;
    }

    uuid_t acq_request_db::add( acq_request* a_acq_request )
    {
        uuid_t t_id = generate_nil_uuid();
        f_db_mutex.lock();
        f_acq_request_db[ a_acq_request->get_id() ] = a_acq_request;
        t_id = a_acq_request->get_id();
        f_db_mutex.unlock();
        return t_id;
    }

    acq_request* acq_request_db::remove( uuid_t a_id )
    {
        acq_request* t_request = NULL;
        f_db_mutex.lock();
        acq_request_db_data::iterator t_acq_request_it;
        t_acq_request_it = f_acq_request_db.find( a_id );
        if( t_acq_request_it != f_acq_request_db.end())
        {
            t_request = t_acq_request_it->second;
            f_acq_request_db.erase( t_acq_request_it );
            f_queue_mutex.lock();
            f_acq_request_queue.remove( t_request );
            f_queue_mutex.unlock();
        }
        f_db_mutex.unlock();
        return t_request;
    }


    void acq_request_db::flush()
    {
        f_db_mutex.lock();
        acq_request_db_data::iterator t_it_counter = f_acq_request_db.begin();
        acq_request_db_data::iterator t_it_deletable;
        while( t_it_counter != f_acq_request_db.end() )
        {
            t_it_deletable = t_it_counter++;
            if( t_it_deletable->second->get_status() > acq_request::running )
            {
                delete t_it_deletable->second;
            }
        }
        f_db_mutex.unlock();
        return;
    }

    void acq_request_db::clear()
    {
        f_db_mutex.lock();
        for( acq_request_db_data::iterator t_acq_request_it = f_acq_request_db.begin(); t_acq_request_it != f_acq_request_db.end(); ++t_acq_request_it )
        {
            delete t_acq_request_it->second;
        }
        f_acq_request_db.clear();
        f_queue_mutex.lock();
        f_acq_request_queue.clear();
        f_queue_mutex.unlock();
        f_db_mutex.unlock();
        return;
    }



    //******************
    // Queue Commands
    //******************

    bool acq_request_db::queue_empty()
    {
        bool t_empty;
        f_queue_mutex.lock();
        t_empty = f_acq_request_queue.empty();
        f_queue_mutex.unlock();
        return t_empty;
    }

    size_t acq_request_db::queue_size()
    {
        size_t t_size;
        f_queue_mutex.lock();
        t_size = f_acq_request_queue.size();
        f_queue_mutex.unlock();
        return t_size;
    }

    uuid_t acq_request_db::enqueue( acq_request* a_acq_request )
    {
        uuid_t t_id = generate_nil_uuid();
        f_db_mutex.lock();
        if( f_acq_request_db.find( a_acq_request->get_id() ) == f_acq_request_db.end() )
        {
            f_acq_request_db[ a_acq_request->get_id() ] = a_acq_request;
        }
        if( a_acq_request->get_status() < acq_request::waiting )
        {
            f_queue_mutex.lock();
            f_acq_request_queue.push_back( a_acq_request );
            a_acq_request->set_status( acq_request::waiting );
            f_queue_mutex.unlock();
            t_id = a_acq_request->get_id();

            // if the queue condition is waiting, release it
            if( f_request_in_queue_condition.is_waiting() )
            {
                //INFO( mtlog, "releasing queue condition" );
                f_request_in_queue_condition.release();
            }
        }
        f_db_mutex.unlock();
        return t_id;
    }

    bool acq_request_db::cancel( uuid_t a_id )
    {
        bool t_result = false;
        f_db_mutex.lock();
        f_queue_mutex.lock();
        acq_request_db_data::iterator t_req_it = f_acq_request_db.find( a_id );
        if( t_req_it != f_acq_request_db.end() )
        {
            if( t_req_it->second->get_status() < acq_request::running )
            {
                if( t_req_it->second->get_status() == acq_request::waiting )
                {
                    f_acq_request_queue.remove( t_req_it->second );
                }
                t_req_it->second->set_status( acq_request::canceled );
            }
            t_result = true;
        }
        f_queue_mutex.unlock();
        f_db_mutex.unlock();
        return t_result;
    }

    acq_request* acq_request_db::pop()
    {
        acq_request* t_acq_request = NULL;
        f_db_mutex.lock();
        f_queue_mutex.lock();
        if( ! f_acq_request_queue.empty() )
        {
            t_acq_request = f_acq_request_queue.front();
            f_acq_request_queue.pop_front();
            t_acq_request->set_status( acq_request::revoked );
        }
        f_queue_mutex.unlock();
        f_db_mutex.unlock();
        return t_acq_request;
    }

    void acq_request_db::clear_queue()
    {
        f_db_mutex.lock();
        f_queue_mutex.lock();
        if( ! f_acq_request_queue.empty() )
        {
            acq_request_queue::iterator t_list_it = f_acq_request_queue.begin();
            while( t_list_it != f_acq_request_queue.end() )
            {
                if( (*t_list_it)->get_status() == acq_request::waiting )
                {
                    (*t_list_it)->set_status( acq_request::canceled );
                }
                f_acq_request_queue.erase( t_list_it );
                t_list_it = f_acq_request_queue.begin();
            }
        }
        f_queue_mutex.unlock();
        f_db_mutex.unlock();
        return;
    }

    void acq_request_db::wait_for_request_in_queue()
    {
        if( queue_empty() )
        {
            // thread cancellation point via call to pthread_cond_wait in queue_condition::wait
            f_request_in_queue_condition.wait();
        }
        return;
    }

    bool acq_request_db::queue_is_active() const
    {
        return f_queue_is_active.load();
    }

    void acq_request_db::wait_for_queue_active()
    {
        if( f_queue_is_active.load() ) return;
        f_queue_active_condition.wait();
        return;
    }

    void acq_request_db::start_queue()
    {
        if( f_queue_is_active.load() ) return;
        f_queue_is_active.store( true );
        f_queue_active_condition.release();
        INFO( mtlog, "Queue processing started" );
        return;
    }

    void acq_request_db::stop_queue()
    {
        f_queue_is_active.store( false );
        INFO( mtlog, "Queue processing stopped" );
        return;
    }



    //********************
    // Request handlers
    //********************

    bool acq_request_db::handle_new_acq_request( const msg_request* a_request, request_reply_package& a_pkg )
    {
        // required
        const param_value* t_file_node = a_request->get_payload().value_at( "file" );
        if( t_file_node == NULL )
        {
            WARN( mtlog, "No or invalid file configuration present; aborting run request" );
            a_pkg.send_reply( R_MESSAGE_ERROR_BAD_PAYLOAD, "No file configuration present; aborting request" );
            return false;
        }

        acq_request* t_acq_req = new acq_request( generate_random_uuid() );
        t_acq_req->set_status( acq_request::created );

        t_acq_req->set_file_config( *t_file_node );
        if( a_request->get_payload().has( "description" ) ) t_acq_req->set_description_config( *(a_request->get_payload().value_at( "description" ) ) );

        t_acq_req->set_mantis_server_commit( TOSTRING(Mantis_GIT_COMMIT) );
        t_acq_req->set_mantis_server_exe( f_exe_name );
        t_acq_req->set_mantis_server_version( TOSTRING(Mantis_VERSION) );
        t_acq_req->set_monarch_commit( TOSTRING(Monarch_GIT_COMMIT) );
        t_acq_req->set_monarch_version( TOSTRING(Monarch_VERSION ) );

        param_node* t_acq_config = f_config_mgr->copy_master_server_config( "acq" );
        t_acq_req->set_acquisition_config( *t_acq_config );
        delete t_acq_config;
        // remove non-enabled devices from the devices node
        param_node* t_dev_node = t_acq_req->node_at( "acquisition" )->node_at( "devices" );
        std::vector< std::string > t_devs_to_remove;
        for( param_node::iterator t_node_it = t_dev_node->begin(); t_node_it != t_dev_node->end(); ++t_node_it )
        {
            try
            {
                if( ! t_node_it->second->as_node().get_value< bool >( "enabled", false ) )
                {
                    t_devs_to_remove.push_back( t_node_it->first );
                }
            }
            catch( exception& )
            {
                WARN( mtlog, "Found non-node param object in \"devices\"" );
            }
        }
        for( std::vector< std::string >::const_iterator it = t_devs_to_remove.begin(); it != t_devs_to_remove.end(); ++it )
        {
            t_dev_node->remove( *it );
        }

        t_acq_req->set_client_commit( a_request->get_sender_commit() );
        t_acq_req->set_client_exe( a_request->get_sender_exe() );
        t_acq_req->set_client_version( a_request->get_sender_version() );
        t_acq_req->set_client_package( a_request->get_sender_package() );

        t_acq_req->set_status( acq_request::acknowledged );

        INFO( mtlog, "Queuing request" );
        enqueue( t_acq_req );

        a_pkg.f_payload.merge( *t_acq_req );
        a_pkg.f_payload.add( "status-meaning", new param_value( acq_request::interpret_status( t_acq_req->get_status() ) ) );
        if( ! a_pkg.send_reply( R_SUCCESS, "Run request succeeded" ) )
        {
            WARN( mtlog, "Failed to send reply regarding the run request" );
        }

        return true;
    }

    bool acq_request_db::handle_get_acq_status_request( const msg_request* a_request, request_reply_package& a_pkg )
    {
        if( ! a_request->get_payload().has( "values" ) || ! a_request->get_payload()[ "values" ].is_array() )
        {
            a_pkg.send_reply( R_MESSAGE_ERROR_BAD_PAYLOAD, "Invalid payload: no <values> array present" );
            return false;
        }
        string t_acq_id_str = a_request->get_payload().array_at( "values" )->get_value( 0 );

        if( t_acq_id_str.empty() )
        {
            a_pkg.send_reply( R_MESSAGE_ERROR_BAD_PAYLOAD, "Invalid/no acquisition id" );
            return false;
        }

        uuid_t t_id = uuid_from_string(t_acq_id_str );
        DEBUG( mtlog, "Requesting status of acquisition <" << t_id << ">" );

        const acq_request* t_request = get_acq_request( t_id );
        if( t_request == NULL )
        {
            a_pkg.send_reply( R_MESSAGE_ERROR_BAD_PAYLOAD, "Did not find acquisition <" + to_string( t_id ) + ">" );
            return false;
        }

        f_db_mutex.lock();
        a_pkg.f_payload.merge( *t_request );
        a_pkg.f_payload.add( "status-meaning", new param_value( acq_request::interpret_status( t_request->get_status() ) ) );
        f_db_mutex.unlock();

        return a_pkg.send_reply( R_SUCCESS, "Acquisition status request succeeded" );
    }

    bool acq_request_db::handle_queue_request( const msg_request* /*a_request*/, request_reply_package& a_pkg )
    {
        param_array* t_queue_array = new param_array();
        f_queue_mutex.lock();
        for( acq_request_queue::const_iterator t_queue_it = f_acq_request_queue.begin(); t_queue_it != f_acq_request_queue.end(); ++t_queue_it )
        {
            param_node* t_acq_node = new param_node();
            t_acq_node->add( "id", new param_value( (*t_queue_it)->get_id_string() ) );
            t_acq_node->add( "file", new param_value( (*t_queue_it)->get_value( "file" ) ) );
            t_queue_array->push_back( t_acq_node );
        }
        f_queue_mutex.unlock();
        a_pkg.f_payload.add( "queue", t_queue_array );
        return a_pkg.send_reply( R_SUCCESS, "Queue request succeeded" );
    }

    bool acq_request_db::handle_queue_size_request( const msg_request* /*a_request*/, request_reply_package& a_pkg )
    {
        a_pkg.f_payload.add( "queue-size", new param_value( (uint32_t)queue_size() ) );
        return a_pkg.send_reply( R_SUCCESS, "Queue size request succeeded" );
    }

    bool acq_request_db::handle_cancel_acq_request( const msg_request* a_request, request_reply_package& a_pkg  )
    {
        if( ! a_request->get_payload().has( "values" ) || ! a_request->get_payload()[ "values" ].is_array() )
        {
            a_pkg.send_reply( R_MESSAGE_ERROR_BAD_PAYLOAD, "Invalid payload: no <values> array present" );
            return false;
        }
        string t_acq_id_str = a_request->get_payload().array_at( "values" )->get_value( 0 );

        if( t_acq_id_str.empty() )
        {
            a_pkg.send_reply( R_MESSAGE_ERROR_BAD_PAYLOAD, "Invalid/no acquisition id" );
            return false;
        }

        uuid_t t_id = uuid_from_string( t_acq_id_str );
        INFO( mtlog, "Canceling acquisition <" << t_id << ">" );

        if( ! cancel( t_id ) )
        {
            a_pkg.send_reply( R_MESSAGE_ERROR_BAD_PAYLOAD, "Failed to cancel acquisition <" + string_from_uuid( t_id ) + ">; it may not exist" );
            return false;
        }

        const acq_request* t_request = get_acq_request( t_id );
        f_db_mutex.lock();
        a_pkg.f_payload.merge( *t_request );
        a_pkg.f_payload.add( "status-meaning", new param_value( acq_request::interpret_status( t_request->get_status() ) ) );
        f_db_mutex.unlock();
        return a_pkg.send_reply( R_SUCCESS, "Cancellation succeeded" );
    }

    bool acq_request_db::handle_clear_queue_request( const msg_request* /*a_request*/, request_reply_package& a_pkg )
    {
        clear_queue();
        return a_pkg.send_reply( R_SUCCESS, "Queue is clear (aside for runs in progress" );
    }

    bool acq_request_db::handle_start_queue_request( const msg_request* /*a_request*/, request_reply_package& a_pkg )
    {
        start_queue();
        return a_pkg.send_reply( R_SUCCESS, "Queue started" );
    }

    bool acq_request_db::handle_stop_queue_request( const msg_request* /*a_request*/, request_reply_package& a_pkg )
    {
        stop_queue();
        return a_pkg.send_reply( R_SUCCESS, "Queue stopped" );
    }


} /* namespace mantis */
