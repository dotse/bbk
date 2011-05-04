#include "ToolsPanel.h"

enum {
  wxID_TOOLS_BOOK
};

ToolsPanel::ToolsPanel( wxWindow *parent )
:wxPanel( parent, wxID_ANY )
{
	// Create the sizer for the main frame and the pages
	m_SizerMain = new wxBoxSizer( wxVERTICAL );

	// Create Notebook
	m_BookMain = new wxNotebook( this, wxID_TOOLS_BOOK, wxDefaultPosition, wxDefaultSize, wxNB_TOP );

	// Create and add the pages to book
	m_PageTPTest		= new TPTestPanel( m_BookMain  );
	m_PageIFConfig		= new IFConfigPanel( m_BookMain );
	m_PageNetStat		= new NetstatPanel( m_BookMain );
	m_PagePing			= new PingPanel( m_BookMain );
	m_PageTraceroute	= new TraceroutePanel( m_BookMain );

	m_BookMain->AddPage( m_PageTPTest, wxT("TPTEST 3") );
	m_BookMain->AddPage( m_PageIFConfig, wxT("IFConfig") );
	m_BookMain->AddPage( m_PageNetStat, wxT("NetStat") );
	m_BookMain->AddPage( m_PagePing, wxT("Ping") );
	m_BookMain->AddPage( m_PageTraceroute, wxT("Traceroute") );

	// Add book to main sizer
	m_SizerMain->Add( m_BookMain, 
				1, 
				wxEXPAND | 
				wxALL, 
				0 );

	this->SetSizer( m_SizerMain );
	//m_SizerMain->SetSizeHints( this );
}


ToolsPanel::~ToolsPanel(void)
{
}
