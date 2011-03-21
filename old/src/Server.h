#pragma once

#include <wx/string.h>

class Server
{
public:
	Server(void);
	~Server(void);

	wxString	Host;
	wxString	TCP_Port;
	wxString	TPTest_Port;

	wxString	HTTP_Path;
	wxString	FTP_Path;

	bool IsUserCreated;

	bool EnableAvailability;
	bool EnableAvailabilityTCP;
	bool EnableAvailabilityICMP;
	bool EnableRTTPacketloss;
	bool EnableThroughput;
	bool EnableThroughputTPTest;
	bool EnableThroughputHTTP;
	bool EnableThroughputFTP;

	bool IsICMP;

};
