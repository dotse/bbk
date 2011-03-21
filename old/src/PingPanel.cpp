#include "PingPanel.h"

enum {
  wxID_PING = wxID_HIGHEST + 1,
  wxID_PING_PROC,
  wxID_PING_TIMER,
  wxID_PING_BUTTON_START
};

BEGIN_EVENT_TABLE( PingPanel, wxPanel )
	EVT_BUTTON(wxID_PING_BUTTON_START, PingPanel::OnStartTest)
	EVT_TIMER(wxID_PING_TIMER, PingPanel::OnTimer)
	EVT_END_PROCESS(wxID_PING_PROC, PingPanel::OnEndProc)
END_EVENT_TABLE()


PingPanel::PingPanel( wxWindow *parent )
:wxPanel( parent, wxID_PING, wxDefaultPosition, wxDefaultSize, 0 )
{
	m_running = false;
	m_die = false;
	m_SizerMain = new wxGridBagSizer( wxVERTICAL );

	m_timer = new wxTimer(this, wxID_PING_TIMER);
	m_proc = new wxProcess(this, wxID_PING_PROC);



	wxButton *sb = new wxButton( this, wxID_PING_BUTTON_START, wxT("Kör Ping") );
	sb->SetToolTip( wxT("Starta kommandot Ping") );

	m_SizerMain->Add( sb,
				wxGBPosition( 0 , 0),
				wxGBSpan( 1, 2 ),
				wxALIGN_CENTER );

	m_Host = new wxTextCtrl( this,
				 -1,
				 wxString( wxT("")),
				 wxDefaultPosition,
				 wxSize(200,20) );
       
	m_Packetsize = new wxTextCtrl( this,
				       -1,
				       wxString( wxT("64") ),
				       wxDefaultPosition,
				       wxSize(200,20) );

	m_TTL = new wxTextCtrl( this,
				-1,
				wxString( wxT("255")),
				wxDefaultPosition,
				wxSize(200,20) );

	m_Delay = new wxTextCtrl( this,
				  -1,
#ifdef WIN32
				  wxString( wxT("5000") ),
#endif
#ifdef UNIX
				  wxString( wxT("1") ),
#endif
				  wxDefaultPosition,
				  wxSize(200,20) );

	m_Count = new wxTextCtrl( this,
				  -1,
				  wxString( wxT("10") ),
				  wxDefaultPosition,
				  wxSize(200,20) );

	m_SizerMain->Add( new wxStaticText( this, 
					    wxID_ANY, 
					    wxT("Värdnamn:"), 
					    wxDefaultPosition, 
					    wxSize(100,20) ), 
			  wxGBPosition( 1 , 0),
			  wxDefaultSpan,
			  wxALIGN_BOTTOM |
			  wxALIGN_RIGHT );

	m_SizerMain->Add( m_Host,
			  wxGBPosition( 1 , 1),
			  wxDefaultSpan,
			  wxALIGN_CENTER_VERTICAL |
			  wxALIGN_LEFT );
	
	m_SizerMain->Add( new wxStaticText( this, 
					    wxID_ANY, 
					    wxT("Antal paket:"), 
					    wxDefaultPosition, 
					    wxSize(100,20) ), 
			  wxGBPosition( 2 , 0),
			  wxDefaultSpan,
			  wxALIGN_BOTTOM |
			  wxALIGN_RIGHT );

	m_SizerMain->Add( m_Count,
			  wxGBPosition( 2 , 1),
			  wxDefaultSpan,
			  wxALIGN_CENTER_VERTICAL |
			  wxALIGN_LEFT );

	
	m_SizerMain->Add( new wxStaticText( this, 
					    wxID_ANY, 
					    wxT("Paketstorlek:"),
					    wxDefaultPosition, 
					    wxSize(100,20) ), 
			  wxGBPosition( 3 , 0),
			  wxDefaultSpan,
			  wxALIGN_BOTTOM |
			  wxALIGN_RIGHT );
	
	m_SizerMain->Add( m_Packetsize,
			  wxGBPosition( 3 , 1),
			  wxDefaultSpan,
			  wxALIGN_CENTER_VERTICAL |
			  wxALIGN_LEFT );
	
	m_SizerMain->Add( new wxStaticText( this, 
					    wxID_ANY, 
					    wxT("Livstid :-)"), 
					    wxDefaultPosition, 
					    wxSize(100,20) ), 
			  wxGBPosition( 4 , 0),
			  wxDefaultSpan,
			  wxALIGN_BOTTOM |
			  wxALIGN_RIGHT );

	m_SizerMain->Add( m_TTL,
			  wxGBPosition( 4 , 1),
			  wxDefaultSpan,
			  wxALIGN_BOTTOM |
			  wxALIGN_LEFT );
	

	m_SizerMain->Add( new wxStaticText( this, 
					    wxID_ANY, 
#ifdef WIN32
					    wxT("Timeout (ms):"), 
#endif
#ifdef UNIX
					    wxT("Timeout (s):"),
#endif
					    wxDefaultPosition, 
					    wxSize(100,20) ), 
			  wxGBPosition( 5 , 0),
			  wxDefaultSpan,
			  wxALIGN_CENTER_VERTICAL |
			  wxALIGN_RIGHT );

	m_SizerMain->Add( m_Delay,
			  wxGBPosition( 5 , 1),
			  wxDefaultSpan,
			  wxALIGN_CENTER_VERTICAL |
			  wxALIGN_LEFT );


	m_TextOut = new wxTextCtrl( this,
				    -1,
				    wxString( wxT("") ),
				    wxDefaultPosition,
				    wxSize(660,240),
				    wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP );
	
	m_SizerMain->Add( m_TextOut,
			  wxGBPosition( 6 , 0),
			  wxGBSpan( 1, 2 ),
			  wxALIGN_CENTER_VERTICAL |
			  wxALIGN_CENTER );

	this->SetSizer( m_SizerMain );
	//m_SizerMain->SetSizeHints( this );
}

PingPanel::~PingPanel(void)
{
}

void PingPanel::OnStartTest(wxCommandEvent& event)
{
	if( !m_running )
	{
	  if( this->m_Host->GetValue().Length() > 0 )
      {
		((wxButton*)this->
			FindWindow( wxID_PING_BUTTON_START ))->
				SetLabel( wxT("Avbryt") );
		this->m_TextOut->Clear();
		this->SetCursor( *wxHOURGLASS_CURSOR );
	  
		wxString cmd;

#ifdef WIN32
		cmd = 
			wxT("ping -n ") + 
			m_Count->GetValue() + 
			wxT(" -i ") + 
			m_TTL->GetValue() + 
			wxT(" -l ") + 
			m_Packetsize->GetValue() + 
			wxT(" -w ") + 
			m_Delay->GetValue() + 
			wxT(" ") + 
			this->m_Host->GetValue();
#endif

#ifdef UNIX
		cmd = wxT("ping -c ") + 
			m_Count->GetValue() + 
			wxT(" -m ") + 
			m_TTL->GetValue() + 
			wxT(" -s ") + 
			m_Packetsize->GetValue() + 
			wxT(" -i ") + 
			m_Delay->GetValue() + 
			wxT(" ") + 
			this->m_Host->GetValue();
#endif

		m_running = true;

		m_proc->Redirect();

		if( wxExecute( cmd, wxEXEC_ASYNC, m_proc ) <= 0 )
		{
			this->m_TextOut->AppendText( wxT("Kommandot misslyckades\n") );
			m_die = true;
	    }
	  
		m_procIn = m_proc->GetInputStream(); 

		if( !m_proc->IsInputOpened() ) {
			wxMessageBox( wxT("Kunde inte starta ping.") );
			return;
		}

		m_timer->Start(1000);

	}
    else
	{
		wxMessageBox( wxT("Skriv in ett värdnamn") );
	}
      
  } // if running
  else
  {
	m_die = true;
  }

}

void PingPanel::OnTimer( wxTimerEvent& event )
{
	if( m_die )
	{
		// Stop the timer and set all variable to indicate stop
		this->m_timer->Stop();
		m_running = false;
		m_die = false;
		((wxButton*)this->
		  FindWindow( wxID_PING_BUTTON_START ))->
					  SetLabel( wxT("Kör Ping") );

		this->SetCursor( *wxSTANDARD_CURSOR );

		// Output last bytes
		while( this->OutputBuffer() );

	}
	else
	{
		this->OutputBuffer();
	}
}

void PingPanel::OnEndProc( wxProcessEvent &event )
{
  m_die = true;
}

bool PingPanel::OutputBuffer()
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
				this->m_TextOut->AppendText( str );
			}
		}

	return m_proc->IsInputAvailable();
}

