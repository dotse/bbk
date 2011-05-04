#pragma once
#include "main.h"
#include "Lists.h"
#include "ResultLog.h"
#include "Result.h"
#include "graphstruct.h"

#include "RTTPacketlossModePanel.h"

#include "gd.h"
#include "wx/mstream.h"

class GraphPanel : public wxPanel
{
public:
	GraphPanel( wxWindow *parent, wxWindowID id );
	~GraphPanel(void);

private:
	wxWindowID		m_Id;

	DECLARE_EVENT_TABLE()
public:
	void OnPaint(wxPaintEvent& event);
};
