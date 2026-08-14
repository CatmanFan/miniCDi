#pragma once
#include <wx/wx.h>

namespace miniCDi
{
	class BasicDrawPane : public wxPanel
	{
	public:
		BasicDrawPane(wxFrame* parent) : wxPanel(parent) { SetBackgroundStyle(wxBG_STYLE_PAINT); }
		wxImage image;
		
		void paintEvent(wxPaintEvent & evt);
		void paintNow();
		
		void render(wxDC& dc);
		
		// some useful events
		/*
		 void mouseMoved(wxMouseEvent& event);
		 void mouseDown(wxMouseEvent& event);
		 void mouseWheelMoved(wxMouseEvent& event);
		 void mouseReleased(wxMouseEvent& event);
		 void rightClick(wxMouseEvent& event);
		 void mouseLeftWindow(wxMouseEvent& event);
		 void keyPressed(wxKeyEvent& event);
		 void keyReleased(wxKeyEvent& event);
		 */
		
		DECLARE_EVENT_TABLE()
	};

	class mainFrame : public wxFrame
	{
		DECLARE_EVENT_TABLE()
		void e_openSystemROM(wxCommandEvent &event);
		void e_about(wxCommandEvent &event);
		void e_reset(wxCommandEvent &event);
		void e_exit(wxCommandEvent &event);
		void e_idle(wxIdleEvent &event);
		// void e_fbPaint(wxPaintEvent &event);

		wxImage m_screen;
		BasicDrawPane* mainPanel;

		enum
		{
			wxID_GAME_PANEL = 6000,
			wxID_OPEN_ROM = 6001,
			wxID_RESET_MACHINE = 6002,
		};

		public:
			mainFrame(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("miniCDi"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(768,560), long style = wxCLOSE_BOX|wxDEFAULT_FRAME_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER|wxSYSTEM_MENU|wxTAB_TRAVERSAL);
			~mainFrame();
	};
}