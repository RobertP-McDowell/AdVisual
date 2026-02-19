#include <FileAccess.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>


int hex_to_dec(char* hex_num, int len) {
	int dec_num = 0;
	int base = 1;
	for (int i = len - 1; i >= 0; i--) {
		char hex_digit = *(hex_num + i);
		// if char is within range 0-9, subtract 48 from ASCII value.:
		if (hex_digit >= '0' && hex_digit <= '9') {
			dec_num += (int(hex_digit) - 48) * base;
			// Increment base by power of hex.
			base = base * 16;
		}
		// if char is within range a-f, subtract 55 from ASCII value.:
		else if (hex_digit >= 'a' && hex_digit <= 'f') {
			dec_num += (int(hex_digit) - 55) * base;
			base = base * 16;
		}
	}
	return dec_num;
}
char* field_idx = nullptr;

void fieldcpy(void* destination, int field_size) {
	memcpy(destination, field_idx, field_size);
	field_idx += field_size;
}

void FileAccess::SaveTrack(string save_path, Track& track) {cout << "Saving not Implemented yet!\n";}

void FileAccess::LoadTrack(const string& load_path, Track& track) {
	cout << "Load File: " << load_path << "\n";
	ifstream file(load_path, ios::in);	if (!file.is_open()) {
		cerr << "Could not open file: " << load_path.c_str() << "\n";
		if (file.bad()) cerr << "Fatal error: badbit is set.\n";
		if (file.fail()) cerr << "" << strerror(errno) << "\n";
		return;
	}
	long pos_in_file = 0;
	
	
	// get length of file:
	file.seekg(0, file.end);
	int length = file.tellg();
	file.seekg(0, file.beg);

	char* file_buffer = new char[length];
	file.read(file_buffer, length);
	field_idx = file_buffer;

	fieldcpy(&track.file_version_major, 2);
	fieldcpy(&track.file_version_minor, 2);
	field_idx += 40; // "Meta data".
	fieldcpy(&track.ticks_per_beat, 2);
	fieldcpy(&track.beats_per_measure, 2);
	fieldcpy(&track.editor_scale.y, 2);
	fieldcpy(&track.editor_scale.x, 2);
	field_idx += 1;            // unused.
	fieldcpy(&track.music_mode, 1);
	field_idx += 90 + 38 + 15; // unused, filler, filler. Specs don't specify why.
	fieldcpy(&track.basic_tempo, 4);
	
	int16_t tempo_event_count;
	fieldcpy(&tempo_event_count, 2);
	
	for (int i = 0; i < tempo_event_count; i++) {
		int event_tick;
		float tempo_multiplier;
		fieldcpy(&event_tick, 2);
		fieldcpy(&tempo_multiplier, 4);
		track.tempo_events.insert({event_tick, tempo_multiplier});
		cout << "Tempo event at: " << event_tick << ", Tempo multiplier: " << tempo_multiplier << ".\n";
	}
	
	for (int voice_idx = 0; voice_idx < 11; voice_idx++) {
		Channel& voice = *(track.GetChannel(voice_idx).get());
		field_idx += 15; // filler.
		fieldcpy(&voice.tick_count, 2);
		int16_t note_number = 0;
		int16_t note_duration = 0;
		int tick_count = 0;
		while(tick_count < voice.tick_count) {
			fieldcpy(&note_number, 2);
			fieldcpy(&note_duration, 2);
			if (note_number != 0) {
				Note new_note(tick_count, (107-note_number)-12, note_duration);
				voice.notes.push_back(new_note);
			}
			tick_count += note_duration;
		}
		field_idx += 15; // filler.
		int16_t ins_event_count;
		fieldcpy(&ins_event_count, 2);
		for (int i = 0; i < ins_event_count; i++) {
			int16_t event_tick;
			char ins_name[9];
			fieldcpy(&event_tick, 2);
			fieldcpy(&ins_name, 9);
			field_idx += 1 + 2 + 15; // filler, unused, filler.
			voice.instrument_events.insert({event_tick, string(ins_name)});
		}
		int16_t volume_event_count;
		fieldcpy(&volume_event_count, 2);
		for (int i = 0; i < volume_event_count; i++) {
			int16_t event_tick;
			float volume_multiplier;
			fieldcpy(&event_tick, 2);
			fieldcpy(&volume_multiplier, 4);
			voice.volume_events.insert({event_tick, volume_multiplier});
			field_idx += 15; // filler.
		}
		int16_t pitch_event_count;
		fieldcpy(&pitch_event_count, 2);
		for (int i = 0; i < pitch_event_count; i++) {
			int16_t event_tick;
			float pitch_multiplier;
			fieldcpy(&event_tick, 2);
			fieldcpy(&pitch_multiplier, 4);
			voice.volume_events.insert({event_tick, pitch_multiplier});
		}
	}
	
	// End file cpy.
	cout << track.file_version_major << "." << track.file_version_minor << "\n";
	cout << track.ticks_per_beat <<"\n";
	cout << "Tempo: " << track.basic_tempo << "\n";
	field_idx = nullptr;
	delete file_buffer;
	file.close();
}



