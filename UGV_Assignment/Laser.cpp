#include "Laser.h"

Laser::Laser(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser)
{

}

error_state Laser::setupSharedMemory()
{
    return error_state();
}

void Laser::threadFunction()
{
    throw gcnew System::NotImplementedException();
}

error_state Laser::processHeartbeats()
{
    return error_state();
}

void Laser::shutdownThreads()
{
    throw gcnew System::NotImplementedException();
}

bool Laser::getShutdownFlag()
{
    return false;
}

error_state Laser::communicate()
{
    return error_state();
}

error_state Laser::checkData()
{
    return error_state();
}

error_state Laser::processSharedMemory()
{
    return error_state();
}

error_state Laser::connect(String^ hostName, int portNumber)
{
    return error_state();
}
