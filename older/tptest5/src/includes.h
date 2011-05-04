#ifndef __INCLUDES_H
#define __INCLUDES_H

#ifdef WIN32
#include <winsock2.h>
#endif
#ifdef UNIX
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#endif
#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/image.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>
#include <wx/button.h>
#include <wx/timer.h>
#include <wx/progdlg.h>
#include <wx/datetime.h>
#include <wx/string.h>
#include <wx/txtstrm.h>
#include <wx/stream.h>
#include <wx/wfstream.h>
#include <wx/tokenzr.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/tooltip.h>
#include <wx/gbsizer.h>
#include <wx/clipbrd.h>
#include <wx/fileconf.h>

#endif
