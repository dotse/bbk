#include "NetstatPanel.h"

#ifdef WIN32
#include "OSInfo.h"
#endif

enum {
  wxID_NETSTAT = wxID_HIGHEST + 1,
  wxID_NETSTAT_PROC,
  wxID_NETSTAT_TIMER,
  wxID_NETSTAT_BUTTON_START
};

BEGIN_EVENT_TABLE( NetstatPanel, wxPanel )
	EVT_BUTTON(wxID_NETSTAT_BUTTON_START, NetstatPanel::OnStartTest)
	EVT_TIMER(wxID_NETSTAT_TIMER, NetstatPanel::OnTimer)
	EVT_END_PROCESS(wxID_NETSTAT_PROC, NetstatPanel::OnEndProc)
END_EVENT_TABLE()


NetstatPanel::NetstatPanel( wxWindow *parent )
:wxPanel( parent, wxID_NETSTAT, wxDefaultPosition, wxDefaultSize, 0 )
{
	m_SizerMain = new wxBoxSizer( wxVERTICAL );

	m_running = false;
	m_die = false;

	m_timer = new wxTimer(this, wxID_NETSTAT_TIMER);
	m_proc = new wxProcess(this, wxID_NETSTAT_PROC);

	wxButton *sb = new wxButton( this, 
				     wxID_NETSTAT_BUTTON_START, 
				     wxT("Kör Netstat") );

	sb->SetToolTip( wxT("Starta kommandot Netstat") );

	m_SizerMain->Add( sb,
			  0,			// make vertically stretchable
			  wxALL |	        // and make border all around
			  wxALIGN_CENTER,	// take a guess
			  10 );			// set border width to 10
	
	m_TextOut = new wxTextCtrl( this,
				    -1,
				    wxString( wxT("") ),
				    wxDefaultPosition,
				    wxSize(660,390),
				    wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP );

	m_SizerMain->Add(	m_TextOut,
				0,
				wxALL |
				wxALIGN_CENTER,
				10 );

	this->SetSizer( m_SizerMain );
	m_SizerMain->SetSizeHints( this );
}

NetstatPanel::~NetstatPanel(void)
{
}

void NetstatPanel::OnStartTest(wxCommandEvent& event)
{
  if( !m_running )
    {
	  ((wxButton*)this->
	   FindWindow( wxID_NETSTAT_BUTTON_START ))->
	    SetLabel( wxT("Avbryt") );
	  this->m_TextOut->Clear();
	  this->SetCursor( *wxHOURGLASS_CURSOR );
	  
	  wxString cmd;

#ifdef WIN32
	  cmd = 
	    wxT("netstat -na ");
#endif

#ifdef MACOSX
	  cmd = 
	    wxT("netstat -nafinet");
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
	    wxMessageBox( wxT("Kunde inte starta netstat.") );
	    return;
	  }

	  m_timer->Start(1000);

    } // if running
  else
    {
      m_die = true;
    }
}

void NetstatPanel::OnTimer( wxTimerEvent& event )
{
	if( m_die )
	{
		// Stop the timer and set all variable to indicate stop
		this->m_timer->Stop();
		m_running = false;
		m_die = false;
		((wxButton*)this->
		  FindWindow( wxID_NETSTAT_BUTTON_START ))->
					  SetLabel( wxT("Kör Netstat") );

		this->SetCursor( *wxSTANDARD_CURSOR );

		// Output last bytes
		while( this->OutputBuffer() );

	}
	else
	{
		this->OutputBuffer();
	}
}

void NetstatPanel::OnEndProc( wxProcessEvent &event )
{
  m_die = true;
}

bool NetstatPanel::OutputBuffer()
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
