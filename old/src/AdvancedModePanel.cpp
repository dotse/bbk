#include "AdvancedModePanel.h"
#include "MainFrame.h"

#include "SimpleModePanel.h"
#include "AvailabilityModePanel.h"
#include "RTTPacketlossModePanel.h"
#include "ThroughputModePanel.h"
#include "ToolsPanel.h"

enum { wxID_ADVANCED_BOOK = wxID_HIGHEST };

BEGIN_EVENT_TABLE( AdvancedModePanel, wxPanel )
	EVT_NOTEBOOK_PAGE_CHANGING(wxID_ADVANCED_BOOK, AdvancedModePanel::OnPageChanging) 
END_EVENT_TABLE()


AdvancedModePanel::AdvancedModePanel( wxWindow *parent )
:wxPanel( parent, wxID_ANY )
{
	// Create the sizer for the main frame and the pages
	m_SizerMain = new wxBoxSizer( wxVERTICAL );

	// Create Notebook
	m_BookMain = new wxNotebook( this, wxID_ADVANCED_BOOK, wxDefaultPosition, wxDefaultSize, wxNB_TOP );

	// Create and add the pages to book
	m_PageSimple		= new SimpleModePanel( m_BookMain );
	m_PageAvailability	= new AvailabilityModePanel( m_BookMain ); 
	m_PageRTTPacketloss = new RTTPacketlossModePanel( m_BookMain );
	m_PageThroughput	= new ThroughputModePanel( m_BookMain );
	m_PageTools			= new ToolsPanel( m_BookMain );



	m_BookMain->AddPage( m_PageSimple, wxT("Enkelt läge") );
        m_BookMain->AddPage( m_PageAvailability, wxT("Tillgänglighet") );
	m_BookMain->AddPage( m_PageRTTPacketloss, 
			     wxT("Svarstider && Paketförluster") );
	m_BookMain->AddPage( m_PageThroughput, wxT("Genomströmning") );
      	m_BookMain->AddPage( m_PageTools, wxT("Verktyg") );

	// Add book to main sizer
	m_SizerMain->Add( m_BookMain, 
				1, 
				wxEXPAND | 
				wxALL, 
				0 );

	this->SetSizer( m_SizerMain );
	m_SizerMain->SetSizeHints( this );	
}

AdvancedModePanel::~AdvancedModePanel(void)
{
}

void AdvancedModePanel::OnPageChanging( wxNotebookEvent &event )
{
	if( MainFrame::GetInstance()->IsPagesLocked() )
	{
		event.Veto();
	}
}

