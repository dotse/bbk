#include "DialogAbout.h"

#include <wx/mimetype.h>
#include <wx/filefn.h>

BEGIN_EVENT_TABLE( DialogAbout, wxDialog )
	EVT_PAINT(DialogAbout::OnPaint)
	EVT_LEFT_DOWN(DialogAbout::OnLeftDown)
END_EVENT_TABLE()

#ifdef UNIX
#define DA_WIDTH 500
#define DA_HEIGHT 600
#endif
#ifdef WIN32
#define DA_WIDTH 420
#define DA_HEIGHT 600
#endif

DialogAbout::DialogAbout(wxWindow *parent)
: wxDialog(parent, wxID_ANY, wxString(wxT("Om TPTEST 5")), wxDefaultPosition, wxSize( DA_WIDTH, DA_HEIGHT ) )
{
	m_Sizer = new wxGridBagSizer( 5, 5 );

	wxImage::AddHandler( new wxJPEGHandler );
	wxImage logo;


	/* No logo 
	if( wxFile::Exists( wxT("GHNlogo.jpg") ) &&
	    logo.LoadFile(wxT("GHNlogo.jpg"), wxBITMAP_TYPE_JPEG ) )
	{
		m_ghnLogo = new wxBitmap( logo );
	}
	else
	{
	  wxMessageDialog( this, wxT("Could not load GHN logo!") ) ;
		this->m_ghnLogo = NULL;
	}
	*/

	wxString strAbout;
	strAbout << TPTEST_VERSION_STRING << wxT("\n\n");
	strAbout << wxT("TPTEST 5 är utvecklat av Gatorhole AB (www.gatorhole.com)") << wxT("\n");
	strAbout << wxT("på uppdrag av II-Stiftelsen (www.iis.se), Konsumentverket (www.kov.se)") << wxT("\n");
	strAbout << wxT("och Post- och Telestyrelsen (www.pts.se).") << wxT("\n\n");

	strAbout << wxT("Stiftelsen Internetinfrastruktur (II-stiftelsen), Konsumentverket, \n");
	strAbout << wxT("Post- och Telestyrelsen, IT-kommissionen, IP Performance Sverige AB,\n");
	strAbout << wxT("Autonomica AB eller Netnod Internet Exchange AB ansvarar inte för att de\n");
	strAbout << wxT("testresultat som levereras av TPTEST är korrekta. Stiftelsen \n");
	strAbout << wxT("Internetinfrastruktur (II-stiftelsen), Konsumentverket, Post- och Telestyrelsen,\n");
	strAbout << wxT("IT-kommissionen, IP Performance Sverige AB, Autonomica AB eller Netnod\n");
	strAbout << wxT("Internet Exchange AB ansvarar inte för driftsäkerhet och tillgänglighet avseende\n");
	strAbout << wxT("mätservrar eller masterserver, och inte heller för att TPTEST fortlöpande finns\n");
	strAbout << wxT("tillgängligt, för att det fortlöpande finns användarstöd eller för uppdateringar\n");
	strAbout << wxT("av programmet.\n");
	strAbout << wxT("\n");
	strAbout << wxT("Varje användare ansvarar själv för att programmet är korrekt konfigurerat för den\n");
	strAbout << wxT("egna datorn. Stiftelsen Internetinfrastruktur (II-stiftelsen), Konsumentverket,\n");
	strAbout << wxT("Post- och Telestyrelsen, IT-kommissionen, IP Performance Sverige AB, \n");
	strAbout << wxT("Autonomica AB eller Netnod Internet Exchange AB tar inte ansvar för \n");
	strAbout << wxT("klientprogrammets funktion i den individuella driftmiljön eller för fel som\n");
	strAbout << wxT("programmet orsakar i operativsystem, andra applikationer eller hårdvara, inte\n");
	strAbout << wxT("heller för skada som användaren orsakar på andra datorer genom användningen \n");
	strAbout << wxT("av TPTEST.");

	m_stxtAbout =
		new wxStaticText( this, 
			-1, 
			strAbout,
			wxDefaultPosition,
			wxDefaultSize,
			wxALIGN_CENTRE );

	m_btnOk = new wxButton(this, wxID_CANCEL, wxT("&Ok")  );
	
	m_Sizer->Add( m_stxtAbout, wxGBPosition( 0, 0 ), wxDefaultSpan, wxALIGN_CENTER | wxALL, 10 );
	m_Sizer->Add( m_btnOk, wxGBPosition( 1,  0 ), wxDefaultSpan, wxALIGN_CENTER | wxALL, 10 );

	this->SetSizer( m_Sizer );
	m_Sizer->SetSizeHints( this );
}

DialogAbout::~DialogAbout(void)
{
  /* No logo
	delete m_ghnLogo;
  */
	delete m_stxtAbout;
	delete m_btnOk;
}

void DialogAbout::OnLeftDown( wxMouseEvent &event )
{
	if( event.m_x > 320 &&
		event.m_x < 400 &&
		event.m_y > 330 &&
		event.m_y < 354 )
	{
	  wxString strHelpURL = wxT("http://www.ghn.se");
	  
	  wxMimeTypesManager manager;
	  wxFileType * filetype = manager.GetFileTypeFromExtension( wxT("html") );
	  wxString command = filetype->GetOpenCommand( strHelpURL );
	  wxExecute(command);
	}

	event.Skip();
}

void DialogAbout::OnPaint( wxPaintEvent &WXUNUSED(event) )
{
  wxPaintDC dc(this);
  /* No logo  
  if( this->m_ghnLogo != NULL )
    {
      dc.DrawBitmap( *this->m_ghnLogo, 320, 330 );
    }	
  */
}
