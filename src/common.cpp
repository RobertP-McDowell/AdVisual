#include <common.h>

unique_ptr<AdPlayer> adplayer = nullptr;
unique_ptr<Track> current_track = nullptr;
unique_ptr<Bank> current_bank = nullptr;
Channel* current_channel = nullptr;
vector<bool> enabled_channels = {};
wxStatusBar* status_bar;
int current_channel_idx = 0;
int cursor_tick = 0;

wxBitmapBundle GetAsset(wxString asset_name, wxSize asset_size) {
	return wxBitmapBundle::FromSVGFile(ASSETS_PATH + asset_name, asset_size);
}

wxString note_number_to_letter(int note_number) {
	wxString note_symbol = "--1";
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

int PianoControl::get_note_number_at_position(wxPoint pos) {
	if (orientation == wxHORIZONTAL) {
		return (pos.y + scroll_offset) / key_width;
	}
	else {
		return (pos.x + scroll_offset) / key_width;
	}
}

void PianoControl::on_lmb_down(wxMouseEvent& event) {
	int note_number = get_note_number_at_position(event.GetPosition());
	adplayer->play_note(note_number, current_channel_idx, instrument);
	playing_note = note_number;
}

void PianoControl::on_lmb_up(wxMouseEvent& event) {
	adplayer->play_note(0, current_channel_idx, instrument);
	playing_note = 0;
}

void PianoControl::on_mouse_motion(wxMouseEvent& event) {
	int note_number = get_note_number_at_position(event.GetPosition());
	if (event.LeftIsDown() && playing_note != note_number) {
		adplayer->play_note(note_number, current_channel_idx, instrument);
		playing_note = note_number;
	}
	status_bar->SetStatusText(note_number_to_letter(note_number));
}