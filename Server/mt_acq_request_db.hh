#ifndef MT_ACQ_REQUEST_DB_HH_
#define MT_ACQ_REQUEST_DB_HH_

#include "mt_atomic.hh"
#include "mt_condition.hh"
#include "mt_mutex.hh"
#include "mt_uuid.hh"

#include <list>
#include <map>
#include <string>

namespace scarab
{
    class param_node;
}

namespace mantis
{
    class acq_request;
    class config_manager;
    class msg_request;

    struct request_reply_package;

//#ifdef _WIN32
//    MANTIS_EXPIMP_TEMPLATE template class MANTIS_API std::list< acq_request* >;
//    MANTIS_EXPIMP_TEMPLATE template class MANTIS_API std::map< unsigned, acq_request* >;
//#endif
    
    class MANTIS_API acq_request_db
    {
        public:
            acq_request_db( config_manager* a_conf_mgr, const std::string& a_exe_name = "unknown" );
            virtual ~acq_request_db();


            //***************
            // DB Commands
            //***************
        public:

            bool empty();

            acq_request* get_acq_request( uuid_t a_id );
            const acq_request* get_acq_request( uuid_t a_id ) const;

            uuid_t add( acq_request* a_acq_request ); /// adds acq_request to the database (but not the queue); returns the assigned acq_request ID number
            acq_request* remove( uuid_t a_id ); /// removes acq_request with id a_id, and returns the pointer to it

            void flush(); /// remove completed & failed acq_requests; removed acq_requests are deleted
            void clear(); /// remove all acq_requests; acq_requests are deleted

        private:
            typedef std::map< uuid_t, acq_request* > acq_request_db_data;

            mutable mutex f_db_mutex;
            acq_request_db_data f_acq_request_db;


            //******************
            // Queue Commands
            //******************
        public:

            bool queue_empty();
            size_t queue_size();

            uuid_t enqueue( acq_request* a_acq_request ); /// adds acq_request to the queue and database; returns the assigned acq_request ID number
            bool cancel( uuid_t a_id ); /// removes acq_request with id a_id from the queue
            acq_request* pop();

            void wait_for_request_in_queue();

            void clear_queue(); /// remove all requests in the queue; removed acq_requests are deleted

            bool queue_is_active() const;

            void wait_for_queue_active();

            void start_queue();
            void stop_queue();

        private:
            typedef std::list< acq_request* > acq_request_queue;

            mutable mutex f_queue_mutex;
            acq_request_queue f_acq_request_queue;

            condition f_request_in_queue_condition;

            atomic_bool f_queue_is_active;
            condition f_queue_active_condition;


            //********************
            // Request handlers
            //********************
        public:

            bool handle_new_acq_request( const msg_request* a_request, request_reply_package& a_pkg );

            bool handle_get_acq_status_request( const msg_request* a_request, request_reply_package& a_pkg );
            bool handle_queue_request( const msg_request* a_request, request_reply_package& a_pkg );
            bool handle_queue_size_request( const msg_request* a_request, request_reply_package& a_pkg );

            bool handle_cancel_acq_request( const msg_request* a_request, request_reply_package& a_pkg );
            bool handle_clear_queue_request( const msg_request* a_request, request_reply_package& a_pkg );

            bool handle_start_queue_request( const msg_request* a_request, request_reply_package& a_pkg );
            bool handle_stop_queue_request( const msg_request* a_request, request_reply_package& a_pkg );

        private:
            config_manager* f_config_mgr;

            std::string f_exe_name;

    };

}

#endif /* MT_ACQ_REQUEST_DB_HH_ */
