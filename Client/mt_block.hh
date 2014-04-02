#ifndef MT_BLOCK_HH_
#define MT_BLOCK_HH_

#include "block_header.pb.h"
#include "thorax.hh"

namespace mantis
{
    /*!
     @class block
     @author D. Furse, N. Oblath

     @brief Single-record data class for mantis buffers; this base class provides the byte-type access functions
    */

    /*!
     @class empty_block
     @author N. Oblath

     @brief Block with no data
    */

    /*!
     @class block_cleanup
     @author N. Oblath

     @brief Memory de-allocation class
    */

    /*!
     @class typed_block
     @author N. Oblath

     @brief Single-record data class for mantis buffers; this templated derived class contains the actual data array
    */

    /*!
     @class block_view
     @author N. Oblath

     @brief Convenience class giving access to the data array cast to a different type
    */



    //**************************************************
    // block
    //**************************************************

    class block
    {
        public:
            block();
            virtual ~block();

            template< typename DataType >
            static block* allocate_block( unsigned a_size );

            block_header_state_t get_state() const;
            void set_state( block_header_state_t a_state );

            bool is_unused() const;
            void set_unused();

            bool is_acquiring() const;
            void set_acquiring();

            bool is_acquired() const;
            void set_acquired();

            bool is_processing() const;
            void set_processing();

            bool is_writing() const;
            void set_writing();

            bool is_written() const;
            void set_written();

            acquisition_id_type get_acquisition_id() const;
            void set_acquisition_id( acquisition_id_type an_id );

            record_id_type get_record_id() const;
            void set_record_id( record_id_type an_id );

            time_nsec_type get_timestamp() const;
            void set_timestamp( time_nsec_type a_timestamp );

            size_t get_data_size() const;
            void set_data_size( size_t a_size );

            block_header* header();
            const block_header* header() const;

            byte_type* data_bytes();
            const byte_type* data_bytes() const;

            size_t get_data_nbytes() const;
            void set_data_nbytes( size_t a_nbytes );

            byte_type** handle();

            void set_cleanup( block_cleanup* a_cleanup );

        protected:
            block_header f_header;

            byte_type* f_data_bytes;
            unsigned f_data_nbytes;

            block_cleanup* f_cleanup;
    };

    template< typename DataType >
    block* block::allocate_block( unsigned a_size )
    {
        block* t_new_block = new block();
        t_new_block->set_data_size( a_size );
        t_new_block->f_data_nbytes = a_size * sizeof( DataType );
        t_new_block->f_data_bytes = new byte_type [ t_new_block->f_data_nbytes ];
        return t_new_block;
    }


    //**************************************************
    // block_cleanup
    //**************************************************

    class block_cleanup
    {
        public:
            block_cleanup() {}
            virtual ~block_cleanup() {}
            virtual bool delete_data() = 0;
    };


    //**************************************************
    // block_view< DataType >
    //**************************************************

    template< typename DataType >
    class block_view
    {
        public:
            typedef DataType data_type;

        public:
            block_view( block* a_block = NULL );
            virtual ~block_view();

            void set_viewed( block* a_block );

            size_t get_data_view_size() const;
            data_type* data_view() const;

        private:
            block* f_block;
            DataType* f_data_view;
            size_t f_view_size;
    };

    template< typename DataType >
    block_view< DataType >::block_view( block* a_block ) :
            f_block( NULL ),
            f_data_view( NULL ),
            f_view_size( 0 )
    {
        if( a_block != NULL ) set_viewed( a_block );
    }

    template< typename DataType >
    block_view< DataType >::~block_view()
    {
    }

    template< typename DataType >
    void block_view< DataType >::set_viewed( block* a_block )
    {
        f_block = a_block;
        f_data_view = reinterpret_cast< DataType* >( a_block->data_bytes() );
        f_view_size = a_block->get_data_size() / sizeof( DataType );
        return;
    }

    template< typename DataType >
    size_t block_view< DataType >::get_data_view_size() const
    {
        return f_view_size;
    }

    template< typename DataType >
    DataType* block_view< DataType >::data_view() const
    {
        return f_data_view;
    }


}

#endif
