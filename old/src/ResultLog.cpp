#include "ResultLog.h"

#include "AppConfig.h"

#include <wx/filename.h>
#include <wx/stdpaths.h>

ResultLog* ResultLog::s_instance = 0;

ResultLog::ResultLog(void)
{
	m_Results = new ResultMap();

	Result *StdResult = new Result();
	StdResult->AddColumn( *new wxString(wxT("Date")) );
	StdResult->AddColumn( *new wxString(wxT("Bandwidth")) );

	Result *AvailResult = new Result();
	AvailResult->AddColumn( *new wxString(wxT("Date")) );
	AvailResult->AddColumn( *new wxString(wxT("TCP_Connects")) );
	AvailResult->AddColumn( *new wxString(wxT("TCP_HostCount")) );
	AvailResult->AddColumn( *new wxString(wxT("ICMP_Sent")) );
	AvailResult->AddColumn( *new wxString(wxT("ICMP_Recieved")) );
	AvailResult->AddColumn( *new wxString(wxT("ICMP_HostCount")) );
	AvailResult->AddColumn( *new wxString(wxT("ICMP_Values")) );
	AvailResult->AddColumn( *new wxString(wxT("TCP_Values")) );

	Result *RTTResult = new Result();
	RTTResult->AddColumn( *new wxString(wxT("Date")) );
	RTTResult->AddColumn( *new wxString(wxT("HostCount")) );
	RTTResult->AddColumn( *new wxString(wxT("Responding")) );
	RTTResult->AddColumn( *new wxString(wxT("Packets_Sent")) );
	RTTResult->AddColumn( *new wxString(wxT("Packets_Recieved")) );
	RTTResult->AddColumn( *new wxString(wxT("RTT_Max")) );
	RTTResult->AddColumn( *new wxString(wxT("RTT_Min")) );
	RTTResult->AddColumn( *new wxString(wxT("RTT_Average")) );
	RTTResult->AddColumn( *new wxString(wxT("Jitter")) );
	RTTResult->AddColumn( *new wxString(wxT("Values")) ); //host,ip->num_packets,recvd,avg,jitter

	Result *ThroughputResult = new Result();
	ThroughputResult->AddColumn( *new wxString(wxT("Date")) );				// 1
	ThroughputResult->AddColumn( *new wxString(wxT("D_TP_Bandwidth")) );		// 2
	ThroughputResult->AddColumn( *new wxString(wxT("U_TP_Bandwidth")) );		// 3
	ThroughputResult->AddColumn( *new wxString(wxT("D_HTTP_Bandwidth")) );	// 4
	ThroughputResult->AddColumn( *new wxString(wxT("D_FTP_Bandwidth")) );	// 5
	ThroughputResult->AddColumn( *new wxString(wxT("D_TP_Values")) );		// 6
	ThroughputResult->AddColumn( *new wxString(wxT("U_TP_Values")) );		// 7
	ThroughputResult->AddColumn( *new wxString(wxT("HTTP_Values")) );		// 8
	ThroughputResult->AddColumn( *new wxString(wxT("FTP_Values")) );			// 9


	(*m_Results)[wxT("standard")] = StdResult;
	(*m_Results)[wxT("availability")] = AvailResult;
	(*m_Results)[wxT("rtt")] = RTTResult;
	(*m_Results)[wxT("throughput")] = ThroughputResult;

	LoadResults();
	
}

ResultLog::~ResultLog(void)
{
  delete this->GetResults( wxString(wxT("standard")) );
  delete this->GetResults( wxString(wxT("availability")) );
  delete this->GetResults( wxString(wxT("rtt")) );
  delete this->GetResults( wxString(wxT("throughput")) );
	delete this->m_Results;
	this->s_instance = 0;
}

bool ResultLog::Load(wxString name)
{

	return false;
}

ResultLog* ResultLog::GetInstance(void)
{
	if( s_instance == 0 )
	{
		s_instance = new ResultLog();
	}
	return s_instance;
}


bool ResultLog::LoadResults(void)
{

	wxFile file;
	wxString strPath, strDefaultCwd;

	strPath = wxStandardPaths::Get().GetUserDataDir();

	if( !wxFileName::DirExists(strPath) ) 
	{
	  wxFileName::Mkdir( strPath, 0777, wxPATH_MKDIR_FULL );

	}

	strDefaultCwd = wxGetCwd();
	wxSetWorkingDirectory( strPath );

	if( !file.Exists(wxT("standard.log")) )
	{
	  file.Create(wxT("standard.log"));
		file.Close();
	}
	if( !file.Exists(wxT("rtt.log") ))
	{
	  file.Create(wxT("rtt.log"));
		file.Close();
	}
	if( !file.Exists(wxT("availability.log")) )
	{
	  file.Create(wxT("availability.log"));
		file.Close();
	}
	if( !file.Exists(wxT("throughput.log")) )
	{
	  file.Create(wxT("throughput.log"));
		file.Close();
	}


	// The standard results
	Result *r = (*m_Results)[wxT("standard")];
	
	wxFileInputStream input( wxT("standard.log") );

	if( input.Ok() )
	{
		wxTextInputStream text( input );

		do {
			wxString str = text.ReadLine();
			
			// Just skip empty lines
			if( str.Trim().Trim(false).Length() == 0 )
				continue;

			wxStringTokenizer tkz(str, wxT("\t"), wxTOKEN_RET_EMPTY );

			StringValueList *row = new StringValueList();

			wxString *Date = new wxString( tkz.GetNextToken() );
			wxString *Bandwidth = new wxString( tkz.GetNextToken() );

			row->Append( Date );
			row->Append( Bandwidth );

			r->AddRow( *row );

		} while( input.IsOk() );
	}


	// The availability results
	r = (*m_Results)[wxT("availability")];
	
	wxFileInputStream input_avail( wxT("availability.log") );

	if( input_avail.Ok() )
	{
		wxTextInputStream text_avail( input_avail );

		do {
			wxString str = text_avail.ReadLine();
			
			// Just skip empty lines
			if( str.Trim().Trim(false).Length() == 0 )
				continue;

			wxStringTokenizer tkz(str, wxT("\t"), wxTOKEN_RET_EMPTY );

			StringValueList *row = new StringValueList();

			wxString *Date = new wxString( tkz.GetNextToken() );
			wxString *TCP_Connects = new wxString( tkz.GetNextToken() );
			wxString *TCP_Count = new wxString( tkz.GetNextToken() );
			wxString *ICMP_Sent = new wxString( tkz.GetNextToken() );
			wxString *ICMP_Rec = new wxString( tkz.GetNextToken() );
			wxString *ICMP_Count = new wxString( tkz.GetNextToken() );
			wxString *ICMP_Values = new wxString( tkz.GetNextToken() );
			wxString *TCP_Values = new wxString( tkz.GetNextToken() );

			// TEMPORARY FIX FOR CHANGE OF FORMAT
			// FROM VERSION 5.0 -> 5.0.1
			ICMP_Values->Replace( wxT(","), wxT("|") );
			TCP_Values->Replace( wxT(","), wxT("|") );

			row->Append( Date );
			row->Append( TCP_Connects );
			row->Append( TCP_Count );
			row->Append( ICMP_Sent );
			row->Append( ICMP_Rec );
			row->Append( ICMP_Count );
			row->Append( ICMP_Values );
			row->Append( TCP_Values );

			r->AddRow( *row );

		} while( input_avail.IsOk() );
	}

	// The rtt packeloss results
	r = (*m_Results)[wxT("rtt")];
	
	wxFileInputStream input_rtt( wxT("rtt.log") );

	if( input_rtt.Ok() )
	{
		wxTextInputStream text_rtt( input_rtt );

		do {
			wxString str = text_rtt.ReadLine();
			
			// Just skip empty lines
			if( str.Trim().Trim(false).Length() == 0 )
				continue;

			wxStringTokenizer tkz(str, wxT("\t"), wxTOKEN_RET_EMPTY );

			StringValueList *row = new StringValueList();

			wxString *Date = new wxString( tkz.GetNextToken() );
			wxString *Responding = new wxString( tkz.GetNextToken() );
			wxString *HostCount = new wxString( tkz.GetNextToken() );
			wxString *Packets_Sent = new wxString( tkz.GetNextToken() );
			wxString *Packets_Recieved = new wxString( tkz.GetNextToken() );
			wxString *RTT_Max = new wxString( tkz.GetNextToken() );
			wxString *RTT_Min = new wxString( tkz.GetNextToken() );
			wxString *RTT_Average = new wxString( tkz.GetNextToken() );
			wxString *Jitter = new wxString( tkz.GetNextToken() );
			wxString *RTT_Values = new wxString( tkz.GetNextToken() );

			// TEMPORARY FIX FOR CHANGE OF FORMAT
			// FROM VERSION 5.0 -> 5.0.1
			RTT_Values->Replace(wxT(","), wxT("|"));

			row->Append( Date );
			row->Append( Responding);
			row->Append( HostCount );
			row->Append( Packets_Sent );
			row->Append( Packets_Recieved );
			row->Append( RTT_Max );
			row->Append( RTT_Min );
			row->Append( RTT_Average );
			row->Append( Jitter );
			row->Append( RTT_Values );

			r->AddRow( *row );

		} while( input_rtt.IsOk() );
	}


	// The throughput packeloss results
	r = (*m_Results)[wxT("throughput")];
	
	wxFileInputStream input_thr( wxT("throughput.log") );

	if( input_thr.Ok() )
	{
		wxTextInputStream text_thr( input_thr );

		do {
			wxString str = text_thr.ReadLine();
			
			// Just skip empty lines
			if( str.Trim().Trim(false).Length() == 0 )
				continue;

			wxStringTokenizer tkz(str, wxT("\t"), wxTOKEN_RET_EMPTY );

			StringValueList *row = new StringValueList();

			wxString *Date = new wxString( tkz.GetNextToken() );
			wxString *TPDown = new wxString( tkz.GetNextToken() );
			wxString *TPUp = new wxString( tkz.GetNextToken() );
			wxString *HTTP = new wxString( tkz.GetNextToken() );
			wxString *FTP = new wxString( tkz.GetNextToken() );
			wxString *D_TP_Values = new wxString( tkz.GetNextToken() );
			wxString *U_TP_Values = new wxString( tkz.GetNextToken() );
			wxString *HTTP_Values = new wxString( tkz.GetNextToken() );
			wxString *FTP_Values = new wxString( tkz.GetNextToken() );

			// TEMPORARY FIX FOR CHANGE OF FORMAT
			// FROM VERSION 5.0 -> 5.0.1
			D_TP_Values->Replace(wxT(","), wxT("|"));
			U_TP_Values->Replace(wxT(","), wxT("|"));
			HTTP_Values->Replace(wxT(","), wxT("|"));
			FTP_Values->Replace(wxT(","), wxT("|"));

			row->Append( Date );
			row->Append( TPDown );
			row->Append( TPUp );
			row->Append( HTTP );
			row->Append( FTP );
			row->Append( D_TP_Values );
			row->Append( U_TP_Values );
			row->Append( HTTP_Values );
			row->Append( FTP_Values );

			r->AddRow( *row );

		} while( input_thr.IsOk() );
	}

	wxSetWorkingDirectory( strDefaultCwd );

	return true;

}




Result* ResultLog::GetResults(wxString name)
{
	return (*m_Results)[name];
}

bool ResultLog::AddResult(wxString resultname, StringValueList &row)
{
	AppConfig *ac = AppConfig::GetInstance();

	wxString maxrows;
	ac->GetValue( wxString(wxT("HISTORY_COUNT")), maxrows );
	long imax;
	maxrows.ToLong( &imax );

	Result *r = GetResults( resultname );

	if( r->GetRowCount() > imax-1 )
	{
		r->DeleteRow( 0 );
	}

	r->AddRow( row );
	return false;
}

StringValueList* ResultLog::GetColumn(wxString resultname, wxString &name)
{
	Result *r = GetResults( resultname );
	return r->GetColumn( name );
}

bool ResultLog::SaveResults(bool _export)
{
  /* ignore _export for now */
  
  wxString strPath, strDefaultCwd;
  
  strPath = wxStandardPaths::Get().GetUserDataDir();
  
  if( !wxFileName::DirExists(strPath) ) 
    {
      wxFileName::Mkdir( strPath, 0777, wxPATH_MKDIR_FULL );
      
    }
  
  strDefaultCwd = wxGetCwd();
  wxSetWorkingDirectory( strPath );
  
  SaveStandardResults(		wxString(wxT("standard.log")) );
  SaveAvailabilityResults(	wxString(wxT("availability.log")) );
  SaveRTTPacketlossResults(	wxString(wxT("rtt.log")) );
  SaveThroughputResults(	wxString(wxT("throughput.log")) );

  wxSetWorkingDirectory( strDefaultCwd );

  return true;
}


bool ResultLog::SaveStandardResults( wxString filename, bool _export )
{
  // The standard reslts
  Result *r = GetResults( wxString(wxT("standard")) );
  
  wxFileOutputStream output( filename );
  wxTextOutputStream text( output );

	wxString strHeader;
	if( _export )
	{
	  this->m_SaveDelimiter = wxT(",");
		StringValueList *header = r->GetHeader();
		for( StringValueList::Node *node = header->GetFirst() ; node ; node = node->GetNext() )
		{
			strHeader << *node->GetData() << m_SaveDelimiter;
		}
		strHeader.SetChar( strHeader.Length()-1, wxT('\n') );
	}
	else
	{
	  m_SaveDelimiter = wxT("\t");
	}

	text.WriteString( strHeader );

	for( int i = 0 ; i < r->GetRowCount() ; i++ )
	{
		StringValueList *row = r->GetRow( i );

		wxString line;

		for( StringValueList::Node *node = row->GetFirst() ; node ; node = node->GetNext() )
		{
			line	<< *node->GetData() << m_SaveDelimiter;
		}

		line.SetChar( line.Length()-1, wxT('\n') );

		text.WriteString( line );
	}

	return true;
}


bool ResultLog::SaveAvailabilityResults( wxString filename, bool _export )
{
	// The availability results
  Result *r = GetResults( wxString(wxT("availability")) );

	wxFileOutputStream output_avail( filename );
	wxTextOutputStream text_avail( output_avail );

	int ColumnCount = (_export ? r->GetColumnCount() -2 : r->GetColumnCount());

	wxString strHeader;
	if( _export )
	{
	  this->m_SaveDelimiter = wxT(",");
		StringValueList *header = r->GetHeader();
		int countheader = 0;
		for( StringValueList::Node *node = header->GetFirst() ; node && countheader++ < ColumnCount ; node = node->GetNext() )
		{
			strHeader << *node->GetData() << m_SaveDelimiter;
		}
		strHeader.SetChar( strHeader.Length()-1, wxT('\n') );
	}
	else
	{
	  m_SaveDelimiter = wxT("\t");
	}

	text_avail.WriteString( strHeader );

	for( int i = 0 ; i < r->GetRowCount() ; i++ )
	{
		StringValueList *row = r->GetRow( i );

		wxString line;

		int countcolumn = 0;
		for( StringValueList::Node *node = row->GetFirst() ; node && countcolumn++ < ColumnCount  ; node = node->GetNext() )
		{
			line	<< *node->GetData() << m_SaveDelimiter;
		}

		line.SetChar( line.Length()-1, wxT('\n') );

		text_avail.WriteString( line );
	}

	return true;
}

bool ResultLog::SaveRTTPacketlossResults( wxString filename, bool _export )
{
	// The RTT results
  Result *r = GetResults( wxString(wxT("rtt")) );

	wxFileOutputStream output_rtt( filename );
	wxTextOutputStream text_rtt( output_rtt );

	int ColumnCount = (_export ? r->GetColumnCount() -1 : r->GetColumnCount());

	wxString strHeader;
	if( _export )
	{
	  m_SaveDelimiter = wxT(",");
		StringValueList *header = r->GetHeader();
		int countheader = 0;
		for( StringValueList::Node *node = header->GetFirst() ; node && countheader++ < ColumnCount ; node = node->GetNext() )
		{
			strHeader << *node->GetData() << m_SaveDelimiter;
		}
				      strHeader.SetChar( strHeader.Length()-1, wxT('\n') );
	}
	else
	{
	  m_SaveDelimiter = wxT("\t");
	}

	text_rtt.WriteString( strHeader );

	for( int i = 0 ; i < r->GetRowCount() ; i++ )
	{
		StringValueList *row = r->GetRow( i );

		wxString line;
		int countcolumn = 0;
		for( StringValueList::Node *node = row->GetFirst() ; node && countcolumn++ < ColumnCount ; node = node->GetNext() )
		{
			line	<< *node->GetData() << m_SaveDelimiter;
		}

		line.SetChar( line.Length()-1, wxT('\n') );

		text_rtt.WriteString( line );
	}

	return true;
}

bool ResultLog::SaveThroughputResults( wxString filename, bool _export )
{
	// The Throughput results
  Result *r = GetResults( wxString(wxT("throughput")) );

	wxFileOutputStream output_thr( filename );
	wxTextOutputStream text_thr( output_thr );

	int ColumnCount = (_export ? r->GetColumnCount() -4 : r->GetColumnCount());

	wxString strHeader;
	if( _export )
	{
	  m_SaveDelimiter = wxT(",");
		StringValueList *header = r->GetHeader();
		int countcolumn = 0;		
		for( StringValueList::Node *node = header->GetFirst() ; node && countcolumn++ < ColumnCount ; node = node->GetNext() )
		{
			strHeader << *node->GetData() << m_SaveDelimiter;
		}
				      strHeader.SetChar( strHeader.Length()-1, wxT('\n') );
	}
	else
	{
	  m_SaveDelimiter = wxT("\t");
	}

	text_thr.WriteString( strHeader );

	for( int i = 0 ; i < r->GetRowCount() ; i++ )
	{
		StringValueList *row = r->GetRow( i );

		wxString line;
		int countcolumn = 0;
		for( StringValueList::Node *node = row->GetFirst() ; node && countcolumn++ < ColumnCount ; node = node->GetNext() )
		{
			line	<< *node->GetData() << m_SaveDelimiter;
		}

		line.SetChar( line.Length()-1, wxT('\n') );

		text_thr.WriteString( line );
	}

	return true;
}


