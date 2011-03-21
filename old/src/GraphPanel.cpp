#include "GraphPanel.h"

#include <wx/stdpaths.h>

BEGIN_EVENT_TABLE( GraphPanel, wxPanel )
	EVT_PAINT(GraphPanel::OnPaint) 
END_EVENT_TABLE()

GraphPanel::GraphPanel( wxWindow *parent, wxWindowID id )
:wxPanel( parent, id, wxDefaultPosition, wxDefaultSize, 0 )
{
	m_Id = id;
}

GraphPanel::~GraphPanel(void)
{
}

void GraphPanel::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);

	/* 
	 * The resising of the graph image does not work as expected on window. 
	 * Book-page change is needed for it to work.
	 *
	 * Also this should be rewritten so that the image is not generated
	 * on every OnPaint.
	 *
	 */

	void * imptr = NULL;
	int imsize;
	Result *result;

	int panel_w, panel_h;

	this->GetSize( &panel_w, &panel_h ); 

	Graphstruct * gs = new Graphstruct;
	gs->xsize = panel_w - 20;
	gs->ysize = panel_h - 20;
	gs->y_axis_top = 50;
	gs->x_axis_left = 50;
	gs->x_axis_right = gs->xsize - 40;
	gs->y_axis_bottom = gs->ysize - 80;

	// Y-axis starts at 0.0
	gs->y_start = 0.0;

	// Show 1 decimal figure for the Y-axis mark values
	gs->y_value_decimals = 1;

	// Set the size of graph points
	gs->point_height = 5;
	gs->point_width = 5;

	// Set various colors:  RRGGBB
	gs->bgcolor = 0xDDDDDD;			// background color
	gs->plot_bgcolor = 0xFFFFFF;	// background color in plot area
	gs->info_bgcolor = 0xDDDDDD;	// info box background color
	gs->point_color = 0x000000; // 0x00A000;	// plot point color
	gs->info_box_maxcolor = 0xFF5050;			// color of "max" text in info box
	gs->info_box_avgcolor = 0x0000FF;			// color of "min" text in info box
	gs->info_box_mincolor = 0xA0A000;			// color of "avg" text in info box
	gs->adash_color = 0x0000FF;					// color of "average" line in plot

	// Turn on info box legend mode 
	// (causes it to be a one-line-thick box drawn at the bottom of image)
	gs->info_legend = 1;

	// Where to place the info box
//	gs->info_x_offset = 50;
//	gs->info_y_offset = 5;

	char gurka[50];
	gs->header = gurka;

	ResultLog *rlog = ResultLog::GetInstance();

	if( m_Id == wxID_SIMPLE_BOOK_RESULT_PAGE_GRAPH )
	{
		result = rlog->GetResults( wxT("standard") );

		strcpy(gs->header, TOCSTR(wxT("Resultat standardtest")) );
		double maxres = 0;

		for( int i = 0 ; i < result->GetRowCount() ; i++ )
		{
			StringValueList *row = result->GetRow( i );

			wxString *Date = row->Item(0)->GetData();
			wxString *Bandwidth = row->Item(1)->GetData();

			wxDateTime d;

			d.ParseFormat( Date->GetData(), wxT("%Y-%m-%d %H:%M:%S") );

			wxString bb = d.Format( wxT("%Y-%m-%d %H:%M:%S") );

			double v;
			Bandwidth->ToDouble( &v );

			if( v <= 0 || isnan(v) ) {
			  continue;
			}

			gs->add_value( d.GetTicks(), v );
			if (v > maxres)
				maxres = v;
		}
		// Y-axis label and divisor
		if (maxres > 100000) {
		  strcpy(gs->y_units, TOCSTR(wxT("Mbit/s")) );
		  gs->y_divisor = 1000000;
		}
		else {
		  strcpy(gs->y_units, TOCSTR(wxT("kbit/s")) );
		  gs->y_divisor = 1000;
		}

	}
	else if( m_Id == wxID_AVAILABILITY_BOOK_PAGE_RESULT_GRAPH )
	{
		result = rlog->GetResults( wxT("availability") );

		gs->y_start = 0;
		gs->y_stop = 100;

		strcpy(gs->y_units, TOCSTR(wxT("%")) );
		strcpy(gs->header, TOCSTR(wxT("Resultat tillgänglighetstest")) );

		for( int i = 0 ; i < result->GetRowCount() ; i++ )
		{
			StringValueList *row = result->GetRow( i );

			wxString *Date = row->Item(0)->GetData();
			wxString *TCP_Connects = row->Item(1)->GetData();
			wxString *TCP_HostCount = row->Item(2)->GetData();
			wxString *ICMP_Sent = row->Item(3)->GetData();
			wxString *ICMP_Recieved = row->Item(4)->GetData();
			wxString *ICMP_HostCount = row->Item(5)->GetData();

			wxDateTime d;

			d.ParseFormat( Date->GetData(), wxT("%Y-%m-%d %H:%M:%S") );

			wxString bb = d.Format( wxT("%Y-%m-%d %H:%M:%S") );

			long iTCP_Connects;
			TCP_Connects->ToLong( &iTCP_Connects );
			long iTCP_HostCount;
			TCP_HostCount->ToLong( &iTCP_HostCount );
			long iICMP_Sent;
			ICMP_Sent->ToLong( &iICMP_Sent );
			long iICMP_Recieved;
			ICMP_Recieved->ToLong( &iICMP_Recieved );
			long iICMP_HostCount;
			ICMP_HostCount->ToLong( &iICMP_HostCount );


            long iICMP_RespondingHosts = 0;

			long PacketsPerHost = 0;
			if( iICMP_HostCount > 0 )
			{
				PacketsPerHost = iICMP_Sent / iICMP_HostCount;
			}

			wxStringTokenizer tkzICMP( *row->Item(6)->GetData(), wxT(","));
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
					double dval = 0;
					val.ToDouble( &dval );

					if( dval > 0 )
					{
						PacketsForThisHost++;
					}
				}

				if( PacketsForThisHost > PacketsPerHost / 2 )
					iICMP_RespondingHosts++;

			}

			long sumHosts = iTCP_HostCount + iICMP_HostCount;
			long sumConnects = iTCP_Connects + iICMP_RespondingHosts;

			int ValueToPlot;

			if( sumHosts > 0 )
			{
				ValueToPlot = (int)(sumConnects*100 / sumHosts);
			}
			else
			{
				continue;
			}

			gs->add_value( d.GetTicks(), ValueToPlot );
		}
	}
	else if( m_Id == wxID_RTT_BOOK_PAGE_RESULT_RTT_GRAPH )
	{
		result = rlog->GetResults( wxT("rtt") );

		strcpy(gs->header, TOCSTR(wxT("Resultat svarstider")) );
		strcpy(gs->y_units, TOCSTR(wxT("ms")) );

		for( int i = 0 ; i < result->GetRowCount() ; i++ )
		{
			StringValueList *row = result->GetRow( i );

			wxString *Date = row->Item(0)->GetData();
			//			wxString *Responding = row->Item(1)->GetData();
			//wxString *HostCount = row->Item(2)->GetData();
			wxString *PkSent = row->Item(3)->GetData();
			wxString *PkRec = row->Item(4)->GetData();
			//wxString *RTTMax = row->Item(5)->GetData();
			//wxString *RTTMin = row->Item(6)->GetData();
			wxString *RTTAvg = row->Item(7)->GetData();

			wxDateTime d;

			d.ParseFormat( Date->GetData(), wxT("%Y-%m-%d %H:%M:%S") );

			wxString bb = d.Format( wxT("%Y-%m-%d %H:%M:%S") );

			double iPkSent;
			PkSent->ToDouble( &iPkSent);
			double iPkRec;
			PkRec->ToDouble( &iPkRec);
			double iRTTAvg;
			RTTAvg->ToDouble( &iRTTAvg );

			if( iRTTAvg > 0 )
			{
				gs->add_value( d.GetTicks(), iRTTAvg );
			}
		}
	}
	else if( m_Id == wxID_RTT_BOOK_PAGE_RESULT_PL_GRAPH )
	{
		result = rlog->GetResults( wxT("rtt") );

		gs->y_start = 0;
		gs->y_stop = 100;

		strcpy(gs->header, TOCSTR(wxT("Resultat paketförluster")) );
		strcpy(gs->y_units, TOCSTR(wxT("%")) );

		for( int i = 0 ; i < result->GetRowCount() ; i++ )
		{
			StringValueList *row = result->GetRow( i );

			wxString *Date = row->Item(0)->GetData();
			//	       		wxString *Responding = row->Item(1)->GetData();
			//			wxString *HostCount = row->Item(2)->GetData();
			wxString *PkSent = row->Item(3)->GetData();
			wxString *PkRec = row->Item(4)->GetData();
			//			wxString *RTTMax = row->Item(5)->GetData();
			//			wxString *RTTMin = row->Item(6)->GetData();
			wxString *RTTAvg = row->Item(7)->GetData();

			wxDateTime d;

			d.ParseFormat( Date->GetData(), wxT("%Y-%m-%d %H:%M:%S") );

			wxString bb = d.Format( wxT("%Y-%m-%d %H:%M:%S") );

			long iPkSent;
			PkSent->ToLong( &iPkSent);
			long iPkRec;
			PkRec->ToLong( &iPkRec);
			long iRTTAvg;
			RTTAvg->ToLong( &iRTTAvg );

			int iPkLoss;
			if( iPkSent > 0 )
			{
				iPkLoss = (iPkSent - iPkRec)*100/iPkSent;
				gs->add_value( d.GetTicks(), iPkLoss );	
			}
		
		}
	}
	else if( m_Id == wxID_RTT_BOOK_PAGE_RESULT_JITTER_GRAPH )
	{
		result = rlog->GetResults( wxT("rtt") );

//		gs->y_start = 0;
//		gs->y_stop = 100;

		strcpy(gs->header, TOCSTR(wxT("Jitter")) );
		strcpy(gs->y_units, TOCSTR(wxT("ms")) );

		for( int i = 0 ; i < result->GetRowCount() ; i++ )
		{
			StringValueList *row = result->GetRow( i );

			wxString *Date = row->Item(0)->GetData();
			//			wxString *Responding = row->Item(1)->GetData();
			//			wxString *HostCount = row->Item(2)->GetData();
			//			wxString *PkSent = row->Item(3)->GetData();
			//			wxString *PkRec = row->Item(4)->GetData();
			//			wxString *RTTMax = row->Item(5)->GetData();
			//			wxString *RTTMin = row->Item(6)->GetData();
			//			wxString *RTTAvg = row->Item(7)->GetData();
			wxString *Jitter = row->Item(8)->GetData();

			wxDateTime d;

			d.ParseFormat( Date->GetData(), wxT("%Y-%m-%d %H:%M:%S") );

			wxString bb = d.Format( wxT("%Y-%m-%d %H:%M:%S") );

			double dJitter;
			Jitter->ToDouble( &dJitter );

			if( dJitter > -1 )
			{
				gs->add_value( d.GetTicks(), dJitter );	
			}
		}
	}
	else if( m_Id == wxID_THROUGHPUT_BOOK_PAGE_RESULT_TPTESTDOWN_GRAPH )
	{
		result = rlog->GetResults( wxT("throughput") );

		strcpy(gs->header, TOCSTR(wxT("Resultat TPTEST nedströms")) );
		double maxres = 0;

		for( int i = 0 ; i < result->GetRowCount() ; i++ )
		{
			StringValueList *row = result->GetRow( i );

			wxString *Date = row->Item(0)->GetData();
			wxString *Bandwidth = row->Item(1)->GetData();

			/* Debug
			wxString tmp;
			tmp += wxT("Date:");
			tmp += *Date;
			tmp += wxT(" Bandwidth:");
			tmp += *Bandwidth; 
			tmp += wxT("\n");
			printf( TOCSTR(tmp) );
			*/

			wxDateTime d;

			d.ParseFormat( Date->GetData(), wxT("%Y-%m-%d %H:%M:%S") );

			wxString bb = d.Format( wxT("%Y-%m-%d %H:%M:%S") );

			double v = 0;
			Bandwidth->ToDouble( &v );
			if( v <= 0 || isnan(v) ) {
			  continue;
			}

			gs->add_value( d.GetTicks(), v*8 );
			if (v > maxres)
				maxres = v;
		}
		// Y-axis label and divisor
		if (maxres > 100000) {
		  strcpy(gs->y_units, TOCSTR(wxT("Mbit/s")) );
		  gs->y_divisor = 1000000;
		}
		else {
		  strcpy(gs->y_units, TOCSTR(wxT("kbit/s")) );
		  gs->y_divisor = 1000;
		}
	}
	else if( m_Id == wxID_THROUGHPUT_BOOK_PAGE_RESULT_TPTESTUP_GRAPH )
	{
		result = rlog->GetResults( wxT("throughput") );

		strcpy(gs->header, TOCSTR(wxT("Resultat TPTEST uppströms")) );
		double maxres = 0;

		for( int i = 0 ; i < result->GetRowCount() ; i++ )
		{
			StringValueList *row = result->GetRow( i );

			wxString *Date = row->Item(0)->GetData();
			wxString *Bandwidth = row->Item(2)->GetData();

			wxDateTime d;

			d.ParseFormat( Date->GetData(), wxT("%Y-%m-%d %H:%M:%S") );

			wxString bb = d.Format( wxT("%Y-%m-%d %H:%M:%S") );

			double v = 0;
			Bandwidth->ToDouble( &v );

			if( v <= 0 || isnan(v) ) {
			  continue;
			}
				
			gs->add_value( d.GetTicks(), v*8 );
			if (v > maxres)
				maxres = v;
		}
		// Y-axis label and divisor
		if (maxres > 100000) {
		  strcpy(gs->y_units, TOCSTR(wxT("Mbit/s")) );
		  gs->y_divisor = 1000000;
		}
		else {
		  strcpy(gs->y_units, TOCSTR(wxT("kbit/s")) );
		  gs->y_divisor = 1000;
		}
	}
	else if( m_Id == wxID_THROUGHPUT_BOOK_PAGE_RESULT_HTTP_GRAPH )
	{
		result = rlog->GetResults( wxT("throughput") );

		strcpy(gs->header, TOCSTR(wxT("Resultat HTTP genomströmning")) );
		double maxres = 0;

		for( int i = 0 ; i < result->GetRowCount() ; i++ )
		{
			StringValueList *row = result->GetRow( i );

			wxString *Date = row->Item(0)->GetData();
			wxString *Bandwidth = row->Item(3)->GetData();

			wxDateTime d;

			d.ParseFormat( Date->GetData(), wxT("%Y-%m-%d %H:%M:%S") );

			wxString bb = d.Format( wxT("%Y-%m-%d %H:%M:%S") );

			double v = 0;
			Bandwidth->ToDouble( &v );

			if( v <= 0 || isnan(v) ) {
			  continue;
			}

			gs->add_value( d.GetTicks(), v );
			if (v > maxres)
				maxres = v;
		}
		// Y-axis label and divisor
		if (maxres > 100000) {
		  strcpy(gs->y_units, TOCSTR(wxT("Mbit/s")) );
		  gs->y_divisor = 1000000;
		}
		else {
		  strcpy(gs->y_units, TOCSTR(wxT("kbit/s")) );
		  gs->y_divisor = 1000;
		}
	}
	else if( m_Id == wxID_THROUGHPUT_BOOK_PAGE_RESULT_FTP_GRAPH )
	{
		result = rlog->GetResults( wxT("throughput") );

		strcpy(gs->header, TOCSTR(wxT("Resultat FTP genomströmning")) );
		double maxres = 0;

		for( int i = 0 ; i < result->GetRowCount() ; i++ )
		{
			StringValueList *row = result->GetRow( i );

			wxString *Date = row->Item(0)->GetData();
			wxString *Bandwidth = row->Item(4)->GetData();

			wxDateTime d;

			d.ParseFormat( Date->GetData(), wxT("%Y-%m-%d %H:%M:%S") );

			wxString bb = d.Format( wxT("%Y-%m-%d %H:%M:%S") );

			double v = 0;
			Bandwidth->ToDouble( &v );

			if( v <= 0 || isnan(v) ) {
			  continue;
			}
			
			gs->add_value( d.GetTicks(), v );
			if (v > maxres)
				maxres = v;
		}
		// Y-axis label and divisor
		if (maxres > 100000) {
		  strcpy(gs->y_units, TOCSTR(wxT("Mbit/s")) );
			gs->y_divisor = 1000000;
		}
		else {
		  strcpy(gs->y_units, TOCSTR(wxT("kbit/s")) );
		  gs->y_divisor = 1000;
		}
	}
	else
	{
		return;
	}

	if (gs->no_values >= 1)
	{
		gs->generate();
		imptr = gdImagePngPtr(gs->image, &imsize);
		wxMemoryInputStream istream(imptr, imsize);
		wxImage myimage_img(istream, wxBITMAP_TYPE_PNG);
		dc.DrawBitmap( wxBitmap(myimage_img), 10, 10 );
	}
	else
	{
	  wxString strPath;

	  strPath << wxStandardPaths::Get().GetResourcesDir();
	  strPath << wxT("/");
	  strPath << wxT("start.jpg");

	  if( wxFile::Exists( strPath ) )

	    {
	      wxImage img;
	      img.LoadFile( strPath, wxBITMAP_TYPE_JPEG );
	      wxBitmap bmp( img ); 
	      dc.DrawBitmap( bmp, 10, 10 ); 
	    }
	}

	delete gs;
}
