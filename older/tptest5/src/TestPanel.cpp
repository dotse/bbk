#include "Testpanel.h"

#include "MainFrame.h"

#include "AppConfig.h"
#include "GraphPanel.h"

wxString START_TEST = wxT("Starta test");
wxString WAIT_TEST = wxT("Avbryt repetioner");

TestPanel::TestPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size)
:wxPanel( parent, id, pos, size )
{
	// Create the main sizer
	m_SizerMain = new wxBoxSizer( wxVERTICAL );

	m_StartButton = new wxButton( this, wxID_START_BUTTON, START_TEST, wxDefaultPosition, wxSize( 150, 30 ) );

	// Add Start Test button to sizer
	m_SizerMain->Add(	m_StartButton,
				0,				// make vertically stretchable
				wxALL |			// and make border all around
				wxALIGN_CENTER,	// take a guess
  				10 );			// set border width to 10
}

TestPanel::~TestPanel(void)
{
}

void TestPanel::PostTest(bool abort,bool hidewindow)
{
// Check for repeat 
	AppConfig *ac = AppConfig::GetInstance();
	
	wxString rep_ena;
	ac->GetValue( wxString( wxT("REPEAT_ENABLED") ), rep_ena );

	if( abort )
	{
		MainFrame::GetInstance()->EnableMenu( true );
		if( m_StartButton != NULL )
		{
			m_StartButton->SetLabel( START_TEST );
			m_StartButton->Enable(true);
		}

		if( MainFrame::GetInstance()->IsRepeatActive() )
		{
			MainFrame::GetInstance()->AbortRepeatTimer();
		}
		if( !hidewindow )
		{
			// This is more anoying than helpfull --- I think.
			//wxMessageBox( "Testet avbröts" );
		}
	}
	else if( rep_ena == wxT("YES") && MainFrame::GetInstance()->IsRepeatActive() )
	{
		m_StartButton->SetLabel( WAIT_TEST );
		m_StartButton->Enable(true);
		// Do nothing
	}
	else // repeat not running and its not an aborted test
	{
		MainFrame::GetInstance()->EnableMenu( true );
		if( m_StartButton != NULL )
		{
			m_StartButton->SetLabel( START_TEST );
			m_StartButton->Enable(true);
		}
		if( !hidewindow )
		{
			// More anoying than helpfull --- I think...
			// wxMessageBox( "Testet är färdigt" );
		}
	}
}

/*
	* Checks if repeat is enabled and disables the menu if it is.
	* Disables start button.
*/
void TestPanel::PreStartTest(void)
{
	AppConfig *ac = AppConfig::GetInstance();
	
	wxString rep_ena;
	ac->GetValue( wxString( wxT("REPEAT_ENABLED") ), rep_ena );

	if( rep_ena == wxT("YES") )
	{
		MainFrame::GetInstance()->EnableMenu( false );
	}

	m_StartButton->Enable(false);
}

void TestPanel::OnStartTest( wxCommandEvent& event )
{
	// If Start Test button is pressed while repeat is active
	if( MainFrame::GetInstance()->IsRepeatActive() )
	{
		PostTest(true);
	}
	// Otherwise its a normal situation, just start the test
	else
	{
		PreStartTest();
		bool ok = ExecuteTest();

		// If test is not executed we do the post test processing here
		// Otherwise the posttest is called from ontestcomplete. (REDESIGN!)
		if( !ok )
		{
			MainFrame::GetInstance()->PostTest();
		}
	}

}

void TestPanel::SetSBToolTip( wxString label )
{
	m_StartButton->SetToolTip( label );
}

void TestPanel::GetListColumn( wxListCtrl *list, int index, int icol, wxString &out )
{
	wxListItem col;
	col.m_itemId = index;
	col.m_col = icol;
	col.m_mask = wxLIST_MASK_TEXT;
	list->GetItem( col );
	out.Append( col.m_text );
}

void TestPanel::CopyToClipboard( wxString &text )
{
	if (wxTheClipboard->Open())
	{
		// This data objects are held by the clipboard, 
		// so do not delete them in the app.
		wxTextDataObject *txtdata = new wxTextDataObject( text );
		wxTheClipboard->SetData( txtdata );
		wxTheClipboard->Close();
	}
}

void TestPanel::GetListAsText( wxListCtrl *list, wxString &text )
{
	long item = -1;
	for ( ;; )
	{
		item = list->GetNextItem(item,
									wxLIST_NEXT_ALL,
									wxLIST_STATE_SELECTED);
		if ( item == -1 )
			break;


		for( int i = 0 ; i < list->GetColumnCount() ; i++ )
		{
			wxString col;
			GetListColumn( list, item, i, col );
			text << col << wxT("\t");
		}
		text.SetChar( text.Length()-1, wxT('\n') );
	}
}

void TestPanel::OnResultListKeyDown(wxListEvent& event)
{
	// Select All
	if( wxGetKeyState(WXK_CONTROL) && event.GetKeyCode() == 65 )
	{
		for( int i = 0 ; i < this->m_ListResult->GetItemCount() ; i++ )
		{
			m_ListResult->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
		}
	}
	else if( wxGetKeyState(WXK_CONTROL) && event.GetKeyCode() == 67 )
	{
		wxString text;
        GetListAsText(  m_ListResult, text );
		CopyToClipboard( text );

	}
}
