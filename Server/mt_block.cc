#include "mt_block.hh"

#include <sys/time.h>

namespace mantis
{

    block::block() :
            f_state( e_written ),
            f_acquisition_id( 0 ),
            f_record_id( 0 ),
            f_timestamp( 0 ),
            f_data( NULL ),
            f_data_size( 4194304 )
    {
    }

    block::~block()
    {
    }

    block::state_type block::get_state() const
    {
        return f_state;
    }
    void block::set_state( block::state_type a_state )
    {
        f_state = a_state;
        return;
    }

    bool block::is_acquiring() const
    {
        if( f_state == e_acquiring )
        {
            return true;
        }
        return false;
    }
    void block::set_acquiring()
    {
        f_state = e_acquiring;
        return;
    }

    bool block::is_acquired() const
    {
        if( f_state == e_acquired )
        {
            return true;
        }
        return false;
    }
    void block::set_acquired()
    {
        f_state = e_acquired;
        return;
    }

    bool block::is_writing() const
    {
        if( f_state == e_writing )
        {
            return true;
        }
        return false;
    }
    void block::set_writing()
    {
        f_state = e_writing;
        return;
    }

    bool block::is_written() const
    {
        if( f_state == e_written )
        {
            return true;
        }
        return false;
    }
    void block::set_written()
    {
        f_state = e_written;
        return;
    }

    const acquisition_id_type& block::get_acquisition_id() const
    {
        return f_acquisition_id;
    }
    void block::set_acquisition_id( const acquisition_id_type& an_id )
    {
        f_acquisition_id = an_id;
        return;
    }

    const record_id_type& block::get_record_id() const
    {
        return f_record_id;
    }
    void block::set_record_id( const record_id_type& an_id )
    {
        f_record_id = an_id;
        return;
    }

    const time_nsec_type& block::get_timestamp() const
    {
        return f_timestamp;
    }
    void block::set_timestamp( const time_nsec_type& a_timestamp )
    {
        f_timestamp = a_timestamp;
        return;
    }

    const size_t& block::get_data_size() const
    {
        return f_data_size;
    }
    void block::set_data_size( const size_t& a_size )
    {
        f_data_size = a_size;
        return;
    }

    data_type* block::data()
    {
        return f_data;
    }

    const data_type* block::data() const
    {
        return f_data;
    }

    data_type** block::handle()
    {
        return &f_data;
    }

}
