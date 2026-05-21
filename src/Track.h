#pragma once

#include <gtkmm.h>
#include <cstdint>
#include <memory>
#include <deque>
#include <vector>
#include <string>
#include <Instrument.h>
#include <map>

using namespace std;
using Gdk::RGBA;

struct Note {
	int offset;
	int pitch;
	int length;
	Note();
	Note(int _offset, int _pitch, int _length);
	bool is_valid() const;
	int get_end_offset() const;
	static const int pitch_range = 107 - 12;
	static const Note Invalid;
};

struct NoteGroup {
	NoteGroup() {}
	NoteGroup(vector<Note> p_notes) : notes(p_notes) {}
	vector<Note> notes;
	// Group editing notes.
	void trim(int start_tick, int end_tick);
	void remove_whitespace(int extra_offset = 0);
	void offset_pitch(int add_pitch);
	void offset_tick(int add_tick);
};

struct Channel {
	Channel(int p_channel_number);
	int8_t channel_number;
	RGBA color = RGBA(1.0, 1.0, 1.0, 1.0);
	deque<Note> notes;
	map<int, string> instrument_events = {{0, "piano1"}}; // Value = Instrument name.
	map<int, float> volume_events = {{0, 1.0f}}; // Value = Volume mulitplier (0.0 - 1.0).
	map<int, float> pitch_events = {{0, 1.0f}}; // Value = Pitch variation (0.0 - 2.0, nominal is 1.0).
	int erase_notes(int offset, int length);
	int add_note(Note new_note);
	NoteGroup copy(int start_tick, int end_tick);
	void paste(NoteGroup* paste_buffer, int paste_start, int paste_length = -1, int relative_start = 0);

	Note get_note_or_invalid(int at_idx);

	void set_instrument_event(int at_tick, string value);
	void set_pitch_event(int at_tick, float value);
	void set_volume_event(int at_tick, float value);
	Instrument* get_instrument_at_tick(int at_tick); // Convience function to get instrument object directly.
	void get_last_instrument_event(int start_tick, int& ret_tick, string& ret_value);
	void get_last_pitch_event(int start_tick, int& ret_tick, float& ret_value);
	void get_last_volume_event(int start_tick, int& ret_tick, float& ret_value);
	Note* get_note_on_tick(int at_tick, int& note_idx);
	Note* get_note_on_tick(int at_tick);
	void clear_channel_data();
	int get_tick_count() const { return (notes.empty() ? 0 : notes.back().offset + notes.back().length); }
};

struct UndoCommand {
	enum RedoCommand {
		ERASE_OLD = 1,
		WRITE_NEW = 2,
		ERASE_NEW = 4
	};
	UndoCommand(Channel* p_channel, int p_command = RedoCommand::ERASE_OLD | RedoCommand::WRITE_NEW) : channel(p_channel), command(p_command) {}
	NoteGroup new_notes; // items should be in order of creation/deletion, not tick offset.
	NoteGroup old_notes;
	int start_tick;
	int end_tick;
	Channel* channel;
	int command;
	bool erase;
	void set_old_notes(NoteGroup& p_old_notes) { old_notes = p_old_notes; }
	void set_new_notes(NoteGroup& p_new_notes) { new_notes = p_new_notes; }
	void undo();
	void redo();
};

struct Track {
	Track();
	vector<Channel> channels;
	Channel* GetChannel(int idx) {return &channels[idx];}
	int get_channel_count() const {return channels.size();}
	
	void set_tempo_event(int at_tick, float value);
	void get_last_tempo_event(int start_tick, int& ret_tick, float& ret_value);
	void clear_track_data(); // Clears all events of track and channel, and notes.
	void save_file(string filename);
	void load_file(string filename);
	int get_tick_count() const {
		int highest_tick_count = 0;
		for (const Channel& channel : channels) {
			if (channel.get_tick_count() > highest_tick_count) highest_tick_count = channel.get_tick_count();
		}
		return highest_tick_count;
	}
	uint16_t file_version_major = 0, file_version_minor = 4, ticks_per_beat = 4, beats_per_measure = 4;
	uint16_t editor_scale_x = 1, editor_scale_y = 1;
	uint8_t melodic_mode = 0; // 0 = percussive, 1 = melodic.
	float basic_tempo = 120;
	// Every event, key == time of event in Ticks. for tempo_events, Value = Tempo multipler (0.01 - 10.0).
	map<int, float> tempo_events = {{0, 1.0f}};
	string file_path = "";

	void add_undo(UndoCommand new_command);
	bool group_undo = true;
	int undo_index = 0;
	vector<UndoCommand> undo_buffer = {};
protected:
	void rol_move_fields();
};

extern Track* current_track;
extern Channel* current_channel;
extern array<bool, 11> enabled_channels;
extern sigc::signal<void()> signal_track_changed;
extern sigc::signal<void()> signal_channel_changed;

