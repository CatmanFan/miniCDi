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
	// EVT_MOTION(BasicDrawPane::mouseControl)
	// EVT_LEFT_DOWN(BasicDrawPane::mouseControl)
	// EVT_LEFT_UP(BasicDrawPane::mouseControl)
	// EVT_RIGHT_DOWN(BasicDrawPane::mouseControl)
	// EVT_RIGHT_UP(BasicDrawPane::mouseControl)

END_EVENT_TABLE()

void BasicDrawPane::paintEvent(wxPaintEvent& WXUNUSED(event))
{
	wxAutoBufferedPaintDC dc(this);
	if (image.IsOk())
	{
		int width, height;
		wxWindow::GetClientSize(&width, &height);
		wxBitmap bmp(image.Scale(width, height, wxIMAGE_QUALITY_BILINEAR));
		dc.DrawBitmap(bmp, 0, 0);
	}
}

static wxStatusBar* statusBar;

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
	EVT_MENU_RANGE(wxID_LANG_ENGLISH, wxID_LANG_JAPANESE, mainFrame::e_changeLanguage)
	EVT_MENU_RANGE(wxID_CONFIG_TESTPLUG, wxID_CONFIG_NTSC, mainFrame::e_toggleEmulationSetting)
END_EVENT_TABLE()

mainFrame::mainFrame(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style)
{
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);
	// this->DragAcceptFiles(true);

	// Create menus (the labels will be filled in by ResetLanguageStrings)
	menuLanguage = new wxMenu;
		for (int i = 0; i < language.count; i++)
		{
			language.items[i] = new wxMenuItem(menuLanguage, language.itemID[i], wxLocale::GetLanguageInfo(language.wxCodes[i])->DescriptionNative, wxEmptyString, wxITEM_CHECK);
			menuLanguage->Append(language.items[i]);
		}

	wxMenu *menuFile = new wxMenu;
		menuCreateMachine = new wxMenuItem(menuFile, wxID_OPEN_ROM, " ");
		menuOpenDisc = new wxMenuItem(menuFile, wxID_OPEN_DISC, " ");
		menuReset = new wxMenuItem(menuFile, wxID_RESET_MACHINE, " ");
		menuExit = new wxMenuItem(menuFile, wxID_EXIT, " ");
		menuLanguageItem = new wxMenuItem(menuFile, wxID_ANY, " ", wxEmptyString, wxITEM_NORMAL, menuLanguage);

		menuFile->Append(menuCreateMachine);
		menuFile->Append(menuOpenDisc);
		menuFile->AppendSeparator();
		menuFile->Append(menuReset);
		menuFile->AppendSeparator();
		menuFile->Append(menuLanguageItem);
		menuFile->AppendSeparator();
		menuFile->Append(menuExit);

	wxMenu *menuEmulation = new wxMenu;
		menuToggleTestPlug = new wxMenuItem(menuFile, wxID_CONFIG_TESTPLUG, " ", wxEmptyString, wxITEM_CHECK);
		menuToggleAnalogColors = new wxMenuItem(menuFile, wxID_CONFIG_ANALOGCOLORS, " ", wxEmptyString, wxITEM_CHECK);
		menuToggleNoFrameLimit = new wxMenuItem(menuFile, wxID_CONFIG_NOFRAMELIMIT, " ", wxEmptyString, wxITEM_CHECK);
		menuToggleNTSC = new wxMenuItem(menuFile, wxID_CONFIG_NTSC, " ", wxEmptyString, wxITEM_CHECK);

		menuEmulation->Append(menuToggleTestPlug);
		menuEmulation->Append(menuToggleNTSC);
		menuEmulation->AppendSeparator();
		menuEmulation->Append(menuToggleAnalogColors);
		menuEmulation->AppendSeparator();
		menuEmulation->Append(menuToggleNoFrameLimit);

	wxMenu *menuHelp = new wxMenu;
		menuAbout = new wxMenuItem(menuFile, wxID_ABOUT, " ");
		menuHelp->Append(menuAbout);

	// Create menu bar
	menuBar = new wxMenuBar;
	menuBar->Append(menuFile, " ");
	menuBar->Append(menuEmulation, " ");
	menuBar->Append(menuHelp, " ");
	this->SetMenuBar(menuBar);

	// Create panel
	mainPanel = new BasicDrawPane(this);
	mainPanel->SetBackgroundColour(wxColour(128, 128, 128));
	// statusBar = this->CreateStatusBar(1, wxSTB_SIZEGRIP, wxID_ANY);

	wxIcon icon(app_icon_xpm);
	SetIcon(icon);

	this->ReloadLanguage(wxLANGUAGE_DEFAULT);
	menuOpenDisc->Enable(false);

	menuToggleTestPlug->Check(MiniCDI::Config::TestPlug);
	menuToggleAnalogColors->Check(MiniCDI::Config::AnalogColors);
	menuToggleNoFrameLimit->Check(MiniCDI::Config::NoFrameLimit);
	menuToggleNTSC->Check(!MiniCDI::Config::PAL);

	this->SetClientSize(384, 280);
	this->Centre(wxBOTH);
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
		cdi->reset();
	}
}

void mainFrame::e_changeLanguage(wxCommandEvent &event)
{
	for (int i = 0; i < language.count; i++)
	{
		if (event.GetId() == language.itemID[i])
		{
			this->ReloadLanguage(language.wxCodes[i]);
			this->Refresh();
			return;
		}
	}
}

void mainFrame::e_toggleEmulationSetting(wxCommandEvent &event)
{
	int id = event.GetId();
	switch (id)
	{
		case wxID_CONFIG_TESTPLUG:
			MiniCDI::Config::TestPlug = !MiniCDI::Config::TestPlug;
			break;

		case wxID_CONFIG_NTSC:
			if (cdi != NULL)
			{
				if (wxMessageBox(_("This will reset the CD-i player. Any unsaved data will be lost.\nContinue anyway?"), _("Are you sure?"), wxICON_QUESTION | wxYES_NO, this) == wxYES)
				{
					MiniCDI::Config::PAL = !MiniCDI::Config::PAL;
					cdi->reset();
				}
			}
			else
			{
				MiniCDI::Config::PAL = !MiniCDI::Config::PAL;
			}
			break;

		case wxID_CONFIG_ANALOGCOLORS:
			MiniCDI::Config::AnalogColors = !MiniCDI::Config::AnalogColors;
			break;

		case wxID_CONFIG_NOFRAMELIMIT:
			MiniCDI::Config::NoFrameLimit = !MiniCDI::Config::NoFrameLimit;
			break;
	}

	menuToggleTestPlug->Check(MiniCDI::Config::TestPlug);
	menuToggleAnalogColors->Check(MiniCDI::Config::AnalogColors);
	menuToggleNoFrameLimit->Check(MiniCDI::Config::NoFrameLimit);
	menuToggleNTSC->Check(!MiniCDI::Config::PAL);
}

void mainFrame::e_exit(wxCommandEvent& WXUNUSED(event))
{
	this->Close();
}

void mainFrame::e_about(wxCommandEvent& WXUNUSED(event))
{
	wxMessageBox(_("miniCDi is a portable CD-i player emulator."),
				 _("About"), wxOK | wxICON_INFORMATION);
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
		// Update pointing device status based on mouse control (wxMouseEvent is not used because it does not update when the mouse is not moving).
		const wxMouseState state = wxGetMouseState();
		wxPoint panel_coords = mainPanel->GetScreenPosition();
		wxPoint mouse_coords = state.GetPosition();
		int x = mouse_coords.x - panel_coords.x, y = mouse_coords.y - panel_coords.y;
		int width, height;
		wxWindow::GetClientSize(&width, &height);

		cdi->pd.set_button(PointingDevice::Button1, state.LeftIsDown());
		cdi->pd.set_button(PointingDevice::Button2, state.RightIsDown());
		cdi->pd.set_coord(x, y, width, height); // This has to be set AFTER `set_button` so that it can be polled

		// Actually run a frame
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