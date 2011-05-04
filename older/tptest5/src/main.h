#pragma once

#include "includes.h"
#include "Lists.h"
#include "AppConfig.h"
#include "ResultLog.h"


/* Forward declaration */
class MainFrame;


enum
{
  // These component-id's are needed on more than
  // one page (I.E the graphpanel and the corresponding
  // panel).
  // + 1000 because we never have more than 999
  // components on one page!
  wxID_START_BUTTON = wxID_HIGHEST + 1000,
  wxID_SIMPLE_BOOK_RESULT_PAGE_GRAPH,
  wxID_AVAILABILITY_BOOK_PAGE_RESULT_GRAPH,
  wxID_THROUGHPUT_BOOK_PAGE_RESULT_TPTESTDOWN_GRAPH,
  wxID_THROUGHPUT_BOOK_PAGE_RESULT_TPTESTUP_GRAPH,
  wxID_THROUGHPUT_BOOK_PAGE_RESULT_HTTP_GRAPH,
  wxID_THROUGHPUT_BOOK_PAGE_RESULT_FTP_GRAPH,
  wxID_RTT_BOOK_PAGE_RESULT_RTT_GRAPH,
  wxID_RTT_BOOK_PAGE_RESULT_PL_GRAPH,
  wxID_RTT_BOOK_PAGE_RESULT_JITTER_GRAPH

}; 


enum
{
	MODE_SIMPLE,
	MODE_ADVANCED
}; 

extern ServerList					*g_ServerList;

#define TPTEST_VERSION_STRING wxT("TPTEST 5.0.2")

#define TOCSTR(var) (const char*)wxString(var, wxConvUTF8).mb_str()

// Look this up, why does it behave diffrent on diffrent OS?
#ifdef MACOSX
#define FROMCSTR(var) wxString(var,wxConvUTF8)
#endif
#ifdef WIN32
#define FROMCSTR(var) wxString::FromAscii(var)
#endif


// Define a new application type, each program should derive a class from wxApp
class TheApp : public wxApp
{
public:
    // override base class virtuals
    // ----------------------------

    // this one is called on application startup and is a good place for the app
    // initialization (doing it here and not in the ctor allows to have an error
    // return: if OnInit() returns false, the application terminates)
    virtual bool OnInit();
	virtual int	 OnExit();

	AppConfig	*GetConfig();

private:
	MainFrame	*m_mframe;
	AppConfig	*m_conf;

};
