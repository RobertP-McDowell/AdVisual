#include <Track.h>
#include <FileAccess.h>
using namespace FileAccess;

const wxColour ChannelColours[11] = { wxColour(255, 100, 100), wxColour(100, 255, 100), wxColour(100, 100, 255), wxColour(140, 40, 208),
		wxColour(255, 100, 255), wxColour(255, 255, 100), wxColour(100, 255, 255), wxColour(255, 255, 255),
		wxColour(255, 255, 255), wxColour(255, 255, 255), wxColour(255, 255, 255) };

Note::Note() {}
Note::Note(int _offset, int _pitch, int _length) : offset(_offset), pitch(_pitch), length(_length) {}

Track::Track() {
	for (int i = 0; i < 11; i++) {
		shared_ptr<Channel> new_channel = make_shared<Channel>();
		new_channel->colour = ChannelColours[i];
		channels.push_back(new_channel);
	}
}

#define insert_or_overwrite_event(tick_to_insert, event_map, value_to_insert) do { \
	auto it = event_map.begin(); \
	for (int i = 0; i < event_map.size(); i++, it++) { \
		int itick = (*it).first; \
		if (itick == tick_to_insert) { \
			it->second = value_to_insert; \
			return; \
		} \
		if (itick > tick_to_insert) { \
			it--; \
			event_map.insert(it, {tick_to_insert, value_to_insert}); \
			return; \
		} \
	} \
	event_map.insert(event_map.end(), {tick_to_insert, value_to_insert}); \
} while(0)

bool frange(float value, float min, float max) {
	return value >= min && value <= max;
}

void Track::set_tempo_event(int at_tick, float value) {
	if (frange(value, 0.01, 10.0)) { insert_or_overwrite_event(at_tick, tempo_events, value); }
	else { tempo_events.erase(at_tick); }
}
void Channel::set_instrument_event(int at_tick, string value) {
	if (!value.empty()) { insert_or_overwrite_event(at_tick, instrument_events, value); }
	else { instrument_events.erase(at_tick); }
}
void Channel::set_pitch_event(int at_tick, float value) {
	if (frange(value, 0.0, 2.0)) { insert_or_overwrite_event(at_tick, pitch_events, value); }
	else { pitch_events.erase(at_tick); }
}
void Channel::set_volume_event(int at_tick, float value) {
	if (frange(value, 0.0, 1.0)) { insert_or_overwrite_event(at_tick, volume_events, value); }
	else { volume_events.erase(at_tick); }
}

// float get_last_float_event(int start_tick, map<int, void*>* event_map) {
#define get_last_event(start_tick, event_map, ret_tick, ret_value) do { \
	for (auto it = event_map.begin(); it != event_map.end(); it++) { \
		if (it->first == start_tick) { \
			ret_tick = it->first; \
			ret_value = it->second; \
			return; \
		} \
		if (it->first > start_tick) { \
			it--; /* Checking value of prior event. */ \
			ret_tick = it->first; \
			ret_value = it->second; \
			return; \
		} \
	} \
	ret_tick = 0; \
	ret_value = event_map.begin()->second; \
} while(0)

// Note that the 'return 1.0f' and 'return ""' should never happen, see the cerr in the 'get_last_event' macro.
void Track::get_last_tempo_event(int start_tick, int& ret_tick, float& ret_value)
		{ get_last_event(start_tick, tempo_events, ret_tick, ret_value); }
void Channel::get_last_instrument_event(int start_tick, int& ret_tick, string& ret_value)
		{ get_last_event(start_tick, instrument_events, ret_tick, ret_value); }
void Channel::get_last_pitch_event(int start_tick, int& ret_tick, float& ret_value)
		{ get_last_event(start_tick, pitch_events, ret_tick, ret_value); }
void Channel::get_last_volume_event(int start_tick, int& ret_tick, float& ret_value)
		{ get_last_event(start_tick, volume_events, ret_tick, ret_value); }

void Channel::clear_channel_data() {
	instrument_events.clear();
	pitch_events.clear();
	volume_events.clear();
	notes.clear();
}

void Channel::erase_notes(int eraser_offset, int eraser_length, int& insert_position) {
	insert_position = notes.size();
	for (int i = notes.size() - 1; i >= 0; i--) {
		Note note = notes[i];
		if (note.offset < eraser_offset + eraser_length && note.offset + note.length > eraser_offset) {
			insert_position = i;
			if (note.offset < eraser_offset) {
				Note note_slice1(note.offset, note.pitch, eraser_offset - note.offset);
				notes.insert(notes.begin() + i + 1, note_slice1);
				insert_position = i + 1;
			}
			if (note.offset + note.length > eraser_offset + eraser_length) {
				Note note_slice2(eraser_offset + eraser_length, note.pitch, (note.offset + note.length) - (eraser_offset + eraser_length));
				notes.insert(notes.begin() + insert_position + 1, note_slice2);
			}
			notes.erase(notes.begin() + i);
		}
	}
}

void Channel::add_note(Note new_note) {
	int insert_position = notes.size();
	erase_notes(new_note.offset, new_note.length, insert_position);
	notes.insert(notes.begin() + insert_position, new_note);
}

void Track::clear_track_data() {
	tempo_events.clear();
	for (shared_ptr<Channel>& channel : channels) {
		channel->clear_channel_data();
	}
	music_mode = 0;
}

void Track::SaveToFile(wxString save_path) {
	cout << "Saving Track File: " << save_path << "\n";

	clear_track_data();
	access_file(save_path, true);
	rol_move_fields();

	file.close();
}

void Track::LoadFromFile(wxString load_path) {
	cout << "Load Track File: " << load_path << "\n";

	clear_track_data();
	access_file(load_path, false);
	rol_move_fields();

	file.close();
}

void Track::rol_move_fields() {
	fieldcpy(&file_version_major, 2);
	fieldcpy(&file_version_minor, 2);
	fieldzero(40); // "Meta data".
	fieldcpy(&ticks_per_beat, 2);
	fieldcpy(&beats_per_measure, 2);
	fieldcpy(&editor_scale.y, 2);
	fieldcpy(&editor_scale.x, 2);
	fieldzero(1);            // unused.
	fieldcpy(&music_mode, 1);
	fieldzero(90 + 38 + 15); // unused, filler, filler. Specs don't specify why.
	fieldcpy(&basic_tempo, 4);
	
	fieldcpy_float_events(tempo_events, 0);
	for (int voice_idx = 0; voice_idx < 11; voice_idx++) {
		Channel& voice = *(GetChannel(voice_idx).get());
		fieldzero(15); // filler.
		if (writing == true) {
			int16_t tick_count = voice.get_tick_count();
			fieldcpy_write(&tick_count, 2);
			int16_t note_end = 0;
			for (Note& note : voice.notes) {
				if (note_end < note.offset) {
					fieldzero(2); // Empty note number is 0, so simply fieldzero 2 bytes.
					int16_t empty_note_length = (note.offset - note_end);
					fieldcpy_write(&empty_note_length, 2); // Empty note length.
				}
				int16_t note_number = int16_t(-((note.pitch+12) - 107));
				int16_t note_length = int16_t(note.length);
				fieldcpy_write(&note_number, 2);
				fieldcpy_write(&note_length, 2);
				note_end = note.offset + note.length;
			}
		}
		else {
			int16_t tick_count;
			fieldcpy_read(&tick_count, 2);
			int16_t note_number = 0;
			int16_t note_duration = 0;
			int16_t current_tick = 0;
			while(current_tick < tick_count) {
				fieldcpy(&note_number, 2);
				fieldcpy(&note_duration, 2);
				if (note_number != 0) {
					Note new_note(current_tick, (107-note_number), note_duration);
					voice.notes.push_back(new_note);
				}
				current_tick += note_duration;
			}
		}
		fieldzero(15); // filler.
		if (writing == true) {
			int16_t ins_event_count = voice.instrument_events.size();
			fieldcpy_write(&ins_event_count, 2);
			for (auto it = voice.instrument_events.begin(); it != voice.instrument_events.end(); it++) {
				int16_t event_tick = it->first;
				char ins_name[9];
				memcpy(ins_name, it->second.c_str(), 9);
				fieldcpy_write(&event_tick, 2);
				fieldcpy_write(&ins_name, 9);
				fieldzero(1 + 2); // filler, unused.
			}
		}
		else {
			int16_t ins_event_count;
			fieldcpy_read(&ins_event_count, 2);
			for (int i = 0; i < ins_event_count; i++) {
				int16_t event_tick;
				char ins_name[9];
				fieldcpy_read(&event_tick, 2);
				fieldcpy_read(&ins_name, 9);
				fieldzero(1 + 2); // filler, unused, filler.
				voice.instrument_events.insert({event_tick, string(ins_name)});
			}
		}
		fieldzero(15); // filler
		fieldcpy_float_events(voice.volume_events, 0);
		fieldzero(15); // filler
		fieldcpy_float_events(voice.pitch_events, 0);
	}
}
