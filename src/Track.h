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
	int16_t tick_count = 0;
	map<int, string> instrument_events; // Value = Instrument name.
	map<int, float> volume_events; // Value = Volume mulitplier (0.0 - 1.0).
	map<int, float> pitch_events; // Value = Pitch variation (0.0 - 2.0, nominal is 1.0).
	void add_note(Note new_note);
	void erase_notes(int offset, int length, int& insert_position);
};

struct Track {
	Track();
	vector<shared_ptr<Channel>> channels;
	shared_ptr<Channel> GetChannel(int idx) const {return channels[idx];}
	int get_channel_count() const {return channels.size();}
	short file_version_major, file_version_minor, ticks_per_beat = 4, beats_per_measure = 4;
	wxSize editor_scale;
	int8_t music_mode = 0; // 0 = percussive, 1 = melodic.
	float basic_tempo = 120;
	// Every event key == time of event in Ticks.
	map<int, float> tempo_events; // Value = Tempo multipler (0.01 - 10.0).
};
