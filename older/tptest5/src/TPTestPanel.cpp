#include "TPTestPanel.h"

#include "Downloader.h"

enum { wxID_MS_URL = wxID_HIGHEST + 1, 
       wxID_BUTTON_REFRESH, 
       wxID_TPTEST3_SERVERLIST,
       wxID_TPTEST,
       wxID_TPTEST_BUTTON_REFRESH_SERVERLIST,
       wxID_TPTEST_BUTTON_START_TCP,
       wxID_TPTEST_BUTTON_START_UDP,
       wxID_SERVER_HOST, 
       wxID_SERVER_PORT, 
       wxID_DIRECTION, 
       wxID_TCPBYTES,
       wxID_TCPTIMEOUT,
       wxID_UDPPPS,
       wxID_UDPSIZE,
       wxID_UDPRATE,
       wxID_UDPTESTTIME,
       wxID_TPTEST_OUTPUT 
 };

BEGIN_EVENT_TABLE( TPTestPanel, wxPanel )
	EVT_BUTTON(wxID_BUTTON_REFRESH, TPTestPanel::UpdateServerList)
	EVT_BUTTON(wxID_TPTEST_BUTTON_START_TCP, TPTestPanel::OnStartTCPTest)
	EVT_BUTTON(wxID_TPTEST_BUTTON_START_UDP, TPTestPanel::OnStartUDPTest)
	EVT_RADIOBOX(wxID_DIRECTION, TPTestPanel::OnDirectionBox)
	EVT_LIST_ITEM_SELECTED(wxID_TPTEST3_SERVERLIST, TPTestPanel::OnItemSelected)


	EVT_TEXT(wxID_UDPPPS, TPTestPanel::RecalculateUDPFields)
	EVT_TEXT(wxID_UDPSIZE, TPTestPanel::RecalculateUDPFields)
	EVT_TEXT(wxID_UDPRATE, TPTestPanel::RecalculateUDPFields)
	EVT_TEXT(wxID_UDPTESTTIME, TPTestPanel::RecalculateUDPFields)

END_EVENT_TABLE()

TPTestPanel::TPTestPanel(wxWindow *parent)
:TestPanel( parent, wxID_TPTEST )
{
	m_SkipUDPTextEvent = false;

	delete m_StartButton;
	m_StartButton = NULL;

	AppConfig *conf = AppConfig::GetInstance();
	wxString strMSHost;
	conf->GetValue( wxString( wxT("TPTEST3_SERVERLIST_URL") ), strMSHost );
	// The sizers
	m_SizerMain	      = new wxGridBagSizer( 0, 0 );

	m_StaticSizerTCPTest  = new wxStaticBoxSizer( wxVERTICAL, 
						      this, 
						      wxT("TCP Test") );

	m_StaticSizerUDPTest   = new wxStaticBoxSizer( wxVERTICAL, 
						       this,
						       wxT("UDP Test") );

	m_SizerGlobal		= new wxGridBagSizer( 5, 5 );
	m_SizerTCPTest		= new wxGridBagSizer( 5, 5 );
	m_SizerUDPTest		= new wxGridBagSizer( 5, 5 );
	// The controls

	// MS Host text
	m_SizerGlobal->Add( new wxStaticText( this, 
					      wxID_ANY, 
					      wxT("Serverlista URL:") ),
			    wxGBPosition( 0, 0 ),
			    wxGBSpan(1, 1),
			    wxALIGN_CENTER_VERTICAL |
			    wxALIGN_RIGHT |
			    wxTOP,
			    5 );


	// MS Host
	m_SizerGlobal->Add( new wxTextCtrl( this, 
					    wxID_MS_URL, 
					    strMSHost,
					    wxDefaultPosition,
					    wxSize( 300, 20) ),
			    wxGBPosition( 0, 1 ),
			    wxGBSpan( 1, 2 ),
			    wxALIGN_CENTER_VERTICAL |
			    wxALIGN_LEFT |
			    wxTOP,
			    5 );

	// MS Refresh Button
	wxButton *BtnRefresh = new wxButton( this, 
					     wxID_BUTTON_REFRESH, 
					     wxT("Uppdatera"),
					     wxDefaultPosition,
					     wxSize( 100, 20) );

	BtnRefresh->SetToolTip( wxT("Uppdatera TPTEST serverlistan") );

	m_SizerGlobal->Add( BtnRefresh,
			    wxGBPosition( 1, 0 ),
			    wxGBSpan( 1, 3 ),
			    wxALIGN_CENTER_VERTICAL |
			    wxALIGN_CENTER );
	
	// Server text
	m_SizerGlobal->Add(	new wxStaticText( this, 
						  wxID_ANY, 
						  wxT("Server:") ),
				wxGBPosition( 2, 0 ),
				wxGBSpan(1, 1),
				wxALIGN_CENTER_VERTICAL |
				wxALIGN_RIGHT );
	

	// Server list ctrl
	m_ServerListCtrl = new wxListCtrl( this, 
					   wxID_TPTEST3_SERVERLIST, 
					   wxDefaultPosition,
					   wxSize( 300, 80),
					   wxLC_REPORT | wxLC_SINGLE_SEL );
	
	m_ServerListCtrl->InsertColumn(0, 
				       wxT("Adress"), 
				       wxLIST_FORMAT_LEFT, 
				       190);

	m_ServerListCtrl->InsertColumn(1, 
				       wxT("Port"), 
				       wxLIST_FORMAT_LEFT, 
				       45);
	
	m_SizerGlobal->Add( m_ServerListCtrl,
			    wxGBPosition( 2, 1 ),
			    wxGBSpan( 1, 2 ),
			    wxALIGN_CENTER_VERTICAL |
			    wxALIGN_LEFT);
	
	// Server Address text
	m_SizerGlobal->Add( new wxStaticText( this, 
					      wxID_ANY, 
					      wxT("Adress:") ),
			    wxGBPosition( 3, 0 ),
			    wxGBSpan(1, 1),
			    wxALIGN_CENTER_VERTICAL |
			    wxALIGN_RIGHT );
	
	// Server Address textbox
	this->m_TPTEST3_ServerURL = new wxTextCtrl( this, 
						    wxID_SERVER_HOST, 
						    wxT(""),
						    wxDefaultPosition,
						    wxSize( 165, 20) );

	m_SizerGlobal->Add( m_TPTEST3_ServerURL,
			    wxGBPosition( 3, 1 ),
			    wxGBSpan(1, 1),
			    wxALIGN_CENTER_VERTICAL |
			    wxALIGN_LEFT );

	// Server Port
	this->m_TPTEST3_ServerPort = new wxTextCtrl( this, 
						     wxID_SERVER_PORT, 
						     wxT(""),
						     wxDefaultPosition,
						     wxSize( 50, 20) );
	
	m_SizerGlobal->Add( m_TPTEST3_ServerPort,
			    wxGBPosition( 3, 2 ),
			    wxGBSpan(1, 1),
			    wxALIGN_CENTER_VERTICAL |
			    wxALIGN_LEFT );



// Test type radio
	
	m_SizerGlobal->Add( new wxStaticText( this, 
					    wxID_ANY, 
					    wxT("Testtyp:") ),
			  wxGBPosition( 4, 0 ),
			  wxGBSpan(1, 1),
			  wxALIGN_CENTER_VERTICAL |
			  wxALIGN_RIGHT );

	// Direction radio

    wxString choices[] =
    {
        wxT("Nedströms"), 
	wxT("Uppströms"),
        wxT("Full duplex")
    };

	m_SelectedDirection = DIRECTION_DOWN;

	wxRadioBox *rType = new wxRadioBox( this,
					    wxID_DIRECTION,
					    wxT(""),
					    wxDefaultPosition,
					    wxDefaultSize,
					    WXSIZEOF(choices), 
					    choices, 
					    1, 
					    wxRA_SPECIFY_ROWS );
	
	m_SizerGlobal->Add( rType,
			  wxGBPosition( 4, 1 ),
			  wxGBSpan( 1, 2 ),
			  wxALIGN_CENTER_VERTICAL |
			  wxALIGN_LEFT ); 
	
	// TCP
	
	m_SizerTCPTest->Add( new wxStaticText( this, 
					       wxID_ANY, 
					       wxT("TCP bytes att överföra:") ),
			     wxGBPosition( 0, 0 ),
			     wxDefaultSpan,
			     wxALIGN_CENTER_VERTICAL |
			     wxALIGN_RIGHT  );
	
	m_SizerTCPTest->Add( new wxTextCtrl( this, 
					     wxID_TCPBYTES, 
					     wxT(""),
					     wxDefaultPosition,
					     wxSize( 100, 20) ),
			     wxGBPosition( 0, 1 ),
			     wxDefaultSpan,
			     wxALIGN_CENTER_VERTICAL,
			     wxALIGN_LEFT );


	// Testtid tex
	m_SizerTCPTest->Add( new wxStaticText( this, 
					       wxID_ANY, 
					       wxT("Timeout:") ),
			     wxGBPosition( 1, 0 ),
			     wxDefaultSpan,
			     wxALIGN_CENTER_VERTICAL |
			     wxALIGN_RIGHT  );

	// Testtid box
	m_SizerTCPTest->Add( new wxTextCtrl( this, 
					     wxID_TCPTIMEOUT, 
					     wxT(""),
					     wxDefaultPosition,
					     wxSize( 50, 20) ),
			     wxGBPosition( 1, 1 ),
			     wxDefaultSpan );

	m_SizerTCPTest->Add( new wxButton( this, 
					   wxID_TPTEST_BUTTON_START_TCP, 
					   wxT("Starta Test"),
					   wxDefaultPosition,
					   wxSize( 100, 20) ),
			     wxGBPosition( 1, 2 ),
			     wxDefaultSpan,
			     wxALIGN_CENTER_VERTICAL,
			     wxALIGN_LEFT );
	// UDP
	m_SizerUDPTest->Add( new wxStaticText( this, 
					       wxID_ANY, 
					       wxT("Paket per sekund:") ),
			     wxGBPosition( 0, 0 ),
			     wxDefaultSpan,
			     wxALIGN_CENTER_VERTICAL |
			     wxALIGN_RIGHT  );
	
	m_SizerUDPTest->Add(	new wxTextCtrl( this, 
						wxID_UDPPPS, 
						wxT(""),
						wxDefaultPosition,
						wxSize( 100, 20) ),
				wxGBPosition( 0, 1 ),
				wxDefaultSpan,
				wxALIGN_CENTER_VERTICAL,
				wxALIGN_LEFT );

	// UDP R2
	m_SizerUDPTest->Add(	new wxStaticText( this, 
						  wxID_ANY, 
						  wxT("Paketstorlek:") ),
				wxGBPosition( 1, 0 ),
				wxDefaultSpan,
				wxALIGN_CENTER_VERTICAL |
				wxALIGN_RIGHT  );

	m_SizerUDPTest->Add(	new wxTextCtrl( this, 
						wxID_UDPSIZE, 
						wxT(""),
						wxDefaultPosition,
						wxSize( 100, 20) ),
				wxGBPosition( 1, 1 ),
				wxDefaultSpan,
				wxALIGN_CENTER_VERTICAL,
				wxALIGN_LEFT );

	// UDP R3
	m_SizerUDPTest->Add( new wxStaticText( this, 
					       wxID_ANY, 
					       wxT("Datahastighet (bit/s):") ),
			     wxGBPosition( 2, 0 ),
			     wxDefaultSpan,
			     wxALIGN_CENTER_VERTICAL |
			     wxALIGN_RIGHT );

	m_SizerUDPTest->Add( new wxTextCtrl( this, 
					     wxID_UDPRATE, 
					     wxT(""),
					     wxDefaultPosition,
					     wxSize( 100, 20) ),
			     wxGBPosition( 2, 1 ),
			     wxDefaultSpan,
			     wxALIGN_CENTER_VERTICAL |
			     wxALIGN_LEFT );
	
	// UDP R4
	m_SizerUDPTest->Add( new wxStaticText( this, 
					       wxID_ANY, 
					       wxT("Testtid:") ),
			     wxGBPosition( 3, 0 ),
			     wxDefaultSpan,
			     wxALIGN_CENTER_VERTICAL |
			     wxALIGN_RIGHT );
	
	m_SizerUDPTest->Add( new wxTextCtrl( this, 
					     wxID_UDPTESTTIME, 
					     wxT(""),
					     wxDefaultPosition,
					     wxSize( 100, 20) ),
			     wxGBPosition( 3, 1 ),
			     wxDefaultSpan,
			     wxALIGN_CENTER_VERTICAL |
			     wxALIGN_LEFT );

	m_SizerUDPTest->Add(	new wxButton( this, 
					      wxID_TPTEST_BUTTON_START_UDP, 
					      wxT("Starta Test"),
					      wxDefaultPosition,
					      wxSize( 100, 20) ),
				wxGBPosition( 3, 2 ),
				wxDefaultSpan,
				wxALIGN_CENTER_VERTICAL |
				wxALIGN_LEFT );

	// Add TCP and UDP to Static sizer

	m_StaticSizerTCPTest->Add( m_SizerTCPTest );
	m_StaticSizerUDPTest->Add( m_SizerUDPTest );

	// Right text out
	m_TextOut = new wxTextCtrl( this,
				    wxID_TPTEST_OUTPUT,
				    wxString(wxT("")),
				    wxDefaultPosition,
				    wxSize(315,415),
				    wxTE_MULTILINE | 
				    wxTE_READONLY | 
				    wxTE_WORDWRAP );


	m_SizerMain->Add( m_SizerGlobal,
			  wxGBPosition(0,0),
			  wxGBSpan(1,1),
			  wxFIXED_MINSIZE );

	m_SizerMain->Add( m_TextOut,
			  wxGBPosition( 0, 1 ),
			  wxGBSpan( 4, 1 ),
			  wxALIGN_TOP |
			  wxALIGN_LEFT,
			  0); 

	m_SizerMain->Add( m_StaticSizerTCPTest,
			  wxGBPosition( 1, 0 ),
			  wxGBSpan( 1, 1 ),
			  wxFIXED_MINSIZE  );
	
	m_SizerMain->Add( m_StaticSizerUDPTest,
			  wxGBPosition( 2, 0 ),
			  wxGBSpan( 1, 1 ),
			  wxFIXED_MINSIZE  );
	
	


	this->SetSizer( m_SizerMain );

	//	m_SizerMain->SetSizeHints( this );
	
	//UpdateServerList( wxCommandEvent() );
}

TPTestPanel::~TPTestPanel(void)
{
}


void TPTestPanel::OnStartTCPTest(wxCommandEvent& event)
{
	TestMarshall *tm = TestMarshall::GetInstance();

	wxTextCtrl *host_ctrl    = (wxTextCtrl*)m_TPTEST3_ServerURL;
	wxTextCtrl *port_ctrl    = (wxTextCtrl*)m_TPTEST3_ServerPort;
	wxTextCtrl *timeout_ctrl = (wxTextCtrl*)
	  this->FindWindow( wxID_TCPTIMEOUT );

	wxTextCtrl  *tcpbytes_ctrl = (wxTextCtrl*)
	  this->FindWindow( wxID_TCPBYTES );

	wxString Host		= host_ctrl->GetValue();
	wxString Port		= port_ctrl->GetValue();
	wxString Timeout	= timeout_ctrl->GetValue();
	wxString Direction;

	if( m_SelectedDirection == DIRECTION_UP )
	  Direction	= wxT("UP");
	else if( m_SelectedDirection == DIRECTION_DOWN )
	  Direction	= wxT("DOWN");
	else if( m_SelectedDirection == DIRECTION_FD )
	{
	  wxMessageBox( wxT("Full duplex kan endast köras med UDP tester.") );
	  return;
	}
	else
	{
	  wxMessageBox( wxT("Du måste välja Uppströms eller nedströms") );
	  return;
	}

	wxString TCPBytes	= tcpbytes_ctrl->GetValue(); 

	wxString UDPPPS;
	wxString UDPPSize;
	wxString UDPRate;
	wxString UDPTestTime;

	if( !tm->InitTestEngine( TPTEST ) )
	  {
	    wxMessageBox( 
			 wxT("Ett fel har uppstått när TPTest försökte initiera testmotorn") );
	    return;
	  }

	if( !tm->SetupTPTest(
			     Host, 
			     Port,
			     Timeout,
			     Direction,
			     TCPBytes,
			     UDPPPS,
			     UDPPSize,
			     UDPRate,
			     UDPTestTime ) )
	  {
	    wxMessageBox( 
			 wxT("Ett fel har uppstått när TPTest ställde in parametrar inför testet") );
	    return;
	  }

	// SetupTPTest sets this mode depending on the input args
	m_SelectedMode = tm->GetTPEngineStruct()->tpMode;

	tm->Execute();
	MainFrame::GetInstance()->PreStartTest((void*)this, (TPTestPanel::OnTestCompleted_Wrapper), NULL );
	MainFrame::GetInstance()->StartTest();
}

void TPTestPanel::OnStartUDPTest(wxCommandEvent& event)
{
	TestMarshall *tm = TestMarshall::GetInstance();

	wxTextCtrl *host_ctrl	  = (wxTextCtrl*)m_TPTEST3_ServerURL;
	wxTextCtrl *port_ctrl	  = (wxTextCtrl*)m_TPTEST3_ServerPort;
	wxTextCtrl *timeout_ctrl  = (wxTextCtrl*)
	  this->FindWindow( wxID_TCPTIMEOUT );

	wxTextCtrl *pps_ctrl      = (wxTextCtrl*)
	  this->FindWindow( wxID_UDPPPS );
	wxTextCtrl *size_ctrl	  = (wxTextCtrl*)
	  this->FindWindow( wxID_UDPSIZE );
	wxTextCtrl *rate_ctrl	  = (wxTextCtrl*)
	  this->FindWindow( wxID_UDPRATE );
	wxTextCtrl *time_ctrl	  = (wxTextCtrl*)
	  this->FindWindow( wxID_UDPTESTTIME );

	wxString Host		= host_ctrl->GetValue();
	wxString Port		= port_ctrl->GetValue();
	wxString Timeout	= timeout_ctrl->GetValue();
	wxString Direction;

	if( m_SelectedDirection == DIRECTION_UP )
	  Direction	= wxT("UP");
	else if( m_SelectedDirection == DIRECTION_DOWN )
	  Direction	= wxT("DOWN");
	else if( m_SelectedDirection == DIRECTION_FD )
	  Direction	= wxT("FD");
	else
	{
	  wxMessageBox( wxT("Testtyp har inget värde") );
	  return;
	}

	wxString TCPBytes;

	wxString UDPPPS			= pps_ctrl->GetValue();
	wxString UDPPSize		= size_ctrl->GetValue();
	wxString UDPRate		= rate_ctrl->GetValue();
	wxString UDPTestTime	= time_ctrl->GetValue();

	if( !tm->InitTestEngine( TPTEST ) )
	{
	  wxMessageBox( wxT("Ett fel har uppstått när TPTest försökte initiera testmotorn") );
	  return;
	}

	long size, pps, time;
	UDPPPS.ToLong( &pps );
	UDPPSize.ToLong( &size );
	UDPTestTime.ToLong( &time );

	if( Host.Trim().Length() == 0 || Port.Trim().Length() == 0 )
	{
	  wxMessageBox( wxT("Du måste välja en server att utföra testet mot.") );
	  return;
	}
	else if( size <= 0 || pps <= 0 )
	{
	  wxMessageBox( wxT("Skriv in värden för Paketstorlek och Paket per sekund eller ange datahastighet.") );
	  return;
	}
	else if( time <= 0 || time > 30 )
	{
	  wxMessageBox( wxT("Du måste ange en testtid mellan 1 och 30 sekunder.") );
	return;
	}
	
	if( !tm->SetupTPTest(
			     Host, 
			     Port,
			     Timeout,
			     Direction,
			     TCPBytes,
			     UDPPPS,
			     UDPPSize,
			     UDPRate,
			     UDPTestTime ) )
	{
	  wxMessageBox( wxT("Testet kunde inte startas. Kontakta supporten för programmet") );
	  return;
	}

	// SetupTPTest sets this mode depending on the input args
	m_SelectedMode = tm->GetTPEngineStruct()->tpMode;

	tm->Execute();
	
	MainFrame::GetInstance()->PreStartTest((void*)this, (TPTestPanel::OnTestCompleted_Wrapper), NULL );
	MainFrame::GetInstance()->StartTest();
}

void TPTestPanel::OnTestCompleted_Wrapper(void* obj, bool arg)
{
	TPTestPanel *panel = (TPTestPanel*)obj;
	panel->OnTestCompleted( arg );
}

void TPTestPanel::OnTestCompleted(bool abort)
{
	TestMarshall *tm = TestMarshall::GetInstance();

	if( !abort )
	{
	  //		TPEngine *engp = tm->GetTPEngineStruct();
		ReportStats();
	}

	PostTest(abort);
	tm->DeinitTestEngine( abort );
}

void TPTestPanel::Report(char* out)
{
	wxTextCtrl *out_ctrl = (wxTextCtrl*)
	  this->FindWindow( wxID_TPTEST_OUTPUT );
	out_ctrl->AppendText( wxString( FROMCSTR(out) ) + wxT("\n") );
}

void TPTestPanel::ClearTextWindow(void)
{
	wxTextCtrl *out_ctrl = (wxTextCtrl*)this->FindWindow( wxID_TPTEST_OUTPUT );
	out_ctrl->Clear();
}

void TPTestPanel::ReportStats(void)
{
  TestMarshall *tm = TestMarshall::GetInstance();

  TPEngine *engp = tm->GetTPEngineStruct();

  engp->tpMode = m_SelectedMode;

  char str[200], str2[200];
  int Hours, Minutes, Seconds, msSend, msRcv;
  double BytesPerSecondSend, BytesPerSecondRcv;
  // unused
  // static double STBytesPerSecond = 0;
  time_t stopTime;
  char tBuf1[ 256 ];
  struct tm *tmPnt;

  time(&stopTime);
  ClearTextWindow();

  msSend = ( engp->stats.StopSend.tv_sec - engp->stats.StartSend.tv_sec ) * 1000;
  msSend += ( engp->stats.StopSend.tv_usec - engp->stats.StartSend.tv_usec ) / 1000;
  if( msSend != 0 )
    BytesPerSecondSend = ( (double)(engp->stats.BytesSent) * 1000.0 )
      / (double)(msSend);
  else
    BytesPerSecondSend = 0.0;

  msRcv = ( engp->stats.StopRecv.tv_sec - engp->stats.StartRecv.tv_sec ) * 1000;
  msRcv += ( engp->stats.StopRecv.tv_usec - engp->stats.StartRecv.tv_usec ) / 1000;
  if( msRcv != 0 )
    BytesPerSecondRcv = ( (double)(engp->stats.BytesRecvd) * 1000.0 )
      / (double)(msRcv);
  else
    BytesPerSecondRcv = 0.0;

  sprintf(str, "Throughput test results:");
  Report(str);
  Report("");

  switch (engp->tpMode) {
  case M_TCP_SEND:
  case M_UDP_SEND:
  case M_UDP_FDX:
    if (gethostname( tBuf1, sizeof tBuf1 ) == SOCKET_ERROR)
      sprintf(str, "Source machine :         %s",
	      inet_ntoa(engp->myLocalAddress));
    else
      sprintf(str, "Source machine :         %s (%s)", tBuf1,
	      inet_ntoa(engp->myLocalAddress));
    sprintf(str2, "Destination machine :    %s (%s)",
	    engp->hostName,
	    inet_ntoa( *((struct in_addr *)&engp->hostIP) ) );
    break;
  case M_TCP_RECV:
  case M_UDP_RECV:
    if (gethostname( tBuf1, sizeof tBuf1 ) == SOCKET_ERROR)
      sprintf(str2, "Destination machine :    %s",
	      inet_ntoa(engp->myLocalAddress));
    else
      sprintf(str2, "Destination machine :    %s (%s)", tBuf1,
	      inet_ntoa(engp->myLocalAddress));
    sprintf(str, "Source machine :         %s (%s)",
	    engp->hostName,
	    inet_ntoa( *((struct in_addr *)&engp->hostIP) ) );
    break;
  default:
    sprintf(str, "Source machine :         Unknown");
    sprintf(str2, "Destination machine :    Unknown");
  }
  Report(str);
  Report(str2);

  switch (m_SelectedMode) {
  case CLM_AUTO:
    sprintf(str, "Type of test :           Send & Receive");
    break;
  case CLM_AUTO_SEND:
    sprintf(str, "Type of test :           Send");
    break;
  case CLM_AUTO_RECV:
    sprintf(str, "Type of test :           Receive");
    break;
  }
  Report(str);

#ifdef USE_GMTIME
  tmPnt = gmtime( (time_t *)(&engp->startTime.tv_sec) );
  sprintf(str, "Test started :           %04d-%02d-%02d %02d:%02d:%02d.%03ld GMT",
          tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
          tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec,
          engp->startTime.tv_usec / 1000L );
  Report(str);
  tmPnt = gmtime( (time_t *)(&stopTime.tv_sec) );
  sprintf(str, "Test ended :             %04d-%02d-%02d %02d:%02d:%02d.%03ld GMT",
          tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
          tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec,
          stopTime.tv_usec / 1000L );
  Report(str);
#else
  tmPnt = localtime( (time_t *)(&engp->startTime) );
  sprintf(str, "Test started :           %04d-%02d-%02d %02d:%02d:%02d",
          tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
          tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec );
  Report(str);
  tmPnt = localtime( (time_t *)(&stopTime) );
  sprintf(str, "Test ended :             %04d-%02d-%02d %02d:%02d:%02d",
          tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
          tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec );
  Report(str);
#endif

  Hours = msSend / 3600000;
  msSend = msSend % 3600000;
  Minutes = msSend / 60000;
  msSend = msSend % 60000;
  Seconds = msSend / 1000;
  msSend = msSend % 1000;

  Report("");
  sprintf(str, "Send statistics" );
  Report(str);
  sprintf(str, "  Send time :            %02d:%02d:%02d.%03d",
          Hours, Minutes, Seconds, msSend );
  Report(str);

  switch (m_SelectedMode) {
  case M_UDP_RECV:
  case M_UDP_SEND:
  case M_UDP_FDX:
    sprintf(str, "  Packets sent :         %d", (unsigned int)engp->stats.PktsSent );
    Report(str);
    sprintf(str, "  Unable to send :       %d", (unsigned int)engp->stats.PktsUnSent );
    Report(str);
    break;
  }

#ifdef NO_QUAD_PRINTF
  sprintf(str, "  Bytes sent :           %.0f  (%s)",
          (double)(engp->stats.BytesSent), UInt32ToString( (UINT32) engp->stats.BytesSent ) );
#else
  sprintf(str, "  Bytes sent :           %qd  (%s)",
          engp->stats.BytesSent, UInt32ToString( engp->stats.BytesSent ) );
#endif
  Report(str);
  sprintf(str, "  Bits/second sent :     %d  (%s)",
          (int)(BytesPerSecondSend * 8.0),
          Int32ToString( (int)(BytesPerSecondSend * 8.0) ) );
  Report(str);

  Hours = msRcv / 3600000;
  msRcv = msRcv % 3600000;
  Minutes = msRcv / 60000;
  msRcv = msRcv % 60000;
  Seconds = msRcv / 1000;
  msRcv = msRcv % 1000;
  Report("");
  sprintf(str, "Receive statistics" );
  Report(str);
  sprintf(str, "  Receive time :         %02d:%02d:%02d.%03d",
          Hours, Minutes, Seconds, msRcv );
  Report(str);

  switch (engp->tpMode) {
  case M_UDP_SEND:
  case M_UDP_RECV:
  case M_UDP_FDX:
    sprintf(str, "  Packets received :     %d", (unsigned int)engp->stats.PktsRecvd );
    Report(str);
    sprintf(str, "  Out-of-order packets : %d", (unsigned int)engp->stats.ooCount );
    Report(str);
    break;
  }

#ifdef NO_QUAD_PRINTF
  sprintf(str, "  Bytes received :       %.0f  (%s)",
          (double)(engp->stats.BytesRecvd), UInt32ToString( (UINT32)engp->stats.BytesRecvd ) );
#else
  sprintf(str, "  Bytes received :       %d  (%s)",
          (unsigned int)engp->stats.BytesRecvd, UInt32ToString( (UINT32)engp->stats.BytesRecvd ) );
#endif
  Report(str);

  sprintf(str, "  Bits/second received : %d  (%s)",
          (int)(BytesPerSecondRcv * 8.0),
          Int32ToString( (int)(BytesPerSecondRcv * 8.0) ) );
  Report(str);
  switch (engp->tpMode) {
  case M_UDP_RECV:
  case M_UDP_SEND:
  case M_UDP_FDX:
    Report("");
    sprintf(str, "Lost packets" );
    Report(str);
    if( engp->stats.PktsSent != 0 ) {
      sprintf(str, "  Packets lost on net :  %d (%.2f%%)",
	      (unsigned int)(engp->stats.PktsSent - engp->stats.PktsRecvd),
	      ( (double)(engp->stats.PktsSent - engp->stats.PktsRecvd) * 100.0 )
	      / (double)(engp->stats.PktsSent + engp->stats.PktsUnSent) );
      Report(str);
      sprintf(str, "  Unable to send :       %d (%.2f%%)", 
	      (unsigned int)engp->stats.PktsUnSent,
	      ( (double)(engp->stats.PktsUnSent) * 100.0 )
	      / (double)(engp->stats.PktsSent + engp->stats.PktsUnSent) );
      Report(str);
      sprintf(str, "  Total packets lost :   %d (%.2f%%)",
	      (unsigned int)(( engp->stats.PktsSent - engp->stats.PktsRecvd ) + engp->stats.PktsUnSent),
	      ( (double)(( engp->stats.PktsSent - engp->stats.PktsRecvd )
			 + engp->stats.PktsUnSent) * 100.0 )
	      / (double)(engp->stats.PktsSent + engp->stats.PktsUnSent) );
      Report(str);
    }
    else {
      sprintf(str, "  Packets lost on net :  %d", (unsigned int)(engp->stats.PktsSent - engp->stats.PktsRecvd)
	      );
      Report(str);
      sprintf(str, "  Unable to send :       %d", (unsigned int)(engp->stats.PktsUnSent) );
      Report(str);
      sprintf(str, "  Total packets lost :   %d",
	      (unsigned int)(( engp->stats.PktsSent - engp->stats.PktsRecvd ) + engp->stats.PktsUnSent) );
      Report(str);
    }
  }
  if( engp->tpMode == M_UDP_FDX ) {
    if( engp->stats.nRoundtrips != 0 ) {
      sprintf(str, "\\r\\nRoundtrip statistics" );
      Report(str);
      sprintf(str, "  Min roundtrip delay :  %d.%03d ms",
	      (unsigned int)(engp->stats.MinRoundtrip / 1000), 
	      (unsigned int)(engp->stats.MinRoundtrip % 1000) );
      Report(str);
      sprintf(str, "  Max roundtrip delay :  %d.%03d ms",
	      (unsigned int)(engp->stats.MaxRoundtrip / 1000), 
	      (unsigned int)(engp->stats.MaxRoundtrip % 1000) );
      Report(str);
#ifdef NO_QUAD_PRINTF
      sprintf(str, "  Avg roundtrip delay :  %.0f.%03.0f ms",
	      (double)(( engp->stats.TotalRoundtrip / engp->stats.nRoundtrips ) / 1000),
	      (double)(( engp->stats.TotalRoundtrip / engp->stats.nRoundtrips ) % 1000) );
#else
      sprintf(str, "  Avg roundtrip delay :  %qd.%03qd ms",
	      (long long int)(( engp->stats.TotalRoundtrip / engp->stats.nRoundtrips ) / 1000),
	      (long long int)(( engp->stats.TotalRoundtrip / engp->stats.nRoundtrips ) % 1000) );
#endif
      Report(str);
    }
  }
  Report("");
}

void TPTestPanel::OnDirectionBox(wxCommandEvent& event)
{
  if( event.GetString() == wxT("Uppströms") )
    m_SelectedDirection = DIRECTION_UP;
  else if( event.GetString() == wxT("Nedströms") )
    m_SelectedDirection = DIRECTION_DOWN;
  else if( event.GetString() == wxT("Full duplex") )
    m_SelectedDirection = DIRECTION_FD;
  else
    m_SelectedDirection = DIRECTION_INVALID;
}

void TPTestPanel::RecalculateUDPFields(wxCommandEvent& event)
{
  if( m_SkipUDPTextEvent )
    {
      event.Skip();
    }
  else if( 
	  event.GetId() == wxID_UDPPPS ||
	  event.GetId() == wxID_UDPSIZE ||
	  event.GetId() == wxID_UDPRATE )
    {
      m_SkipUDPTextEvent = true;
      
      wxTextCtrl *pps_ctrl  = (wxTextCtrl*)this->FindWindow( wxID_UDPPPS );
      wxTextCtrl *size_ctrl = (wxTextCtrl*)this->FindWindow( wxID_UDPSIZE );
      wxTextCtrl *rate_ctrl = (wxTextCtrl*)this->FindWindow( wxID_UDPRATE );
      
      unsigned int pps, size, rate;
      pps_ctrl->GetValue().ToLong( (long*)&pps );
      size_ctrl->GetValue().ToLong( (long*)&size );
      rate_ctrl->GetValue().ToLong( (long*)&rate );
      
      if( event.GetId() == wxID_UDPRATE )
	{
	  // init test engine, needed for calculating the udp fields
	  TestMarshall *tm = TestMarshall::GetInstance();
	  
	  tm->RecalculateUDPFields( rate, pps, size );
	  
	  wxString Size;
	  Size << size;
	  size_ctrl->SetValue( Size );
	  
	  wxString PPS;
	  PPS << pps;
	  pps_ctrl->SetValue( PPS );
	  
	  
	}
      
      
      if( event.GetId() == wxID_UDPPPS ||
	  event.GetId() == wxID_UDPSIZE )
	{
	  wxString Rate;
	  Rate << size*8*pps;
	  rate_ctrl->SetValue( Rate );
	}
      
      m_SkipUDPTextEvent = false;
    }
  
}


void TPTestPanel::UpdateServerList( wxCommandEvent &event )
{
  wxTextCtrl *t = (wxTextCtrl*)this->FindWindow( wxID_MS_URL );
  Downloader dl;
  dl.downloadFile( TOCSTR(t->GetValue()) );
  
  char	       *slist        = dl.content();
  unsigned int	slist_length = dl.contentLength();
  
  wxString strList( FROMCSTR(slist), slist_length );
  
  wxString line;
  do
    {
      if( strList.Find( wxChar(wxT('\n')) ) == -1 )
	break;
      
      line = strList.BeforeFirst( wxChar(wxT('\n')) );
      strList = strList.AfterFirst( wxChar(wxT('\n')) );
      
      wxStringTokenizer tkz(line, wxT(" \t") );
      
      Server *serv = new Server();
      
      //int c = tkz.CountTokens();
      
      serv->Host	= tkz.GetNextToken();
      serv->TPTest_Port	= tkz.GetNextToken();
      
      if( serv->TPTest_Port.Length() == 0 ||
	  serv->Host.Length() == 0 )
	{
	  delete serv;
	  continue;
	}
      
      serv->TPTest_Port = serv->TPTest_Port.BeforeLast( wxT('\r') );
      
      m_ServerList.Append(serv);
      
    } while( line.Length() > 0 );
  
  m_ServerListCtrl->DeleteAllItems();
  for ( ServerList::Node *node = m_ServerList.GetFirst(); node; node = node->GetNext() )
    {
      Server *current = node->GetData();
      
      wxListItem item;
      int id = m_ServerListCtrl->GetItemCount();
      item.SetMask(wxLIST_MASK_TEXT);
      item.SetId(id);
      item.SetText( current->Host );
      
      m_ServerListCtrl->InsertItem( item );
      m_ServerListCtrl->SetItem( id, 1, wxString( current->TPTest_Port ) );
    }
}


void TPTestPanel::OnItemSelected(wxListEvent& event)
{
  // unused
  //  int SelectedIndex = event.GetIndex();
  
  wxString strHost, strPort;
  
  wxListItem item;
  item.m_itemId = event.GetIndex();
  item.m_mask = wxLIST_MASK_TEXT;
  
  item.m_col = 0;
  m_ServerListCtrl->GetItem( item );
  strHost = item.m_text;
  
  item.m_col = 1;
  m_ServerListCtrl->GetItem( item );
  strPort = item.m_text;
  
  m_TPTEST3_ServerURL->SetValue( strHost );
  m_TPTEST3_ServerPort->SetValue( strPort );
}


//
// Unused functions - redesign this later (low prio)
//

bool TPTestPanel::ExecuteTest(void)
{
	// Do nothing, tptest3 do not repeat
	return true;
}

bool TPTestPanel::ExecuteTest_Wrapper(void* obj)
{
	// Do nothing, tptest3 do not repeat
	return true;
}

void TPTestPanel::RefreshSettingsList(void)
{
	// Do nothing, tptest3 doesn't have a settings list
}

