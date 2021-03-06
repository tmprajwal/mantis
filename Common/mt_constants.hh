/*
 * mt_constants.hh
 *
 *  Created on: Jan 23, 2015
 *      Author: nsoblath
 */

#ifndef MT_CONSTANTS_HH_
#define MT_CONSTANTS_HH_

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace mantis
{
    // API export macros for windows
#ifdef _WIN32
#  ifdef MANTIS_API_EXPORTS
#    define MANTIS_API __declspec(dllexport)
#    define MANTIS_EXPIMP_TEMPLATE
#  else
#    define MANTIS_API __declspec(dllimport)
#    define MANTIS_EXPIMP_TEMPLATE extern
#  endif
#else
#  define MANTIS_API
#endif



    // Executable return constants

#define RETURN_SUCCESS 1
#define RETURN_ERROR -1
#define RETURN_CANCELED -2
#define RETURN_REVOKED -3



    // AMQP message constants
    // Conforming to the dripline wire protocol: https://github.com/project8/hardware/wiki/Wire-Protocol
    // Please be sure that these constants are kept in sync with the dripline constants.

    // Operation constants
#define OP_SET  0
#define OP_GET  1
#define OP_CONFIG 6
#define OP_SEND 7
#define OP_RUN  8
#define OP_CMD  9
#define OP_UNKNOWN UINT_MAX

    // Message type constants
#define T_REPLY   2
#define T_REQUEST 3
#define T_ALERT   4
#define T_INFO    5


    // Return codes
#define R_SUCCESS                           0

#define R_WARNING_NO_ACTION_TAKEN           1

#define R_AMQP_ERROR                      100
#define R_AMQP_ERROR_BROKER_CONNECTION    101
#define R_AMQP_ERROR_ROUTINGKEY_NOTFOUND  102

#define R_DEVICE_ERROR                    200
#define R_DEVICE_ERROR_CONNECTION         201
#define R_DEVICE_ERROR_NO_RESP            202

#define R_MESSAGE_ERROR                   300
#define R_MESSAGE_ERROR_NO_ENCODING       301
#define R_MESSAGE_ERROR_DECODING_FAIL     302
#define R_MESSAGE_ERROR_BAD_PAYLOAD       303
#define R_MESSAGE_ERROR_INVALID_VALUE     304
#define R_MESSAGE_ERROR_TIMEOUT           305
#define R_MESSAGE_ERROR_INVALID_METHOD    306
#define R_MESSAGE_ERROR_ACCESS_DENIED      307
#define R_MESSAGE_ERROR_INVALID_KEY        308

}

#endif /* MT_CONSTANTS_HH_ */
