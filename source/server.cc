#include "server.hh"

#include "exception.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <iostream>
using std::cout;
using std::endl;

namespace mantis
{

    server::server() :
            f_socket( 0 ),
            f_address( NULL ),
            f_connections(),
            f_index( 0 )
    {
    }

    server::~server()
    {
    }

    void server::open( const int& a_port )
    {
        cout << "opening server socket on port <" << a_port << ">" << endl;

        //prepare address structure
        socklen_t t_socket_length = sizeof(sockaddr_in);
        f_address = new sockaddr_in();
        memset( f_address, 0, t_socket_length );
        f_address->sin_family = AF_INET;
        f_address->sin_addr.s_addr = INADDR_ANY;
        f_address->sin_port = htons( a_port );

        cout << "server address prepared..." << endl;

        //open socket
        f_socket = ::socket( AF_INET, SOCK_STREAM, 0 );
        if( f_socket < 0 )
        {
            throw exception() << "could not create socket";
            return;
        }

        cout << "server socket open..." << endl;

        //bind socket
        if( ::bind( f_socket, (const sockaddr*) (f_address), t_socket_length ) < 0 )
        {
            throw exception() << "could not bind socket";
            return;
        }

        cout << "server socket bound..." << endl;

        //start listening
        ::listen( f_socket, 10 );

        cout << "server socket listening..." << endl;

        return;
    }

    void server::close()
    {
        //delete connections
        for( int t_index = 0; t_index < f_index; t_index++ )
        {
            delete f_connections[ t_index ];
        }

        //clean up server address
        delete f_address;

        //close server socket
        ::close( f_socket );
    }

    connection* server::get_connection()
    {
        int t_socket = 0;
        sockaddr_in* t_address = NULL;

        //prepare the new address
        socklen_t t_socket_length = sizeof(sockaddr_in);
        t_address = new sockaddr_in();
        memset( &t_address, 0, t_socket_length );

        //prepare the new socket
        t_socket = ::accept( f_socket, (sockaddr*) (t_address), &t_socket_length );
        if( t_socket < 0 )
        {
            throw exception() << "could not accept connection";
        }

        //return a new connection
        if( f_index < 16 )
        {
            connection* t_connection = new connection( t_socket, t_address );
            f_connections[f_index++] = t_connection;
            return t_connection;
        }
        else
        {
            throw exception() << "too many connections open";
            return NULL;
        }
    }

}