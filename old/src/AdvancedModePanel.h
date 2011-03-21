#pragma once

#include "main.h"

class AdvancedModePanel;	// forward declarations
class SimpleModePanel;		// forward declarations
class AvailabilityModePanel;	// forward declarations
class RTTPacketlossModePanel;	// forward declarations
class ThroughputModePanel;		// forward declarations
class ToolsPanel;				// forward declarations

class AdvancedModePanel : public wxPanel
{
public:
	AdvancedModePanel(	wxWindow *parent );
	~AdvancedModePanel(void);

	void OnPageChanging( wxNotebookEvent &event );

	wxNotebook		*m_BookMain;

	SimpleModePanel		*m_PageSimple;
	AvailabilityModePanel	*m_PageAvailability;
	RTTPacketlossModePanel	*m_PageRTTPacketloss;
	ThroughputModePanel	*m_PageThroughput;
	ToolsPanel		*m_PageTools;

	wxBoxSizer		*m_SizerMain;

private:
	DECLARE_EVENT_TABLE()
};

