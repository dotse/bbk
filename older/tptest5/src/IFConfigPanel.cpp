#include "IFConfigPanel.h"

enum {
  wxID_IFCONFIG = wxID_HIGHEST + 1,
  wxID_IFCONFIG_PROC,
  wxID_IFCONFIG_TIMER,
  wxID_IFCONFIG_BUTTON_START
};

BEGIN_EVENT_TABLE( IFConfigPanel, wxPanel )
	EVT_BUTTON(wxID_IFCONFIG_BUTTON_START, IFConfigPanel::OnStartTest)
	EVT_TIMER(wxID_IFCONFIG_TIMER, IFConfigPanel::OnTimer)
	EVT_END_PROCESS(wxID_IFCONFIG_PROC, IFConfigPanel::OnEndProc)
END_EVENT_TABLE()


IFConfigPanel::IFConfigPanel( wxWindow *parent )
:wxPanel( parent, wxID_IFCONFIG, wxDefaultPosition, wxDefaultSize, 0 )
{
	m_SizerMain = new wxBoxSizer( wxVERTICAL );

	m_running = false;
	m_die = false;
	m_timer = new wxTimer(this, wxID_IFCONFIG_TIMER);
	m_proc = new wxProcess(this, wxID_IFCONFIG_PROC);

	wxButton *sb = new wxButton( this, 
				     wxID_IFCONFIG_BUTTON_START, 
				     wxT("Kör IFConfig") );
	sb->SetToolTip( wxT("Starta kommandot IFConfig") );

	m_SizerMain->Add( sb,
			  0,				// make vertically stretchable
			  wxALL |			// and make border all around
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

IFConfigPanel::~IFConfigPanel(void)
{
}

void IFConfigPanel::OnStartTest(wxCommandEvent& event)
{
  if( !m_running )
    {
	  ((wxButton*)this->
	   FindWindow( wxID_IFCONFIG_BUTTON_START ))->
	    SetLabel( wxT("Avbryt") );
	  this->m_TextOut->Clear();
	  this->SetCursor( *wxHOURGLASS_CURSOR );
	  
	  wxString cmd;

#ifdef WIN32
	cmd = wxT("ipconfig /all");
#endif

#ifdef UNIX
	cmd = wxT("ifconfig -a");
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
	    wxMessageBox( wxT("Kunde inte starta ifconfig.") );
	    return;
	  }

	  m_timer->Start(1000);

    } // if running
  else
    {
      m_die = true;
    }
	
	
	//	char *output;
	
	m_TextOut->Clear();

	wxString cmd;


}

void IFConfigPanel::OnTimer( wxTimerEvent& event )
{
	if( m_die )
	{
		// Stop the timer and set all variable to indicate stop
		this->m_timer->Stop();
		m_running = false;
		m_die = false;
		((wxButton*)this->
		  FindWindow( wxID_IFCONFIG_BUTTON_START ))->
					  SetLabel( wxT("Kör IFConfig") );

		this->SetCursor( *wxSTANDARD_CURSOR );

		// Output last bytes
		while( this->OutputBuffer() );

	}
	else
	{
		this->OutputBuffer();
	}
}

void IFConfigPanel::OnEndProc( wxProcessEvent &event )
{
  m_die = true;
}

bool IFConfigPanel::OutputBuffer()
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
