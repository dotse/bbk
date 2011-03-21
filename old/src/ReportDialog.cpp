#include "ReportDialog.h"

#ifdef WIN32
#include <shlobj.h>
#include "OSInfo.h"
#endif

#include "TcpWindowSize.h"

enum {	wxID_BUTTON_CLOSE = wxID_HIGHEST,
		wxID_REPORT_TIMER, 
		wxID_BUTTON_SAVE,
		wxID_REPORT_PROC };

BEGIN_EVENT_TABLE( ReportDialog, wxDialog )
	EVT_BUTTON(wxID_BUTTON_CLOSE, ReportDialog::OnButtonClose)
	EVT_CLOSE(ReportDialog::OnClose)
	EVT_BUTTON(wxID_BUTTON_SAVE, ReportDialog::OnButtonSave)
	EVT_TIMER(wxID_REPORT_TIMER, ReportDialog::OnTimer)
	EVT_END_PROCESS(wxID_REPORT_PROC, ReportDialog::OnEndProc)
END_EVENT_TABLE()


ReportDialog::ReportDialog( wxWindow *parent )
:wxDialog( parent, wxID_ANY, _T("Rapport"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE  )
{
	m_SizerMain = new wxGridBagSizer( 0, 5 );

	m_proc_running = false;
	m_timer = new wxTimer(this, wxID_REPORT_TIMER);

	m_Output = new wxTextCtrl( this, 
				   wxID_ANY, 
				   wxT(""),
				   wxDefaultPosition,
				   wxSize( 570, 230 ),
				   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH );

	m_SizerMain->Add( m_Output, 
			  wxGBPosition( 0, 0 ), 
			  wxDefaultSpan,
			  wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxEXPAND );

	m_ButtonSaveToFile = new wxButton( this, wxID_BUTTON_SAVE, wxT("Spara till fil") );



	m_SizerMain->Add( m_ButtonSaveToFile, 
						wxGBPosition( 1, 1 ), 
						wxDefaultSpan,
						wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL );

	// and set the sizer(s)
	this->SetSizer( m_SizerMain );
	m_SizerMain->SetSizeHints( this );

}

ReportDialog::~ReportDialog(void)
{
}

void ReportDialog::OnButtonClose(wxCommandEvent& event)
{
  wxCloseEvent wce = wxCloseEvent();
  this->OnClose(wce);
}

void ReportDialog::OnClose(wxCloseEvent& event)
{
  m_Output->Clear();
  m_timer->Stop();

  if( m_Done )
    this->Deinit();

    m_Closed = true;
}

void ReportDialog::OnButtonSave(wxCommandEvent& event)
{
    wxFileDialog dialog
                 (
                    this,
                    wxT("Spara rapport"),
                    wxEmptyString,
                    wxT("TPTest5 rapport.txt"),
                    wxT("Text files (*.txt)|*.txt"),
					wxSAVE | wxOVERWRITE_PROMPT | wxCHANGE_DIR 
				);

	wxString dir;
#ifdef WIN32
	HRESULT result = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, wxStringBuffer(dir, MAX_PATH));
	if(SUCCEEDED(result)) // or FAILED(result)
	{
			// Do something
	} 
#endif
	dialog.SetDirectory(dir);

    if (dialog.ShowModal() == wxID_OK)
    {
		wxFileOutputStream output( dialog.GetPath() );
		wxTextOutputStream text( output );

		text.WriteString( m_Output->GetValue() );
	}

}

void ReportDialog::Init(void)
{
	this->Show();

	m_dummy = 0;
	m_step = 0;
	m_Closed = 0;
	m_TPTestInitialized = 0;
	m_stepDone = false;
	m_Done = false;

	this->SetCursor( *wxHOURGLASS_CURSOR );
	
	this->m_dlgProgress = 
	  new wxProgressDialog(
			       wxString(wxT("Skapar rapport (detta tar flera minuter)")), 
			       wxString(WAIT_STRING),
			       100,
			       this,
			       wxPD_AUTO_HIDE | wxPD_CAN_ABORT );

	this->m_dlgProgress->SetSize( 400, 150 );
	this->m_dlgProgress->MakeModal(false);

	this->m_ButtonSaveToFile->Enable(false);

	m_ButtonSaveToFile->Enable(false);

	m_timer->Start(1);
}

bool ReportDialog::Run(int step)
{
	wxString output;

	if( step == 0 )
	{
		// Do the tests
		m_Done = false;

		m_Output->Clear();

		m_Output->AppendText( wxString(TPTEST_VERSION_STRING) );
		m_Output->AppendText( wxT("\n") );
		m_Output->AppendText( wxString(wxDateTime::Now().Format(wxT("%Y-%m-%d %H:%M:%S"))) );
		m_Output->AppendText( wxT("\n") );
		m_Output->AppendText( wxT("\n") );


#ifdef WIN32
		// OS information
		OSInfo osinfo;
		std::string str_osinfo = osinfo.GetOSInfoString();
		if (str_osinfo.length() < 1) {
		  m_Output->AppendText( wxString(wxT("[Failed to get OS version info]\n")) );
		}
		else {
			m_Output->AppendText( wxString( FROMCSTR(str_osinfo.c_str()) ) );
		}
#endif	       
		m_stepDone = true;
	}
	else if( step == 1 )
	{
		// Report TCP Window size
		TcpWindowSize tcpwsz;
		output = FROMCSTR(tcpwsz.GetStr());
		m_Output->AppendText( wxString(output) );
		m_stepDone = true;
	}
	else if( step == 2 )
	{
		// External IP
		Downloader *dl = new Downloader();
		dl->downloadFile( TOCSTR(wxT("http://ghn.se/cgi-bin/whatsmyip")) );
		if( dl->content() != NULL )
		{
		  m_Output->AppendText( wxString( wxT("External IP Address: ") ) );
		  m_Output->AppendText( FROMCSTR(dl->content()) );
		}
		else
		{
		  m_Output->AppendText( wxString(wxT("Cannot determine external IP. No connection to TPTEST host.\n\n") ) );
		}
		delete dl;
		m_stepDone = true;
	}
	else if( step == 3 )
	  {
	    // ifconfig
		wxString cmd;
#ifdef WIN32
	    cmd << wxT("ipconfig /all");
#endif
#ifdef MACOSX
		cmd << wxT("ifconfig");
#endif
		m_proc = new wxProcess(this, wxID_REPORT_PROC);

		m_proc->Redirect();

		if( wxExecute( cmd, wxEXEC_ASYNC, m_proc ) > 0 )
		{
			m_proc_running = true;
			m_procIn = m_proc->GetInputStream();
		}
		else
		{
			this->m_Output->AppendText( wxT("Kan inte köra ifconfig.\n\n") );
			m_stepDone = true;
			m_stepError = true;
		}
	  }
	else if( step == 4 )
	  {
	    // netstat
		wxString cmd;
#ifdef WIN32
	    cmd << wxT("netstat -na");
#endif
#ifdef MACOSX
		cmd << wxT("netstat -nafinet");
#endif
		m_proc = new wxProcess(this, wxID_REPORT_PROC);

		m_proc->Redirect();

		if( wxExecute( cmd, wxEXEC_ASYNC, m_proc ) > 0 )
		{
			m_proc_running = true;
			m_procIn = m_proc->GetInputStream();
		}
		else
		{
			this->m_Output->AppendText( wxT("Kan inte köra netstat.\n\n") );
			m_stepDone = true;
			m_stepError = true;
		}
	  }
	else if( step == 5 )
	{
		// Standard TPTest
		TestMarshall *tm = TestMarshall::GetInstance();
		m_TPTestInitialized = tm->InitTestEngine( FULL_TPTEST );
		if( m_TPTestInitialized && tm->SetupFullTPTest() )
		{
			tm->Execute();
		}
		else
		{
			this->m_Output->AppendText( wxT("Kan inte köra TPTEST.\n\n") );
			m_stepDone = true;
			m_stepError = true;
		}
	}
	else if( step == 6 )
	{
		// traceroute
		if( ::g_ServerList->GetCount() > 0 )
		{
			Server *srv = ::g_ServerList->Item(0)->GetData();
			m_szTracerouteServer = (wxChar*)srv->Host.c_str();

			wxString cmd;

#ifdef WIN32		       
			cmd << wxT("tracert -w 1000 -h 20 ") << srv->Host;
#endif
#ifdef UNIX
			cmd << wxT("traceroute ") << srv->Host;
#endif
			m_proc = new wxProcess(this, wxID_REPORT_PROC);

			m_proc->Redirect();

			if( wxExecute( cmd, wxEXEC_ASYNC, m_proc ) > 0 )
			{
				m_proc_running = true;
				m_procIn = m_proc->GetInputStream();
			}
			else
			{
				m_stepDone = true;
				m_stepError = true;
				this->m_Output->AppendText( wxT("traceroute kunde inte starta på din dator.\n\n") );
			}

		} // if servers
		else
		{
			m_stepDone = true;
			m_stepError = true;
			this->m_Output->AppendText( wxT("Ingen giltig server i serverlistan.\n\n") );
		}
	}
	else
	{
		return false;
	}
	return true;
}

void ReportDialog::OnTimer( wxTimerEvent& event )
{
	if( m_dummy > 99 )
		m_dummy = 0;

	TestMarshall *tm = TestMarshall::GetInstance();

	wxString title;

	if( m_step == 0 ) // Init and OS Info
	{
		if( NextStep() )
		{
		  this->m_Output->AppendText( wxT("\n\n----------------------------\n\n") );
		  Run(1);
		}
	}
	else if( m_step == 1 ) // TCP window size
	{
		if( NextStep() )
		{
		  this->m_Output->AppendText( wxT("\n\n----------------------------\n\n") );
		  Run(2);
		}
	}
	else if( m_step == 2 ) // External IP
	{
	  title << wxT("Kontrollerar extern IP");
	  if( NextStep() )
	    {
	      this->m_Output->AppendText( wxT("\n\n----------------------------\n\n") );
	      Run(3);
	    }

	}
	else if( m_step == 3 )
	{
	  title << wxT("Kontrollerar dina nätverksinställningar");

	  if( NextStep() )
		{
		  this->m_Output->AppendText( wxT("\n\n----------------------------\n\n") );
		  Run(4);
		  return;
		}

		if( m_proc_running ) {
			this->OutputBuffer();
		}
		else {
			while( this->OutputBuffer() );
		}

		m_stepDone = !m_proc_running;

	}
	else if( m_step == 4 ) // netstat 
	{
	  title << wxT("Kontrollerar din nuvarande nätverkskopplingar");
	  if( NextStep() )
	    {
	      this->m_Output->AppendText( wxT("\n\n----------------------------\n\n") );
	      Run(5);
	      return;
	    }

		if( m_proc_running ) {
			this->OutputBuffer();
		}
		else {
			while( this->OutputBuffer() );
		}

		m_stepDone = !m_proc_running;

	}
	else if( m_step == 5 ) // tptest downstream
	{
	  title << FROMCSTR(tm->GetProgressString());
	  if( NextStep() )
	    {
	      this->m_Output->AppendText( wxT("\n\n----------------------------\n\n") );
	      Run(6);
	      return;
	    }

		if( tm->GetCompletedStatus() )
		{
			// TCP nedströms
			TPEngine *engp = tm->GetDownstreamTPEngineStruct(0);
			wxString Bandwidth( FROMCSTR( Int32ToString( (int)engp->bestTCPRecvRate*8) ) );
			m_Output->AppendText( wxT("\n\nTCP nedströms : ") );
			m_Output->AppendText( Bandwidth );
			m_Output->AppendText( wxT("\n") );
			Bandwidth.Clear();

			// TCP uppströms
			engp = tm->GetUpstreamTPEngineStruct(0);
			Bandwidth << FROMCSTR( Int32ToString( (int)engp->bestTCPSendRate*8) );
			m_Output->AppendText( wxT("\nTCP uppströms : ") );
			m_Output->AppendText( Bandwidth );
			m_Output->AppendText( wxT("\n") );
			Bandwidth.Clear();


			// UDP nedströms
			engp = tm->GetDownstreamTPEngineStruct(1);
			Bandwidth << FROMCSTR( Int32ToString( (int)engp->bestUDPRecvRate*8 ) );
			m_Output->AppendText( wxT("\nUDP nedströms : ") );
			m_Output->AppendText( Bandwidth );
			m_Output->AppendText( wxT("\n") );
			Bandwidth.Clear();

			// UDP uppströms
			engp = tm->GetUpstreamTPEngineStruct(1);
			Bandwidth << FROMCSTR( Int32ToString( (int)engp->bestUDPSendRate*8) );
			m_Output->AppendText( wxT("\nUDP uppströms : ") );
			m_Output->AppendText( Bandwidth );
			m_Output->AppendText( wxT("\n") );
			Bandwidth.Clear();

			m_stepDone = true;
		}

	}
	else if( m_step == 6 )
	{
	  title << wxT("Utför traceroute");

		if( m_proc_running ) {
			this->OutputBuffer();
		}
		else {
			while( this->OutputBuffer() );
		}

		m_stepDone = !m_proc_running;
	}
	
	bool Ok = this->m_dlgProgress->Update( m_dummy++, title );

	if( !m_Closed && (m_step == 6 && m_stepDone) || !Ok )
	{
	  TestMarshall *tm = TestMarshall::GetInstance();

	  this->m_Output->AppendText( wxT("\n\n----------- END --------------\n\n") );

	  this->m_timer->Stop();

	  if( m_TPTestInitialized )
	    tm->DeinitTestEngine(!Ok);

	  this->SetCursor( *wxSTANDARD_CURSOR );
	  this->m_dlgProgress->Show(false);
	  this->m_ButtonSaveToFile->Enable(true);
	  m_Done = true;
	}
}

void ReportDialog::Deinit()
{
	    this->SetCursor( *wxSTANDARD_CURSOR );
	    delete m_dlgProgress;
	    m_dlgProgress = NULL;
		delete m_proc;
	    this->Show(false);
}

bool ReportDialog::IsDone(void)
{
  return m_Done;
}

bool ReportDialog::NextStep(void)
{
  if( m_stepDone )
    {
      m_stepDone = false;
      m_step++;
      return true;
    }
  return false;
}

void ReportDialog::OnEndProc( wxProcessEvent &event )
{
	m_proc_running = false;
}

bool ReportDialog::OutputBuffer()
{
	
	    if( m_proc->IsInputAvailable() )
        {
			char buffer[1024];
		    m_procIn->Read(buffer, 1023);
		    buffer[m_procIn->LastRead()] = '\0';
		
		    wxString str(buffer, wxConvUTF8);
		
			if( str.Length() > 0 )
			{
#ifdef WIN32
				str.Replace( wxT("\r"), wxT("") );
				str.Replace( wxT("†"), wxT("å") );
				str.Replace( wxT("„"), wxT("ä") );
				str.Replace( wxT("”"), wxT("ö") );
				str.Replace( wxT(""), wxT("Å") );
				str.Replace( wxT("Ž"), wxT("Ä") );
				str.Replace( wxT("™"), wxT("Ö") );
#endif
				this->m_Output->AppendText( str );
			}
		}

	return m_proc->IsInputAvailable();
}