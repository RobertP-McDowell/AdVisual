#pragma once

#include <gtkmm.h>

class HelpWindow : public Gtk::Window {
public:
	HelpWindow();
protected:
	Gtk::ScrolledWindow scrolled_window;
	Gtk::TextView text_view;
};