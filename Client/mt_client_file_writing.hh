/*
 * mt_client_file_writing.hh
 *
 *  Created on: Nov 26, 2013
 *      Author: nsoblath
 */

#ifndef MT_CLIENT_FILE_WRITING_HH_
#define MT_CLIENT_FILE_WRITING_HH_

namespace mantis
{
    class buffer;
    class client_worker;
    class condition;
    class file_writer;
    class record_receiver;
    class request_dist;
    class server;
    class thread;

    class client_file_writing
    {
        public:
            client_file_writing( request_dist* a_connection_to_server, int a_write_port );
            virtual ~client_file_writing();

            void wait_for_finish();

        private:
            server* f_server;
            condition *f_buffer_condition;
            buffer *f_buffer;
            record_receiver* f_receiver;
            file_writer *f_writer;
            client_worker* f_worker;
            thread* f_thread;
    };

} /* namespace mantis */
#endif /* MT_CLIENT_FILE_WRITING_HH_ */
