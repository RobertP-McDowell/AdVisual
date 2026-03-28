#include <common.h>

unique_ptr<AdPlayer> adplayer = nullptr;
unique_ptr<Track> current_track = nullptr;
unique_ptr<Bank> current_bank = nullptr;
Channel* current_channel = nullptr;
int current_channel_idx = 0;
int cursor_tick = 0;

wxBitmapBundle GetAsset(wxString asset_name, wxSize asset_size) {
	return wxBitmapBundle::FromSVGFile(ASSETS_PATH + asset_name, asset_size);
}

void PianoControl::on_lmb_down(wxMouseEvent& event) {
	int note_number = event.GetPosition().y / key_width;
	adplayer->play_note(note_number, 0, instrument);
	playing_note = note_number;
}

void PianoControl::on_mouse_motion(wxMouseEvent& event) {
	int note_number = event.GetPosition().y / key_width;
	if (event.LeftIsDown() && playing_note != note_number) {
		adplayer->play_note(note_number, 0, instrument);
		playing_note = note_number;
	}
}