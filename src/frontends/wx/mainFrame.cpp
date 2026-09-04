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
#define CONTROL_MOUSE_ONLY

/*******************************************************************************
// BasicDrawPane
*******************************************************************************/

BEGIN_EVENT_TABLE(BasicDrawPane, wxPanel)
	EVT_PAINT(BasicDrawPane::e_paintEvent)
	EVT_KEY_DOWN(BasicDrawPane::e_keyControl)
	EVT_KEY_UP(BasicDrawPane::e_keyControl)
	// EVT_MOTION(BasicDrawPane::e_mouseControl)
	// EVT_LEFT_DOWN(BasicDrawPane::e_mouseControl)
	// EVT_LEFT_UP(BasicDrawPane::e_mouseControl)
	// EVT_RIGHT_DOWN(BasicDrawPane::e_mouseControl)
	// EVT_RIGHT_UP(BasicDrawPane::e_mouseControl)
END_EVENT_TABLE()

void BasicDrawPane::e_paintEvent(wxPaintEvent& WXUNUSED(event))
{
	wxAutoBufferedPaintDC dc(this);
	if (image.IsOk())
	{
		int width, height;
		wxWindow::GetClientSize(&width, &height);
		wxBitmap bmp(image.Scale(width, height, wxIMAGE_QUALITY_NEAREST));
		dc.DrawBitmap(bmp, 0, 0);
	}
}

void BasicDrawPane::e_keyControl(wxKeyEvent& event)
{
	#ifndef CONTROL_MOUSE_ONLY

		if (cdi != NULL)
		{
			switch (event.GetKeyCode())
			{
				case WXK_LEFT:
				case WXK_NUMPAD4:
				case WXK_HOME:
					cdi->pd.set_button(PointingDevice::Left, event.GetEventType() != wxEVT_KEY_UP);
					break;

				case WXK_RIGHT:
				case WXK_NUMPAD6:
				case WXK_END:
					cdi->pd.set_button(PointingDevice::Right, event.GetEventType() != wxEVT_KEY_UP);
					break;

				case WXK_UP:
				case WXK_NUMPAD8:
				case WXK_PAGEUP:
					cdi->pd.set_button(PointingDevice::Up, event.GetEventType() != wxEVT_KEY_UP);
					break;

				case WXK_DOWN:
				case WXK_NUMPAD2:
				case WXK_PAGEDOWN:
					cdi->pd.set_button(PointingDevice::Down, event.GetEventType() != wxEVT_KEY_UP);
					break;

				case WXK_RETURN:
					cdi->pd.set_button(PointingDevice::Button1, event.GetEventType() != wxEVT_KEY_UP);
					break;

				case WXK_SPACE:
					cdi->pd.set_button(PointingDevice::Button2, event.GetEventType() != wxEVT_KEY_UP);
					break;
			}
		}

	#endif

	event.Skip();
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
	EVT_MENU_RANGE(wxID_LANG_ENGLISH, wxID_LANG_JAPANESE, mainFrame::e_changeLanguage)
	EVT_MENU_RANGE(wxID_VIEW_RESIZE1X, wxID_VIEW_RESIZE2X, mainFrame::e_resizeView)
	EVT_MENU_RANGE(wxID_CONFIG_TESTPLUG, wxID_CONFIG_RESETPD, mainFrame::e_toggleEmulationSetting)
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

	wxMenu *menuView = new wxMenu;
		menuResize1x = new wxMenuItem(menuView, wxID_VIEW_RESIZE1X, " ");
		menuResize2x = new wxMenuItem(menuView, wxID_VIEW_RESIZE2X, " ");

		menuView->Append(menuResize1x);
		menuView->Append(menuResize2x);

	wxMenu *menuEmulation = new wxMenu;
		menuToggleTestPlug = new wxMenuItem(menuEmulation, wxID_CONFIG_TESTPLUG, " ", wxEmptyString, wxITEM_CHECK);
		menuToggleLLTest = new wxMenuItem(menuEmulation, wxID_CONFIG_LLTEST, " ", wxEmptyString, wxITEM_CHECK);
		menuToggleAnalogColors = new wxMenuItem(menuEmulation, wxID_CONFIG_ANALOGCOLORS, " ", wxEmptyString, wxITEM_CHECK);
		menuToggleNoFrameLimit = new wxMenuItem(menuEmulation, wxID_CONFIG_NOFRAMELIMIT, " ", wxEmptyString, wxITEM_CHECK);
		menuToggleNTSC = new wxMenuItem(menuEmulation, wxID_CONFIG_NTSC, " ", wxEmptyString, wxITEM_CHECK);
		menuResetPD = new wxMenuItem(menuEmulation, wxID_CONFIG_RESETPD, " ");

		menuEmulation->Append(menuResetPD);
		menuEmulation->AppendSeparator();
		menuEmulation->Append(menuToggleTestPlug);
		menuEmulation->Append(menuToggleLLTest);
		menuEmulation->AppendSeparator();
		menuEmulation->Append(menuToggleNTSC);
		menuEmulation->Append(menuToggleAnalogColors);
		menuEmulation->AppendSeparator();
		menuEmulation->Append(menuToggleNoFrameLimit);

	wxMenu *menuHelp = new wxMenu;
		menuAbout = new wxMenuItem(menuHelp, wxID_ABOUT, " ");
		menuHelp->Append(menuAbout);

	// Create menu bar
	menuBar = new wxMenuBar;
	menuBar->Append(menuFile, " ");
	menuBar->Append(menuView, " ");
	menuBar->Append(menuEmulation, " ");
	menuBar->Append(menuHelp, " ");
	this->SetMenuBar(menuBar);

	// Create panel
	mainPanel = new BasicDrawPane(this);
	mainPanel->SetBackgroundColour(wxColour(128, 128, 128));
	statusBar = this->CreateStatusBar(1, wxSTB_SIZEGRIP, wxID_ANY);

	wxIcon icon(app_icon_xpm);
	SetIcon(icon);

	this->ReloadLanguage(wxLANGUAGE_DEFAULT);
	menuOpenDisc->Enable(false);
	menuResetPD->Enable(false);

	menuToggleTestPlug->Check(MiniCDI::Config.TestPlug);
	menuToggleLLTest->Check(MiniCDI::Config.PCB_LLTest);
	menuToggleAnalogColors->Check(MiniCDI::Config.AnalogColors);
	menuToggleNoFrameLimit->Check(MiniCDI::Config.NoFrameLimit);
	menuToggleNTSC->Check(!MiniCDI::Config.PAL);

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
		statusBar->SetStatusText("Reset");
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

void mainFrame::e_resizeView(wxCommandEvent &event)
{
	if (!this->IsMaximized())
	{
		int id = event.GetId();
		switch (id)
		{
			default:
			case wxID_VIEW_RESIZE1X:
				this->SetClientSize(384, 280);
				break;

			case wxID_VIEW_RESIZE2X:
				this->SetClientSize(768, 560);
				break;
		}
	}
}

void mainFrame::e_toggleEmulationSetting(wxCommandEvent &event)
{
	int id = event.GetId();
	switch (id)
	{
		case wxID_CONFIG_TESTPLUG:
			MiniCDI::Config.TestPlug = !MiniCDI::Config.TestPlug;
			statusBar->SetStatusText(wxString::Format("Test plug %s", MiniCDI::Config.TestPlug ? "connected" : "disconnected"));
			break;

		case wxID_CONFIG_LLTEST:
			MiniCDI::Config.PCB_LLTest = !MiniCDI::Config.PCB_LLTest;
			statusBar->SetStatusText(wxString::Format("%s PCB low-level test on boot", MiniCDI::Config.PCB_LLTest ? "Enabled" : "Disabled"));
			break;

		case wxID_CONFIG_NTSC:
			if (cdi != NULL)
			{
				if (wxMessageBox(_("This will reset the CD-i player. Any unsaved data will be lost.\nContinue anyway?"), _("Are you sure?"), wxICON_QUESTION | wxYES_NO, this) == wxYES)
				{
					MiniCDI::Config.PAL = !MiniCDI::Config.PAL;
					statusBar->SetStatusText("Reset");
					cdi->reset();
				}
			}
			else
			{
				MiniCDI::Config.PAL = !MiniCDI::Config.PAL;
			}
			break;

		case wxID_CONFIG_ANALOGCOLORS:
			MiniCDI::Config.AnalogColors = !MiniCDI::Config.AnalogColors;
			break;

		case wxID_CONFIG_NOFRAMELIMIT:
			MiniCDI::Config.NoFrameLimit = !MiniCDI::Config.NoFrameLimit;
			statusBar->SetStatusText(wxString::Format("Frame throttling %s", MiniCDI::Config.NoFrameLimit ? "disabled" : "enabled"));
			break;

		case wxID_CONFIG_RESETPD:
			if (cdi != NULL) cdi->reset_pd();
			return;
	}

	menuToggleTestPlug->Check(MiniCDI::Config.TestPlug);
	menuToggleLLTest->Check(MiniCDI::Config.PCB_LLTest);
	menuToggleAnalogColors->Check(MiniCDI::Config.AnalogColors);
	menuToggleNoFrameLimit->Check(MiniCDI::Config.NoFrameLimit);
	menuToggleNTSC->Check(!MiniCDI::Config.PAL);
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
		if (menuResetPD) menuResetPD->Enable(true);

		MiniCDI::Config.PointerAdvance = 3;
		statusBar->SetStatusText(wxString::Format("Loaded from %s.rom", rom.stem().c_str()));
	}
}

void mainFrame::e_openDisc(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog openFileDialog(this, _("Open disc"), "", "", "Binary (*.bin)|*.bin", wxFD_OPEN|wxFD_FILE_MUST_EXIST);

	if (openFileDialog.ShowModal() == wxID_OK && cdi != NULL)
	{
		cdi->swap_disc(openFileDialog.GetPath());
		statusBar->SetStatusText(wxString::Format("Inserted disc %s", openFileDialog.GetPath().mb_str()));
	}
}

void mainFrame::e_idle(wxIdleEvent& WXUNUSED(event))
{
	if (cdi != NULL && this->IsActive())
	{
		#ifdef CONTROL_MOUSE_ONLY

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

		#endif

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