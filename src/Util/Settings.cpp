#include <Util/Settings.h>
#include <FileAccess.h>
#include <common.h>
#include <regex>

using namespace Gtk;

static string ini_file_path = "/home/robert/Desktop/AdVisual.ini";
map<string, map<string, string>> AppSettings::msettings = {};

SettingsWindow::SettingsWindow() {
	set_modal(true);
	set_size_request(800, 600);
	set_title("AdVisual Settings at " + ini_file_path);
	shared_ptr<TextBuffer> text_buffer = TextBuffer::create();

	if (!FileAccess::access_file(ini_file_path, false)) { return; }
	char* c_str_buffer = (char*)malloc(FileAccess::file_length);
	FileAccess::fieldcpy_char(&c_str_buffer[0], FileAccess::file_length);
	Glib::ustring ustring_buffer(&c_str_buffer[0], &c_str_buffer[0] + FileAccess::file_length);
	free(c_str_buffer); // Free malloc ptr!
	FileAccess::close_file();

	set_child(grid);
	grid.attach(scrolled_window, 0, 0, 3, 1);
	grid.set_column_homogeneous(false);
	scrolled_window.set_child(text_view);
	scrolled_window.set_expand(true);
	text_buffer->set_text(ustring_buffer);
	text_view = TextView(text_buffer);
	text_view.set_editable(true);

	//cancel_button = Button("Cancel");
	//cancel_button.signal_clicked().connect(mem_fun(*this, &SettingsWindow::on_cancel));
	//grid.attach(cancel_button, 1, 1);

	//apply_button = Button("Save & Apply");
	//apply_button.signal_clicked().connect(mem_fun(*this, &SettingsWindow::on_save_and_apply));
	//grid.attach(apply_button, 2, 1);
}

void SettingsWindow::on_save_and_apply() {
	// Save settings to file.
	if (!FileAccess::access_file(ini_file_path, true)) { return; }
	Glib::ustring ustring_buffer = text_view.get_buffer()->get_text();
	FileAccess::file.writeString(ustring_buffer.c_str(), ustring_buffer.length());
	FileAccess::close_file();
	// Parse updated settings and apply.
	AppSettings::parse_ini_file(ini_file_path);
	AppSettings::apply_settings();
}

void SettingsWindow::on_cancel() {
	close();
}

void AppSettings::define_default_settings() {
	// We don't predefine any settings inline, but we do define groups
	// to help with user warnings.
	msettings["common"] = {};
	msettings["common_shortcuts"] = {};
	msettings["composer_shortcuts"] = {};
	msettings["insmaker_shortcuts"] = {};

	parse_ini_file(ini_file_path);
}

void AppSettings::apply_settings() {
	apply_group_shortcuts("actions", "common_shortcuts");
	apply_group_shortcuts("composer", "composer_shortcuts");
	apply_group_shortcuts("insmaker", "insmaker_shortcuts");
}

void AppSettings::apply_group_shortcuts(string action_group, string setting_group) {
	map<string, string> shortcut_settings = AppSettings::get_group(setting_group);
	for (auto const& pair : shortcut_settings) {
		if (pair.second.empty()) { continue; }
		app->set_accel_for_action(action_group + "." + pair.first, pair.second);
	}
}

void print_matches(smatch& matches) {
	cout << "{ ";
	for (int i = 0; i < matches.size(); ++i) {
		cout << "\"" << matches[i] << "\" ";
	}
	cout << "}\n";
}

string trim_whitespace(string base_str) {
	int real_start = base_str.find_first_not_of(" ");
	int real_end = base_str.find_last_not_of(" ") + 1;
	return base_str.substr(real_start, real_end - real_start);
}

map<string, string> AppSettings::get_group(string group) {
	return msettings[group];
}

string AppSettings::get_setting(string group, string key) {
	return msettings[group][key];
}

void AppSettings::set_setting(string group, string key, string value) {
	msettings[group][key] = value;
}

void AppSettings::parse_ini_file(string file_path) {
	if (!FileAccess::access_file(file_path, false)) {
		cerr << "Could not find settings file\n";
		return;
	}
	DBPRINT("Start parse settings");
	const int max_line_len = 128;
	const regex comment_regex("^[^;]*", regex_constants::ECMAScript | regex_constants::icase);
	const regex brackets_regex("\\[(.+?)\\]", regex_constants::ECMAScript | regex_constants::icase);
	const regex key_regex("^[^=]*", regex_constants::ECMAScript | regex_constants::icase);

	char* c_line_buffer = (char*)malloc(max_line_len);
	string current_group_str = "";
	map<string, string>* current_group = nullptr;
	int current_line = -1;
	while (!FileAccess::error) {
		current_line++;
		// Parses line by line.
		int max_read_len = min(int(FileAccess::file_length - FileAccess::file.pos()), max_line_len);
		if (max_read_len <= 0) { break; }
		int line_length = FileAccess::fieldcpy_char(&c_line_buffer[0], max_read_len, '\n');
		string line_buffer(&c_line_buffer[0], &c_line_buffer[line_length]);
		smatch matches;
		if (regex_search(line_buffer, matches, comment_regex)) {
			line_buffer = matches[0];
		}
		if (regex_search(line_buffer, matches, brackets_regex)) {
			string current_group_str = trim_whitespace(matches[1]);
			if (!msettings.contains(current_group_str)) {
				cerr << "No previous setting group by the name " << current_group_str << "!\n";
				msettings[current_group_str] = {};
			}
			current_group = &msettings[current_group_str];
			DBPRINT("Line " << current_line << " starts a group, \"" << current_group_str << "\"");
			continue;
		}
		long equals_delim = line_buffer.find("=");
		if (equals_delim > 0) {
			// Can only be a key value pair now.
			string key_str = trim_whitespace(line_buffer.substr(0, equals_delim));
			string value_str = ""; // Possibly empty value.
			if (equals_delim != line_buffer.length()-1) {
				value_str = trim_whitespace(line_buffer.substr(equals_delim + 1));
			}
			if (current_group == nullptr) {
				cerr << "Definition before any target group was set!\n";
				continue;
			}
			(*current_group)[key_str] = value_str;
			DBPRINT("Line " << current_line << " is a definition, " << key_str << "=" << value_str);
		}
	}
	DBPRINT("End parse settings");
	free(c_line_buffer); // Free malloc ptr!
	FileAccess::close_file();
}