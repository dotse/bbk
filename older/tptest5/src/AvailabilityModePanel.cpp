#include "AvailabilityModePanel.h"

#include "MainFrame.h"
#include "TestMarshall.h"
#include "ResultLog.h"
#include "GraphPanel.h"
#include "AvailabilityDetail.h"

enum { wxID_BUTTON_REMOVE_RESULT = wxID_HIGHEST + 1,
	wxID_AVAILABILITY,
	wxID_AVAILABILITY_BOOK,
	wxID_AVAILABILITY_BOOK_PAGE_SETTINGS,
	wxID_AVAILABILITY_BOOK_PAGE_SETTINGS_LIST,
	wxID_AVAILABILITY_BOOK_PAGE_SETTINGS_CHECKBOX_ENABLED,
	wxID_AVAILABILITY_BOOK_PAGE_SETTINGS_CHECKBOX_ICMP,
	wxID_AVAILABILITY_BOOK_PAGE_SETTINGS_CHECKBOX_TCP,
	wxID_AVAILABILITY_BOOK_PAGE_SETTINGS_BUTTON_UPDATE,
	wxID_AVAILABILITY_BOOK_PAGE_RESULT_TEXT,
	wxID_AVAILABILITY_BOOK_PAGE_RESULT_TEXT_LIST
 };

BEGIN_EVENT_TABLE( AvailabilityModePanel, wxPanel )
	EVT_BUTTON(wxID_START_BUTTON, AvailabilityModePanel::OnStartTest)
	EVT_LIST_ITEM_SELECTED(wxID_AVAILABILITY_BOOK_PAGE_SETTINGS_LIST, AvailabilityModePanel::OnSettingsListItemSelected)
	EVT_LIST_ITEM_ACTIVATED(wxID_AVAILABILITY_BOOK_PAGE_RESULT_TEXT_LIST, AvailabilityModePanel::OnResultDetail)
	EVT_CHECKBOX(wxID_AVAILABILITY_BOOK_PAGE_SETTINGS_CHECKBOX_ENABLED, AvailabilityModePanel::OnCheckBoxEnable)
	EVT_CHECKBOX(wxID_AVAILABILITY_BOOK_PAGE_SETTINGS_CHECKBOX_TCP, AvailabilityModePanel::OnCheckBoxTCP)
	EVT_CHECKBOX(wxID_AVAILABILITY_BOOK_PAGE_SETTINGS_CHECKBOX_ICMP, AvailabilityModePanel::OnCheckBoxICMP)
	EVT_BUTTON(wxID_BUTTON_REMOVE_RESULT, AvailabilityModePanel::OnRemoveResult)
	EVT_LIST_KEY_DOWN(wxID_AVAILABILITY_BOOK_PAGE_RESULT_TEXT_LIST, AvailabilityModePanel::OnResultListKeyDown)
END_EVENT_TABLE()

AvailabilityModePanel::AvailabilityModePanel(wxWindow *parent )
:TestPanel( parent, wxID_AVAILABILITY, wxDefaultPosition, wxDefaultSize )
{
  this->SetSBToolTip( wxT("Starta ett test som mäter tillgänlighet.\nDe servrar som testet kommer att utföras mot ser du under fliken inställningar.") );

	m_SelectedSettingsServer = NULL;
	m_ServerList = new ServerList();

	// Create the sizers
	m_SizerPageSettings = new wxBoxSizer( wxVERTICAL );
	m_SizerPageTextResult = new wxBoxSizer( wxVERTICAL );
	m_SizerPageGraphResult = new wxBoxSizer( wxVERTICAL );

	// Create Result Notebook
	m_BookResult = new wxNotebook( this, wxID_AVAILABILITY_BOOK );

	// Add book to main sizer
	m_SizerMain->Add( m_BookResult, 
				1, 
				wxEXPAND | 
				wxALL |
				wxALIGN_CENTER, 
				0 );

	// Create and add the pages to book
	m_PageGraphResult = new GraphPanel( m_BookResult, wxID_AVAILABILITY_BOOK_PAGE_RESULT_GRAPH );
	m_PageTextResult = new wxPanel( m_BookResult, wxID_AVAILABILITY_BOOK_PAGE_RESULT_TEXT );
	m_PageSettings = new wxPanel( m_BookResult, wxID_AVAILABILITY_BOOK_PAGE_SETTINGS );

	m_BookResult->AddPage( m_PageGraphResult, wxT("Resultat som graf") );
	m_BookResult->AddPage( m_PageTextResult, wxT("Resultat som lista") );
	m_BookResult->AddPage( m_PageSettings, wxT("Inställningar") );

	// Create the Settings List control and create it's columns
	m_ListSettings = new wxListCtrl( 
			m_PageSettings, 
			wxID_AVAILABILITY_BOOK_PAGE_SETTINGS_LIST, 
			wxDefaultPosition, 
			wxDefaultSize, 
			wxLC_REPORT | wxLC_SINGLE_SEL );

	m_ListSettings->InsertColumn(0, wxT(" "), wxLIST_FORMAT_LEFT, 25);
	m_ListSettings->InsertColumn(1, wxT("Adress"), wxLIST_FORMAT_LEFT, 250);
	m_ListSettings->InsertColumn(2, wxT("TCP port"), wxLIST_FORMAT_LEFT, 172);
	m_ListSettings->InsertColumn(3, wxT("TCP"), wxLIST_FORMAT_LEFT, 60);
	m_ListSettings->InsertColumn(4, wxT("ICMP"), wxLIST_FORMAT_LEFT, 60);

	m_SizerPageSettings->Add(	m_ListSettings,
								1,
								wxEXPAND |
								wxALIGN_CENTER |
								wxBOTTOM,
								10 );

	// Create the Result List control and create it's columns
	m_ListResult = new wxListCtrl( 
			m_PageTextResult, 
			wxID_AVAILABILITY_BOOK_PAGE_RESULT_TEXT_LIST, 
			wxDefaultPosition, 
			wxDefaultSize, 
			wxLC_REPORT );

	m_ListResult->InsertColumn(0, wxT("Datum"), wxLIST_FORMAT_LEFT, 250);
	m_ListResult->InsertColumn(1, wxT("Tillgänglighet"), wxLIST_FORMAT_LEFT, 172);

	m_SizerPageTextResult->Add( m_ListResult, 
								1,
								wxEXPAND |
								wxALL |
								wxALIGN_CENTER,
								0 );

	m_SizerPageTextResult->Add( new wxButton( m_PageTextResult, wxID_BUTTON_REMOVE_RESULT, wxT("Ta bort valda resultat") ), 
								0,
								wxALL |
								wxALIGN_CENTER,
								1 );


	m_SizerPageSettingsEdit = new wxBoxSizer( wxHORIZONTAL );


	m_SizerPageSettingsEdit->Add(
				     new wxStaticText( m_PageSettings, wxID_ANY, wxT("Inställningar för valt system:") ),
		1,
		wxRIGHT |
		wxLEFT,
		10 );


	// Enabled checkbox

	m_CheckBoxEnable = new wxCheckBox( m_PageSettings, 
					   wxID_AVAILABILITY_BOOK_PAGE_SETTINGS_CHECKBOX_ENABLED, 
					   wxT("Aktiverad") );

	m_CheckBoxEnable->SetToolTip( wxString( wxT("Aktivera test av tillgänglighet för denna server") ) );

	m_SizerPageSettingsEdit->Add(
					m_CheckBoxEnable,
					1,
					wxRIGHT |
					wxBOTTOM,
					10 );

	// TCP checkbox

	m_CheckBoxEnableTCP = new wxCheckBox( m_PageSettings, wxID_AVAILABILITY_BOOK_PAGE_SETTINGS_CHECKBOX_TCP, wxT("TCP aktiverad") );
	m_CheckBoxEnableTCP->SetToolTip( wxString( wxT("Kör TCP ping tester mot denna server") ) );

	m_SizerPageSettingsEdit->Add(
					m_CheckBoxEnableTCP,
					1,
					wxRIGHT |
					wxBOTTOM,
					10 );

	// ICMP checkbox

	m_CheckBoxEnableICMP = new wxCheckBox( m_PageSettings, wxID_AVAILABILITY_BOOK_PAGE_SETTINGS_CHECKBOX_ICMP, wxT("ICMP aktiverad") );
	m_CheckBoxEnableICMP->SetToolTip( wxString( wxT("Kör ICMP ping tester mot denna server") ) );

	m_SizerPageSettingsEdit->Add(
					m_CheckBoxEnableICMP,
					1,
					wxRIGHT |
					wxBOTTOM,
					10 );

	m_SizerPageSettings->Add( m_SizerPageSettingsEdit, 0, wxALIGN_LEFT );

	this->SetSizer( m_SizerMain );
	m_PageSettings->SetSizer( m_SizerPageSettings );
	m_PageTextResult->SetSizer( m_SizerPageTextResult );
	m_PageGraphResult->SetSizer( m_SizerPageGraphResult );

	m_SizerPageTextResult->Fit( m_ListResult );

	m_SizerMain->SetSizeHints( this );
	m_SizerPageTextResult->SetSizeHints( m_PageTextResult );
	m_SizerPageSettings->SetSizeHints( m_PageSettings );


	// --------

	RefreshSettingsList();
	RefreshResultList();
}

AvailabilityModePanel::~AvailabilityModePanel(void)
{
	delete m_ServerList;
}

void AvailabilityModePanel::OnTestCompleted_Wrapper(void* obj, bool arg)
{
	AvailabilityModePanel *panel = (AvailabilityModePanel*)obj;
	panel->OnTestCompleted( arg );
}

void AvailabilityModePanel::OnTestCompleted(bool abort)
{
	TestMarshall *tm = TestMarshall::GetInstance();
	wxString strKey;

	if( !abort )
	{
		struct tcp_ping_arg_struct *tp = tm->GetTCPPingStruct();
		struct icmp_ping_arg_struct *ip = tm->GetICMPPingStruct();

		ResultLog *rlog = ResultLog::GetInstance();

		StringValueList *row = new StringValueList();

		row->Append( new wxString(wxDateTime::Now().Format( wxT("%Y-%m-%d %H:%M:%S") )) );

		wxString *TCP_Connects = new wxString();
		*TCP_Connects << (int)tp->connects;
		row->Append( TCP_Connects );

		wxString *TCP_Hosts = new wxString();
		*TCP_Hosts << (int)tp->number_hosts;
		row->Append( TCP_Hosts );

		wxString *ICMP_Sent = new wxString();
		*ICMP_Sent << (int)ip->packets_sent;
		row->Append( ICMP_Sent );

		wxString *ICMP_Rec = new wxString();
		*ICMP_Rec << (int)ip->packets_received;
		row->Append( ICMP_Rec );

		wxString *ICMP_HostCount = new wxString();
		*ICMP_HostCount << (int)ip->number_hosts;
		row->Append( ICMP_HostCount );

		ServerList *slist = (ServerList*)ip->userdata;
		wxString ICMP_values;
		for( int i = 0 ; i < (int)ip->number_hosts ; i++ )
		{
			Server *server = slist->Item( i )->GetData();
			for( int ii = 0 ; ii < (int)ip->num_packets ; ii++ )
			{
				struct valuestruct *vs = 
				  &ip->values[(i*(int)ip->num_packets)+ii];
				ICMP_values << server->Host << wxT("|") 
					    << vs->timestamp.tv_sec 
					    << wxT("|") << vs->value << wxT("|");
			}
		}
		wxString *ICMP_val = new wxString( ICMP_values.BeforeLast( wxT('|') ) );
		row->Append( ICMP_val );

		slist = (ServerList*)tp->userdata;
		wxString TCP_values;
		for( int i = 0 ; i < (int)tp->number_hosts ; i++ )
		{
			struct valuestruct *vs = &tp->values[i];

			Server *server = slist->Item( i )->GetData();

			TCP_values << server->Host << wxT("|") << server->TCP_Port << wxT("|") 
				   << vs->timestamp.tv_sec << wxT("|") << vs->value << wxT("|");
		}
		wxString *TCP_val = new wxString( TCP_values.BeforeLast( wxT('|') ) );
		row->Append( TCP_val );

		rlog->AddResult( wxT("availability"), *row );

		RefreshResultList();
		this->m_PageGraphResult->Refresh();
	}

	tm->DeinitTestEngine(abort);
	PostTest(abort);
}


void AvailabilityModePanel::RefreshSettingsList(void)
{
	m_ListSettings->DeleteAllItems();
	m_ServerList->Clear();
	for ( ServerList::Node *node = g_ServerList->GetFirst(); node; node = node->GetNext() )
	{
		Server *current = node->GetData();
		
		if( !current->IsICMP &&
			current->TCP_Port.Trim().Trim(false).Length() == 0 )
		{
			continue;
		}

		m_ServerList->Append( current );

        wxListItem item;
		int id = m_ListSettings->GetItemCount();
		item.SetMask(wxLIST_MASK_TEXT);
		item.SetId(id);

		wxString Enabled = wxT("X");

		if( !current->EnableAvailability )
		{
			item.SetTextColour( wxColour(180,180,180) );
			Enabled = wxT("");
		}

		
		item.SetText( Enabled );
		item.SetData( current );

		m_ListSettings->InsertItem( item );

		
		m_ListSettings->SetItem( id, 1, current->Host );
		m_ListSettings->SetItem( id, 2, current->TCP_Port );

		if( current->EnableAvailabilityTCP )
		  m_ListSettings->SetItem( id, 3, wxT("X") );

		if( current->IsICMP && current->EnableAvailabilityICMP )
		  m_ListSettings->SetItem( id, 4, wxT("X") );

	}
}

void AvailabilityModePanel::OnSettingsListItemSelected(wxListEvent& event)
{
	wxListItem item = event.GetItem();
	
	long l = item.GetData();
	Server *s = *((Server**)&l);

	m_SelectedSettingsServerIndex = item.GetId();
	m_SelectedSettingsServer = s;

	m_CheckBoxEnable->SetValue( s->EnableAvailability );
	m_CheckBoxEnableTCP->SetValue( s->EnableAvailabilityTCP );
	m_CheckBoxEnableICMP->SetValue( s->EnableAvailabilityICMP );
}

void AvailabilityModePanel::OnCheckBoxEnable(wxCommandEvent &event)
{
	if( m_SelectedSettingsServer != NULL )
	{
		m_SelectedSettingsServer->EnableAvailability = m_CheckBoxEnable->GetValue();
		RefreshSettingsList();
		FocusList();
	}
}

void AvailabilityModePanel::OnCheckBoxTCP(wxCommandEvent &event)
{
	if( m_SelectedSettingsServer != NULL )
	{
		if( m_SelectedSettingsServer->TCP_Port.Trim().Trim(false).Length() == 0 )
		{
			m_CheckBoxEnableTCP->SetValue(false);
		}

		m_SelectedSettingsServer->EnableAvailabilityTCP = m_CheckBoxEnableTCP->GetValue();
		RefreshSettingsList();
		FocusList();
	}
}

void AvailabilityModePanel::OnCheckBoxICMP(wxCommandEvent &event)
{
	if( m_SelectedSettingsServer != NULL )
	{
		if( !m_SelectedSettingsServer->IsICMP )
		{
			m_CheckBoxEnableICMP->SetValue(false);
		}

		m_SelectedSettingsServer->EnableAvailabilityICMP = m_CheckBoxEnableICMP->GetValue();
		RefreshSettingsList();
		FocusList();
	}
}

void AvailabilityModePanel::RefreshResultList(void)
{
	ResultLog *rlog = ResultLog::GetInstance();
	Result *result = rlog->GetResults( wxT("availability") );

	m_ListResult->DeleteAllItems();

	for( int i = result->GetRowCount()-1 ; i > -1  ; i-- )
	{
		StringValueList *row = result->GetRow( i );

		wxListItem item;
		int id = m_ListResult->GetItemCount();
		item.SetMask(wxLIST_MASK_TEXT);
		item.SetId(id);
		item.SetData( (int)i );
		item.SetText( *row->Item(0)->GetData() );

		int iTCP_Connects;
		row->Item(1)->GetData()->ToLong( (long*)&iTCP_Connects );
		int iTCP_HostCount;
		row->Item(2)->GetData()->ToLong( (long*)&iTCP_HostCount);

		int iICMP_Sent;
		row->Item(3)->GetData()->ToLong( (long*)&iICMP_Sent );
		int iICMP_Rec;
		row->Item(4)->GetData()->ToLong( (long*)&iICMP_Rec);
		int iICMP_HostCount;
		row->Item(5)->GetData()->ToLong( (long*)&iICMP_HostCount);

		long PacketsPerHost = 0;

		if( iICMP_HostCount > 0 )
		{
			PacketsPerHost = iICMP_Sent / iICMP_HostCount;
		}
		
		int iICMP_RespondingHosts = 0;

		wxStringTokenizer tkzICMP( *row->Item(6)->GetData(), wxT("|"));

		while ( tkzICMP.HasMoreTokens() )
		{
			int PacketsForThisHost = 0;
			wxString host;
			wxString timestamp;

			for( int i = 0 ; i < PacketsPerHost ; i++ )
			{
				host		= tkzICMP.GetNextToken();
				timestamp	= tkzICMP.GetNextToken();

				// this should not happen
				if( timestamp.Length() == 0 )
					break;

				wxString val		= tkzICMP.GetNextToken();
				double dval;
				val.ToDouble( &dval );

				if( dval > 0 )
				{
					PacketsForThisHost++;
				}
			}
	
			if( PacketsForThisHost > (PacketsPerHost/2) )
				iICMP_RespondingHosts++;

		}

		wxString Availability;

		int sumHosts = iTCP_HostCount + iICMP_HostCount;
		int sumConnects = iTCP_Connects + iICMP_RespondingHosts;

		if( sumHosts > 0 )
		{
		  Availability << (sumConnects*100 / sumHosts);
		  
		  Availability << wxT("% (") << sumConnects 
			       << wxT("/") << sumHosts << wxT(")");
		}
		else
		{
		  Availability << wxT("0% (0/0)");
		}


		m_ListResult->InsertItem( item );
		m_ListResult->SetItem( id, 1, Availability );
	}

}

void AvailabilityModePanel::FocusList(void)
{
  this->m_ListSettings->SetFocus();
  this->m_ListSettings->SetItemState(m_SelectedSettingsServerIndex, 
				     wxLIST_STATE_SELECTED, 
				     wxLIST_STATE_SELECTED);
}


void AvailabilityModePanel::OnRemoveResult(wxCommandEvent& event)
{
	ResultLog *rl = ResultLog::GetInstance();
	Result *r = rl->GetResults( wxT("availability") );

    long id = m_ListResult->GetNextItem(-1, wxLIST_NEXT_ALL,
                                        wxLIST_STATE_SELECTED);
	while( id != -1 )
	{	
		wxListItem item;
		item.m_mask = wxLIST_MASK_DATA;
		item.SetId( id );
		m_ListResult->GetItem( item );

		int index = (int)item.GetData();

		r->DeleteRow( index );

		id = m_ListResult->GetNextItem(id, wxLIST_NEXT_ALL,
                                       wxLIST_STATE_SELECTED);
	}
	RefreshResultList();
}

void AvailabilityModePanel::OnResultDetail(wxListEvent& event)
{
	long id = -1;
                                        
	while( id == -1 )
	{
		id = m_ListResult->GetNextItem(-1, wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
	}

	if( id != -1 )
	{
		wxListItem item;
		item.SetId( id );
		m_ListResult->GetItem( item );

		wxString s;
		s << id;

		AvailabilityDetail *ad = new AvailabilityDetail(this, m_ListResult->GetItemCount() - 1 - id);
		ad->Show();
	}
}

bool AvailabilityModePanel::ExecuteTest(void)
{
	TestMarshall *tm = TestMarshall::GetInstance();

	MainFrame::GetInstance()->PreStartTest((void*)this, (AvailabilityModePanel::OnTestCompleted_Wrapper), (AvailabilityModePanel::ExecuteTest_Wrapper));

	tm->InitTestEngine( AVAILABILITY );

	if( tm->SetupAvailabilityTest(this->m_ServerList) )
	{
		tm->Execute();
		MainFrame::GetInstance()->StartTest();
		return true;
	}
	else
	{
	  wxMessageBox( wxT("Inga servrar hittade eller inställda för aktuellt test.") );
	  tm->DeinitTestEngine();
	  PostTest(true);
	  return false;
	}
}

bool AvailabilityModePanel::ExecuteTest_Wrapper(void* obj)
{
	AvailabilityModePanel *amp = (AvailabilityModePanel*)obj;
	return amp->ExecuteTest();
}

