#include <Track.h>
#include <FileAccess.h>
#include <debug.h>
#include <cstring>

using namespace FileAccess;

const RGBA ChannelColors[11] = { RGBA(1.0, 0.4, 0.4), RGBA(0.4, 1.0, 0.4), RGBA(0.4, 0.4, 1.0), RGBA(0.45, 0.3, 0.7),
		RGBA(1.0, 0.4, 1.0), RGBA(1.0, 1.0, 0.4), RGBA(1.0, 1.0, 1.0),
		RGBA(0.4, 1.0, 1.0), RGBA(1.0, 0.0, 0.0), RGBA(0.0, 1.0, 0.0), RGBA(0.0, 0.0, 1.0) }; // SD, TD, CY, HH.

Track* current_track;
Channel* current_channel;
array<bool, 11> enabled_channels;
sigc::signal<void()> signal_track_changed;
sigc::signal<void()> signal_channel_changed;

const Note Note::Invalid(0, -1, 0);
Note::Note() {}
Note::Note(int _offset, int _pitch, int _length) : offset(_offset), pitch(_pitch), length(_length) {}
bool Note::is_valid() const { return pitch > 0; }
int Note::get_end_offset() const { return offset + length; }
void Note::set_end_offset(int value) { length = (value - offset); }

int NoteGroup::erase(int eraser_offset, int eraser_length) {
	int insert_position = notes.size();
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
		else if (note.offset > eraser_offset) {
			insert_position = i;
		}
	}
	return insert_position;
}
int NoteGroup::add(Note new_note) {
	int insert_position = erase(new_note.offset, new_note.length);
	notes.insert(notes.begin() + insert_position, new_note);
	return insert_position;
}
void NoteGroup::trim(int start_tick, int end_tick) {
	for (auto it = notes.rbegin(); it != notes.rend(); it++) {
		Note& note = *it;
		if (note.offset < end_tick && note.get_end_offset() > start_tick) {
			if (note.offset < start_tick) {
				note.offset = start_tick;
			}
			if (note.get_end_offset() > end_tick) {
				note.length = end_tick - note.offset;
			}
			if (note.length <= 0) {
				notes.erase(next(it).base());
			}
		}
		else { notes.erase(next(it).base()); }
	}
}
void NoteGroup::make_gap(int start_tick, unsigned int gap_length) {
	if (notes.empty()) { return; }
	Iterator it = split(start_tick);
	it = begin();
	for (it; it != end(); it++) {
		Note& note = *it;
		if (note.offset >= start_tick) {
			break;
		}
	}
	for (it; it != end(); it++) {
		Note& note = *it;
		note.offset += gap_length;
	}
}
void NoteGroup::erase_gap(int start_tick, unsigned int gap_length) {
	if (notes.empty()) { return; }
	erase(start_tick, gap_length);
	if (start_tick - gap_length < 0) {
		erase(start_tick, -(start_tick - gap_length));
	}
	start_tick = start_tick + gap_length;
	Iterator it = begin();
	for (it; it != end(); it++) {
		Note& note = *it;
		if (note.offset >= start_tick) {
			break;
		}
	}
	for (it; it != end(); it++) {
		Note& note = *it;
		note.offset -= gap_length;
	}
}
NoteGroup::Iterator NoteGroup::split(int tick, Iterator start_pos) {
	Iterator pos = start_pos;
	for (pos; pos != end(); pos++) {
		Note& note = *pos;
		if (note.get_end_offset() > tick) {
			if (note.offset < tick) {
				Note new_note = note;
				new_note.offset = tick;
				new_note.length = note.get_end_offset() - tick;
				note.set_end_offset(tick);
				insert(pos+1, new_note);
			}
			break;
		}
	}
	return pos;
}

void NoteGroup::remove_whitespace(int extra_offset) {
	if (notes.empty()) { return; }
	int start_offset = notes[0].offset - extra_offset;
	for (Note& note : notes) {
		note.offset -= start_offset;
	}
}
void NoteGroup::offset_pitch(int add_pitch) {
	if (notes.empty()) { return; }
	for (Note& note : notes) {
		note.pitch += add_pitch;
	}
}
void NoteGroup::offset_tick(int add_tick) {
	if (notes.empty()) { return; }
	for (Note& note : notes) {
		note.offset += add_tick;
	}
}

void NoteGroup::print_notes() {
	cout << "NoteGroup<" << notes.size() << ">: { ";
	for (Note& note : notes) {
		cout << "(" << note.offset << "," << note.pitch << "," << note.length << ")" << " ";
	}
	cout << " }\n";
}

Channel::Channel(int p_channel_number) : channel_number(p_channel_number) {
	switch (channel_number) {
	case 6:
		instrument_events.begin()->second = "bdrum1";
		break;
	case 7:
		instrument_events.begin()->second = "snare1";
		break;
	case 8:
		instrument_events.begin()->second = "tom1";
		break;
	case 9:
		instrument_events.begin()->second = "cymbal1";
		break;
	case 10:
		instrument_events.begin()->second = "hihat1";
		break;
	default:
		break;
	}
	color = ChannelColors[p_channel_number];
	notes = {};
}

Track::Track() {
	for (int i = 0; i < 11; i++) {
		channels.push_back(Channel(i));
	}
	current_track = this;
	current_channel = GetChannel(0);
	enabled_channels.fill(true);
	signal_track_changed.emit();
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
	ret_tick = event_map.begin()->first; \
	ret_value = event_map.begin()->second; \
} while(0)

void Track::get_last_tempo_event(int start_tick, int& ret_tick, float& ret_value)
		{ get_last_event(start_tick, tempo_events, ret_tick, ret_value); }
void Channel::get_last_instrument_event(int start_tick, int& ret_tick, string& ret_value)
		{ get_last_event(start_tick, instrument_events, ret_tick, ret_value); }
void Channel::get_last_pitch_event(int start_tick, int& ret_tick, float& ret_value)
		{ get_last_event(start_tick, pitch_events, ret_tick, ret_value); }
void Channel::get_last_volume_event(int start_tick, int& ret_tick, float& ret_value)
		{ get_last_event(start_tick, volume_events, ret_tick, ret_value); }

Instrument* Channel::get_instrument_at_tick(int at_tick) {
	string last_ins_val;
	int last_ins_tick;
	get_last_instrument_event(at_tick, last_ins_tick, last_ins_val);
	Instrument* last_ins = current_bank->find_instrument(last_ins_val);
	if (last_ins == nullptr) { last_ins = &Instrument::default_instrument; }
	return last_ins;
}

Note* Channel::get_note_on_tick(int at_tick, int& ret_idx) {
	for (ret_idx = 0; ret_idx < notes.size(); ret_idx++) {
		Note& note = notes[ret_idx];
		if (note.offset > at_tick) { return nullptr; }
		if (note.offset + note.length > at_tick) { return &note; }
	}
	return nullptr;
}

Note* Channel::get_note_on_tick(int at_tick) {
	for (Note& note : notes) {
		if (note.offset > at_tick) { return nullptr; }
		if (note.offset + note.length > at_tick) { return &note; }
	}
	return nullptr;
}

void Channel::clear_channel_data() {
	instrument_events.clear();
	pitch_events.clear();
	volume_events.clear();
	notes.clear();
}

int Channel::erase_notes(int eraser_offset, int eraser_length) {
	return notes.erase(eraser_offset, eraser_length);
}

int Channel::add_note(Note new_note) {
	return notes.add(new_note);
}

NoteGroup Channel::copy(int start_tick, int end_tick) {
	NoteGroup copy_buffer;
	if (start_tick - end_tick == 0) {
		return copy_buffer; // Return if no range is set.
	}
	int first_note_idx = -1;
	get_note_on_tick(start_tick, first_note_idx);
	for (int i = first_note_idx; i < notes.size(); i++) {
		Note& note = notes[i];
		if (note.get_end_offset() > start_tick) {
			if (note.offset < end_tick) {
				copy_buffer.notes.push_back(note);
			}
			else {
				break;
			}
		}
	}
	return copy_buffer;
}

void Channel::paste(NoteGroup* paste_buffer, int paste_start, int paste_length, int relative_start) {
	if (!paste_buffer || paste_buffer->notes.empty()) { return; }
	if (paste_length <= 0) {
		paste_length = (paste_buffer->notes.back().get_end_offset() - relative_start);
	}
	int paste_end = paste_start + paste_length;
	for (Note new_note : paste_buffer->notes) {
		new_note.offset += paste_start - relative_start;
		if (new_note.offset >= paste_end) { break; }
		if (new_note.get_end_offset() >= paste_end) {
			new_note.length = (paste_end) - new_note.offset;
			add_note(new_note);
			break;
		}
		add_note(new_note);
	}
}

Note Channel::get_note_or_invalid(int at_idx) {
	if (at_idx < 0 || at_idx >= notes.size() || notes.empty()) {
		return Note::Invalid;
	}
	return notes[at_idx];
}

void Track::clear_track_data() {
	tempo_events.clear();
	for (Channel& channel : channels) {
		channel.clear_channel_data();
	}
	melodic_mode = 0;
}

void Track::save_file(string save_path) {
	string try_path = (!save_path.empty() ? save_path : file_path);
	cout << "Saving Track File: " << try_path << "\n";
	if (!access_file(try_path, true)) return;
	rol_move_fields();
	file_path = try_path;

	ios_file.close();
}

void Track::load_file(string load_path) {
	string try_path = (!load_path.empty() ? load_path : file_path);
	cout << "Load Track File: " << try_path << "\n";
	if (!access_file(try_path, false)) return;
	clear_track_data();
	rol_move_fields();
	file_path = try_path;

	ios_file.close();
	signal_track_changed.emit();
}

void Track::rol_move_fields() {
	fieldcpy_uint16(&file_version_major, 2);
	fieldcpy_uint16(&file_version_minor, 2);
	DBPRINT("File version " << int(file_version_major) << "." << int(file_version_minor));
	fieldzero(40); // "Meta data".
	fieldcpy_uint16(&ticks_per_beat, 2);
	fieldcpy_uint16(&beats_per_measure, 2);
	fieldcpy_uint16(&editor_scale_y, 2);
	fieldcpy_uint16(&editor_scale_x, 2);
	fieldzero(1);            // unused.
	fieldcpy_uint8(&melodic_mode, 1);
	fieldzero(90 + 38 + 15); // unused, filler, filler. Specs don't specify how they're different.
	basic_tempo = 60.0f;
	fieldcpy_float(&basic_tempo);
	DBPRINT("basic_tempo " << basic_tempo << " pos " << file.pos());
	fieldcpy_float_events(tempo_events, 0);
	for (int voice_idx = 0; voice_idx < 11; voice_idx++) {
		Channel& voice = *GetChannel(voice_idx);
		fieldzero(15); // filler.
		uint16_t tick_count = voice.get_tick_count();
		fieldcpy_uint16(&tick_count, 2);
		DBPRINT("process notes, " << tick_count << " ticks");
		if (writing == true) {
			uint16_t note_end = 0;
			for (Note& note : voice.notes) {
				if (note_end < note.offset) {
					uint16_t empty_note_length = (note.offset - note_end);
					fieldzero(2); // Empty note number is 0, so simply fieldzero 2 bytes.
					fieldcpy_uint16(&empty_note_length, 2); // Empty note length.
				}
				uint16_t note_number = int16_t(-(note.pitch - 107));
				uint16_t note_length = int16_t(note.length);
				fieldcpy_uint16(&note_number, 2);
				fieldcpy_uint16(&note_length, 2);
				note_end = note.offset + note.length;
				// DBPRINT("note_number " << note_number << ", duration " << note_length << ", tick " << note.offset);
			}
		}
		else {
			uint16_t note_number = 0;
			uint16_t note_duration = 0;
			uint16_t current_tick = 0;
			while(current_tick < tick_count) {
				fieldcpy_uint16(&note_number, 2);
				fieldcpy_uint16(&note_duration, 2);
				if (note_number != 0) {
					Note new_note(current_tick, (107-note_number), note_duration);
					voice.notes.push_back(new_note);
				}
				// DBPRINT("note_number " << note_number << ", duration " << note_duration << ", tick " << current_tick);
				current_tick += note_duration;
			}
		}
		fieldzero(15); // filler.
		uint16_t ins_event_count = voice.instrument_events.size();
		fieldcpy_uint16(&ins_event_count, 2);
		DBPRINT("ins_event_count " << ins_event_count);
		if (writing == true) {
			for (auto it = voice.instrument_events.begin(); it != voice.instrument_events.end(); it++) {
				uint16_t event_tick = it->first;
				char ins_name[9];
				memcpy(ins_name, it->second.c_str(), 9);
				fieldcpy_uint16(&event_tick, 2);
				fieldcpy_char(&ins_name[0], 9);
				fieldzero(1 + 2); // filler, unused.
			}
		}
		else {
			for (int i = 0; i < ins_event_count; i++) {
				uint16_t event_tick;
				char ins_name[9];
				fieldcpy_uint16(&event_tick, 2);
				fieldcpy_char(&ins_name[0], 9);
				fieldzero(1 + 2); // filler, unused, filler.
				DBPRINT("insi " << i << ", event_tick " << event_tick << ", ins_name " << ins_name);
				voice.instrument_events.insert({event_tick, string(ins_name)});
			}
		}
		fieldzero(15); // filler
		fieldcpy_float_events(voice.volume_events, 0);
		fieldzero(15); // filler
		fieldcpy_float_events(voice.pitch_events, 0);
		DBPRINT("Finished copying channel " << voice_idx << ", file position: " << file.pos());
	}
	DBPRINT("Finished moving rol file\n");
}
