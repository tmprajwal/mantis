#ifndef MANTISPX1500SOURCE_HPP_
#define MANTISPX1500SOURCE_HPP_

#include "MantisActor.hpp"

#include "MantisEnv.hpp"
#include <cstddef> // px1500.h refers to NULL without including cstddef
#include "px1500.h"

class MantisPX1500 :
    public MantisActor
{
    public:
        static MantisPX1500* digFromEnv( safeEnvPtr& env );
        virtual ~MantisPX1500();

        void Initialize();
        void Execute();
        void Finalize();

    private:
        MantisPX1500();

        HPX4 fHandle;
        unsigned int fPciRecordLength;
        unsigned long fRunDurationLastRecord;
        unsigned int fAcquisitionCount;
        unsigned long fRecordCount;
        unsigned long fLiveMicroseconds;
        unsigned long fDeadMicroseconds;

        double fAcquisitionRate;
        unsigned int fChannelMode;
        unsigned int fRecordLength;
        size_t fBufferCount;

        bool StartAcquisition();
        bool Acquire( MantisBufferRecord::DataType* anAddress );
        bool StopAcquisition();
};

#endif