#include "TestDetailDialog.h"

#include "ResultLog.h"

TestDetailDialog::TestDetailDialog(	wxWindow* parent, 
					wxWindowID id, 
					const wxString title, 
					const wxPoint& pos, 
					const wxSize& size,
					long style,
					const wxString name)
 :wxDialog( parent, id, title, pos, size, style, name )
{
  m_SizerMain = new wxGridBagSizer( 0, 5 );
  m_rl = ResultLog::GetInstance();
  
}

TestDetailDialog::~TestDetailDialog(void)
{
}
