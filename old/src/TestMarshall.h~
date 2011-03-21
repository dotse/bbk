#pragma once
#include "main.h"

#include <string>
using namespace std;

#include "batch.h"
#include "executor.h"
#include "tp_test.h"
#include "tcp_ping.h"
#include "icmp_ping.h"
#include "tpengine.h"
#include "tpclient.h"
#include "tpcommon.h" 
#include "icmp_ping.h"
#include "curl_get.h"

enum enumTestType { STANDARD, AVAILABILITY, RTT, THROUGHPUT, TPTEST, FULL_TPTEST };

class TestMarshall
{
public:
	TestMarshall(void);
	~TestMarshall(void);
	static TestMarshall* GetInstance(void);

	bool SetupStandardTest(void);
	bool SetupAvailabilityTest(ServerList *slist);
	bool SetupRTTTest(ServerList *slist);
	bool SetupThroughputest(ServerList *slist);
	bool SetupFullTPTest();
	bool TestMarshall::SetupTPTest( wxString &Host, 
								wxString &Port,
								wxString &Timeout,
								wxString &Direction,
								wxString &TCPBytes,
								wxString &UDPPPS,
								wxString &UDPPSize,
								wxString &UDPRate,
								wxString &UDPTestTime );

	bool RecalculateUDPFields( unsigned int rate, unsigned int &pps, unsigned int &psize );

	bool InitTestEngine( int TestType );
	bool DeinitTestEngine(bool abort = false);
	bool Execute(void);

	int		GetProgress(void);
	const char *	GetProgressString(void);
	bool		GetRunningState(void);
	bool		GetCompletedStatus(void);
	int		GetLastError(void);
	int		GetTestType(void);

	TPEngine*	GetDownstreamTPEngineStruct(int index = 0);
	TPEngine*	GetUpstreamTPEngineStruct(int index = 0);
	int		GetDownstreamTPEngineCount(void);
	int		GetUpstreamTPEngineCount(void);
	TPEngine*	GetTPEngineStruct(int index = 0);
	curl_get_arg_struct*	GetCurlHTTPStruct();
	curl_get_arg_struct*	GetCurlFTPStruct();
	int			GetTPEngineCount(void);
	tcp_ping_arg_struct*	GetTCPPingStruct(void);
	icmp_ping_arg_struct*	GetICMPPingStruct(void);

private:
	struct TPEngine			**m_engp_downstream;
	struct TPEngine			**m_engp_upstream;
	struct thread_arg_struct	**m_engp_downstream_ta; // thread arg for batched tcp ping
	struct thread_arg_struct	**m_engp_upstream_ta; // thread arg for batched icmp ping
	int							m_count_downstream_engp;
	int							m_count_upstream_engp;


	struct tcp_ping_arg_struct	*m_tp;
	struct icmp_ping_arg_struct	*m_ip;
	struct thread_arg_struct	*m_tp_ta; // thread arg for batched tcp ping
	struct thread_arg_struct	*m_ip_ta; // thread arg for batched icmp ping

	struct curl_get_arg_struct	*m_http;
	struct curl_get_arg_struct	*m_ftp;
	struct thread_arg_struct	*m_http_ta;
	struct thread_arg_struct	*m_ftp_ta;

	struct batch_arg_struct		*m_batch;

	struct thread_arg_struct	*m_ta; // the main thread arg struct

	string						m_strLastError;
	int							m_iTestType;

	// for availability tests
	ServerList					*m_TCP_PingList;
	ServerList					*m_ICMP_PingList;	

	ServerList					*m_TPTestList;
	ServerList					*m_HTTPList;
	ServerList					*m_FTPList;

#ifdef UNIX
	pthread_t	m_thread;
	void* (*m_test_executor)(void*);
#endif
#ifdef WIN32
	HANDLE		m_thread;
	DWORD		m_threadid;
	LPTHREAD_START_ROUTINE m_test_executor;
#endif 

	static TestMarshall	*s_instance;

};
