#pragma once
#include <wx/wx.h>
#include "mainFrame.hpp"

namespace miniCDi
{
	class uiLauncher : public wxApp
	{
	public:
		uiLauncher();
		~uiLauncher();

	private:
		miniCDi::mainFrame* frame = nullptr;

	public:
		virtual int OnRun();
		virtual bool OnInit();
	};
};

DECLARE_APP(miniCDi::uiLauncher)