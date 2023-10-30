#pragma once
#using <System.dll>
#include "UGVModule.h"
#include "Laser.h"
#include "GNSS.h"
#include "VC.h"
#include "Display.h"
#include "Controller.h"

ref struct ThreadProperties
{
    ThreadStart^ ThreadStart_;
    bool Critical;
    String^ ThreadName;
    uint8_t BitID;

    ThreadProperties(ThreadStart^ start, bool crit, uint8_t bit_id, String^ threadName)
    {
        ThreadStart_ = start;
        Critical = crit;
        ThreadName = threadName;
        BitID = bit_id;

    }

};

ref class ThreadManagement : public UGVModule {
public:
    // Create shared memory objects
    error_state setupSharedMemory();

    // Send/Recieve data from shared memory structures
    error_state processSharedMemory() override;

    void shutdownModules();

    // Get Shutdown signal for module, from Thread Management SM
    bool getShutdownFlag() override;

    // Thread function for TMM
    void threadFunction() override;

    error_state processHeartbeats();

private:
    array<Thread^>^ ThreadList;
    array<ThreadProperties^>^ ThreadPropertiesList;
    array<Stopwatch^>^ StopwatchList;

};
