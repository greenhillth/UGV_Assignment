#pragma once

#include <UGVModule.h>

ref class ThreadManagement : public UGVModule {
public:
    // Create shared memory objects
    error_state setupSharedMemory();

    // Send/Recieve data from shared memory structures
    error_state processSharedMemory() override;

    void shutdownModules();

    // Get Shutdown signal for module, from Thread Management SM
    bool getShutdownFlag() override;

    // Thread function for PMM
    void threadFunction() override;

private:
    // Add any additional data members or helper functions here
};
