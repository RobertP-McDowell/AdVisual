#include <Track.h>


const wxColour ChannelColours[11] = { wxColour(255, 100, 100), wxColour(100, 255, 100), wxColour(100, 100, 255), wxColour(140, 40, 208),
		wxColour(255, 100, 255), wxColour(255, 255, 100), wxColour(100, 255, 255), wxColour(255, 255, 255),
		wxColour(255, 255, 255), wxColour(255, 255, 255), wxColour(255, 255, 255) };

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

void Track::clear_track_data() {
	tempo_events.clear();
	for (shared_ptr<Channel>& channel : channels) {
		channel->clear_channel_data();
	}
	music_mode = 0;
}

void Channel::clear_channel_data() {
	instrument_events.clear();
	pitch_events.clear();
	volume_events.clear();
	notes.clear();
}

void Channel::erase_notes(int eraser_offset, int eraser_length, int& insert_position) {
	insert_position = notes.size();
	for (int i = notes.size() - 1; i >= 0; i--) {
		Note& note = notes[i];
		if (note.offset < eraser_offset + eraser_length && note.offset + note.length > eraser_offset) {
			insert_position = i;
			if (note.offset < eraser_offset) {
				Note note_slice1(note.offset, note.pitch, eraser_offset - note.offset);
				notes.insert(notes.begin() + i+1, note_slice1);
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

Note::Note() {}
Note::Note(int _offset, int _pitch, int _length) : offset(_offset), pitch(_pitch), length(_length) {}

