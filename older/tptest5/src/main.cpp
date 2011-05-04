#include "includes.h"
#include "main.h"

#include "MainFrame.h"

// the application icon
#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMAC__)
#include "rc/appicon.xpm"
#endif

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------




IMPLEMENT_APP(TheApp)

ServerList					*g_ServerList;

// 'Main program' equivalent: the program execution "starts" here
bool TheApp::OnInit()
{
	wxImage::AddHandler(new wxPNGHandler());
	wxImage::AddHandler(new wxJPEGHandler());

#ifdef MACOSX
	wxSetWorkingDirectory( wxT("tptest5.app") );
#endif

    g_ServerList = new ServerList();
    g_ServerList->DeleteContents( true );

    m_conf = AppConfig::GetInstance();

	this->m_mframe = new MainFrame(this);
	this->m_mframe->Show(true);

	// Do this so that the window is fully painted before we start the slow
	// process of loading the remote serverlist.
	this->m_mframe->Refresh();
	this->m_mframe->Update();

	if( !GetConfig()->RemoteLoadServerList() )
	{
	  wxMessageBox( wxT("Kunde inte hämta serverlistan från referensservern") );
	}

	GetConfig()->LoadServerList();

    this->m_mframe->RefreshPanels();
	
	return true;
}

int TheApp::OnExit()
{

	ResultLog *rlog = ResultLog::GetInstance();
	// We need to save results because the user might have removed some manually
	// The results are saved after each successfully completed test.
	rlog->SaveResults();
	delete rlog;

	m_conf->SaveServerList();

	g_ServerList->Clear();

	wxTheClipboard->Flush();

	delete m_conf;
	delete g_ServerList;

	return 0;
}

AppConfig* TheApp::GetConfig()
{
	return m_conf;
}
