#include "HistorySettingDialog.h"

enum { wxID_CHOICE_COUNT = wxID_HIGHEST, wxID_BUTTON_OK };
				   	
BEGIN_EVENT_TABLE( HistorySettingDialog, wxDialog )
	EVT_BUTTON(wxID_BUTTON_OK, HistorySettingDialog::OnOk)
END_EVENT_TABLE()


HistorySettingDialog::HistorySettingDialog(wxWindow *parent)
:wxDialog( parent, wxID_ANY, wxT("Mätvärdeshistorik"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE )
{
	AppConfig *conf = AppConfig::GetInstance();
	
	m_SizerMain = new wxGridBagSizer( 0, 5 );

	wxString strCount;
	conf->GetValue( wxString( wxT("HISTORY_COUNT") ), strCount );

	m_ctrlCount	= new wxChoice( this, wxID_CHOICE_COUNT );
	m_ctrlCount->Append( wxString( wxT("10") ) );
	m_ctrlCount->Append( wxString( wxT("30") ) );
	m_ctrlCount->Append( wxString( wxT("100") ) );
	m_ctrlCount->Append( wxString( wxT("200") ) );
	m_ctrlCount->Append( wxString( wxT("500") ) );

	m_ctrlCount->SetSelection( m_ctrlCount->FindString( strCount ) );


	m_SizerMain->Add( new wxStaticText( this, wxID_ANY, wxT("Antal mätvärden att spara:") ),
						wxGBPosition( 0, 0 ),
						wxDefaultSpan,
						wxALL,
						5 );

	m_SizerMain->Add( m_ctrlCount,
						wxGBPosition( 0, 1 ),
						wxDefaultSpan,
						wxALL,
						5 );

	m_btnOk = new wxButton( this, wxID_BUTTON_OK, wxT("Ok") );

	m_SizerMain->Add( m_btnOk, 
						wxGBPosition( 1,0 ),
						wxGBSpan( 1, 2 ),
						wxALL |
						wxALIGN_CENTER,
						5 );

	this->SetSizer( m_SizerMain );
	m_SizerMain->SetSizeHints( this );

}

HistorySettingDialog::~HistorySettingDialog(void)
{
	AppConfig *conf = AppConfig::GetInstance();
	wxString history_count = m_ctrlCount->GetStringSelection();
	conf->SetValue( wxString( wxT("HISTORY_COUNT") ), history_count );
}

void HistorySettingDialog::OnOk( wxCommandEvent& event )
{
	this->Close();
}
