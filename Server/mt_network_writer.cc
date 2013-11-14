#include "mt_network_writer.hh"

#include <cstring> // for memcpy()
#include <iostream>
#include <sstream>
using std::cout;
using std::endl;
using std::stringstream;

namespace mantis
{
    network_writer::network_writer( buffer* a_buffer, condition* a_condition ) :
            writer( a_buffer, a_condition ),
            f_record_dist( NULL ),
            f_client( NULL )
    {
    }
    network_writer::~network_writer()
    {
        delete f_client;
        delete f_record_dist;
    }

    void network_writer::initialize( request* a_request )
    {
        writer::initialize( a_request );

        cout << "[network_writer] opening write connection..." << endl;

        f_record_dist = new record_dist();

        f_client = new client( a_request->write_host(), a_request->write_port() );
        f_record_dist->set_connection( f_client );

        return;
    }

    void network_writer::finalize( response* a_response )
    {
        writer::finalize( a_response );

        delete f_client;
        f_client = NULL;

        delete f_record_dist;
        f_record_dist = NULL;
    }

    bool network_writer::write( block* a_block )
    {
        return f_record_dist->push_record( a_block );
    }

}
