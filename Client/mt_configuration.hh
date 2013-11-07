/*
 * mt_configuration.hh
 *
 *  Created on: Nov 4, 2013
 *      Author: nsoblath
 */

#ifndef MT_CONFIGURATION_HH_
#define MT_CONFIGURATION_HH_

#include "document.h"

#include <string>

namespace mantis
{

    class configuration : public rapidjson::Document
    {
        public:
            configuration();
            virtual ~configuration();

            configuration& operator+=( configuration& rhs );

            void print() const;
    };

} /* namespace mantis */
#endif /* MT_CONFIGURATION_HH_ */
