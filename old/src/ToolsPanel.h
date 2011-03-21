#pragma once
#include "includes.h"
#include "main.h"
#include "TestMarshall.h"
#include "MainFrame.h"
#include "ResultLog.h"
#include "TPTestPanel.h"
#include "IFConfigPanel.h"
#include "NetstatPanel.h"
#include "PingPanel.h"
#include "TraceroutePanel.h"

class ToolsPanel : public wxPanel
{
public:
	ToolsPanel( wxWindow *parent);
	~ToolsPanel(void);

private:
	wxNotebook			*m_BookMain;
	wxNotebookPage		*m_PageTPTest;
	wxNotebookPage		*m_PageIFConfig;
	wxNotebookPage		*m_PageNetStat;
	wxNotebookPage		*m_PagePing;
	wxNotebookPage		*m_PageTraceroute;

	wxBoxSizer			*m_SizerMain;

};
