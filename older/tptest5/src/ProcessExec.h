#ifdef __PROCESS_EXEC_H
#define __PROCESS_EXEC_H

#include <wx/process.h>

class ProcessExec : public wxProcess
{
 public:
  ProcessExec( wxString cmd );
  
 private:
  void OnTerminte( int pid, int status );

}






#endif


