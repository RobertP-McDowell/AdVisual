#pragma once

#include <gtkmm.h>
#include <string>
#include <map>

using namespace std;

class AppSettings {
public:
	static void initialize();
	static void apply_settings();
	static map<string, string> get_group(string group);
	static string get_group(string group, string key);
	static string get_setting(string group, string key);
	static void set_setting(string group, string key, string value);
	static void parse_ini_file(string file_path);
	static void apply_group_shortcuts(string action_group, string setting_group);
	static void write_default_file();
	static shared_ptr<Gtk::CssProvider> css_provider;
protected:
	static map<string, map<string, string>> msettings;
};

class SettingsWindow : public Gtk::Window {
public:
	SettingsWindow();
protected:
	void on_save_and_apply();
	void on_cancel();
	Gtk::Grid grid;
	Gtk::ScrolledWindow scrolled_window;
	Gtk::TextView text_view;
};