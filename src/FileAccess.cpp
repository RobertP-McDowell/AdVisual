#include <FileAccess.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <common.h>
#include <boost/endian/conversion.hpp>

using namespace boost::endian;
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
	if (writing == true) fieldcpy_write(object, field_size);
	else fieldcpy_read(object, field_size);
}

// Same as fieldcpy, but ensures little endianness for integer types.
void fieldcpyLE16(uint16_t* object, int field_size) {
	if (writing == true) fieldcpy_write(object, field_size);
	else fieldcpy_read(object, field_size);
	native_to_little(*object);
}
void fieldcpyLE32(uint32_t* object, int field_size) {
	if (writing == true) fieldcpy_write(object, field_size);
	else fieldcpy_read(object, field_size);
	native_to_little(*object);
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

void track_move_fields(Track& track, bool write) {
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
	for (int voice_idx = 0; voice_idx < 11; voice_idx++) {
		Channel& voice = *(track.GetChannel(voice_idx).get());
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


void FileAccess::SaveTrack(string save_path, Track& track) {
	cout << "Saving Track File: " << save_path << "\n";
	file = make_unique<fstream>(save_path, ios::out);
	if (!file->is_open()) {
		cerr << "Could not open file: " << save_path << "\n";
		if (file->bad()) cerr << "Fatal error: badbit is set.\n";
		if (file->fail()) cerr << strerror(errno) << "\n";
		return;
	}

	track_move_fields(track, true);

	file->close();
}

void FileAccess::LoadTrack(string load_path, Track& track) {
	cout << "Load Track File: " << load_path << "\n";
	track.clear_track_data();
	file = make_unique<fstream>(load_path, ios::in);
	if (!file->is_open()) {
		cerr << "Could not open file: " << load_path << "\n";
		if (file->bad()) cerr << "Fatal error: badbit is set.\n";
		if (file->fail()) cerr << strerror(errno) << "\n";
		return;
	}

	track_move_fields(track, false);

	file->close();
}

void bank_move_OPLFM_fields(OPLFM& opl) {
	fieldcpy(&opl.level_scaling, 1);
	fieldcpy(&opl.frequency_multiplier, 1);
	fieldcpy(&opl.feedback, 1);
	fieldcpy(&opl.attack_rate, 1);
	fieldcpy(&opl.sustain_level, 1);
	fieldcpy(&opl.sustain_sound, 1);
	fieldcpy(&opl.decay_rate, 1);
	fieldcpy(&opl.release_rate, 1);
	fieldcpy(&opl.output_level, 1);
	fieldcpy(&opl.tremelo, 1);
	fieldcpy(&opl.vibrato, 1);
	fieldcpy(&opl.envelope_scaling, 1);
	//fieldcpy(&opl., 1);
	//fieldcpy(&opl., 1);
	fieldzero(1);
	DBPRINT("level_scaling: " << int(opl.level_scaling) << " freq_mul: " << int(opl.frequency_multiplier) <<
		" feedback: " << int(opl.feedback) << " atk_rt: " << int(opl.attack_rate) << " sustain_lvl: " << int(opl.sustain_level) <<
		" envelope_scaling: " << int(opl.envelope_scaling) << " decay_rt: " << int(opl.decay_rate) <<
		" release_rt: " << int(opl.release_rate) << " output_level: " << int(opl.output_level) <<
		" tremelo: " << int(opl.tremelo) << " vibrato: " << int(opl.vibrato));
}

void bank_move_fields(Bank& bank, bool write) {
	file->seekg(0); // Initializing.
	file_pos = 0;
	writing = write;
	DBPRINT("Writing to bank.");
	fieldcpy(&bank.file_version_major, 1);
	fieldcpy(&bank.file_version_minor, 1);
	DBPRINT("File version " << int(bank.file_version_major) << "." << int(bank.file_version_minor) << "\n");
	fieldzero(6); // Simply says "ADLIB-"
	uint16_t num_of_ins_used;
	uint16_t num_of_ins;
	uint32_t name_offset;
	uint32_t data_offset;
	uint16_t byte_size_of_instrument = 17;
	fieldcpyLE16(&num_of_ins_used, 2);
	fieldcpyLE16(&num_of_ins, 2);
	vector<uint16_t> data_indices;
	data_indices.resize(num_of_ins_used);
	fieldcpyLE32(&name_offset, 4);
	fieldcpyLE32(&data_offset, 4);
	DBPRINT("2Ins used: " << num_of_ins_used << " Ins count: " << num_of_ins <<
		" name offset: " << name_offset << " data_offset: " << data_offset << "\n");
	fieldzero(8); // Padding.
	for (uint16_t insi = 0; insi < num_of_ins_used; insi++) {
		uint16_t data_index = data_offset + (insi * sizeof(uint16_t));
		Instrument new_instrument;
		fieldcpyLE16(&data_index, 2);
		fieldcpy(&new_instrument.flags, 1);
		fieldcpy(&new_instrument.name, 9);
		bank.instruments.push_back(new_instrument);
		data_indices[data_index] = insi;
		DBPRINT(insi << " / " << num_of_ins << "Ins Flags: " << int(new_instrument.flags) << " Ins name: " << new_instrument.name);
	}
	for (uint16_t insi = num_of_ins_used; insi < num_of_ins; insi++) {
		fieldzero(2+1+9); // memzero unused instruments.
	}
	DBPRINT("data_offset: " << data_offset << " should equal file pos: " << file_pos);
	for (uint16_t insi = 0; insi < num_of_ins_used; insi++) {
		Instrument& new_instrument = bank.instruments[data_indices.at(insi)];
		fieldcpy(&new_instrument.percussion_mode, 1);
		fieldcpy(&new_instrument.voice_number, 1);
		bank_move_OPLFM_fields(new_instrument.modulator);
		bank_move_OPLFM_fields(new_instrument.carrier);
		fieldcpy(&new_instrument.modulator.waveform, 1);
		fieldcpy(&new_instrument.carrier.waveform, 1);
		DBPRINT("mWave: " << new_instrument.modulator.waveform << " cWave: " << new_instrument.carrier.waveform <<
			"name: " << new_instrument.name << " Data index: " << insi << " / " << num_of_ins <<
			" Instrument index: " << data_indices.at(insi));
	}
	for (uint16_t insi = num_of_ins_used; insi < num_of_ins; insi++) {
		fieldzero(byte_size_of_instrument); // memzero unused instrument data.
	}
}

void FileAccess::SaveBank(string save_path, Bank& bank) {
	cout << "Saving Bank not yet implemented!\n";
}

Bank FileAccess::LoadBank(string load_path) {
	Bank bank;
	cout << "Load Bank File: " << load_path << "\n";
	file = make_unique<fstream>(load_path, ios::in);
	if (!file->is_open()) {
		cerr << "Could not open file: " << load_path << "\n";
		if (file->bad()) cerr << "Fatal error: badbit is set.\n";
		if (file->fail()) cerr << strerror(errno) << "\n";
		return bank;
	}

	bank_move_fields(bank, false);

	file->close();
	return bank;
}
