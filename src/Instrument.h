#pragma once

#include <cstdint>
#include <memory>
#include <deque>
#include <vector>
#include <map>
#include <common.h>

using namespace std;

struct OPLFM {
	uint8_t attack_rate = 0, decay_rate = 0, sustain_level = 0, release_rate = 0;
	uint8_t frequency_multiplier = 0, feedback = 0, output_level = 0, level_scaling = 0, waveform = 0;
	uint8_t sustain_sound = false, envelope_scaling = false, vibrato = false, tremelo = false;
};

class Instrument {
public:
	uint8_t percussion_mode = 0, voice_number = 0;
	bool additive_synth = false;
	OPLFM carrier;
	OPLFM modulator;
	uint8_t flags = 0; // 0 unused "record", 1 otherwise.
	char name[9] = {'\0'};
};

class Bank {
public:
	uint8_t file_version_major = 0;
	uint8_t file_version_minor = 0;
	vector<Instrument> instruments = {};
	void clear_bank_data() { instruments.clear(); }
};
