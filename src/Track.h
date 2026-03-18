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
	map<int, string> instrument_events = {{0, "Piano1"}}; // Value = Instrument name.
	map<int, float> volume_events = {{0, 1.0f}}; // Value = Volume mulitplier (0.0 - 1.0).
	map<int, float> pitch_events = {{0, 1.0f}}; // Value = Pitch variation (0.0 - 2.0, nominal is 1.0).
	void add_note(Note new_note);
	void erase_notes(int offset, int length, int& insert_position);
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
	vector<shared_ptr<Channel>> channels = {};
	shared_ptr<Channel> GetChannel(int idx) const {return channels[idx];}
	int get_channel_count() const {return channels.size();}
	
	void set_tempo_event(int at_tick, float value);
	void get_last_tempo_event(int start_tick, int& ret_tick, float& ret_value);
	void set_events(shared_ptr<Channel> on_channel, int at_tick, float tempo_event, string instrument_event, float pitch_event, float volume_event);
	void clear_track_data(); // Clears all events of track and channel, and notes.
	void SaveToFile(wxString filename);
	void LoadFromFile(wxString filename);
	int get_tick_count() const {
		int highest_tick_count = 0;
		for (shared_ptr<Channel> channel : channels) {
			if (channel->get_tick_count() > highest_tick_count) highest_tick_count = channel->get_tick_count();
		}
		return highest_tick_count;
	}
	
	short file_version_major, file_version_minor, ticks_per_beat = 4, beats_per_measure = 4;
	wxSize editor_scale;
	int8_t music_mode = 0; // 0 = percussive, 1 = melodic.
	float basic_tempo = 120;
	// Every event, key == time of event in Ticks. for tempo_events, Value = Tempo multipler (0.01 - 10.0).
	map<int, float> tempo_events = {{0, 1.0f}};
	wxString file_path = wxEmptyString;
protected:
	void rol_move_fields();
};
