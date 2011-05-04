#include "AppConfig.h"

#include "main.h"

AppConfig* AppConfig::s_instance = 0;

/*

	serverlist.txt
		Host
		TPTest Port
		TCP Port
		HTTP Path
		FTP Path
		IsUserCreated
		EnableAvailability
		EnableAvailabilityTCP
		EnableAvailabilityICMP
		EnableRTTPacketloss
		EnableThroughput
		EnableThroughputTPTest
		EnableThroughputHTTP
		EnableThroughputFTP
		IsICMP

*/

AppConfig::AppConfig(void)
{
	wxFile file;
	if( wxFile::Exists( wxT("tptest.conf") ) )
	{
	  file.Open( wxT("tptest.conf") );
	  wxFileInputStream	input( file );
	  m_fileconfig = new wxFileConfig( input );
	}
	else
	{
	  file.Create( wxT("tptest.conf") ); // for writing
	  file.Close(); 
	  file.Open( wxT("tptest.conf") ); // for reading
	  wxFileInputStream input( file );
	  m_fileconfig = new wxFileConfig( input );
	}

	wxString str;
	wxString strKey;

	GetValue( wxT("TPTEST5_SERVERLIST_URL"), str );
	
	if( str.Trim().Length() == 0 ) 
	{
		m_fileconfig->Write( wxT("TPTEST5_SERVERLIST_URL"), wxT("http://referens.sth.ip-performance.se/serverlist.txt") );
	}

	str.Clear();
	GetValue( wxT("TPTEST3_SERVERLIST_URL"), str );
	if( str.Trim().Length() == 0 ) 
	{
		m_fileconfig->Write( wxT("TPTEST3_SERVERLIST_URL"), wxT("http://referens.sth.ip-performance.se/tptest3serverlist.txt") );
	}

	str.Clear();
	GetValue( wxT("HISTORY_COUNT"), str );
	if( str.Trim().Length() == 0 ) 
	{
		m_fileconfig->Write( wxT("HISTORY_COUNT"), wxT("30") );
	}

	str.Clear();
	GetValue( wxT("REPEAT_ENABLED"), str );
	if( str.Trim().Length() == 0 ) 
	{
		m_fileconfig->Write( wxT("REPEAT_ENABLED"), wxT("NO") );
	}

	str.Clear();
	GetValue( wxT("REPEAT_INTERVAL"), str );
	if( str.Trim().Length() == 0 ) 
	{
		m_fileconfig->Write( wxT("REPEAT_INTERVAL"), wxT("15") );
	}

	str.Clear();
	GetValue( wxT("REPEAT_TIME"), str );
	if( str.Trim().Length() == 0 ) 
	{
		m_fileconfig->Write( wxT("REPEAT_TIME"), wxT("2") );
	}

}

AppConfig::~AppConfig(void)
{
	SaveConfig();
	delete m_fileconfig;
}

void AppConfig::SaveConfig(void)
{
  wxFileOutputStream	output( wxT("tptest.conf") );
  m_fileconfig->Save( output );
}

bool AppConfig::SetValue( wxString key, wxString &val )
{
	return m_fileconfig->Write( key, val );
}

bool AppConfig::GetValue( wxString key, wxString &val )
{
	return m_fileconfig->Read( key, &val );
}

bool AppConfig::LoadServerList(void)
{
	wxFile file;
	if( !file.Exists( wxT("serverlist.txt") ) )
	{
	  file.Create( wxT("serverlist.txt") );
	  file.Close();
	}

	file.Open( wxT("serverlist.txt") );
	wxFileInputStream input( file );
	wxTextInputStream text( input );

	if( input.Ok() )
	{

		do {
			wxString str = text.ReadLine();
			wxStringTokenizer tkz(str, wxT(" \t"), wxTOKEN_RET_EMPTY );

			Server *serv = new Server();

			//int c = tkz.CountTokens();

			serv->Host		= tkz.GetNextToken();
			serv->TPTest_Port	= tkz.GetNextToken();
			serv->TCP_Port		= tkz.GetNextToken();
			serv->HTTP_Path		= tkz.GetNextToken();
			serv->FTP_Path		= tkz.GetNextToken();

			if( tkz.GetNextToken() == wxT("1") )
				serv->IsUserCreated = true;
			else
				serv->IsUserCreated = false;


			// If server is user created we load it
			if( !serv->IsUserCreated )
			{
				delete serv;
				continue;
			}

			if( tkz.GetNextToken() == wxT("1") )
				serv->EnableAvailability = true;
			else
				serv->EnableAvailability = false;

			if( tkz.GetNextToken() == wxT("1") )
				serv->EnableAvailabilityTCP = true;
			else
				serv->EnableAvailabilityTCP = false;

			if( tkz.GetNextToken() == wxT("1") )
				serv->EnableAvailabilityICMP = true;
			else
				serv->EnableAvailabilityICMP = false;

			if( tkz.GetNextToken() == wxT("1") )
				serv->EnableRTTPacketloss = true;
			else
				serv->EnableRTTPacketloss = false;

			if( tkz.GetNextToken() == wxT("1") )
				serv->EnableThroughput = true;
			else
				serv->EnableThroughput = false;

			if( tkz.GetNextToken() == wxT("1") )
				serv->EnableThroughputTPTest = true;
			else
				serv->EnableThroughputTPTest = false;

			if( tkz.GetNextToken() == wxT("1") )
				serv->EnableThroughputHTTP = true;
			else
				serv->EnableThroughputHTTP = false;

			if( tkz.GetNextToken() == wxT("1") )
				serv->EnableThroughputFTP = true;
			else
				serv->EnableThroughputFTP = false;

			if( tkz.GetNextToken() == wxT("1") )
				serv->IsICMP = true;
			else
				serv->IsICMP = false;


			if( serv->Host.Length() > 0 )
				g_ServerList->Append( serv );

		} while( input.IsOk() );

	}

	return true;
}

bool AppConfig::SaveServerList(void)
{
  wxFileOutputStream output( wxT("serverlist.txt") );
  wxTextOutputStream text( output );

	for ( ServerList::Node *node = g_ServerList->GetFirst(); node; node = node->GetNext() )
	{
		Server *s = node->GetData();

		if( s->IsUserCreated )
		{
			wxString line;

			line	<< s->Host << wxT("\t")
				<< s->TPTest_Port << wxT("\t")
				<< s->TCP_Port << wxT("\t")
				<< s->HTTP_Path << wxT("\t")
				<< s->FTP_Path << wxT("\t")
				<< (s->IsUserCreated?wxT("1"):wxT("0")) << wxT("\t")
				<< (s->EnableAvailability?wxT("1"):wxT("0")) << wxT("\t")
				<< (s->EnableAvailabilityTCP?wxT("1"):wxT("0")) << wxT("\t")
				<< (s->EnableAvailabilityICMP?wxT("1"):wxT("0")) << wxT("\t")
				<< (s->EnableRTTPacketloss?wxT("1"):wxT("0")) << wxT("\t") 
				<< (s->EnableThroughput?wxT("1"):wxT("0")) << wxT("\t")
				<< (s->EnableThroughputTPTest?wxT("1"):wxT("0")) << wxT("\t") 
				<< (s->EnableThroughputHTTP?wxT("1"):wxT("0")) << wxT("\t")
				<< (s->EnableThroughputFTP?wxT("1"):wxT("0")) << wxT("\t")
				<< (s->IsICMP?wxT("1"):wxT("0")) << wxT("\n");

			text.WriteString( line );
		}

	}

	return true;
}


bool AppConfig::RemoteLoadServerList(void)
{
  Downloader *dl = new Downloader();
	
  wxString serverlist_url;
  this->GetValue( wxT("TPTEST5_SERVERLIST_URL"), serverlist_url);
  dl->downloadFile( (const char*)serverlist_url.mb_str() );

	if( dl->GetResponseCode() >= 400 )
	{
		delete dl;
		return false;
	}
	else if( dl->contentLength() <= 0 )
	{
		delete dl;
		return false;
	}

	wxString        content( dl->content(), wxConvUTF8);
	wxChar		*slist((wxChar*)content.c_str());
	unsigned int	slist_length = dl->contentLength();

	wxString strList( slist, (size_t)slist_length );



	wxString line;
	do
	{
		if( strList.Find( wxChar('\n') ) == -1 )
			break;

		line = strList.BeforeFirst( wxChar('\n') );
		strList = strList.AfterFirst( wxChar('\n') );

		line.Replace( wxT("\r"), wxT("") );

		if( line.Length() == 0 )
			continue;

		wxStringTokenizer tkz(line, wxT(" \t"), wxTOKEN_RET_EMPTY );

		Server *serv = new Server();

		//int c = tkz.CountTokens();

		serv->Host		= tkz.GetNextToken();
		serv->TPTest_Port	= tkz.GetNextToken();
		serv->TCP_Port		= tkz.GetNextToken();
		serv->HTTP_Path		= tkz.GetNextToken();
		serv->FTP_Path		= tkz.GetNextToken();

		if( tkz.GetNextToken() == wxT("1") )
			serv->IsUserCreated = true;
		else
			serv->IsUserCreated = false;

		// Don't load user created servers
		if( serv->IsUserCreated )
		{
			delete serv;
			continue;
		}

		if( tkz.GetNextToken() == wxT("1") )
			serv->EnableAvailability = true;
		else
			serv->EnableAvailability = false;

		if( tkz.GetNextToken() == wxT("1") )
			serv->EnableAvailabilityTCP = true;
		else
			serv->EnableAvailabilityTCP = false;

		if( tkz.GetNextToken() == wxT("1") )
			serv->EnableAvailabilityICMP = true;
		else
			serv->EnableAvailabilityICMP = false;

		if( tkz.GetNextToken() == wxT("1") )
			serv->EnableRTTPacketloss = true;
		else
			serv->EnableRTTPacketloss = false;

		if( tkz.GetNextToken() == wxT("1") )
			serv->EnableThroughput = true;
		else
			serv->EnableThroughput = false;

		if( tkz.GetNextToken() == wxT("1") )
			serv->EnableThroughputTPTest = true;
		else
			serv->EnableThroughputTPTest = false;

		if( tkz.GetNextToken() == wxT("1") )
			serv->EnableThroughputHTTP = true;
		else
			serv->EnableThroughputHTTP = false;

		if( tkz.GetNextToken() == wxT("1") )
			serv->EnableThroughputFTP = true;
		else
			serv->EnableThroughputFTP = false;

		if( tkz.GetNextToken() == wxT("1") )
			serv->IsICMP = true;
		else
			serv->IsICMP = false;


		if( serv->Host.Length() > 0 )
			g_ServerList->Append( serv );

	} while( line.Length() > 0 );

	delete dl;

	return true;
}

AppConfig* AppConfig::GetInstance(void)
{
	if( s_instance == 0 )
	{
		s_instance = new AppConfig();
	}
	return s_instance;
}

