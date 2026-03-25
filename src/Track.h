#pragma once

#include <wx/wx.h>
#include <memory>
#include <deque>
#include <vector>
#include <map>

using namespace std;

struct Note {
	int offset;
	int pitch;
	int length;
	Note();
	Note(int _offset, int _pitch, int _length);
	static const int pitch_range = 107 - 12;
};

struct Channel {
	wxColour colour = *wxWHITE;
	deque<Note> notes;
	map<int, string> instrument_events = {{0, "PIANO1"}}; // Value = Instrument name.
	map<int, float> volume_events = {{0, 1.0f}}; // Value = Volume mulitplier (0.0 - 1.0).
	map<int, float> pitch_events = {{0, 1.0f}}; // Value = Pitch variation (0.0 - 2.0, nominal is 1.0).
	void add_note(Note new_note);
	void erase_notes(int offset, int length, int& insert_position);
	Note* get_note_on_tick(int at_tick) {
		for (Note& note : notes) {
			if (note.offset > at_tick) { return nullptr; }
			if (note.offset + note.length > at_tick) { return &note; }
		}
		return nullptr;
	}
	void set_instrument_event(int at_tick, string value);
	void set_pitch_event(int at_tick, float value);
	void set_volume_event(int at_tick, float value);
	void get_last_instrument_event(int start_tick, int& ret_tick, string& ret_value);
	void get_last_pitch_event(int start_tick, int& ret_tick, float& ret_value);
	void get_last_volume_event(int start_tick, int& ret_tick, float& ret_value);
	void clear_channel_data();
	int get_tick_count() const { return (notes.empty() ? 0 : notes.back().offset + notes.back().length); }
};

struct Track {
	Track();
	vector<Channel> channels = {};
	Channel* GetChannel(int idx) {return &channels[idx];}
	int get_channel_count() const {return channels.size();}
	
	void set_tempo_event(int at_tick, float value);
	void get_last_tempo_event(int start_tick, int& ret_tick, float& ret_value);
	void clear_track_data(); // Clears all events of track and channel, and notes.
	void save_file(wxString filename);
	void load_file(wxString filename);
	int get_tick_count() const {
		int highest_tick_count = 0;
		for (const Channel& channel : channels) {
			if (channel.get_tick_count() > highest_tick_count) highest_tick_count = channel.get_tick_count();
		}
		return highest_tick_count;
	}
	
	short file_version_major = 0, file_version_minor = 4, ticks_per_beat = 4, beats_per_measure = 4;
	wxSize editor_scale;
	int8_t rhythm_mode = 0; // 0 = percussive, 1 = melodic.
	float basic_tempo = 120;
	// Every event, key == time of event in Ticks. for tempo_events, Value = Tempo multipler (0.01 - 10.0).
	map<int, float> tempo_events = {{0, 1.0f}};
	wxString file_path = wxEmptyString;
protected:
	void rol_move_fields();
};
