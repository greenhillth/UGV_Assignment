#pragma once
#using <System.dll>
#include "Controller.h"
#include "UGVModule.h"
#include "Laser.h"
#include "GNSS.h"
#include "VC.h"
#include "Display.h"

ref struct ThreadProperties
{
    ThreadStart^ ThreadStart_;
    bool Critical;
    String^ ThreadName;
    uint8_t BitID;
    int status;

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
    ThreadManagement();

    // Send/Recieve data from shared memory structures
    error_state processSharedMemory() override;

    void shutdownModules();

    // Get Shutdown signal for module, from Thread Management SM
    bool getShutdownFlag() override;

    // Thread function for TMM
    void threadFunction() override;

    void run();

    error_state processHeartbeats();

private:
     SM_Laser^ SM_LASER;
     SM_GNSS^ SM_GPS;
     SM_VehicleControl^ SM_VC;
     SM_Display^ SM_DISPLAY;

    array<Thread^>^ ThreadList;
    array<ThreadProperties^>^ ThreadPropertiesList;
    array<Stopwatch^>^ StopwatchList;

};
