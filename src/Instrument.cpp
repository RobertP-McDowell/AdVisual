#include <Instrument.h>
#include <FileAccess.h>
#include <common.h>

using namespace FileAccess;

OPLFM Instrument::default_carrier(  13, 2, 15-8 , 4, false, true,  1, 0, false, 63-63, 0, false, 0);
OPLFM Instrument::default_modulator(15, 1, 15-10, 3, false, false, 1, 3, false, 63-48, 2, false, 0);

Instrument Instrument::default_instrument(default_carrier, default_modulator);

void Bank::add_instrument(Instrument new_instrument) {
	if (strlen(new_instrument.name) == 0) {
		cerr << "Assign a name to add instrument!\n";
		return;
	}
	if (isspace(new_instrument.name[0]) == true) {
		cerr << "Can't have preceeding space on name!\n";
		return;
	}
	for (auto insit = instruments.begin(); insit != instruments.end(); insit++) {
		int name_cmp = strcmp(new_instrument.name, insit->name);
		if (name_cmp == 0) {
			cerr << "Instrument name '" << new_instrument.name << "' Already taken.\n";
			return;
		}
		if (name_cmp < 0) {
			instruments.insert(insit, new_instrument);
			return;
		}
	}
	instruments.push_back(new_instrument);
}

void Bank::delete_instrument(char name[9]) {
	for (auto insit = instruments.begin(); insit != instruments.end(); insit++) {
		if (strcmp(insit->name, name) == 0) {
			instruments.erase(insit);
			cout << "Delete instrument '" << name << "'\n";
			return;
		}
	}
	cerr << "Could not find and delete instrument of name '" << name << "'\n";
}

Instrument* Bank::find_instrument(wxString name) {
	char char_name[9];
	name = name.MakeUpper();
	name.resize(8);
	memcpy(&char_name, name.c_str(), 9);
	return find_instrument(char_name);
}
Instrument* Bank::find_instrument(char name[9]) {
	for (Instrument& ins : instruments) {
		if (strcmp(name, ins.name) == 0) {
			return &ins;
		}
	}
	DBPRINT("No instrument found by the name " << name << ", returning default instrument in its stead.");
	return &Instrument::default_instrument;
}

void Bank::save_file(wxString save_path) {
	cout << "Saving Bank File: " << save_path << "\n";
	if (!access_file(save_path, true)) return;
	bnk_move_fields();
	file_path = save_path;
	ios_file.close();
}

void Bank::load_file(wxString load_path) {
	cout << "Loading Bank File: " << load_path << "\n";
	if (!access_file(load_path, false)) return;
	clear_bank_data();
	bnk_move_fields();
	file_path = load_path;
	ios_file.close();
	return;
}

void bank_move_OPLFM_fields(OPLFM& opl) {
	// I have no idea why, but ksl values 1 and 2 are inverted. But 0 and 3 are the same?????
	opl.ksl = (opl.ksl == 1 || opl.ksl == 2 ? 3 - opl.ksl : opl.ksl);
	fieldcpy_uint8(&opl.ksl, 1);
	opl.ksl = (opl.ksl == 1 || opl.ksl == 2 ? 3 - opl.ksl : opl.ksl);
	fieldcpy_uint8(&opl.frequency_multiplier, 1);
	fieldcpy_uint8(&opl.feedback, 1);
	fieldcpy_uint8(&opl.attack_rate, 1);
	fieldcpy_uint8(&opl.sustain_level, 1);
	fieldcpy_uint8(&opl.sustain_sound, 1);
	fieldcpy_uint8(&opl.decay_rate, 1);
	fieldcpy_uint8(&opl.release_rate, 1);
	fieldcpy_uint8(&opl.output_level, 1);
	fieldcpy_uint8(&opl.tremelo, 1);
	fieldcpy_uint8(&opl.vibrato, 1);
	fieldcpy_uint8(&opl.ksr, 1);
	opl.additive_synth = (opl.additive_synth ^ 1);
	fieldcpy_uint8(&opl.additive_synth, 1); // Connector modulator only.
	opl.additive_synth = (opl.additive_synth ^ 1);
	//DBPRINT("level_scaling: " << int(opl.level_scaling) << " freq_mul: " << int(opl.frequency_multiplier) <<
	//	" feedback: " << int(opl.feedback) << " atk_rt: " << int(opl.attack_rate) << " sustain_lvl: " << int(opl.sustain_level) <<
	//	" envelope_scaling: " << int(opl.envelope_scaling) << " decay_rt: " << int(opl.decay_rate) <<
	//	" release_rt: " << int(opl.release_rate) << " output_level: " << int(opl.output_level) <<
	//	" tremelo: " << int(opl.tremelo) << " vibrato: " << int(opl.vibrato));
}

void Bank::bnk_move_fields() {
	DBPRINT("Writing to bank.");
	fieldcpy_uint8(&file_version_major, 1);
	fieldcpy_uint8(&file_version_minor, 1);
	DBPRINT("File version " << int(file_version_major) << "." << int(file_version_minor) << "\n");
	char signature[6] = {'A', 'D', 'L', 'I', 'B', '-'}; // Not null terminated!
	//fieldcpy_char(&signature[0], 5); // Simply says "ADLIB-"
	fieldzero(6); // Signature.
	uint16_t byte_size_of_instrument_name = 12;
	uint16_t byte_size_of_instrument_data = 17;
	uint16_t num_of_ins_used = instruments.size();
	uint16_t num_of_ins = instruments.size();
	uint32_t name_offset = 28; // name offset in theory should be a constant 28 bytes from start.
	uint32_t data_offset = name_offset + (num_of_ins_used * 12);
	fieldcpy_uint16(&num_of_ins_used, 2);
	fieldcpy_uint16(&num_of_ins, 2);
	vector<uint16_t> data_indices;
	data_indices.resize(num_of_ins_used);
	fieldcpy_uint32(&name_offset, 4);
	fieldcpy_uint32(&data_offset, 4);
	DBPRINT("Ins used: " << num_of_ins_used << " Ins count: " << num_of_ins <<
		" name offset: " << name_offset << " data_offset: " << data_offset << "\n");
	fieldzero(8); // Padding.
	for (uint16_t insi = 0; insi < num_of_ins_used; insi++) {
		uint16_t data_index = insi;
		if (writing == false) {
			Instrument new_instrument;
			instruments.push_back(new_instrument);
		}
		Instrument& ins = instruments.at(insi);
		fieldcpy_uint16(&data_index, 2);
		fieldcpy_uint8(&ins.flags, 1);
		fieldcpy_char(&ins.name[0], 9-1); // Is null terminated
		data_indices[data_index] = insi;
		DBPRINT(insi << " / " << num_of_ins << "Ins Flags: " << int(ins.flags) << " Ins name: " << ins.name << " Data at: " << data_index);
	}
	for (uint16_t insi = num_of_ins_used; insi < num_of_ins; insi++) {
		fieldzero(2+1+9); // memzero unused instruments.
	}
	DBPRINT("data_offset: " << data_offset << " should equal current file position, which is: " << file.pos());
	for (uint16_t insi = 0; insi < num_of_ins_used; insi++) {
		Instrument& ins = instruments[data_indices.at(insi)];
		fieldcpy_uint8(&ins.percussion_mode, 1);
		fieldcpy_uint8(&ins.voice_number, 1);
		bank_move_OPLFM_fields(ins.modulator);
		bank_move_OPLFM_fields(ins.carrier);
		fieldcpy_uint8(&ins.modulator.waveform, 1);
		fieldcpy_uint8(&ins.carrier.waveform, 1);
		DBPRINT("mWave: " << ins.modulator.waveform << " cWave: " << ins.carrier.waveform <<
			"name: " << ins.name << " Data index: " << insi << " / " << num_of_ins <<
			" Instrument index: " << data_indices.at(insi));
	}
	for (uint16_t insi = num_of_ins_used; insi < num_of_ins; insi++) {
		fieldzero(byte_size_of_instrument_data); // memzero unused instrument data.
	}
}
