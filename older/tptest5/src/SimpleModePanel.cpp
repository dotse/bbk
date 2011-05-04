#include "SimpleModePanel.h"

#include "MainFrame.h"
#include "AdvancedModePanel.h"

// new way of doint the ID on pages
enum { wxID_BUTTON_REMOVE_RESULT = wxID_HIGHEST + 1,
       wxID_SIMPLE,
       wxID_SIMPLE_BOOK_RESULT,
       wxID_SIMPLE_BOOK_RESULT_PAGE_TEXT_LIST,
       wxID_SIMPLE_BOOK_RESULT_PAGE_TEXT
};


BEGIN_EVENT_TABLE( SimpleModePanel, wxPanel )
	EVT_BUTTON(wxID_START_BUTTON, SimpleModePanel::OnStartTest)
	EVT_BUTTON(wxID_BUTTON_REMOVE_RESULT, SimpleModePanel::OnRemoveResult)
	EVT_LIST_ITEM_SELECTED(wxID_SIMPLE_BOOK_RESULT_PAGE_TEXT_LIST, SimpleModePanel::OnResultListItemSelected)
	EVT_LIST_KEY_DOWN(wxID_SIMPLE_BOOK_RESULT_PAGE_TEXT_LIST, SimpleModePanel::OnResultListKeyDown)
END_EVENT_TABLE()




SimpleModePanel::SimpleModePanel( wxWindow *parent )
:TestPanel( parent, wxID_SIMPLE, wxDefaultPosition, wxDefaultSize )
{
  this->SetSBToolTip( wxString( wxT("Starta ett nedströms TCP test.\nTestet kommer bara att utföras mot den första servern i serverlistan.")) 
		      );

	// Create the Text Result Sizer
	m_SizerPageTextResult = new wxBoxSizer( wxVERTICAL );

	// Create Result Notebook
	m_BookResult = new wxNotebook( this, wxID_SIMPLE_BOOK_RESULT );

	// Add book to main sizer
	m_SizerMain->Add( m_BookResult, 
				1, 
				wxEXPAND | 
				wxALL |
				wxALIGN_CENTER, 
				0 );

	// Create and add the pages to book
	m_PageTextResult = new wxPanel( m_BookResult, wxID_SIMPLE_BOOK_RESULT_PAGE_TEXT );
	m_PageGraphResult = new GraphPanel( m_BookResult, wxID_SIMPLE_BOOK_RESULT_PAGE_GRAPH );

	m_BookResult->AddPage( m_PageGraphResult, wxT("Resultat som graf") );
	m_BookResult->AddPage( m_PageTextResult, wxT("Resultat som lista") );


	// Create the Result List control and create it's columns
	m_ListResult = new wxListCtrl( 
			m_PageTextResult, 
			wxID_SIMPLE_BOOK_RESULT_PAGE_TEXT_LIST, 
			wxDefaultPosition, 
			wxDefaultSize, 
			wxLC_REPORT );

	m_ListResult->InsertColumn(0, wxT("Datum"), wxLIST_FORMAT_LEFT, 250);
	m_ListResult->InsertColumn(1, wxT("Bandbredd"), wxLIST_FORMAT_LEFT, 172);

	m_SizerPageTextResult->Add( m_ListResult, 
								1,
								wxEXPAND |
								wxALL |
								wxALIGN_CENTER,
								0 );

	m_SizerPageTextResult->Add( new wxButton( m_PageTextResult, wxID_BUTTON_REMOVE_RESULT, 
						  wxT("Ta bort valda resultat") ), 
				    0,
				    wxALL |
				    wxALIGN_CENTER,
				    1 );


	this->SetSizer( m_SizerMain );
	m_SizerMain->SetSizeHints( this );

	m_PageTextResult->SetSizer( m_SizerPageTextResult );
	m_SizerPageTextResult->SetSizeHints( m_PageTextResult );


	// --------

	RefreshResultList();

}

SimpleModePanel::~SimpleModePanel(void)
{
}

void SimpleModePanel::OnPaintGraphPanel( wxPaintEvent &WXUNUSED(event) )
{
}

bool SimpleModePanel::ExecuteTest_Wrapper(void* obj)
{
	SimpleModePanel *smp = (SimpleModePanel*)obj;
	return smp->ExecuteTest();
}

bool SimpleModePanel::ExecuteTest(void)
{
	TestMarshall *tm = TestMarshall::GetInstance();

	MainFrame::GetInstance()->PreStartTest((void*)this, (SimpleModePanel::OnTestCompleted_Wrapper), (SimpleModePanel::ExecuteTest_Wrapper));

	tm->InitTestEngine( STANDARD );

	if( tm->SetupStandardTest() )
	{
	  tm->Execute();
	  MainFrame::GetInstance()->StartTest();
	  return true;
	}
	else
	{
	  wxMessageBox( wxT("Testet kan inte starta. Ingen testserver hittades.") );
	  tm->DeinitTestEngine();
	  return false;
	}
}

void SimpleModePanel::OnTestCompleted_Wrapper(void* obj, bool arg)
{
	SimpleModePanel *panel = (SimpleModePanel*)obj;
	panel->OnTestCompleted( arg );
}

void SimpleModePanel::OnTestCompleted(bool abort)
{
	TestMarshall *tm = TestMarshall::GetInstance();

	if( !abort )
	{
		TPEngine *engp = tm->GetTPEngineStruct();

		ResultLog *rlog = ResultLog::GetInstance();

		StringValueList *row = new StringValueList();

		row->Append( new wxString(wxDateTime::Now().Format( wxT("%Y-%m-%d %H:%M:%S") )) );
		wxString *Bandwidth = new wxString();
		*Bandwidth << (int)engp->bestTCPRecvRate * 8;
		row->Append( Bandwidth );

		rlog->AddResult( wxString( wxT("standard") ), *row );

		RefreshResultList();

		this->m_PageGraphResult->Refresh();
	}

	tm->DeinitTestEngine(abort);
	PostTest(abort);
}

bool SimpleModePanel::RefreshResultList(void)
{
	ResultLog *rlog = ResultLog::GetInstance();
	Result *result = rlog->GetResults( wxString( wxT("standard") ) );

	m_ListResult->DeleteAllItems();

	for( int i = result->GetRowCount()-1 ; i > -1 ; i-- )
	{
		StringValueList *row = result->GetRow( i );

		wxListItem item;
		int id = m_ListResult->GetItemCount();
		item.SetMask(wxLIST_MASK_TEXT);
		item.SetId(id);
		item.SetData( (int)i );

		item.SetText( *row->Item(0)->GetData() );
		m_ListResult->InsertItem( item );

		int iBandwidth;
		row->Item(1)->GetData()->ToLong( (long*)&iBandwidth );

		wxString Bandwidth( FROMCSTR(Int32ToString(iBandwidth)) );
		m_ListResult->SetItem( id, 1, Bandwidth );
	}

	return true;
}

void SimpleModePanel::OnRemoveResult(wxCommandEvent& event)
{
	ResultLog *rl = ResultLog::GetInstance();

	Result *r = rl->GetResults( wxString( wxT("standard") ) );

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

void SimpleModePanel::OnResultListItemSelected(wxListEvent& event)
{
//	wxListItem item = event.GetItem();
//	m_SelectedResultIndex = item.GetId();
}

//
// Unused - redesign later (low prio)
//

void SimpleModePanel::RefreshSettingsList(void)
{
	// Do nothing
}



