#pragma once

#include <gtkmm.h>
#include <string>
using namespace std;

class MetaDataWindow : public Gtk::Window {
public:
	MetaDataWindow();
	void set_base_string(string* p_string_ptr);
	void set_max_length(size_t p_max_len);
protected:
	void on_change_text();
	bool on_close_request();
	size_t max_length = 0;
	Gtk::ScrolledWindow scrolled_window;
	Gtk::TextView text_view;
	string* string_ptr;
};