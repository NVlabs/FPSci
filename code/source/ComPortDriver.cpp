// Description : Com port driver that allows to control an output directly
// from the PC's RS232 serial comms port
//
// This works both on "real" RS232 comms ports and USB to comms port adapters, which is important
// since not many PCs these days have a native RS232 port.
//
// We do not use comm port TX and RX line as we are not doing any communications through it.

#include "ComPortDriver.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
// Ctor
//
CComPortDriver::CComPortDriver()
{
    m_PortNumber = 0;
    m_IsOpen = FALSE;
    m_hCom = NULL;
}

//
// Dtor. Closes the port automatically if the CComPortDriver object goes out of scope
//
CComPortDriver::~CComPortDriver()
{
    DWORD err;
    if (m_IsOpen)
    {
        Close(err);
    }
}

//
// Description  : Open a comms port given port number (1-100).
//					Returns an error number by reference.
//					Returns TRUE if successful, or FALSE otherwise
//
bool CComPortDriver::Open(const int port_no, DWORD &dwError)
{
    DCB dcb;
    dwError = ERROR_SUCCESS;
    bool fSuccess;
    //CString port = "", str;
    std::string portStr = "";
    HANDLE hCom;	// temp handle

    //
    // Allow port number in 1 - 100 range
    //
    if ((port_no < 1) || (port_no > 100))
    {
        return FALSE;
    }

    //
    // If already open, return FALSE
    //
    if (m_IsOpen)
    {
        return FALSE;
    }

    // sanity check
    _ASSERT(m_hCom == NULL);

    //// simple port.Format("COM%d",m_CommPort); works only up to port 9
    //// see http://support.microsoft.com/default.aspx?scid=kb;EN-US;q115831
    //port.Format(L"\\\\.\\COM%d", port_no);
    //portStr = "\\\\.\\COM%d" + port_no;
    //portStr = "COM%d" + port_no;
    //portStr = L"\\\\.\\COM2";

    //
    // Open port using CreateFile function
    //
    hCom = CreateFile("\\\\.\\COM2",
        GENERIC_READ | GENERIC_WRITE,
        0,    /* comm devices must be opened w/exclusive-access */
        NULL, /* no security attrs */
        OPEN_EXISTING, /* comm devices must use OPEN_EXISTING */
        0,    /* not overlapped I/O */
        NULL  /* hTemplate must be NULL for comm devices */
        );


    ////hCom = CreateFile((LPCWSTR)portStr.c_str(),
    //hCom = CreateFile(L"\\\\.\\COM2",
    //    GENERIC_READ | GENERIC_WRITE,
    //    0,    /* comm devices must be opened w/exclusive-access */
    //    NULL, /* no security attrs */
    //    OPEN_EXISTING, /* comm devices must use OPEN_EXISTING */
    //    0,    /* not overlapped I/O */
    //    NULL  /* hTemplate must be NULL for comm devices */
    //    );

    //
    // Get last error
    //
    if (hCom == INVALID_HANDLE_VALUE)
    {
        dwError = GetLastError();
        return FALSE;
    }

    //
    // Get the current configuration.
    //
    fSuccess = GetCommState(hCom, &dcb);
    if (!fSuccess)
    {
        dwError = GetLastError();
        return FALSE;
    }

    //
    // use some common settings to call set Comms State. Otherwise modem control line
    // will not work
    //
    dcb.fOutxCtsFlow = TRUE;
    dcb.fOutxDsrFlow = TRUE;
    dcb.fDsrSensitivity = TRUE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;	// important
    dcb.fRtsControl = RTS_CONTROL_ENABLE;	// important

    //
    // these setting that control serial communications mode hardly matter, 
    // but still set to some reasonable values
    //
    dcb.BaudRate = 9600;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    // set comm port settings here
    fSuccess = SetCommState(hCom, &dcb);

    //
    // Check if failed
    //
    if (!fSuccess)
    {
        dwError = GetLastError();
        return FALSE;
    }

    //
    // This is probably unnecessary but copied from working example just in case
    //
    if (!PurgeComm(hCom, PURGE_TXCLEAR | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_RXABORT))
    {
        dwError = GetLastError();
        return FALSE;
    }

    //
    // Set comms timeouts, again, probably not necessary but doesn't hurt
    //
    bool result;
    COMMTIMEOUTS ctmo;
    ctmo.ReadIntervalTimeout = 1000;
    ctmo.ReadTotalTimeoutMultiplier = 0;
    ctmo.ReadTotalTimeoutConstant = 10000;
    ctmo.WriteTotalTimeoutMultiplier = 0;
    ctmo.WriteTotalTimeoutConstant = 10000;
    result = SetCommTimeouts(hCom, &ctmo);

    //
    // Check if failed
    //
    if (!result)
    {
        dwError = GetLastError();
        return FALSE;
    }

    //
    // Now set member variables to correct values
    //
    m_IsOpen = TRUE;
    m_hCom = hCom;
    m_PortNumber = port_no;

    return TRUE;
}

//
// Description  : Close comm port. Returns (by reference) an error code returned by
//					GetLastError if failed.
//					Returns TRUE if successful, and FALSE if not (including if port 
//					is not already open).
//
bool CComPortDriver::Close(DWORD &dwError)
{
    dwError = ERROR_SUCCESS;

    //
    // Need to be open and have a valid handle before trying to close the port
    //
    if (m_IsOpen && m_hCom)
    {
        if (!CloseHandle(m_hCom))
        {
            dwError = GetLastError();
            return FALSE;
        }
        m_IsOpen = FALSE;
        m_hCom = NULL;
        return TRUE;
    }
    return FALSE;
}

//
// Function		: SetDtr
// Description  : Set DTR line on comm port
//
bool CComPortDriver::SetDtr()
{
    if (m_IsOpen)
    {
        _ASSERT(m_hCom);
        DWORD status = SETDTR;
        return EscapeCommFunction(m_hCom, status);
    }
    return FALSE;
}

//
// Function		: ClearDtr
// Description  : Clear DTR line on comm port
//
bool CComPortDriver::ClearDtr()
{
    if (m_IsOpen)
    {
        _ASSERT(m_hCom);
        DWORD status = CLRDTR;
        return EscapeCommFunction(m_hCom, status);
    }
    return FALSE;
}

//
// Function		: SetRTS
// Description  : Set RTS line on comm port
//
bool CComPortDriver::SetRts()
{
    if (m_IsOpen)
    {
        _ASSERT(m_hCom);
        DWORD status = SETRTS;
        return EscapeCommFunction(m_hCom, status);
    }
    return FALSE;
}

//
// Function		: ClearRts
// Description  : Clear RTS line on comm port
//
bool CComPortDriver::ClearRts()
{
    if (m_IsOpen)
    {
        _ASSERT(m_hCom);
        DWORD status = CLRRTS;
        return EscapeCommFunction(m_hCom, status);
    }
    return FALSE;
}

//
// Function		: GetStatus
// Description  :  Returns the status of input lines of the comms port.
//					Not used and hence not fully tested.
//
bool CComPortDriver::GetStatus(DWORD &status)
{
    if (m_IsOpen)
    {
        _ASSERT(m_hCom);
        return GetCommModemStatus(m_hCom, &status);
    }

    return FALSE;
}

//
// Function		: IsOpen
// Description  : A helper function that returns the status of comms port (TRUE for open)
//
bool CComPortDriver::IsOpen()
{
    return m_IsOpen;
}

//
// Function		: GetPortNumber
// Description  : Gets the port number of the currently open port
//
int CComPortDriver::GetPortNumber()
{
    return m_PortNumber;
}