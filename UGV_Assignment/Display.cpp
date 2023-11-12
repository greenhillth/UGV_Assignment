#include "Display.h"

#define DISPLAY_PORT  28000


Display::Display(SM_ThreadManagement^ SM_TM, SM_Display^ SM_DISPLAY) : SM_DISPLAY(SM_DISPLAY),
    NetworkedModule(SM_TM, SM_DISPLAY, gcnew String(DISPLAY_ADDRESS), DISPLAY_PORT)
{
    cli = gcnew cliInterface(SM_TM, SM_DISPLAY);
    SM_DISPLAY->connectionHandles[1] = Client;
}

void Display::sendDisplayData(array<double>^ xData, array<double>^ yData, NetworkStream^ stream) {
    // Serialize the data arrays to a byte array
    //(format required for sending)
    array<Byte>^ dataX =
        gcnew array<Byte>(SM_DISPLAY->LaserData->x->Length * sizeof(double));
    Buffer::BlockCopy(SM_DISPLAY->LaserData->x, 0, dataX, 0, dataX->Length);
    array<Byte>^ dataY =
        gcnew array<Byte>(SM_DISPLAY->LaserData->y->Length * sizeof(double));
    Buffer::BlockCopy(SM_DISPLAY->LaserData->y, 0, dataY, 0, dataY->Length);
    // Send byte data over connection
    Stream->Write(dataX, 0, dataX->Length);
    Thread::Sleep(10);
    Stream->Write(dataY, 0, dataY->Length);
}

error_state Display::connect(String^ hostName, int portNumber)
{
    try
    {
        Client = gcnew TcpClient(hostName, portNumber);
    }
    catch (System::Net::Sockets::SocketException^ e)
    {
        timeout->Start();
        return ERR_CONNECTION;
    }
    SM_DISPLAY->connectionStatus[1]->Start();
    SM_DISPLAY->connectionHandles[1] = Client;
    Stream = Client->GetStream();

    Client->NoDelay = true;
    Client->ReceiveTimeout = 500;
    Client->SendTimeout = 500;
    Client->ReceiveBufferSize = 1024;
    Client->SendBufferSize = 1024;

    ReadData = gcnew array<unsigned char>(128);
    SendData = gcnew array<unsigned char>(128);

    return SUCCESS;
}

error_state Display::connectionReattempt() {
    
    if (timeout->Elapsed.Seconds > 10) {
        timeout->Reset();
        connectionAttempts++;
        return connect(DNS, PORT);
    }
    return ERR_CONNECTION;
}

error_state Display::communicate()
{
    sendDisplayData(SM_DISPLAY->LaserData->x, SM_DISPLAY->LaserData->y, Stream);

    return SUCCESS;
}

error_state Display::processSharedMemory()
{
    
    return SUCCESS;
}

error_state Display::processHeartbeats()
{
    if ((SM_TM->heartbeat & bit_DISPLAY) == 0)
    {
        SM_TM->heartbeat |= bit_DISPLAY;	// put laser flag up
        Watch->Restart();					// Restart stopwatch
    }
    else
    {
        if (Watch->ElapsedMilliseconds > CRASH_LIMIT_MS) {

            shutdownThreads();
            return ERR_TMS_TIMEOUT;
        }
    }
    return SUCCESS;
}

void Display::shutdownThreads()
{
    SM_TM->shutdown = 0xFF;
}

bool Display::getShutdownFlag() { return SM_TM->shutdown & bit_DISPLAY; }


void Display::threadFunction()
{
    Console::WriteLine("Display thread is starting");
    Watch = gcnew Stopwatch;    //TODO - init stopwatches in constructor
    cli->init(NETWORK);
    
    auto connection = connect(DNS, PORT);
    SM_TM->ThreadBarrier->SignalAndWait();
    Watch->Start();
    while (!getShutdownFlag()) {
        processHeartbeats();
        if (Console::KeyAvailable) { 
            pressedKey = Console::ReadKey(true).Key;
        }
        if (connection == SUCCESS) { communicate(); }
        cli->update();
        if (pressedKey != ConsoleKey::Clear) { processKey(); }
    }
    Console::WriteLine("Display thread is terminating");
}

void Display::processKey() {
    switch (pressedKey) {
    case ConsoleKey::M:
        cli->changeWindow(MAIN);
        break;
    case ConsoleKey::N:
        cli->changeWindow(NETWORK);
        break;
    case ConsoleKey::Q:
        shutdownThreads();
        break;
    case ConsoleKey::R:
        connectionReattempt();
        cli->forceRefresh();
        break;
    }
    pressedKey = ConsoleKey::Clear;
}

cliInterface::cliInterface(SM_ThreadManagement^ ThreadInfo, SM_Display^ displayData)
    : ThreadInfo(ThreadInfo), displayData(displayData), activeWindow(NETWORK), reinitialise(false)
{
    elemPositions = gcnew array<uint8_t, 3>(7, 6, 2) {
        { { 8, 6 }, { 26,6 }, { 44,6 }, { 62,6 }, { 80,6 }, { 98,6 } },   // Thread co-ords
        { { 28, 22 }, { 55, 22 } },                                         // GPS co-ord
        { { 50, 13 } },                                                   // CMD co-ord
        { { 20, 18 }, { 20, 19 }, { 20, 20 }, { 4, 21 } },               // Controller co-ords
        { { 98, 17 }, { 100, 18 }, { 97, 19 }, { 95, 20 }, { 103, 21} },              // Connection co-ords
        { { 28, 7 }, { 28, 9 }, { 28, 11 }, { 28, 13 }, { 28, 15 } },                                                                 //Network co-ords
        {}                                                                  //GPS Log co-ords
    };
}

void cliInterface::init(window selectedWindow)
{
    //TODO - set appropriate buffer and window sizes to make display more robust
    Console::CursorVisible = false;
    Console::Clear();

    switch (selectedWindow) {
    case MAIN:
    Console::WriteLine(
        "=======================================================================================================================\n" +
        "                                                UGV DISPLAY WINDOW v2.00                                               \n" +
        "=======================================================================================================================\n" +
        "                                                    THREAD STATUS                                                      \n" +
        "        Thread #1         Thread #2         Thread #3         Thread #4         Thread #5         Thread #6            \n" +
        "            TMM              Laser             GNSS           Controller      Vehicle Control      Display             \n" +
        "        Initialising      Initialising      Initialising      Initialising      Initialising      Initialising         \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                     LAST COMMAND SENT:                                                \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                ||                                                     \n" +
        "    Controller Inputs:                              ____________||__                      Connection Status:           \n" +
        "    [button] : [value]                           [=|   WEEDER      |]                     Laser - Connected            \n" +
        "    left trigger  :                               ~_|_____________ |                      Display - Connected          \n" +
        "    right trigger :                                 //||      || ||                       GNSS - Connected             \n" +
        "    right stick   :                                (_)(_)    (_)(_)                       VC - Connected               \n" +
        "                                                                                          Controller - Connected       \n" +
        "                       CRC:                    Coords:  x,y,z                        Uptime:                           \n" +
        "=======================================================================================================================\n" +
        "=======================================================================================================================\n" +
        "(press Q to quit, or N to switch to Networking Menu)");
    break;
    case NETWORK:
    Console::WriteLine(
        "=======================================================================================================================\n" +
        "                                                UGV DISPLAY WINDOW v2.00                                               \n" +
        "=======================================================================================================================\n" +
        "                                                     NETWORK INFO                                                      \n" +
        "                                                                                                                       \n" +
        "        Process             Connection Status       Local IP Address             Remote IP Address        Uptime       \n" +
        "                                                                                                                       \n" +
        "        Laser                                                                                                          \n" +
        "                                                                                                                       \n" +
        "        Display                                                                                                        \n" +
        "                                                                                                                       \n" +
        "        GNSS                                                                                                           \n" +
        "                                                                                                                       \n" +
        "        VC                                                                                                             \n" +
        "                                                                                                                       \n" +
        "        Controller                                                                                                     \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        " Press R to reattempt connection                                                                                       \n" +
        "=======================================================================================================================\n" +
        "=======================================================================================================================\n" +
        "(press Q to quit, or M to switch to Main Menu)");
    break;
    
    case GPSLOGS:
    Console::WriteLine(
        "=======================================================================================================================\n" +
        "                                                UGV DISPLAY WINDOW v2.00                                               \n" +
        "=======================================================================================================================\n" +
        "                                                         GPS LOGS                                                      \n" +
        "                                                                                                                       \n" +
        "      Timestamp           Northing            Easting            Heading                               CRC             \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "                                                                                                                       \n" +
        "=======================================================================================================================\n" +
        "=======================================================================================================================\n" +
        "(press Q to quit)");
    break;
    }
}

void cliInterface::update()
{
    if(activeWindow != requestedWindow) {
        init(requestedWindow);
        activeWindow = requestedWindow;
        reinitialise = true;
    }
    switch (activeWindow) {
    case MAIN:
        updateThreadStatus();
        updateGPS();
        updateCMD();
        updateController();
        updateConnectionStatus();
        updateUptime();
        break;
    case NETWORK:
        updateNetwork();
        break;
    case GPSLOGS:
        updateGPSLogs();
        break;
    };
}

void cliInterface::updateUptime() {
    TimeSpan time = displayData->uptime->Elapsed;
    Console::SetCursorPosition(93, 22);
    Console::Write("{0:00}:{1:00}:{2:00}.{3:000}       ", time.Hours, time.Minutes, time.Seconds, time.Milliseconds);
}

void cliInterface::forceRefresh()
{
    reinitialise = true;
}

void cliInterface::updateThreadStatus()
{
    auto green = ConsoleColor::Green;
    auto red = ConsoleColor::Red;
    for (int i = 0; i < 6; i++) {
        auto status = displayData->threadStatus[i];
        Console::SetCursorPosition(elemPositions[THREAD, i, 0], elemPositions[THREAD, i, 1]);       //set cursor position
        if (status == Threading::ThreadState::Running) { Console::ForegroundColor = green; Console::Write("  Running   "); }                    //write 
        else if (status == Threading::ThreadState::Stopped) { Console::ForegroundColor = red; Console::Write("  Stopped   "); }
        else if (status == Threading::ThreadState::Suspended) { Console::Write(" Suspended  "); }
        else if (status == Threading::ThreadState::Background) { Console::Write("Background  "); }
        else if (status == Threading::ThreadState::WaitSleepJoin) { Console::Write("  Waiting   "); }
        else { Console::Write("doin stuff"); }
        Console::ResetColor();
    }
}

void cliInterface::updateGPS()
{
    auto data = displayData->GPSData;
    Console::SetCursorPosition(elemPositions[GPS, 0, 0], elemPositions[GPS, 0, 1]);       //set cursor position
    Console::Write("{0:X} ", data->CRC);
    
    Console::SetCursorPosition(elemPositions[GPS, 1, 0], elemPositions[GPS, 1, 1]);       //set cursor position
    Console::Write("{0:N}, {1:N}, {2:N}    ",data->Northing, data->Easting, data->Height);
}

void cliInterface::updateCMD()
{
    if (displayData->sentCommand != nullptr) {
        Console::SetCursorPosition(elemPositions[CMD, 0, 0], elemPositions[CMD, 0, 1]);       //set cursor position
        Console::Write("   "+ displayData->sentCommand + "   ");
    }
}

void cliInterface::updateController()
{   
    auto inputs = displayData->Controller->GetState();
    if (displayData->Controller != nullptr) {
        double controllerVals[3] = { inputs.leftTrigger, inputs.rightTrigger, inputs.rightThumbX};
        bool aButton = inputs.buttonA;
        for (int i = 0; i < 3; i++) {
            Console::SetCursorPosition(elemPositions[CONTROLLER, i, 0], elemPositions[CONTROLLER, i, 1]);       //set cursor position
            Console::Write("{0:N}      ", controllerVals[i]);
        }
        Console::SetCursorPosition(elemPositions[CONTROLLER, 3, 0], elemPositions[CONTROLLER, 3, 1]);
        if (aButton) {Console::Write("Button A pressed");}
        else { Console::Write("                "); }
    }
    
}

void cliInterface::updateConnectionStatus()
{
    auto green = ConsoleColor::Green;
    auto red = ConsoleColor::Red;
    auto status = displayData->connectionStatus;
    for (int i = 0; i < status->Length; i++) {
        Console::SetCursorPosition(elemPositions[CONNECTION, i, 0], elemPositions[CONNECTION, i, 1]);
        if (status[i]->IsRunning) { Console::ForegroundColor = green; Console::Write("Connected   "); }
        else { Console::ForegroundColor = red; Console::Write("Disconnected"); }
    }
    Console::ResetColor();
}

void cliInterface::updateNetwork() {
    auto handles = displayData->connectionHandles;

    for (int i = 0; i < 4; i++) {
        Console::SetCursorPosition(elemPositions[5, i, 0], elemPositions[5, i, 1]);
        if (displayData->connectionStatus[i]->IsRunning) {
            if (reinitialise)
            {
                Console::ForegroundColor = ConsoleColor::Green;
                Console::Write("Connected               {0}              {1}",
                    getLocalIPAddress(handles[i]->Client), getRemoteIPAddress(handles[i]->Client));
            }
            Console::CursorLeft = 106;
            Console::Write("{0:00}:{1:00}:{2:00}", displayData->connectionStatus[i]->Elapsed.Hours, displayData->connectionStatus[i]->Elapsed.Minutes, displayData->connectionStatus[i]->Elapsed.Seconds);
        }
        else if ((handles[i]->Connected)) {
            Console::ForegroundColor = ConsoleColor::Green;
            Console::Write("Connected               {0}              {1}",
                 getLocalIPAddress(handles[i]->Client), getRemoteIPAddress(handles[i]->Client));
        }
        else {
            Console::ForegroundColor = ConsoleColor::Red;
            Console::Write("Disconnected                 N/A                           N/A                                ");
           
        }
        Console::ResetColor();
    }
    Console::SetCursorPosition(elemPositions[5, 4, 0], elemPositions[5, 4, 1]);
    if (displayData->connectionStatus[4]->IsRunning) {
        auto time = displayData->connectionStatus[4]->Elapsed;
        Console::ForegroundColor = ConsoleColor::Green;
        Console::Write("Connected");
        Console::ResetColor();
        Console::Write("                    N/A                           N/A       "
            +"         {0:00}:{1:00}:{2:00}", time.Hours, time.Minutes, time.Seconds);
    }
    else {
        Console::ForegroundColor = ConsoleColor::Red;
        Console::Write("Disconnected                                                                                 ");
        Console::ResetColor();
    }
    reinitialise = false;
    // add controller data

        
}

void cliInterface::updateGPSLogs() {

}

String^ getRemoteIPAddress(Socket^ s) {
    return (((IPEndPoint^)(s->RemoteEndPoint))->Address)->ToString() + ":" + ((IPEndPoint^)(s->RemoteEndPoint))->Port.ToString();
}

String^ getLocalIPAddress(Socket^ s) {
    return (((IPEndPoint^)(s->LocalEndPoint))->Address)->ToString() + ":" + ((IPEndPoint^)(s->LocalEndPoint))->Port.ToString();
}