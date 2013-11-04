#include "mt_parser.hh"
#include "mt_server.hh"
#include "mt_condition.hh"
#include "mt_queue.hh"
#include "mt_buffer.hh"
#include "mt_receiver.hh"
#include "mt_worker.hh"
#include "mt_digitizer_px1500.hh"
#include "mt_writer.hh"
#include "mt_thread.hh"
using namespace mantis;

#include <iostream>
using std::cout;
using std::endl;

int main( int argc, char** argv )
{
    parser t_parser( argc, argv );

    cout << "[mantis_server] creating objects..." << endl;

    server t_server( t_parser.get_required< int >( "port" ) );

    condition t_buffer_condition;
    buffer t_buffer( 512 );

    digitizer_px1500 t_digitizer( &t_buffer, &t_buffer_condition );
    writer t_writer( &t_buffer, &t_buffer_condition );

    condition t_queue_condition;
    queue t_queue;

    receiver t_receiver( &t_server, &t_queue, &t_queue_condition );
    worker t_worker( &t_digitizer, &t_writer, &t_queue, &t_queue_condition, &t_buffer_condition );

    cout << "[mantis_server] starting threads..." << endl;

    thread t_queue_thread( &t_queue );
    thread t_receiver_thread( &t_receiver );
    thread t_worker_thread( &t_worker );

    t_queue_thread.start();
    t_receiver_thread.start();
    t_worker_thread.start();

    cout << "[mantis_server] joining threads..." << endl;

    t_queue_thread.join();
    t_receiver_thread.join();
    t_worker_thread.join();

    cout << "[mantis_server] shutting down..." << endl;

    return 0;
}

