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
		void mouseControl(wxMouseEvent &event);
		
		DECLARE_EVENT_TABLE()
	};

	class mainFrame : public wxFrame
	{
		DECLARE_EVENT_TABLE()
		void e_openSystemROM(wxCommandEvent &event);
		void e_openDisc(wxCommandEvent &event);
		void e_about(wxCommandEvent &event);
		void e_reset(wxCommandEvent &event);
		void e_exit(wxCommandEvent &event);
		void e_idle(wxIdleEvent &event);

		wxMenuItem *menuOpenDisc;
		BasicDrawPane* mainPanel;

		enum
		{
			wxID_GAME_PANEL = 6000,
			wxID_OPEN_ROM = 6001,
			wxID_OPEN_DISC = 6002,
			wxID_RESET_MACHINE = 6003,
		};

		public:
			mainFrame(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("miniCDi"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(-1,-1), long style = wxCLOSE_BOX|wxDEFAULT_FRAME_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER|wxSYSTEM_MENU|wxTAB_TRAVERSAL);
			~mainFrame();
	};
}