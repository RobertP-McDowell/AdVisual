#include <FileAccess.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>

bool writing = false;
unique_ptr<fstream> file = nullptr; // for writing operations.
long file_pos = 0;

void fieldcpy_write(void* object, int field_size) {
	file->write(static_cast<char*>(object), field_size);
	file_pos += field_size;
	file->seekg(file_pos);
}

void fieldcpy_read(void* object, int field_size) {
	file->read(static_cast<char*>(object), field_size);
	file_pos += field_size;
	file->seekg(file_pos);
}
// Checks and calls fieldcpy_read or write
void fieldcpy(void* object, int field_size) {
	if (writing == true) {
		fieldcpy_write(object, field_size);
	}
	else {
		fieldcpy_read(object, field_size);
	}
}

void fieldzero(int field_size) {
	if (writing == true) {
		vector<char> buffer = {};
		buffer.resize(field_size, 0);
		file->write(buffer.data(), field_size);
	}
	file_pos += field_size;
	file->seekg(file_pos);
}

void fieldcpy_float_events(map<int, float>& event_map, int loop_spacing) {
	if (writing) {
		int16_t event_count = event_map.size();
		fieldcpy_write(&event_count, 2);
		for (auto it = event_map.begin(); it != event_map.end(); it++) {
			int16_t event_tick = it->first;
			float event_value = it->second;
			fieldcpy_write(&event_tick, 2);
			fieldcpy_write(&event_value, 4);
			fieldzero(loop_spacing);
			//cout << "Tempo event at: " << it->first << ", Tempo multiplier: " << it->second << ".\n";
		}
	}
	else {
		int16_t event_count;
		fieldcpy_read(&event_count, 2);
		for (int i = 0; i < event_count; i++) {
			int16_t event_tick;
			float event_value;
			fieldcpy_read(&event_tick, 2);
			fieldcpy_read(&event_value, 4);
			event_map.insert({event_tick, event_value});
			fieldzero(loop_spacing);
			//cout << "Tempo event at: " << event_tick << ", Tempo multiplier: " << event_value << ".\n";
		}
	}
}

void move_fields(Track& track, bool write) {
	file->seekg(0); // Initializing.
	file_pos = 0;
	writing = write;
	fieldcpy(&track.file_version_major, 2);
	fieldcpy(&track.file_version_minor, 2);
	fieldzero(40); // "Meta data".
	fieldcpy(&track.ticks_per_beat, 2);
	fieldcpy(&track.beats_per_measure, 2);
	fieldcpy(&track.editor_scale.y, 2);
	fieldcpy(&track.editor_scale.x, 2);
	fieldzero(1);            // unused.
	fieldcpy(&track.music_mode, 1);
	fieldzero(90 + 38 + 15); // unused, filler, filler. Specs don't specify why.
	fieldcpy(&track.basic_tempo, 4);
	
	fieldcpy_float_events(track.tempo_events, 0);
	cout << " idx: " << file_pos << "\n";
	for (int voice_idx = 0; voice_idx < 11; voice_idx++) {
		Channel& voice = *(track.GetChannel(voice_idx).get());
		fieldzero(15); // filler.
		if (writing == true) {
			int16_t tick_count = voice.get_tick_count();
			fieldcpy_write(&tick_count, 2);
			int16_t note_end = 0;
			cout << voice_idx << "Save: ";
			for (Note& note : voice.notes) {
				if (note_end < note.offset) {
					fieldzero(2); // Empty note number is 0, so simply fieldzero 2 bytes.
					int16_t empty_note_length = (note.offset - note_end);
					fieldcpy_write(&empty_note_length, 2); // Empty note length.
					cout << "(" << 0 << "," << empty_note_length << ") ";
				}
				int16_t note_number = int16_t(-((note.pitch+12) - 107));
				int16_t note_length = int16_t(note.length);
				fieldcpy_write(&note_number, 2);
				fieldcpy_write(&note_length, 2);
				cout << "(" << note_number << "," << note_length << ") ";
				note_end = note.offset + note.length;
			}
			cout << "\n";
		}
		else {
			int16_t tick_count;
			fieldcpy_read(&tick_count, 2);
			int16_t note_number = 0;
			int16_t note_duration = 0;
			int16_t current_tick = 0;
			cout << voice_idx << "Load: ";
			while(current_tick < tick_count) {
				fieldcpy(&note_number, 2);
				fieldcpy(&note_duration, 2);
				cout << "(" << (107-note_number) << "," << note_duration << ") ";
				if (note_number != 0) {
					Note new_note(current_tick, (107-note_number), note_duration);
					voice.notes.push_back(new_note);
				}
				current_tick += note_duration;
			}
			cout << "\n";
		}
		fieldzero(15); // filler.
		cout << "Voice: " << voice_idx << " idx: " << file_pos << "Ins Start\n";
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
		cout << "Voice: " << voice_idx << " idx: " << file_pos << " Ins End\n";
		fieldzero(15); // filler
		fieldcpy_float_events(voice.volume_events, 0);
		fieldzero(15); // filler
		fieldcpy_float_events(voice.pitch_events, 0);
	}
}


void FileAccess::SaveTrack(string save_path, Track& track) {
	cout << "Saving File: " << save_path << "\n";
	file = make_unique<fstream>(save_path, ios::out);
	if (!file->is_open()) {
		cerr << "Could not open file: " << save_path << "\n";
		if (file->bad()) cerr << "Fatal error: badbit is set.\n";
		if (file->fail()) cerr << strerror(errno) << "\n";
		return;
	}

	move_fields(track, true);

	// End file cpy.
	cout << track.file_version_major << "." << track.file_version_minor << "\n";
	cout << track.ticks_per_beat <<"\n";
	cout << "Tempo: " << track.basic_tempo << "\n";
	cout << "Done Saving\n";
	file->close();
}

void FileAccess::LoadTrack(string load_path, Track& track) {
	cout << "Load File: " << load_path << "\n";
	track.clear_track_data();
	file = make_unique<fstream>(load_path, ios::in);
	if (!file->is_open()) {
		cerr << "Could not open file: " << load_path << "\n";
		if (file->bad()) cerr << "Fatal error: badbit is set.\n";
		if (file->fail()) cerr << strerror(errno) << "\n";
		return;
	}

	move_fields(track, false);

	// End file cpy.
	cout << track.file_version_major << "." << track.file_version_minor << "\n";
	cout << track.ticks_per_beat <<"\n";
	cout << "Tempo: " << track.basic_tempo << "\n";
	cout << "Done Loading\n";
	file->close();
}



