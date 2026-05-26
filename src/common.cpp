#include <common.h>

shared_ptr<Gtk::Application> app;
shared_ptr<Gtk::Label> status;
shared_ptr<Gio::SimpleActionGroup> common_action_group;

#ifdef WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <winreg.h>
	string get_advisual_dir() {
		// Note that INSTALL_PATH is unreliable at best on windows.
		// Therefore we use a definition from the windows registry.
		HKEY h_key;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\AdVisual\\AdVisual", 0, KEY_READ, &h_key) != ERROR_SUCCESS) {
			cerr << "Could not open AdVisual registry to get install path\n";
			return (string)INSTALL_PATH; // Send INSTALL_PATH as last resort.
		}
		string str_key = ""; // Need the default key, so leave blank.
		CHAR buffer[512];
		DWORD buffer_size = sizeof(buffer);
		if (RegQueryValueEx(h_key, str_key.c_str(), 0, NULL, (LPBYTE)buffer, &buffer_size) != ERROR_SUCCESS) {
			cerr << "Could not get AdVisual install path from registry\n";
			return (string)INSTALL_PATH; // Send INSTALL_PATH as last resort.
		}
		string str_value = buffer;
		return str_value + "/";
	}
#else
	string get_advisual_dir() {
		return (string)INSTALL_PATH;
	}
#endif

int sign(int val) {
	return (val > 0) - (val < 0);
}

void string_to_upper(string& str) {
	transform(str.begin(), str.end(), str.begin(), ::toupper);
}

void line_exclusion(int p1, int l1, int p2, int l2, int& out_p, int& out_length) {
	if (p1 != p2 && p1 + l1 != p2 + l2) { // Rect is resizing in both directions.
		out_p = min(p1, p2);
		out_length = max(p1 + p1, p2 + l2) - out_p;
		return;
	}
	if (p1 != p2) { // Rect is resizing left.
		out_p = min(p1, p2);
		out_length = abs(p2 - p1);
	}
	else if (p1 + l1 != p2 + l2) { // Rect is resizing right.
		out_p = p1; // We already know new_rect.x and old_rect.x are equal.
		out_p += min(l1, l2);
		out_length = max(l1 - l2, l2 - l1);
	}
}

string note_number_to_letter(int note_number) {
	string note_symbol = "--1";
	// Add 3 to make up for the pitch start cuttof. Add an octave so it wraps from pitch start.
	int letter = (((note_number - 3) + full_octave) % full_octave); 
	char modifier = '-';
	if (letter >= 4) {
		letter += 1;
	}
	if (letter >= 10) {
		letter += 1;
	}
	modifier = (letter % 2 == 1 ? '-' : '#');
	letter = (letter / 2);
	note_symbol[0] = char(71 - letter);
	note_symbol[1] = modifier;
	note_symbol[2] = char(48 + (note_number / full_octave));
	return note_symbol;
}

float get_float_from_string(string str_val, float min, float max) {
	if (str_val.empty()) return -1.0;
	float ret_float = 1.0;
	try {
		ret_float = stof(str_val);
		ret_float = clamp(ret_float, min, max);
	}
	catch (invalid_argument e) {
		cerr << str_val << " Not a float!\n";
		ret_float = -1.0;
	}
	catch (out_of_range e) {
		cerr << str_val << " Float out of range\n";
		ret_float = -1.0;
	}
	return ret_float;
}

Gtk::ToggleButton create_image_button(string image_name, bool large_icon) {
using namespace Gtk;
	ToggleButton bttn;
	Image bttn_img(ICON_PATH(image_name));
	bttn_img.add_css_class(large_icon ? "large-image" : "small-image");
	bttn_img.set_icon_size(IconSize::LARGE);
	bttn.set_child(bttn_img);
	return bttn;
}

Gtk::ToggleButton create_picture_button(string picture_name) {
using namespace Gtk;
	ToggleButton bttn;
	Gtk::Picture pic = Gtk::Picture(ICON_PATH(picture_name));
	pic.set_size_request(10, 10);
	pic.set_can_shrink(true);
	pic.set_content_fit(Gtk::ContentFit::SCALE_DOWN);
	bttn.set_child(pic);
	return bttn;
}
