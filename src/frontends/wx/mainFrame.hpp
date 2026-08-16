#pragma once
#include <wx/wx.h>

namespace miniCDi
{
	class BasicDrawPane : public wxPanel
	{
	public:
		BasicDrawPane(wxFrame* parent) : wxPanel(parent) { SetBackgroundStyle(wxBG_STYLE_PAINT); }
		wxImage image;

		void paintEvent(wxPaintEvent &event);
		// void mouseControl(wxMouseEvent &event);

		DECLARE_EVENT_TABLE()
	};

	class mainFrame : public wxFrame
	{
		DECLARE_EVENT_TABLE()
		void e_openSystemROM(wxCommandEvent &event);
		void e_openDisc(wxCommandEvent &event);
		void e_changeLanguage(wxCommandEvent &event);
		void e_toggleEmulationSetting(wxCommandEvent &event);
		void e_about(wxCommandEvent &event);
		void e_reset(wxCommandEvent &event);
		void e_exit(wxCommandEvent &event);
		void e_idle(wxIdleEvent &event);

		enum
		{
			wxID_GAME_PANEL = 6000,
			wxID_OPEN_ROM,
			wxID_OPEN_DISC,
			wxID_RESET_MACHINE,
			wxID_CONFIG_TESTPLUG,
			wxID_CONFIG_ANALOGCOLORS,
			wxID_CONFIG_NOFRAMELIMIT,
			wxID_CONFIG_NTSC,

			wxID_LANG_ENGLISH,
			wxID_LANG_FRENCH,
			wxID_LANG_JAPANESE,
		};

		BasicDrawPane* mainPanel;

		wxMenuBar *menuBar;
		// File
		wxMenuItem *menuCreateMachine;
		wxMenuItem *menuOpenDisc;
		wxMenuItem *menuReset;
		wxMenuItem *menuExit;
		// Emulation
		wxMenuItem *menuToggleTestPlug;
		wxMenuItem *menuToggleAnalogColors;
		wxMenuItem *menuToggleNoFrameLimit;
		wxMenuItem *menuToggleNTSC;
		// Help
		wxMenuItem *menuAbout;

		struct
		{
			#define LANG_COUNT 2

			int current;
			int count = LANG_COUNT;
			long wxCodes[LANG_COUNT]
			{
				wxLANGUAGE_ENGLISH,
				wxLANGUAGE_JAPANESE
			};
			int itemID[LANG_COUNT]
			{
				wxID_LANG_ENGLISH,
				wxID_LANG_JAPANESE
			};
			wxMenuItem *items[LANG_COUNT];

			#undef LANG_COUNT
		} language;
		wxMenu *menuLanguage;
		wxMenuItem *menuLanguageItem;
		wxLocale* locale;

		void ReloadLanguage(long code)
		{
			if (code == wxLANGUAGE_DEFAULT)
				code = wxLocale::GetSystemLanguage();

			if (language.current == code)
				goto end;

			language.current = -1;
			for (int i = 0; i < language.count; i++)
			{
				if (code == language.wxCodes[i])
				{
					language.current = code;
					break;
				}
			}

			if (language.current < 0)
				language.current = wxLANGUAGE_ENGLISH;

			if (locale != NULL)
			{
				delete locale;
				locale = NULL;
			}

			// load language if possible, fall back to english otherwise
			if (wxLocale::IsAvailable(language.current))
			{
				locale = new wxLocale(language.current);
				locale->AddCatalogLookupPathPrefix("./lang");
				locale->AddCatalog(wxT("miniCDi"));

				if (!locale->IsOk())
				{
					delete locale;
					locale = new wxLocale(wxLANGUAGE_ENGLISH);
					language.current = wxLANGUAGE_ENGLISH;
				}
			}
			else
			{
				locale = new wxLocale(wxLANGUAGE_ENGLISH);
				language.current = wxLANGUAGE_ENGLISH;
			}

			// Set labels
			if (menuBar != NULL)
			{
				menuBar->SetMenuLabel(0, _("&File"));
				menuBar->SetMenuLabel(1, _("&Emulation"));
				menuBar->SetMenuLabel(2, _("&Help"));
			}

			// File
			menuCreateMachine->SetItemLabel(wxString(_("Create &CD-i machine...")) + wxT('\t') + wxT("Ctrl+C"));
			menuOpenDisc->SetItemLabel(wxString(_("Open &disc...")) + wxT('\t') + wxT("Ctrl+O"));
			menuReset->SetItemLabel(wxString(_("&Reset")) + wxT('\t') + wxT("F1"));
			menuLanguageItem->SetItemLabel(wxString(_("Language")));
			menuExit->SetItemLabel(wxString(_("E&xit")) + wxT('\t') + wxT("Alt+F4"));
			// Emulation
			menuToggleTestPlug->SetItemLabel(wxString(_("&Connect test plug")));
			menuToggleAnalogColors->SetItemLabel(wxString(_("&Analog colours")));
			menuToggleNoFrameLimit->SetItemLabel(wxString(_("Disable frame &limit")));
			menuToggleNTSC->SetItemLabel(wxString(_("Set machine as &NTSC")));
			// Help
			menuAbout->SetItemLabel(wxString(_("&About")));

			end:
			for (int i = 0; i < language.count; i++)
				language.items[i]->Check(language.current == language.wxCodes[i]);
		}

		public:
			mainFrame(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("miniCDi"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(-1,-1), long style = wxCLOSE_BOX|wxDEFAULT_FRAME_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER|wxSYSTEM_MENU|wxTAB_TRAVERSAL);
			~mainFrame();
	};
}