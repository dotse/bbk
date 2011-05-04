#include "ThroughputDetail.h"


#include "Result.h"
#include "ResultLog.h"
#include "TestMarshall.h"

enum { wxID_DIALOG_THROUGHPUT_DETAIL = wxID_HIGHEST };

ThroughputDetail::ThroughputDetail(wxWindow *parent, int row)
:TestDetailDialog( parent, wxID_DIALOG_THROUGHPUT_DETAIL, wxT("Detaljer för Genomströmning"), wxDefaultPosition, wxSize(700,400), wxDEFAULT_DIALOG_STYLE )
{
	m_List = new wxListCtrl( 
				this, 
				wxID_ANY, 
				wxDefaultPosition, 
				wxSize( 700, 400 ), 
				wxLC_REPORT | wxLC_SINGLE_SEL );

	m_List->InsertColumn(0, wxT("Typ"), wxLIST_FORMAT_LEFT, 130);
	m_List->InsertColumn(1, wxT("Adress"), wxLIST_FORMAT_LEFT, 300);
	m_List->InsertColumn(2, wxT("Genomströmning"), wxLIST_FORMAT_LEFT, 200);

	m_SizerMain->Add( m_List, wxGBPosition( 0, 0 ), wxDefaultSpan, wxEXPAND );

	this->SetSizer( m_SizerMain );

	RefreshList( row );
}

ThroughputDetail::~ThroughputDetail(void)
{
}

void ThroughputDetail::RefreshList(int irow)
{
  Result *result = m_rl->GetResults( wxString( wxT("throughput") ) );

	StringValueList *row = result->GetRow( irow );

	// TPTEST Downstream
	wxStringTokenizer tkzD_TPVals( *row->Item(5)->GetData(), wxT("|"));
	while ( tkzD_TPVals.HasMoreTokens() )
	{
		wxString host		= tkzD_TPVals.GetNextToken();
		wxString bps		= tkzD_TPVals.GetNextToken();

		// this should not happen
		if( host.Length() == 0 )
			break;

		wxString strBPS;

		long lBPS;
		bps.ToLong( &lBPS );
		if( lBPS > 0 )
		{
		  wxString str( FROMCSTR(Int32ToString(lBPS*8)) );
		  strBPS << str;
		}
		else
		{
		  strBPS << wxT("-");
		}

		wxListItem item;
		int id = m_List->GetItemCount();
		item.SetMask(wxLIST_MASK_TEXT);
		item.SetId(id);
		item.SetText( wxString( wxT("TPTEST nedströms") ) );

		m_List->InsertItem( item );
		m_List->SetItem( id, 1, host );
		m_List->SetItem( id, 2, strBPS );
	}

	// TPTEST Upstream
	wxStringTokenizer tkzU_TPVals( *row->Item(6)->GetData(), wxT("|"));
	while ( tkzU_TPVals.HasMoreTokens() )
	{
		wxString host		= tkzU_TPVals.GetNextToken();
		wxString bps		= tkzU_TPVals.GetNextToken();

		// this should not happen
		if( host.Length() == 0 )
			break;

		wxString strBPS;

		long lBPS;
		bps.ToLong( &lBPS );
		if( lBPS > 0 )
		{
		  wxString str( FROMCSTR(Int32ToString(lBPS*8)) );
		  strBPS << str;
		}
		else
		{
		  strBPS << wxT("-");
		}

		wxListItem item;
		int id = m_List->GetItemCount();
		item.SetMask(wxLIST_MASK_TEXT);
		item.SetId(id);
		item.SetText( wxString( wxT("TPTEST uppströms")) );

		m_List->InsertItem( item );
		m_List->SetItem( id, 1, host );
		m_List->SetItem( id, 2, strBPS );
	}

	// HTTP
	wxStringTokenizer tkzHTTPVals( *row->Item(7)->GetData(), wxT("|"));
	while ( tkzHTTPVals.HasMoreTokens() )
	{
		wxString host		= tkzHTTPVals.GetNextToken();
		wxString bps		= tkzHTTPVals.GetNextToken();

		// this should not happen
		if( host.Length() == 0 )
			break;

		wxString strBPS;

		long lBPS;
		bps.ToLong( &lBPS );
		if( lBPS > 0 )
		{
		  wxString str( FROMCSTR(Int32ToString(lBPS)) );
		  strBPS << str;
		}
		else
		{
		  strBPS << wxT("-");
		}

		wxListItem item;
		int id = m_List->GetItemCount();
		item.SetMask(wxLIST_MASK_TEXT);
		item.SetId(id);
		item.SetText( wxString( wxT("HTTP") ) );

		m_List->InsertItem( item );
		m_List->SetItem( id, 1, host );
		m_List->SetItem( id, 2, strBPS );
	}

	// FTP
	wxStringTokenizer tkzFTPVals( *row->Item(8)->GetData(), wxT("|"));
	while ( tkzFTPVals.HasMoreTokens() )
	{
		wxString host		= tkzFTPVals.GetNextToken();
		wxString bps		= tkzFTPVals.GetNextToken();

		// this should not happen
		if( host.Length() == 0 )
			break;

		wxString strBPS;

		long lBPS;
		bps.ToLong( &lBPS );
		if( lBPS > 0 )
		{
		  wxString str( FROMCSTR(Int32ToString(lBPS)) );
		  strBPS << str;
		}
		else
		{
		  strBPS << wxT("-");
		}

		wxListItem item;
		int id = m_List->GetItemCount();
		item.SetMask(wxLIST_MASK_TEXT);
		item.SetId(id);
		item.SetText( wxString( wxT("FTP") ) );

		m_List->InsertItem( item );
		m_List->SetItem( id, 1, host );
		m_List->SetItem( id, 2, strBPS );
	}
}
