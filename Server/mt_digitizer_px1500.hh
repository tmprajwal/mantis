#ifndef MT_DIGITIZER_PX1500_HH_
#define MT_DIGITIZER_PX1500_HH_

#include "mt_digitizer.hh"

#include "mt_atomic.hh"
#include "mt_block.hh"
#include "mt_condition.hh"

#include "px1500.h"
#include "thorax.hh"

//#include <semaphore.h>

namespace mantis
{
    typedef px4_sample_t px1500_data_t;

    struct block_cleanup_px1500 : block_cleanup
    {
        block_cleanup_px1500( px1500_data_t* a_data, HPX4* a_dig_ptr );
        virtual ~block_cleanup_px1500();
        virtual bool delete_data();
        bool f_triggered;
        px1500_data_t* f_data;
        HPX4* f_dig_ptr;
    };

    class digitizer_px1500 : public digitizer
    {
        public:
            static unsigned bit_depth_px1500();
            static unsigned data_type_size_px1500();

        public:
            digitizer_px1500();
            virtual ~digitizer_px1500();

            bool allocate( buffer* a_buffer, condition* a_condition );
            bool initialize( request* a_request );
            void execute();
            void cancel();
            void finalize( response* a_response );

            bool write_mode_check( request_file_write_mode_t mode );

            unsigned bit_depth();
            unsigned data_type_size();

            // thread-safe getter
            bool get_canceled();
            // thread-safe setter
            void set_canceled( bool a_flag );

        private:
            static const unsigned s_bit_depth;
            static const unsigned s_data_type_size;

            //sem_t* f_semaphore;

            HPX4 f_handle;
            bool f_allocated;

            buffer* f_buffer;
            condition* f_condition;

            record_id_type f_record_last;
            record_id_type f_record_count;
            acquisition_id_type f_acquisition_count;
            time_nsec_type f_live_time;
            time_nsec_type f_dead_time;

            atomic_bool f_canceled;
            condition f_cancel_condition;

            bool start();
            bool acquire( block* a_block, timespec& a_time_stamp );
            bool stop();
    };
}

#endif
