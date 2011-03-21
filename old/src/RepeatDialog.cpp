#include "Repeatdialog.h"

enum { wxID_CHOICE_TIME = wxID_HIGHEST, wxID_CHOICE_INTERVAL, wxID_BUTTON_OK };

BEGIN_EVENT_TABLE( RepeatDialog, wxDialog )
	EVT_BUTTON(wxID_BUTTON_OK, RepeatDialog::OnOk)
END_EVENT_TABLE()

RepeatDialog::RepeatDialog(wxWindow *parent)
:wxDialog( parent, wxID_ANY, wxT("Repetera"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE )
{
	AppConfig *conf = AppConfig::GetInstance();
	
	m_SizerMain = new wxGridBagSizer( 0, 5 );

	m_ctrlEnable	= new wxCheckBox( this, wxID_ANY, wxT("Aktivera funktionen repetera") );

	wxString strEnable;
	conf->GetValue( wxString( wxT("REPEAT_ENABLED") ), strEnable );

	if( strEnable == wxT("YES") )
		m_ctrlEnable->SetValue(true);
	else
		m_ctrlEnable->SetValue(false);

	wxString strTime;
	conf->GetValue( wxString( wxT("REPEAT_TIME") ), strTime );

	wxString strWait;
	conf->GetValue( wxString( wxT("REPEAT_INTERVAL") ), strWait );

	m_ctrlTime		= new wxChoice( this, wxID_CHOICE_TIME );
	m_ctrlTime->Append( wxString( wxT("2") ) );
	m_ctrlTime->Append( wxString( wxT("6") ) );
	m_ctrlTime->Append( wxString( wxT("12") ) );
	m_ctrlTime->Append( wxString( wxT("24") ) );

	m_ctrlTime->SetSelection( m_ctrlTime->FindString( strTime ) );


	m_ctrlWait		= new wxChoice( this, wxID_CHOICE_INTERVAL );

	m_ctrlWait->Append( wxString( wxT("15") ) );
	m_ctrlWait->Append( wxString( wxT("30") ) );
	m_ctrlWait->Append( wxString( wxT("60") ) );

	m_ctrlWait->SetSelection( m_ctrlWait->FindString( strWait ) );

	m_SizerMain->Add( m_ctrlEnable, 
						wxGBPosition( 0, 0 ), 
						wxGBSpan( 1, 2 ),
						wxALL,
						5 );

	m_SizerMain->Add( new wxStaticText( this, wxID_ANY, wxT("Testperiod i timmar:") ),
						wxGBPosition( 1, 0 ),
						wxDefaultSpan,
						wxALL,
						5 );

	m_SizerMain->Add( m_ctrlTime,
						wxGBPosition( 1, 1 ),
						wxDefaultSpan,
						wxALL,
						5 );

	m_SizerMain->Add( new wxStaticText( this, wxID_ANY, wxT("Antal minuter mellan varje test:") ),
						wxGBPosition( 2, 0 ),
						wxDefaultSpan,
						wxALL,
						5 );

	m_SizerMain->Add( m_ctrlWait,
						wxGBPosition( 2, 1 ),
						wxDefaultSpan,
						wxALL,
						5 );

	m_btnOk = new wxButton( this, wxID_BUTTON_OK, wxT("Ok") );

	m_SizerMain->Add( m_btnOk, 
						wxGBPosition( 3,0 ),
						wxGBSpan( 1, 2 ),
						wxALL |
						wxALIGN_CENTER,
						5 );

	this->SetSizer( m_SizerMain );
	m_SizerMain->SetSizeHints( this );

}

RepeatDialog::~RepeatDialog(void)
{
	AppConfig *conf = AppConfig::GetInstance();

	wxString repeat_time = m_ctrlTime->GetStringSelection();
	wxString repeat_interval = m_ctrlWait->GetStringSelection();
	wxString repeat_enabled = m_ctrlEnable->GetValue() ? wxString( wxT("YES") ) : wxString( wxT("NO") );

	conf->SetValue( wxString( wxT("REPEAT_TIME") ), repeat_time );
	conf->SetValue( wxString( wxT("REPEAT_INTERVAL") ), repeat_interval );
	conf->SetValue( wxString( wxT("REPEAT_ENABLED") ), repeat_enabled );
}

void RepeatDialog::OnOk( wxCommandEvent& event )
{
	this->Close();
}
