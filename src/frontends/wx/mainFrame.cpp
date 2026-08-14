#include "mainFrame.hpp"
#include "cdi/common.hpp"

#include <wx/dcbuffer.h>
#include <wx/wfstream.h>

#include <SDL2/SDL.h>
#include <filesystem>

#include "app_icon.xpm"

using namespace miniCDi;

static PhilipsCDI *cdi = NULL;
#define CDI_SCREEN_WIDTH 768
#define CDI_SCREEN_HEIGHT 280

/*******************************************************************************
// BasicDrawPane
*******************************************************************************/

BEGIN_EVENT_TABLE(BasicDrawPane, wxPanel)

	// some useful events
	/*
	 EVT_LEAVE_WINDOW(BasicDrawPane::mouseLeftWindow)
	 EVT_KEY_DOWN(BasicDrawPane::keyPressed)
	 EVT_KEY_UP(BasicDrawPane::keyReleased)
	 EVT_MOUSEWHEEL(BasicDrawPane::mouseWheelMoved)
	 */

	// catch paint events
	EVT_PAINT(BasicDrawPane::paintEvent)
	EVT_MOTION(BasicDrawPane::mouseControl)
	EVT_LEFT_DOWN(BasicDrawPane::mouseControl)
	EVT_LEFT_UP(BasicDrawPane::mouseControl)
	EVT_RIGHT_DOWN(BasicDrawPane::mouseControl)
	EVT_RIGHT_UP(BasicDrawPane::mouseControl)

END_EVENT_TABLE()

void BasicDrawPane::paintEvent(wxPaintEvent& WXUNUSED(event))
{
	wxAutoBufferedPaintDC dc(this);
	if (image.IsOk())
	{
		int width, height;
		wxWindow::GetClientSize(&width, &height);
		wxBitmap bmp(image.Scale(width, height, wxIMAGE_QUALITY_BOX_AVERAGE));
		dc.DrawBitmap(bmp, 0, 0);
	}
}

void BasicDrawPane::mouseControl(wxMouseEvent& WXUNUSED(event))
{
	if (cdi != NULL)
	{
		const wxMouseState state = wxGetMouseState();
		cdi->pd.set_button(PointingDevice::Button1, state.LeftIsDown());
		cdi->pd.set_button(PointingDevice::Button2, state.RightIsDown());

		wxPoint panel_coords = this->GetScreenPosition();
		wxPoint mouse_coords = state.GetPosition();
		int x = mouse_coords.x - panel_coords.x, y = mouse_coords.y - panel_coords.y;
		int width, height;
		wxWindow::GetClientSize(&width, &height);
		cdi->pd.set_coord(x, y, width, height);
	}
}

/*******************************************************************************
// SDLPanel Class
*******************************************************************************/

BEGIN_EVENT_TABLE(mainFrame, wxFrame)
	EVT_IDLE(mainFrame::e_idle)
	EVT_MENU(wxID_OPEN_ROM, mainFrame::e_openSystemROM)
	EVT_MENU(wxID_OPEN_DISC, mainFrame::e_openDisc)
	EVT_MENU(wxID_RESET_MACHINE, mainFrame::e_reset)
	EVT_MENU(wxID_EXIT, mainFrame::e_exit)
	EVT_MENU(wxID_ABOUT, mainFrame::e_about)
END_EVENT_TABLE()

mainFrame::mainFrame(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style)
{
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);
	// this->DragAcceptFiles(true);

	// Create menus
	wxMenu *menuFile = new wxMenu;
	menuOpenDisc = new wxMenuItem(menuFile, wxID_OPEN_DISC, wxString(_("Open &disc...")) + wxT('\t') + wxT("Ctrl+O"), wxEmptyString, wxITEM_NORMAL);
	menuOpenDisc->Enable(false);

	menuFile->Append(wxID_OPEN_ROM, wxString(_("Create &CD-i machine...")) + wxT('\t') + wxT("Ctrl+C"));
	menuFile->Append(menuOpenDisc);
	menuFile->AppendSeparator();
	menuFile->Append(wxID_RESET_MACHINE, wxString(_("&Reset")) + wxT('\t') + wxT("F1"));
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT, wxString(_("E&xit")) + wxT('\t') + wxT("Alt+F4"));

	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT, wxString(_("&About")));

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(menuFile, _("File"));
	menuBar->Append(menuHelp, _("Help"));
	this->SetMenuBar(menuBar);

	// Create panel
	mainPanel = new BasicDrawPane(this);
	mainPanel->SetBackgroundColour(wxColour(128, 128, 128));

	this->SetClientSize(384, 280);
	this->Centre(wxBOTH);

	wxIcon icon(app_icon_xpm);
	SetIcon(icon);
}

mainFrame::~mainFrame()
{
	if (cdi != NULL) {
		delete cdi;
	}
}

void mainFrame::e_reset(wxCommandEvent& WXUNUSED(event))
{
	if (cdi != NULL) {
		// if (wxMessageBox(_("All unsaved data will be lost.\nAre you sure you want to continue?"), _("Reset machine"), wxICON_QUESTION | wxYES_NO, this) == wxYES)
			cdi->reset();
	}
}

void mainFrame::e_exit(wxCommandEvent& WXUNUSED(event))
{
	this->Close();
}

void mainFrame::e_about(wxCommandEvent& WXUNUSED(event))
{
	wxMessageBox(wxT("wx-sdl tutorial\nCopyright (C) 2005 John Ratliff"),
				 wxT("about wx-sdl tutorial"), wxOK | wxICON_INFORMATION);
}

void mainFrame::e_openSystemROM(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog openFileDialog(this, _("Open system ROM"), "", "", "ROM (*.rom)|*.rom", wxFD_OPEN|wxFD_FILE_MUST_EXIST);
	if (openFileDialog.ShowModal() == wxID_OK)
	{
		if (cdi != NULL) { delete cdi; cdi = NULL; }
		cdi = new PhilipsCDI;
		const std::filesystem::path rom = std::string(openFileDialog.GetPath().mb_str());
		enum CDi::BoardType board = rom.stem().compare("cdi490a") == 0 ? CDi::MonoIV
								  : rom.stem().compare("cdi220c") == 0 ? CDi::MonoII
								  : CDi::MonoI;
		cdi->init(rom.string(), board);
		mainPanel->image = wxImage(CDI_SCREEN_WIDTH, CDI_SCREEN_HEIGHT);
		if (menuOpenDisc) menuOpenDisc->Enable(true);
	}
}

void mainFrame::e_openDisc(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog openFileDialog(this, _("Open disc"), "", "", "Binary (*.bin)|*.bin", wxFD_OPEN|wxFD_FILE_MUST_EXIST);

	if (openFileDialog.ShowModal() == wxID_OK && cdi != NULL)
		cdi->swap_disc(openFileDialog.GetPath());
}

void mainFrame::e_idle(wxIdleEvent& WXUNUSED(event))
{
	if (cdi != NULL)
	{
		cdi->run(false);

		// Update screen
		uint8_t *rgb = mainPanel->image.GetData();
		for (int y = 0; y < CDI_SCREEN_HEIGHT; y++) {
			for (int x = 0; x < CDI_SCREEN_WIDTH; x++) {
				uint32_t color = cdi->get_display()[CDI_SCREEN_WIDTH * y + x];
				#if SDL_BYTEORDER == SDL_BIG_ENDIAN
					*rgb++ = (color >> 8) & 0xFF;
					*rgb++ = (color >> 16) & 0xFF;
					*rgb++ = (color >> 24) & 0xFF;
				#else
					*rgb++ = (color >> 24) & 0xFF;
					*rgb++ = (color >> 16) & 0xFF;
					*rgb++ = (color >> 8) & 0xFF;
				#endif
			}
		}
		mainPanel->Refresh();
	}
}