#include "AvailabilityDetail.h"

#include "Result.h"
#include "ResultLog.h"

enum { wxID_DIALOG_AVAILABILITY_DETAIL = wxID_HIGHEST };

AvailabilityDetail::AvailabilityDetail( wxWindow *parent, int row)
  :TestDetailDialog( parent, 
		     wxID_DIALOG_AVAILABILITY_DETAIL, 
		     wxT("Detaljer för tillgänglighetstest"), 
		     wxDefaultPosition, 
		     wxSize(700,400), 
		     wxDEFAULT_DIALOG_STYLE )
{
	m_List = new wxListCtrl( 
				this, 
				wxID_DIALOG_AVAILABILITY_DETAIL, 
				wxDefaultPosition, 
				wxSize( 700, 400 ), 
				wxLC_REPORT | wxLC_SINGLE_SEL );

	m_List->InsertColumn(0, wxT("Typ"), wxLIST_FORMAT_LEFT, 50);
	m_List->InsertColumn(1, wxT("Adress"), wxLIST_FORMAT_LEFT, 350);

	m_List->InsertColumn(2, wxT("Tillgänglighet"), wxLIST_FORMAT_LEFT, 100);

	m_SizerMain->Add( m_List, wxGBPosition( 0, 0 ), wxDefaultSpan, wxEXPAND );

	this->SetSizer( m_SizerMain );

	RefreshList( row );
}

AvailabilityDetail::~AvailabilityDetail(void)
{
}

void AvailabilityDetail::RefreshList(int irow)
{
	Result *result = m_rl->GetResults( wxT("availability") );

	StringValueList *row = result->GetRow( irow );

	// ICMP details

	wxString	*ICMP_Sent		= row->Item(3)->GetData();
	//wxString	*ICMP_Recvd		= row->Item(4)->GetData();
	wxString	*ICMP_HostCount	= row->Item(5)->GetData();

	long iICMP_Sent;
	ICMP_Sent->ToLong( &iICMP_Sent );
	long iICMP_HostCount;
	ICMP_HostCount->ToLong( &iICMP_HostCount );

	long PacketsPerHost = 0;
	
	if( iICMP_HostCount > 0 )
	{
		PacketsPerHost = iICMP_Sent / iICMP_HostCount;
	}

	long iICMP_RespondingHosts = 0;

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

		if( PacketsForThisHost > PacketsPerHost / 2 )
			iICMP_RespondingHosts++;

		wxString strVal = (PacketsForThisHost > (PacketsPerHost / 2) ? wxT("Ok") : wxT("Ej nåbar"));

		wxListItem item;
		int id = m_List->GetItemCount();
		item.SetMask(wxLIST_MASK_TEXT);
		item.SetId(id);
		item.SetText( wxT("ICMP") );

		m_List->InsertItem( item );
		m_List->SetItem( id, 1, host );
		m_List->SetItem( id, 2, strVal );
	}

	// TCP details
	wxStringTokenizer tkzTCP( *row->Item(7)->GetData(), wxT("|"));
	while ( tkzTCP.HasMoreTokens() )
	{
		wxString host		= tkzTCP.GetNextToken();
		wxString port		= tkzTCP.GetNextToken();
		wxString timestamp	= tkzTCP.GetNextToken();

		// this should not happen
		if( timestamp.Length() == 0 )
			break;

		wxString val		= tkzTCP.GetNextToken();
		double dval;
		val.ToDouble( &dval );
		
		wxString strVal = (dval > 0 ? wxT("Ok") : wxT("Ej nåbar"));

		wxListItem item;
		int id = m_List->GetItemCount();
		item.SetMask(wxLIST_MASK_TEXT);
		item.SetId(id);
		item.SetText( wxT("TCP") );

		m_List->InsertItem( item );
		m_List->SetItem( id, 1, host << wxT(":") << port );
		m_List->SetItem( id, 2, strVal );
	}

}

