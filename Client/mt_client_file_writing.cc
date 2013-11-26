/*
 * mt_client_file_writing.cc
 *
 *  Created on: Nov 26, 2013
 *      Author: nsoblath
 */

#include "mt_client_file_writing.hh"

#include "mt_buffer.hh"
#include "mt_client_worker.hh"
#include "mt_condition.hh"
#include "mt_exception.hh"
#include "mt_file_writer.hh"
#include "mt_record_receiver.hh"
#include "mt_request_dist.hh"
#include "mt_server.hh"
#include "mt_thread.hh"

#include <iostream>
using std::cerr;
using std::cout;
using std::endl;

namespace mantis
{

    client_file_writing::client_file_writing( request_dist* a_connection_to_server, int a_write_port ) :
            f_server( NULL ),
            f_receiver( NULL ),
            f_worker( NULL ),
            f_thread( NULL ),
            f_buffer_condition( NULL ),
            f_buffer( NULL ),
            f_writer( NULL )
    {
        // objects for receiving and writing data
        try
        {
            f_server = new server( a_write_port );
        }
        catch( exception& e)
        {
            a_connection_to_server->get_client_status()->set_state( client_status_state_t_error );
            a_connection_to_server->push_client_status();
            throw exception() << "unable to create record-receiver server: " << e.what();
        }

        f_buffer_condition = new condition();
        f_buffer = new buffer( a_connection_to_server->get_status()->buffer_size(), a_connection_to_server->get_status()->record_size() );

        f_receiver = new record_receiver( f_server, f_buffer, f_buffer_condition );
        f_receiver->set_data_chunk_size( a_connection_to_server->get_status()->data_chunk_size() );

        f_writer = new file_writer();
        f_writer->set_buffer( f_buffer, f_buffer_condition );

        f_worker = new client_worker( a_connection_to_server->get_request(), f_receiver, f_writer, f_buffer_condition );

        f_thread = new thread( f_worker );

        cout << "[mantis_client] starting record receiver" << endl;

        try
        {
            f_thread->start();
        }
        catch( exception& e )
        {
            throw exception() << "unable to start record-receiving server";
        }
    }

    client_file_writing::~client_file_writing()
    {
        delete f_thread;
        delete f_worker;
        delete f_writer;
        delete f_receiver;
        delete f_buffer;
        delete f_buffer_condition;
        delete f_server;
    }

    void client_file_writing::wait_for_finish()
    {
        f_thread->join();
        return;
    }

} /* namespace mantis */
