#include "mt_digitizer_px14400.hh"

#include "mt_buffer.hh"
#include "mt_condition.hh"
#include "mt_exception.hh"
#include "mt_factory.hh"
#include "logger.hh"
#include "mt_iterator.hh"

#include <cmath> // for ceil()
#include <cstring>
#include <errno.h>
//#include <fcntl.h> // for O_CREAT and O_EXCL
#include <sstream>

namespace mantis
{
    LOGGER( mtlog, "digitizer_px14400" );

    MT_REGISTER_DIGITIZER( digitizer_px14400, "px14400" );

    const unsigned digitizer_px14400::s_data_type_size = sizeof( digitizer_px14400::data_type );
    unsigned digitizer_px14400::data_type_size_px14400()
    {
        return digitizer_px14400::s_data_type_size;
    }

    digitizer_px14400::digitizer_px14400() :
            //f_semaphore( NULL ),
            f_buffer( NULL ),
            f_condition( NULL ),
            f_allocated( false ),
            f_handle(),
            f_start_time( 0 ),
            f_record_last( 0 ),
            f_record_count( 0 ),
            f_acquisition_count( 0 ),
            f_live_time( 0 ),
            f_dead_time( 0 ),
            f_canceled( false ),
            f_cancel_condition()
    {
        get_calib_params( px14400_bits, s_data_type_size, px14400_min_val, px14400_range, false, &f_params );
        /*
        errno = 0;
        f_semaphore = sem_open( "/digitizer_px14400", O_CREAT | O_EXCL );
        if( f_semaphore == SEM_FAILED )
        {
            if( errno == EEXIST )
            {
                throw exception() << "digitizer_px14400 is already in use";
            }
            else
            {
                throw exception() << "semaphore error: " << strerror( errno );
            }
        }
         */
    }

    digitizer_px14400::~digitizer_px14400()
    {
        if( f_buffer != NULL ) deallocate( f_buffer );
        /*
        if( f_semaphore != SEM_FAILED )
        {
            sem_close( f_semaphore );
        }
         */
    }

    bool digitizer_px14400::allocate( buffer* a_buffer, condition* a_condition )
    {
        f_buffer = a_buffer;
        f_condition = a_condition;

        int t_result;

        INFO( mtlog, "connecting to digitizer card..." );

        // SN of the px14400 card is 100954
        DEBUG( mtlog, "trying the SN, 100954" );
        t_result = ConnectToDevicePX14( &f_handle, 100954 );
        if( t_result == SIG_SUCCESS )
        {
            WARN( mtlog, "connection worked using serial number: 100954" );
        }
        else
        {
            for( unsigned i = 0; i <= 16; ++i )
            {
                DEBUG( mtlog, "trying device number " << i );
                t_result = ConnectToDevicePX14( &f_handle, i );
                if( t_result == SIG_SUCCESS )
                {
                    WARN( mtlog, "connection worked using board number: " << i );
                    break;
                }
            }
        }
        //t_result = ConnectToDevicePX14( &f_handle, 0 );
        if( t_result != SIG_SUCCESS )
        {
            DumpLibErrorPX14( t_result, "failed to connect to digitizer card: " );
            return false;
        }

        //INFO( mtlog, "setting power up defaults..." );

        t_result = SetPowerupDefaultsPX14( f_handle );
        if( t_result != SIG_SUCCESS )
        {
            DumpLibErrorPX14( t_result, "failed to enter default state: " );
            return false;
        }

        INFO( mtlog, "allocating dma buffer..." );

        try
        {
            for( unsigned int index = 0; index < f_buffer->size(); index++ )
            {
                block* t_new_block = new block();
                t_result = AllocateDmaBufferPX14( f_handle, f_buffer->block_size(), reinterpret_cast< data_type** >( t_new_block->handle() ) );
                if( t_result != SIG_SUCCESS )
                {
                    std::stringstream t_buff;
                    t_buff << "failed to allocate dma memory for block " << index <<": ";
                    DumpLibErrorPX14( t_result, t_buff.str().c_str() );
                    return false;
                }
                t_new_block->set_data_size( f_buffer->block_size() );
                t_new_block->set_data_nbytes( f_buffer->block_size() * digitizer_px14400::s_data_type_size );
                f_buffer->set_block( index, t_new_block );
            }
        }
        catch( exception& e )
        {
            ERROR( mtlog, "unable to allocate buffer: " << e.what() );
            return false;
        }

        f_allocated = true;
        return true;
    }

    bool digitizer_px14400::deallocate( buffer* a_buffer )
    {
        if( f_allocated && a_buffer == f_buffer )
        {
            int t_result;

            INFO( mtlog, "deallocating dma buffer..." );

            iterator t_it( f_buffer );
            for( size_t index = 0; index < f_buffer->size(); index++ )
            {
                f_buffer->delete_block( index );
            }
            f_allocated = false;
            f_buffer = NULL;

            INFO( mtlog, "disconnecting from digitizer card..." );

            t_result = DisconnectFromDevicePX14( f_handle );
            if( t_result != SIG_SUCCESS )
            {
                DumpLibErrorPX14( t_result, "failed to disconnect from digitizer card: " );
                return false;
            }
            return true
        }
        ERROR( mtlog, "Cannot deallocate buffer that was not allocated by this digitizer" );
        return false;
    }

    bool digitizer_px14400::initialize( request* a_request )
    {
        int t_result;

        //INFO( mtlog, "resetting counters..." );

        f_record_last = (record_id_type) (ceil( (double) (a_request->rate() * a_request->duration() * 1.e3) / (double) (f_buffer->block_size()) ));
        f_record_count = 0;
        f_acquisition_count = 0;
        f_live_time = 0;
        f_dead_time = 0;

        //INFO( mtlog, "setting run mode..." );

        if( a_request->mode() == request_mode_t_single )
        {
            t_result = SetActiveChannelsPX14( f_handle, PX14CHANNEL_ONE );
            if( t_result != SIG_SUCCESS )
            {
                DumpLibErrorPX14( t_result, "failed to activate channel 1: " );
                return false;
            }

        }
        if( a_request->mode() == request_mode_t_dual_separate || a_request->mode() == request_mode_t_dual_interleaved )
        {
            t_result = SetActiveChannelsPX14( f_handle, PX14CHANNEL_DUAL );
            if( t_result != SIG_SUCCESS )
            {
                DumpLibErrorPX14( t_result, "failed to activate channels 1 and 2: " );
                return false;
            }
        }

        //INFO( mtlog, "setting clock rate..." );

        t_result = SetInternalAdcClockRatePX14( f_handle, a_request->rate() );
        if( t_result != SIG_SUCCESS )
        {
            DumpLibErrorPX14( t_result, "failed to set clock rate: " );
            return false;
        }

        return true;
    }
    void digitizer_px14400::execute()
    {
        iterator t_it( f_buffer, "dig-px14400" );

        timespec t_live_start_time;
        timespec t_live_stop_time;
        timespec t_dead_start_time;
        timespec t_dead_stop_time;
        timespec t_stamp_time;

        //INFO( mtlog, "waiting" );

        f_condition->wait();

        INFO( mtlog, "loose at <" << t_it.index() << ">" );

        //start acquisition
        if( start() == false )
        {
            ERROR( mtlog, "unable to start acquisition" );
            return;
        }

        //start timing
        get_time_monotonic( &t_live_start_time );
        f_start_time = time_to_nsec( t_live_start_time );

        //go go go go
        while( true )
        {
            //check if we've written enough
            if( f_record_count == f_record_last || f_canceled.load() )
            {
                //mark the block as written
                t_it->set_written();

                //get the time and update the number of live nanoseconds
                get_time_monotonic( &t_live_stop_time );		

                f_live_time += time_to_nsec( t_live_stop_time ) - time_to_nsec( t_live_start_time );

                //halt the pci acquisition
                stop();

                //GET OUT
                if( f_canceled.load() )
                {
                    INFO( mtlog, "was canceled mid-run" );
                    f_cancel_condition.release();
                }
                else
                {
                    INFO( mtlog, "finished normally" );
                }
                return;
            }

            t_it->set_acquiring();

            if( acquire( t_it.object(), t_stamp_time ) == false )
            {
                //mark the block as written
                t_it->set_written();

                //get the time and update the number of live microseconds
                f_live_time += time_to_nsec( t_live_stop_time ) - time_to_nsec( t_live_start_time );

                //halt the pci acquisition
                stop();

                //GET OUT
                INFO( mtlog, "finished abnormally because acquisition failed" );
                return;
            }

            t_it->set_acquired();

            if( +t_it == false )
            {
                INFO( mtlog, "blocked at <" << t_it.index() << ">" );

                //stop live timer
                get_time_monotonic( &t_live_stop_time );

                //accumulate live time
                f_live_time += time_to_nsec( t_live_stop_time ) - time_to_nsec( t_live_start_time );

                //halt the pci acquisition
                if( stop() == false )
                {
                    //GET OUT
                    INFO( mtlog, "finished abnormally because halting streaming failed" );
                    return;
                }

                //start dead timer
                get_time_monotonic( &t_dead_start_time );

                //wait
                f_condition->wait();

                //stop dead timer
                get_time_monotonic( &t_dead_stop_time );

                //accumulate dead time
                f_dead_time += time_to_nsec( t_dead_stop_time ) - time_to_nsec( t_dead_start_time );

                //start acquisition
                if( start() == false )
                {
                    //GET OUT
                    INFO( mtlog, "finished abnormally because starting streaming failed" );
                    return;
                }

                //increment block
                ++t_it;

                //start live timer
                get_time_monotonic( &t_live_start_time );;

                INFO( mtlog, "loose at <" << t_it.index() << ">" );
            }
        }

        return;
    }
    void digitizer_px14400::cancel()
    {
        if( ! f_canceled.load() )
        {
            f_canceled.store( true );
            f_cancel_condition.wait();
        }
        return;
    }
    void digitizer_px14400::finalize( param_node* a_response )
    {
        //INFO( mtlog, "calculating statistics..." );
        double t_livetime = (double) (f_live_time) * SEC_PER_NSEC;
        double t_deadtime = (double) f_dead_time * SEC_PER_NSEC;
        double t_mb_recorded = (double) (4 * f_record_count);

        param_node* t_resp_node = new param_node();
        param_value t_value;
        t_resp_node->add( "record-count", t_value << f_record_count );
        t_resp_node->add( "acquisition-count", t_value << f_acquisition_count );
        t_resp_node->add( "livetime", t_value << t_livetime );
        t_resp_node->add( "deadtime", t_value << t_deadtime );
        t_resp_node->add( "mb-recorded", t_value << t_mb_recorded );
        t_resp_node->add( "digitization-rate", t_value << t_mb_recorded / t_livetime );

        a_response->add( "digitizer-test", t_resp_node );

        return;
    }

    bool digitizer_px14400::start()
    {
        int t_result = BeginBufferedPciAcquisitionPX14( f_handle, PX14_FREE_RUN );
        if( t_result != SIG_SUCCESS )
        {
            DumpLibErrorPX14( t_result, "failed to begin dma acquisition: " );
            return false;
        }

        return true;
    }
    bool digitizer_px14400::acquire( block* a_block, timespec& a_stamp_time )
    {
        a_block->set_record_id( f_record_count );
        a_block->set_acquisition_id( f_acquisition_count );

        int t_result = GetPciAcquisitionDataFastPX14( f_handle, f_buffer->block_size(), reinterpret_cast< data_type* >( a_block->data_bytes() ), 0 );
        if( t_result != SIG_SUCCESS )
        {
            DumpLibErrorPX14( t_result, "failed to acquire dma data over pci: " );
            t_result = EndBufferedPciAcquisitionPX14( f_handle );
            return false;
        }

        // the timestamp is acquired after the data is transferred to avoid the problem on the px1500 where
        // the first record can take unusually long to be acquired.
        // it's done here too for consistency
        get_time_monotonic( &a_stamp_time );
        a_block->set_timestamp( time_to_nsec( a_stamp_time ) - f_start_time );

        ++f_record_count;

        return true;
    }
    bool digitizer_px14400::stop()
    {
        int t_result = EndBufferedPciAcquisitionPX14( f_handle );
        if( t_result != SIG_SUCCESS )
        {
            DumpLibErrorPX14( t_result, "failed to end dma acquisition: " );
            return false;
        }
        ++f_acquisition_count;
        return true;
    }

    unsigned digitizer_px14400::data_type_size()
    {
        return digitizer_px14400::s_data_type_size;
    }


    bool digitizer_px14400::get_canceled()
    {
        return f_canceled.load();
    }

    void digitizer_px14400::set_canceled( bool a_flag )
    {
        f_canceled.store( a_flag );
        return;
    }


    //***********************************
    // Block Cleanup px14400
    //***********************************

    block_cleanup_px14400::block_cleanup_px14400( byte_type* a_memblock, HPX14* a_dig_ptr ) :
                f_triggered( false ),
                f_memblock( a_memblock ),
                f_dig_ptr( a_dig_ptr )
    {}
    block_cleanup_px14400::~block_cleanup_px14400()
    {}
    bool block_cleanup_px14400::delete_memblock()
    {
        if( f_triggered ) return true;
        int t_result = FreeDmaBufferPX14( *f_dig_ptr, reinterpret_cast< digitizer_px14400::data_type* >( f_memblock ) );
        if( t_result != SIG_SUCCESS )
        {
            DumpLibErrorPX14( t_result, "failed to deallocate dma memory: " );
            return false;
        }
        f_triggered = true;
        return true;
    }


    //***********************************
    // test_digitizer_px14400
    //***********************************

    bool test_digitizer_px14400::run_test()
    {
        int t_result;
        HPX14 f_handle;

        INFO( mtlog, "beginning allocation phase" );

        DEBUG( mtlog, "connecting to digitizer card..." );

        t_result = ConnectToDevicePX14( &f_handle, 1 );
        if( t_result != SIG_SUCCESS )
        {
            DumpLibErrorPX14( t_result, "failed to connect to digitizer card: " );
            return false;
        }

        DEBUG( mtlog, "setting power up defaults..." );

        t_result = SetPowerupDefaultsPX14( f_handle );
        if( t_result != SIG_SUCCESS )
        {
            DumpLibErrorPX14( t_result, "failed to enter default state: " );
            return false;
        }

        DEBUG( mtlog, "allocating dma buffer..." );

        block* t_block = NULL;
        // for the px14400, there is no minimum record size listed
        unsigned t_rec_size = 16384;

        try
        {
            t_block = new block();
            t_result = AllocateDmaBufferPX14( f_handle, t_rec_size, (digitizer_px14400::data_type**)t_block->handle() );
            if( t_result != SIG_SUCCESS )
            {
                DumpLibErrorPX14( t_result, "failed to allocate dma memory: " );
                return false;
            }
            t_block->set_data_size( t_rec_size );
            t_block->set_data_nbytes( t_rec_size * px14400_data_type_size );
        }
        catch( exception& e )
        {
            ERROR( mtlog, "unable to allocate buffer: " << e.what() );
            return false;
        }

        INFO( mtlog, "allocation complete!\n" );



        INFO( mtlog, "beginning initialization phase" );

        DEBUG( mtlog, "setting run mode..." );

        t_result = SetActiveChannelsPX14( f_handle, PX14CHANNEL_ONE );
        if( t_result != SIG_SUCCESS )
        {
            DumpLibErrorPX14( t_result, "failed to activate channel 1: " );
            return false;
        }

        DEBUG( mtlog, "setting clock rate..." );

        t_result = SetInternalAdcClockRatePX14( f_handle, 200. );
        if( t_result != SIG_SUCCESS )
        {
            DumpLibErrorPX14( t_result, "failed to set clock rate: " );
            return false;
        }

        INFO( mtlog, "initialization complete!\n" );


        INFO( mtlog, "beginning run phase" );

        DEBUG( mtlog, "beginning acquisition" );

        t_result = BeginBufferedPciAcquisitionPX14( f_handle, PX14_FREE_RUN );
        if( t_result != SIG_SUCCESS )
        {
            DumpLibErrorPX14( t_result, "failed to begin dma acquisition: " );
            return false;
        }

        DEBUG( mtlog, "acquiring a record" );

        t_result = GetPciAcquisitionDataFastPX14( f_handle, t_rec_size, (digitizer_px14400::data_type*)t_block->data_bytes(), 0 );
        if( t_result != SIG_SUCCESS )
        {
            DumpLibErrorPX14( t_result, "failed to acquire dma data over pci: " );
            t_result = EndBufferedPciAcquisitionPX14( f_handle );
            return false;
        }

        DEBUG( mtlog, "ending acquisition..." );

        t_result = EndBufferedPciAcquisitionPX14( f_handle );
        if( t_result != SIG_SUCCESS )
        {
            DumpLibErrorPX14( t_result, "failed to end dma acquisition: " );
            return false;
        }

        block_view< digitizer_px14400::data_type > t_block_view;
        std::stringstream t_str_buff;
        for( unsigned i = 0; i < 99; ++i )
        {
            t_str_buff << t_block_view.data_view()[ i ] << ", ";
        }
        t_str_buff << t_block_view.data_view()[ 99 ];
        DEBUG( mtlog, "the first 100 samples taken:\n" << t_str_buff.str() );

        INFO( mtlog, "run complete!\n" );


        INFO( mtlog, "beginning finalization phase" );

        DEBUG( mtlog, "deallocating dma buffer" );

        delete t_block;

        INFO( mtlog, "finalization complete!\n" );


        return true;
    }

}
