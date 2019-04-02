#pragma once

#include <G3D/G3D.h>
//#include <atlstr.h>
#include <string>

class CComPortDriver
{
public:
    CComPortDriver();
    ~CComPortDriver();

public:
    bool	Open(const int port_no, DWORD &err);
	bool	Close(DWORD &err);
	bool	SetDtr();
	bool	ClearDtr();
	bool	SetRts();
	bool	ClearRts();
	bool	GetStatus(DWORD &status); // get the status of 4 input lines
	bool	IsOpen(); // check if port is open
    int		GetPortNumber(); // get port number

protected:
    int		m_PortNumber;		// port number
	bool	m_IsOpen;			// is comms port open
    HANDLE	m_hCom;				// a handle to the comms port
};