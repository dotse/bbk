#include "RTTPacketlossDetail.h"


#include "Result.h"
#include "ResultLog.h"

enum { wxID_DIALOG_RTTPACKETLOSS_DETAIL = wxID_HIGHEST };

RTTPacketLossDetail::RTTPacketLossDetail(wxWindow *parent, int row)
:TestDetailDialog( parent, wxID_DIALOG_RTTPACKETLOSS_DETAIL, wxT("Detaljer för Svarstider & Paketförluster"), wxDefaultPosition, wxSize(700,400), wxDEFAULT_DIALOG_STYLE )
{
	m_List = new wxListCtrl( 
				this, 
				wxID_ANY, 
				wxDefaultPosition, 
				wxSize( 700, 400 ), 
				wxLC_REPORT | wxLC_SINGLE_SEL );

	m_List->InsertColumn(0, wxT("Adress"), wxLIST_FORMAT_LEFT, 250);
	m_List->InsertColumn(1, wxT("Paketförlust"), wxLIST_FORMAT_LEFT, 120);
	m_List->InsertColumn(2, wxT("Svarstid"), wxLIST_FORMAT_LEFT, 100);
	m_List->InsertColumn(3, wxT("Jitter"), wxLIST_FORMAT_LEFT, 100);

	m_SizerMain->Add( m_List, wxGBPosition( 0, 0 ), wxDefaultSpan, wxEXPAND );

	this->SetSizer( m_SizerMain );

	RefreshList( row );
}

RTTPacketLossDetail::~RTTPacketLossDetail(void)
{
}

void RTTPacketLossDetail::RefreshList(int irow)
{
  Result *result = m_rl->GetResults( wxString( wxT("rtt") ) );

	StringValueList *row = result->GetRow( irow );

	wxStringTokenizer tkzVals( *row->Item(9)->GetData(), wxT("|"));
	while ( tkzVals.HasMoreTokens() )
	{
		wxString host		= tkzVals.GetNextToken();
		wxString numpk		= tkzVals.GetNextToken();
		wxString recieved	= tkzVals.GetNextToken();
		wxString avg		= tkzVals.GetNextToken();
		wxString jitter		= tkzVals.GetNextToken();

		// this should not happen
		if( host.Length() == 0 )
			break;

		wxString Packetloss;
		wxString Average;
		wxString Jitter;


		long iNumPk;
		numpk.ToLong( &iNumPk );
		long iRecPk;
		recieved.ToLong( &iRecPk );
		if( iNumPk > 0 )
		{
			Packetloss << (iNumPk-iRecPk)*100/iNumPk 
				   << wxT("% (") << (iNumPk-iRecPk) 
				   << wxT("/") << iNumPk << wxT(")");
		}
		else
		{
		  Packetloss << wxT("-");
		}

		double dAvg;
		avg.ToDouble( &dAvg );
		if( dAvg > -1 )
		{
			Average << dAvg;
		}
		else
		{
		  Average << wxT("-");
		}

		double dJitter;
		jitter.ToDouble( &dJitter);
		if( dJitter > -1 )
		{
			Jitter << dJitter;
		}
		else
		{
		  Jitter << wxT("-");
		}

		wxListItem item;
		int id = m_List->GetItemCount();
		item.SetMask(wxLIST_MASK_TEXT);
		item.SetId(id);
		item.SetText( host );

		m_List->InsertItem( item );
		m_List->SetItem( id, 1, Packetloss );
		m_List->SetItem( id, 2, Average );
		m_List->SetItem( id, 3, Jitter );
	}
}
