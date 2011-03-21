#include "RTTPacketlossModePanel.h"

#include "MainFrame.h"
#include "TestMarshall.h"
#include "ResultLog.h"
#include "GraphPanel.h"
#include "RTTPacketLossDetail.h"

enum { wxID_BUTTON_REMOVE_RESULT = wxID_HIGHEST + 1,
       wxID_RTT,
       wxID_RTT_START_BUTTON,
       wxID_RTT_BOOK,
       wxID_RTT_BOOK_PAGE_SETTINGS,
       wxID_RTT_BOOK_PAGE_SETTINGS_LIST,
       wxID_RTT_BOOK_PAGE_SETTINGS_CHECKBOX_ENABLED,
       wxID_RTT_BOOK_PAGE_RESULT_TEXT,
       wxID_RTT_BOOK_PAGE_RESULT_TEXT_LIST
 };

BEGIN_EVENT_TABLE( RTTPacketlossModePanel, wxPanel )
	EVT_BUTTON(wxID_START_BUTTON, RTTPacketlossModePanel::OnStartTest)
	EVT_NOTEBOOK_PAGE_CHANGED(wxID_RTT_BOOK, RTTPacketlossModePanel::OnPageChanged)
	EVT_LIST_ITEM_SELECTED(wxID_RTT_BOOK_PAGE_SETTINGS_LIST, RTTPacketlossModePanel::OnSettingsListItemSelected)
	EVT_LIST_ITEM_ACTIVATED(wxID_RTT_BOOK_PAGE_RESULT_TEXT_LIST, RTTPacketlossModePanel::OnResultDetail)
	EVT_CHECKBOX(wxID_RTT_BOOK_PAGE_SETTINGS_CHECKBOX_ENABLED, RTTPacketlossModePanel::OnCheckBoxEnable)
	EVT_LIST_KEY_DOWN(wxID_RTT_BOOK_PAGE_RESULT_TEXT_LIST, RTTPacketlossModePanel::OnResultListKeyDown)
	EVT_BUTTON(wxID_BUTTON_REMOVE_RESULT, RTTPacketlossModePanel::OnRemoveResult)
END_EVENT_TABLE()

RTTPacketlossModePanel::RTTPacketlossModePanel( wxWindow *parent )
:TestPanel( parent, wxID_RTT, wxDefaultPosition, wxDefaultSize )
{
	this->SetSBToolTip( 
			   wxString( wxT("Starta ett test som mäter svarstider och paketförluster.\nDe tester som kommer att utföras ser du under fliken inställningar.") ) 
			    );

	m_Initialized = false;
	m_SelectedSettingsServer = NULL;
	m_ServerList = new ServerList();

	// Create the sizers
	m_SizerPageSettings = new wxBoxSizer( wxVERTICAL );
	m_SizerPageTextResult = new wxBoxSizer( wxVERTICAL );
	m_SizerPageGraphRTTResult = new wxBoxSizer( wxVERTICAL );
	m_SizerPageGraphPLResult = new wxBoxSizer( wxVERTICAL );
	m_SizerPageGraphJitterResult = new wxBoxSizer( wxVERTICAL );

	// Create Result Notebook
	m_BookResult = new wxNotebook( this, wxID_RTT_BOOK );

	// Add book to main sizer
	m_SizerMain->Add( m_BookResult, 
				1, 
				wxEXPAND | 
				wxALL |
				wxALIGN_CENTER, 
				0 );

	// Create and add the pages to book
	m_PageSettings = new wxPanel( m_BookResult, wxID_RTT_BOOK_PAGE_SETTINGS );
	m_PageTextResult = new wxPanel( m_BookResult, wxID_RTT_BOOK_PAGE_RESULT_TEXT );
	m_PageGraphRTTResult = new GraphPanel( m_BookResult, wxID_RTT_BOOK_PAGE_RESULT_RTT_GRAPH );
	m_PageGraphPLResult = new GraphPanel( m_BookResult, wxID_RTT_BOOK_PAGE_RESULT_PL_GRAPH );
	m_PageGraphJitterResult = new GraphPanel( m_BookResult, wxID_RTT_BOOK_PAGE_RESULT_JITTER_GRAPH );
	m_BookResult->AddPage( m_PageGraphRTTResult, wxT("Svarstider som graf") );
	m_BookResult->AddPage( m_PageGraphPLResult, wxT("Paketförluster som graf") );
	m_BookResult->AddPage( m_PageGraphJitterResult, wxT("Jitter som graf") );
	m_BookResult->AddPage( m_PageTextResult, wxT("Resultat som lista") );
	m_BookResult->AddPage( m_PageSettings, wxT("Inställningar") );


	// Create the Settings List control and create it's columns
	m_ListSettings = new wxListCtrl( 
			m_PageSettings, 
			wxID_RTT_BOOK_PAGE_SETTINGS_LIST, 
			wxDefaultPosition, 
			wxDefaultSize, 
			wxLC_REPORT | wxLC_SINGLE_SEL );

	m_ListSettings->InsertColumn(0, wxT(" "), wxLIST_FORMAT_LEFT, 25);
	m_ListSettings->InsertColumn(1, wxT("Adress"), wxLIST_FORMAT_LEFT, 650);

	m_SizerPageSettings->Add(	m_ListSettings,
								1,
								wxEXPAND |
								wxALIGN_CENTER |
								wxBOTTOM,
								10 );

	// Create the Result List control and create it's columns
	m_ListResult = new wxListCtrl( 
			m_PageTextResult, 
			wxID_RTT_BOOK_PAGE_RESULT_TEXT_LIST, 
			wxDefaultPosition, 
			wxDefaultSize, 
			wxLC_REPORT );

	m_ListResult->InsertColumn(0, wxT("Datum"), wxLIST_FORMAT_LEFT, 250);
	m_ListResult->InsertColumn(1, wxT("Svarstid"), wxLIST_FORMAT_LEFT, 200);
	m_ListResult->InsertColumn(2, wxT("Paketförlust"), wxLIST_FORMAT_LEFT, 100);
	m_ListResult->InsertColumn(3, wxT("Jitter"), wxLIST_FORMAT_LEFT, 100);

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

	m_CheckBoxEnable = new wxCheckBox( m_PageSettings, wxID_RTT_BOOK_PAGE_SETTINGS_CHECKBOX_ENABLED, wxT("Aktiverad") );
	m_CheckBoxEnable->SetToolTip( wxString(wxT("Aktivera test av svarstider och paketförluster för denna server")) );

	m_SizerPageSettingsEdit->Add(
					m_CheckBoxEnable,
					1,
					wxRIGHT |
					wxBOTTOM,
					10 );


	m_SizerPageSettings->Add( m_SizerPageSettingsEdit, 0, wxALIGN_LEFT );

	this->SetSizer( m_SizerMain );
	m_SizerMain->SetSizeHints( this );

	m_PageSettings->SetSizer( m_SizerPageSettings );
	m_PageTextResult->SetSizer( m_SizerPageTextResult );
	m_PageGraphRTTResult->SetSizer( m_SizerPageGraphRTTResult );
	m_PageGraphPLResult->SetSizer( m_SizerPageGraphPLResult );
	m_PageGraphJitterResult->SetSizer( m_SizerPageGraphJitterResult );

	m_SizerPageSettings->SetSizeHints( m_PageSettings );
	m_SizerPageTextResult->SetSizeHints( m_PageTextResult );

	// --------
	m_Initialized = true;
	RefreshGUI();
}

RTTPacketlossModePanel::~RTTPacketlossModePanel(void)
{
	delete m_ServerList;
}

void RTTPacketlossModePanel::OnTestCompleted_Wrapper(void* obj, bool arg)
{
	RTTPacketlossModePanel *panel = (RTTPacketlossModePanel*)obj;
	panel->OnTestCompleted( arg );
}

void RTTPacketlossModePanel::OnTestCompleted(bool abort)
{

	TestMarshall *tm = TestMarshall::GetInstance();

	if( !abort )
	{
		char charbuf[200];
		struct icmp_ping_arg_struct *ip = tm->GetICMPPingStruct();

		ResultLog *rlog = ResultLog::GetInstance();

		StringValueList *row = new StringValueList();

		row->Append( new wxString(wxDateTime::Now().Format( wxT("%Y-%m-%d %H:%M:%S")) ) );

		wxString *ICMP_Hosts = new wxString();
		*ICMP_Hosts << (int)ip->number_hosts;
		row->Append( ICMP_Hosts );

		wxString *ICMP_Responding = new wxString();
		*ICMP_Responding << (int)ip->responding_hosts;
		row->Append( ICMP_Responding );

		wxString *ICMP_PkSent = new wxString();
		*ICMP_PkSent << (int)ip->packets_sent;
		row->Append( ICMP_PkSent );

		wxString *ICMP_PkRec = new wxString();
		*ICMP_PkRec << (int)ip->packets_received;
		row->Append( ICMP_PkRec );

		if( ip->responding_hosts > 0 )
		{
		  sprintf(charbuf, "%.2f", ip->rtt_max*1000.0);
		  wxString *ICMP_RTTMax = new wxString();
		  *ICMP_RTTMax << wxString( charbuf, wxConvUTF8);
		  row->Append( ICMP_RTTMax );
		  
		  sprintf(charbuf, "%.2f", ip->rtt_min*1000.0);
		  wxString *ICMP_RTTMin = new wxString();
		  *ICMP_RTTMin << wxString( charbuf, wxConvUTF8);
		  row->Append( ICMP_RTTMin );
		  
		  sprintf(charbuf, "%.2f", ip->rtt_avg*1000.0);
		  wxString *ICMP_RTTAvg = new wxString();
		  *ICMP_RTTAvg << wxString( charbuf, wxConvUTF8);
		  row->Append( ICMP_RTTAvg );
		}
		else
		{
		  row->Append( new wxString( wxT("-1") ) );
		  row->Append( new wxString( wxT("-1") ) );
		  row->Append( new wxString( wxT("-1") ) );
		}

		ServerList *slist = (ServerList*)ip->userdata;
		wxString ICMP_values;
		wxString *SessionJitter;
		double sessionjitter = 0.0;
		int sessionjittercount = 0;
		for( unsigned int i = 0 ; i < ip->number_hosts ; i++ )
		{
			double avg = 0;
			double jitter = 0;
			unsigned int ii;
			int recvd = 0;

			// Go through the valuestructs and calculate average rtt
			for( ii = 0 ; ii < ip->num_packets ; ii++ )
			{
				struct valuestruct *vs = &ip->values[i*ip->num_packets+ii];
				
				if( vs->value != -1 )
				{
					avg += vs->value;
					recvd++;
				}
			}

			if( recvd > 0 )
				avg = avg / (double)recvd;
			else
				avg = -1;

			// calculate jitter, based on avg rtt
			if (recvd > 1) 
			{
				for ( ii = 0; ii < ip->num_packets; ii++ ) 
				{
					struct valuestruct *vs = &ip->values[i*ip->num_packets+ii];
					if (vs->value != -1)
						jitter += fabs(vs->value - avg);
				}
				jitter /= (double)(recvd-1);  // yes, recvd-1 here
			}
			else
			{
				jitter = -1;
			}

			if (jitter != -1)
			{
				sessionjitter += jitter;
				sessionjittercount += 1;
			}

			Server *server = slist->Item( i )->GetData();
			ICMP_values << server->Host << wxT("|") 
				    << ip->num_packets << wxT("|") 
				    << recvd << wxT("|") << avg 
				    << wxT("|") << jitter << wxT("|");
		}

		// calculate average jitter for all tests in this session
		if (sessionjittercount > 0)
		{
			sessionjitter /= (double)sessionjittercount;
			sprintf(charbuf, "%.2f", sessionjitter*1000.0);
			SessionJitter = new wxString(charbuf, wxConvUTF8);
		}
		else
		{
		  SessionJitter = new wxString( wxT("-1") );
		}

		row->Append( SessionJitter );

		wxString *ICMP_val = new wxString( ICMP_values.BeforeLast( wxT('|') ) );
		row->Append( ICMP_val );


		rlog->AddResult( wxString( wxT("rtt") ), *row );

		RefreshResultList();
		this->m_PageGraphPLResult->Refresh();
		this->m_PageGraphRTTResult->Refresh();
	}

	tm->DeinitTestEngine(abort);
	PostTest(abort);
}

void RTTPacketlossModePanel::RefreshResultList(void)
{
	ResultLog *rlog = ResultLog::GetInstance();

	Result *result = rlog->GetResults( wxString( wxT("rtt") ) );

	m_ListResult->DeleteAllItems();

	for( int i = result->GetRowCount()-1 ; i > -1  ; i-- )
	{
		StringValueList *row = result->GetRow( i );

		wxListItem item;
		int id = m_ListResult->GetItemCount();
		item.SetMask(wxLIST_MASK_TEXT);
		item.SetId(id);
		item.SetData( i );
		
		item.SetText( *row->Item(0)->GetData() );
		m_ListResult->InsertItem( item );

		int iICMP_HostCount;
		row->Item(1)->GetData()->ToLong( (long*)&iICMP_HostCount);
		int iICMP_Responding;
		row->Item(2)->GetData()->ToLong( (long*)&iICMP_Responding );
		int iICMP_PkSent;
		row->Item(3)->GetData()->ToLong( (long*)&iICMP_PkSent);
		int iICMP_PkRec;
		row->Item(4)->GetData()->ToLong( (long*)&iICMP_PkRec);

		wxString *ICMP_RTTMax	= row->Item(5)->GetData();
		wxString *ICMP_RTTMin	= row->Item(6)->GetData();
		wxString *ICMP_RTTAvg	= row->Item(7)->GetData();
		wxString *ICMP_Jitter	= row->Item(8)->GetData();
		
		wxString RTT;
		wxString Packetloss;
		wxString Jitter;

		if( iICMP_Responding > 0 )
		{
		  RTT << *ICMP_RTTAvg << wxT("ms (max:") 
		      << *ICMP_RTTMax << wxT(" min:") 
		      << *ICMP_RTTMin << wxT(")");


			double dICMP_Jitter;
			ICMP_Jitter->ToDouble( &dICMP_Jitter );
			Jitter << *ICMP_Jitter << wxT("ms");

		}
		else
		{
		  RTT << wxT("-");
		  Jitter << wxT("-");
		}

		if( iICMP_PkSent > 0 )
		{
		  Packetloss << (iICMP_PkSent-iICMP_PkRec)*100/iICMP_PkSent;
		  Packetloss << wxT("% (") << (iICMP_PkSent-iICMP_PkRec) 
			     << wxT("/") << iICMP_PkSent << wxT(")");
		}
		else
		{
		  Packetloss << wxT("-");
		}


		m_ListResult->SetItem( id, 1, RTT );
		m_ListResult->SetItem( id, 2, Packetloss );
		m_ListResult->SetItem( id, 3, Jitter );
	}

}



void RTTPacketlossModePanel::RefreshSettingsList(void)
{
	m_ListSettings->DeleteAllItems();
	m_ServerList->Clear();

	for ( ServerList::Node *node = g_ServerList->GetFirst(); node; node = node->GetNext() )
	{
		Server *current = node->GetData();
		
		if( !current->IsICMP )
		{
			continue;
		}

		m_ServerList->Append( current );

		wxListItem item;
		int id = m_ListSettings->GetItemCount();
		item.SetMask(wxLIST_MASK_TEXT);
		item.SetId(id);

		wxString Enabled = wxT("X");

		if( !current->EnableRTTPacketloss )
		{
			item.SetTextColour( wxColour(180,180,180) );
			Enabled = wxT("");
		}

		
		item.SetText( Enabled );
		item.SetData( current );

		m_ListSettings->InsertItem( item );

		
		m_ListSettings->SetItem( id, 1, current->Host );

	}
}

void RTTPacketlossModePanel::RefreshGUI(void)
{
	RefreshSettingsList();
	RefreshResultList();
}


void RTTPacketlossModePanel::OnPageChanged(wxNotebookEvent& event)
{
	if( m_Initialized )
	{
		RefreshGUI();
	}
}

void RTTPacketlossModePanel::OnSettingsListItemSelected(wxListEvent& event)
{
	wxListItem item = event.GetItem();
	
	long l = item.GetData();
	Server *s = *((Server**)&l);

	m_SelectedSettingsServerIndex = item.GetId();

	m_SelectedSettingsServer = s;

	m_CheckBoxEnable->SetValue( s->EnableRTTPacketloss );
}

void RTTPacketlossModePanel::OnCheckBoxEnable(wxCommandEvent &event)
{
	if( m_SelectedSettingsServer != NULL )
	{
		m_SelectedSettingsServer->EnableRTTPacketloss = m_CheckBoxEnable->GetValue();
		RefreshSettingsList();
		FocusList();
	}
}

void RTTPacketlossModePanel::FocusList(void)
{
		this->m_ListSettings->SetFocus();
		this->m_ListSettings->SetItemState(m_SelectedSettingsServerIndex, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
}

void RTTPacketlossModePanel::OnResultDetail(wxListEvent& event)
{
	long id = -1;
                                        
	id = m_ListResult->GetNextItem(id, wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);

	if( id == -1 )
		return;

	if( id != -1 )
	{
		wxListItem item;
		item.SetId( id );
		m_ListResult->GetItem( item );

		wxString s;
		s << id;

		RTTPacketLossDetail *rd = new RTTPacketLossDetail(this, m_ListResult->GetItemCount() - 1 - id);
		rd->ShowModal();
//		delete rd;
	}
}

bool RTTPacketlossModePanel::ExecuteTest(void)
{
	TestMarshall *tm = TestMarshall::GetInstance();

	MainFrame::GetInstance()->PreStartTest((void*)this, (RTTPacketlossModePanel::OnTestCompleted_Wrapper), (RTTPacketlossModePanel::ExecuteTest_Wrapper));

	tm->InitTestEngine( RTT );

	if( tm->SetupRTTTest( m_ServerList ) )
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

bool RTTPacketlossModePanel::ExecuteTest_Wrapper(void* obj)
{
	RTTPacketlossModePanel *rmp = (RTTPacketlossModePanel*)obj;
	return rmp->ExecuteTest();
}

void RTTPacketlossModePanel::OnRemoveResult(wxCommandEvent& event)
{
	ResultLog *rl = ResultLog::GetInstance();

	Result *r = rl->GetResults( wxString( wxT("rtt") ) );

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

