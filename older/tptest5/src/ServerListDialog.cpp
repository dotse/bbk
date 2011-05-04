#include "ServerListDialog.h"

#include "MainFrame.h"

enum {
  wxID_DIALOG_SERVERLIST = wxID_HIGHEST + 1,
  wxID_DIALOG_SERVERLIST_LISTCTRL,
  wxID_DIALOG_SERVERLIST_EDIT_HOST,
  wxID_DIALOG_SERVERLIST_EDIT_TCPPORT,
  wxID_DIALOG_SERVERLIST_EDIT_TPTESTPORT,
  wxID_DIALOG_SERVERLIST_EDIT_HTTPPATH,
  wxID_DIALOG_SERVERLIST_EDIT_FTPPATH,
  wxID_DIALOG_SERVERLIST_CHECKBOX_ICMP,
  wxID_DIALOG_SERVERLIST_BUTTON_UPDATE,
  wxID_DIALOG_SERVERLIST_BUTTON_ADD,
  wxID_DIALOG_SERVERLIST_BUTTON_DELETE,
};

BEGIN_EVENT_TABLE( ServerListDialog, wxDialog )
	EVT_LIST_ITEM_SELECTED(wxID_DIALOG_SERVERLIST_LISTCTRL, ServerListDialog::OnItemSelected)
	EVT_BUTTON(wxID_DIALOG_SERVERLIST_BUTTON_ADD, ServerListDialog::OnButtonAdd)
	EVT_BUTTON(wxID_DIALOG_SERVERLIST_BUTTON_UPDATE, ServerListDialog::OnButtonUpdate)
	EVT_BUTTON(wxID_DIALOG_SERVERLIST_BUTTON_DELETE, ServerListDialog::OnButtonDelete)
	EVT_BUTTON(wxID_BUTTON_REFRESH_SERVERLIST, ServerListDialog::OnButtonRefresh)
	EVT_BUTTON(wxID_BUTTON_CLOSE, ServerListDialog::OnButtonClose)
END_EVENT_TABLE()

ServerListDialog::ServerListDialog( wxWindow *parent )
:wxDialog( parent, wxID_DIALOG_SERVERLIST, wxT("Serverlista"), wxDefaultPosition, wxSize(700,400), wxDEFAULT_DIALOG_STYLE )
{
	// Create the main sizer
	m_SizerMain = new wxBoxSizer( wxVERTICAL );

	m_ListCtrl = 
			new wxListCtrl( 
				this, 
				wxID_DIALOG_SERVERLIST_LISTCTRL, 
				wxDefaultPosition, 
				wxDefaultSize, 
				wxLC_REPORT | wxLC_SINGLE_SEL );

	m_ListCtrl->InsertColumn(0, wxT("Adress"), wxLIST_FORMAT_LEFT, 150);
	m_ListCtrl->InsertColumn(1, wxT("TPTest port"), wxLIST_FORMAT_LEFT, 100);
	m_ListCtrl->InsertColumn(2, wxT("TCP port"), wxLIST_FORMAT_LEFT, 70);
	m_ListCtrl->InsertColumn(3, wxT("HTTP sökväg"), wxLIST_FORMAT_LEFT, 150);
	m_ListCtrl->InsertColumn(4, wxT("FTP sökväg"), wxLIST_FORMAT_LEFT, 150);
	m_ListCtrl->InsertColumn(5, wxT("ICMP"), wxLIST_FORMAT_LEFT, 80);

	m_SizerMain->Add( m_ListCtrl,
					  1,
					  wxEXPAND |
					  wxALL,
					  0 );



	m_SizerEdit = new wxGridBagSizer( 0, 5 );

	// Host
	m_TextCtrlHost = 
			new wxTextCtrl( this, 
					wxID_DIALOG_SERVERLIST_EDIT_HOST, 
					wxT(""),
					wxDefaultPosition,
					wxSize( 150, 20) );

	m_SizerEdit->Add( new wxStaticText( this, wxID_ANY, wxT("Adress:") ), 
			  wxGBPosition( 0, 0 ), 
			  wxDefaultSpan, 
			  wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL );

	m_SizerEdit->Add( m_TextCtrlHost, wxGBPosition( 0, 1 ), wxDefaultSpan );

	// TPTest Port
	m_TextCtrlTPTestPort =
			new wxTextCtrl( this, 
					wxID_DIALOG_SERVERLIST_EDIT_TPTESTPORT, 
					wxT(""),
					wxDefaultPosition,
					wxSize( 50, 20) );

	m_SizerEdit->Add( new wxStaticText( this, wxID_ANY, wxT("TPTEST port:") ), wxGBPosition( 0, 2 ), wxDefaultSpan, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL );
	m_SizerEdit->Add( m_TextCtrlTPTestPort, wxGBPosition( 0, 3 ), wxDefaultSpan );


	// TCP Port
	m_TextCtrlTCPPort =
			new wxTextCtrl( this, 
					wxID_DIALOG_SERVERLIST_EDIT_TCPPORT, 
					wxT(""),
					wxDefaultPosition,
					wxSize( 50, 20) );

	m_SizerEdit->Add( new wxStaticText( this, wxID_ANY, wxT("TCP ping port:") ), wxGBPosition( 0, 4 ), wxDefaultSpan, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL  );
	m_SizerEdit->Add( m_TextCtrlTCPPort, wxGBPosition( 0, 5 ), wxDefaultSpan );

	// HTTP Path
	m_TextCtrlHTTP_Path =
			new wxTextCtrl( this, 
					wxID_DIALOG_SERVERLIST_EDIT_HTTPPATH, 
					wxT(""),
					wxDefaultPosition,
					wxSize( 150, 20) );

	m_SizerEdit->Add( new wxStaticText( this, wxID_ANY, wxT("HTTP sökväg:") ), wxGBPosition( 1, 0 ), wxDefaultSpan, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL );
	m_SizerEdit->Add( m_TextCtrlHTTP_Path, wxGBPosition( 1, 1 ), wxDefaultSpan );

	// FTP Path
	m_TextCtrlFTP_Path =
			new wxTextCtrl( this, 
					wxID_DIALOG_SERVERLIST_EDIT_FTPPATH, 
					wxT(""),
					wxDefaultPosition,
					wxSize( 150, 20) );

	m_SizerEdit->Add( new wxStaticText( this, wxID_ANY, wxT("FTP sökväg:") ), wxGBPosition( 1, 2 ), wxDefaultSpan, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL  );
	m_SizerEdit->Add( m_TextCtrlFTP_Path, wxGBPosition( 1, 3 ), wxDefaultSpan );

	// ICMP Ping Enabled
	m_CheckBoxICMP = new wxCheckBox( this, wxID_DIALOG_SERVERLIST_CHECKBOX_ICMP, wxT("") );

	m_SizerEdit->Add( new wxStaticText( this, wxID_ANY, wxT("ICMP:") ), wxGBPosition( 1, 4 ), wxDefaultSpan, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL );
	m_SizerEdit->Add(	m_CheckBoxICMP, wxGBPosition( 1, 5 ), wxDefaultSpan,  wxALIGN_CENTER_VERTICAL );

	// Add Button
	m_ButtonAdd = new wxButton( this, wxID_DIALOG_SERVERLIST_BUTTON_ADD, wxT("Lägg till"), wxDefaultPosition, wxSize( 70, 18 ) );

	m_SizerEdit->Add(	m_ButtonAdd, wxGBPosition( 2, 0 ), wxGBSpan( 1, 4), wxALIGN_RIGHT | wxTOP | wxBOTTOM, 5 );


	// Update Button
	m_ButtonUpdate = new wxButton( this, wxID_DIALOG_SERVERLIST_BUTTON_UPDATE, wxT("Updatera"), wxDefaultPosition, wxSize( 100, 18 ) );

	m_SizerEdit->Add(	m_ButtonUpdate, wxGBPosition( 2, 4 ), wxGBSpan( 1, 1), wxALIGN_RIGHT | wxTOP | wxBOTTOM, 5 );

	// Delete Button
	m_ButtonDelete = new wxButton( this, wxID_DIALOG_SERVERLIST_BUTTON_DELETE, wxT("Ta bort"), wxDefaultPosition, wxSize( 70, 18 ) );

	m_SizerEdit->Add(	m_ButtonDelete, wxGBPosition( 2, 5 ), wxGBSpan( 1, 1), wxALIGN_RIGHT | wxTOP | wxBOTTOM, 5 );

	// Refresh button
	m_ButtonRefresh = new wxButton( this, wxID_BUTTON_REFRESH_SERVERLIST, wxT("Uppdatera serverlistan"), wxDefaultPosition, wxSize(200,18) );

	m_SizerEdit->Add( m_ButtonRefresh, wxGBPosition( 3, 0 ), wxGBSpan( 1, 3 ), wxALIGN_LEFT | wxTOP | wxBOTTOM, 5 );

	// Close button
	m_ButtonClose = new wxButton( this, wxID_BUTTON_CLOSE, wxT("Stäng fönstret") );
	m_SizerEdit->Add( m_ButtonClose, wxGBPosition( 3, 4 ), wxGBSpan( 1, 2 ), wxALIGN_RIGHT | wxTOP | wxBOTTOM, 5 );

	m_SizerMain->Add( m_SizerEdit, wxALIGN_LEFT );


	//        m_SizerMain->SetSizeHints(this);

	this->SetSizer( m_SizerMain );

	this->RefreshServerList();

}

ServerListDialog::~ServerListDialog(void)
{
}


void ServerListDialog::RefreshServerList(void)
{
	m_ListCtrl->DeleteAllItems();
	for ( ServerList::Node *node = g_ServerList->GetFirst(); node; node = node->GetNext() )
	{
		Server *current = node->GetData();
		
        wxListItem item;
		int id = m_ListCtrl->GetItemCount();
		item.SetMask(wxLIST_MASK_TEXT);
		item.SetId(id);
		
		item.SetText( current->Host );
		m_ListCtrl->InsertItem( item );
		
		m_ListCtrl->SetItem( id, 1, current->TPTest_Port );

		m_ListCtrl->SetItem( id, 2, current->TCP_Port );

		m_ListCtrl->SetItem( id, 3, current->HTTP_Path);

		m_ListCtrl->SetItem( id, 4, current->FTP_Path);

		if( current->IsICMP )
		  m_ListCtrl->SetItem( id, 5, wxT("X") );

		if( current->FTP_Path.Trim().Trim(false).Length() == 0 )
		{
			current->EnableThroughputFTP = false;
		}
		else
		{
			current->EnableThroughputFTP = true;
		}

		if( current->HTTP_Path.Trim().Trim(false).Length() == 0 )
		{
			current->EnableThroughputHTTP = false;
		}
		else
		{
			current->EnableThroughputHTTP = true;
		}

		if( current->TCP_Port.Trim().Trim(false).Length() == 0 )
		{
			current->EnableAvailabilityTCP = false;
		}
		else
		{
			current->EnableAvailabilityTCP = true;
		}

		if( current->TPTest_Port.Trim().Trim(false).Length() == 0 )
		{
			current->EnableThroughputTPTest = false;
		}
		else
		{
			current->EnableThroughputTPTest = true;
		}

		if( !current->IsICMP )
		{
			current->EnableAvailabilityICMP = false;
		}
		else
		{
			current->EnableAvailabilityICMP = true;
		}


	}
}

void ServerListDialog::OnItemSelected(wxListEvent& event)
{
	m_SelectedIndex = event.GetIndex();

	Server *s = g_ServerList->Item( m_SelectedIndex )->GetData();

	m_TextCtrlHost->SetValue( s->Host );
	m_TextCtrlTCPPort->SetValue( s->TCP_Port );
	m_TextCtrlTPTestPort->SetValue( s->TPTest_Port );

	if( s->IsICMP )
		m_CheckBoxICMP->SetValue(true);
	else
		m_CheckBoxICMP->SetValue(false);

	m_TextCtrlHTTP_Path->SetValue( s->HTTP_Path );
	m_TextCtrlFTP_Path->SetValue( s->FTP_Path );
}

void ServerListDialog::OnButtonAdd(wxCommandEvent& event)
{
	Server *s = new Server();

	s->Host = m_TextCtrlHost->GetValue();
	s->TCP_Port = m_TextCtrlTCPPort->GetValue();
	s->TPTest_Port = m_TextCtrlTPTestPort->GetValue();

	if( m_CheckBoxICMP->IsChecked() )
		s->IsICMP = true;
	else
		s->IsICMP = false;

	s->HTTP_Path = m_TextCtrlHTTP_Path->GetValue();
	s->FTP_Path = m_TextCtrlFTP_Path->GetValue();

	s->IsUserCreated = true;

	g_ServerList->Append( s );

	RefreshServerList();
	RefreshMainWindow();
}

void ServerListDialog::OnButtonUpdate(wxCommandEvent& event)
{
	if( m_SelectedIndex < 0 || m_SelectedIndex >= (int)g_ServerList->GetCount() )
	{
		return;
	}

	Server *s = g_ServerList->Item( m_SelectedIndex )->GetData();

	s->Host = m_TextCtrlHost->GetValue();
	s->TCP_Port = m_TextCtrlTCPPort->GetValue();
	s->TPTest_Port = m_TextCtrlTPTestPort->GetValue();

	if( m_CheckBoxICMP->IsChecked() )
		s->IsICMP = true;
	else
		s->IsICMP = false;

	s->HTTP_Path = m_TextCtrlHTTP_Path->GetValue();
	s->FTP_Path = m_TextCtrlFTP_Path->GetValue();

	s->IsUserCreated = true;

	RefreshServerList();
	RefreshMainWindow();
}

void ServerListDialog::OnButtonDelete(wxCommandEvent& event)
{
	if( m_SelectedIndex > -1 )
	{
		g_ServerList->Erase( g_ServerList->Item( m_SelectedIndex ) );
		m_SelectedIndex = -1;
		RefreshServerList();
		RefreshMainWindow();
	}
}

void ServerListDialog::OnButtonRefresh(wxCommandEvent& event)
{
	ServerList::Node *node = g_ServerList->GetFirst();

	while( node )
	{
		ServerList::Node *nextnode = node->GetNext();

		Server *current = node->GetData();	

		if( !current->IsUserCreated )
		{
			// We dont have to delete the actual Server object
			// wxList does this for us.

			// Delete the node from the list
			g_ServerList->DeleteNode( node );

		}
		node = nextnode;
	}

	AppConfig *conf = AppConfig::GetInstance();
	conf->RemoteLoadServerList();
	RefreshServerList();
	RefreshMainWindow();
}

void ServerListDialog::OnButtonClose(wxCommandEvent& event)
{
	AppConfig *conf = AppConfig::GetInstance();
	conf->SaveServerList();
	this->Close();
}

void ServerListDialog::RefreshMainWindow(void)
{
	MainFrame::GetInstance()->RefreshPanels();
}
