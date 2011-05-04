#include "TraceroutePanel.h"

enum {
  wxID_TRACEROUTE = wxID_HIGHEST + 1,
  wxID_TRACEROUTE_PROC,
  wxID_TRACEROUTE_TIMER,
  wxID_TRACEROUTE_BUTTON_START
};

BEGIN_EVENT_TABLE( TraceroutePanel, wxPanel )
	EVT_BUTTON(wxID_TRACEROUTE_BUTTON_START, TraceroutePanel::OnStartTest)
	EVT_TIMER(wxID_TRACEROUTE_TIMER, TraceroutePanel::OnTimer)
	EVT_END_PROCESS(wxID_TRACEROUTE_PROC, TraceroutePanel::OnEndProc)
END_EVENT_TABLE()


TraceroutePanel::TraceroutePanel( wxWindow *parent )
:wxPanel( parent, wxID_TRACEROUTE, wxDefaultPosition, wxDefaultSize, 0 )
{
	m_running	= false;
	m_die		= false;
	m_SizerMain = new wxBoxSizer( wxVERTICAL );
	m_SizerSettings = new wxBoxSizer( wxHORIZONTAL );

	m_timer = new wxTimer(this, wxID_TRACEROUTE_TIMER);

	m_proc = new wxProcess(this, wxID_TRACEROUTE_PROC);

	wxButton *sb = new wxButton( this, 
				     wxID_TRACEROUTE_BUTTON_START, 
				     wxT("Kör Traceroute") );
	sb->SetToolTip( wxT("Starta kommandot Traceroute") );
	
	m_SizerMain->Add( sb,
			  0,		// make vertically stretchable
			  wxALL |		// and make border all around
			  wxALIGN_CENTER,	// take a guess
			  10 );			// set border width to 10
	

	m_Host = new wxTextCtrl( this,
				 -1,
				 wxString(wxT("")),
				 wxDefaultPosition,
				 wxSize(200,20) );
	

	m_SizerSettings->Add(	new wxStaticText( this, 
						  wxID_ANY, 
						  wxT("Värdnamn:") ),
				0,
				wxALL | wxALIGN_CENTER_VERTICAL,
				1);
	
	m_SizerSettings->Add(	m_Host,
				0,
				wxALL | wxALIGN_CENTER_VERTICAL,
				1 );


	// Text OUT
	m_TextOut = new wxTextCtrl( this,
				    -1,
				    wxString( wxT("")),
				    wxDefaultPosition,
				    wxSize(660,368),
				    wxTE_MULTILINE | 
				    wxTE_READONLY | 
				    wxTE_WORDWRAP );



	// Add to the sizers
	m_SizerMain->Add( m_SizerSettings,
			  0,
			  wxALIGN_CENTER |
			  wxALL,
			  0);
	
	m_SizerMain->Add(	
			 m_TextOut,
			 0,
			 wxALL |
			 wxALIGN_CENTER,
			 10 );
	
	this->SetSizer( m_SizerMain );
	m_SizerMain->SetSizeHints( this );
}

TraceroutePanel::~TraceroutePanel(void)
{
}

void TraceroutePanel::OnStartTest(wxCommandEvent& event)
{
  if( !m_running )
    {
      if( this->m_Host->GetValue().Length() > 0 )
      {
	  ((wxButton*)this->
	   FindWindow( wxID_TRACEROUTE_BUTTON_START ))->
	    SetLabel( wxT("Avbryt") );
	  this->m_TextOut->Clear();
	  this->SetCursor( *wxHOURGLASS_CURSOR );
	  
	  wxString cmd;

#ifdef WIN32
	  cmd = 
	    wxT("tracert ") + 
	    this->m_Host->GetValue();
#endif

#ifdef UNIX
	  cmd = 
	    wxT("traceroute ") + 
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
	    wxMessageBox( wxT("Kunde inte starta traceroute.") );
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


void TraceroutePanel::OnTimer( wxTimerEvent& event )
{
	if( m_die )
	{
		// Stop the timer and set all variable to indicate stop
		this->m_timer->Stop();
		m_running = false;
		m_die = false;
		((wxButton*)this->
		  FindWindow( wxID_TRACEROUTE_BUTTON_START ))->
					  SetLabel( wxT("Kör Traceroute") );

		this->SetCursor( *wxSTANDARD_CURSOR );

		// Output last bytes
		while( this->OutputBuffer() );

	}
	else
	{
		this->OutputBuffer();
	}
}

void TraceroutePanel::OnEndProc( wxProcessEvent &event )
{
  m_die = true;
}

bool TraceroutePanel::OutputBuffer()
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