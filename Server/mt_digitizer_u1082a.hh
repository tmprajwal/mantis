#ifndef MT_DIGITIZER_U1082A_HH_
#define MT_DIGITIZER_U1082A_HH_

#include "mt_digitizer.hh"

#include "mt_atomic.hh"
#include "mt_block.hh"
#include "mt_condition.hh"

#include "AgMD1Fundamental.h"

//#include <semaphore.h>

namespace mantis
{
    void PrintU1082AError( ViSession a_handle, ViStatus a_status, const std::string& a_prepend_msg );

    class block_cleanup_u1082a;

    class digitizer_u1082a : public digitizer
    {
        public:
            typedef ViInt8 data_type;

            static unsigned data_type_size_u1082a();

        public:
            digitizer_u1082a();
            virtual ~digitizer_u1082a();

            bool allocate( buffer* a_buffer, condition* a_condition );
            bool initialize( request* a_request );
            void execute();
            void cancel();
            void finalize( response* a_response );

            bool write_mode_check( request_file_write_mode_t mode );

            unsigned data_type_size();

            // thread-safe getter
            bool get_canceled();
            // thread-safe setter
            void set_canceled( bool a_flag );

        private:
            static const unsigned s_data_type_size;

            //sem_t* f_semaphore;

            ViSession f_handle;
            bool f_allocated;

            buffer* f_buffer;
            condition* f_condition;

            time_nsec_type f_start_time;

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


    class block_cleanup_u1082a : public block_cleanup
    {
        public:
            block_cleanup_u1082a( byte_type* a_data, ViSession a_handle );
            virtual ~block_cleanup_u1082a();
            virtual bool delete_data();
        private:
            bool f_triggered;
            byte_type* f_data;
            ViSession f_handle;
    };


    class test_digitizer_u1082a : public test_digitizer
    {
        public:
            test_digitizer_u1082a() {}
            virtual ~test_digitizer_u1082a() {}
    
            bool run_test();

    };

}

#endif