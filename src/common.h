#pragma once

#include <gtkmm.h>
#include <iostream>
#include <memory>
#include <string>
#include <debug.h>

using namespace std;
using Gdk::RGBA;
using sigc::mem_fun;

extern shared_ptr<Gtk::Application> app;
extern shared_ptr<Gtk::Label> status;

#define ICON_PATH(file_name) (get_advisual_dir() + (string)"share/advisual/icons/" + string(file_name))

const int pitch_range = 108 - 12;
const int middle_c = pitch_range / 2;
const int full_octave = 12;

enum
{
	ID_COMPOSER = 1,
	ID_INSMAKER,
	ID_LOAD_TRACK,
	ID_SAVE_TRACK,
	ID_SAVE_TRACK_AS,
	ID_PLAY_TRACK,
	ID_LOAD_BANK,
	ID_SAVE_BANK,
	ID_SAVE_BANK_AS,
	ID_FILE_MENU,
	ID_HELP_MENU,
	ID_TRACK_START = 20, // Track specific enums start here.
	ID_TEMPO_EVENT,
	ID_INSTRUMENT_EVENT,
	ID_PITCH_EVENT,
	ID_VOLUME_EVENT,
	ID_VOICE_START, // Voices/Channel range.
	ID_VOICE_END = ID_VOICE_START + 11,
	ID_PREVIEW_CHANNELS,
	ID_TRACK_MENU,
	ID_RHYTHM_MODE,
	ID_BASIC_TEMPO,
	ID_TICKS_PER_BEAT,
	ID_BEATS_PER_MEASURE,
	ID_AUDIO_FEEDBACK,
	ID_FOLLOW_CURSOR,
	ID_INSMAKER_START, // INSMAKER specific enums start here.
	ID_INSTRUMENT_FIELD,
	ID_INSTRUMENT_MENU,
	ID_CREATE_INSTRUMENT,
	ID_DELETE_INSTRUMENT,
	ID_COPY_INSTRUMENT,
	ID_ADDITIVE_SYNTH,
	ID_PERCUSSION_MODE,
	ID_MELODIC_INSTRUMENT,
	ID_BASS_INSTRUMENT,
	ID_SNARE_INSTRUMENT,
	ID_TOM_INSTRUMENT,
	ID_CYMBAL_INSTRUMENT,
	ID_HIHAT_INSTRUMENT
};

struct vec2 {
	vec2() : x(0), y(0) {}
	vec2(int p_v) : x(p_v), y(p_v) {}
	vec2(int p_x, int p_y) : x(p_x), y(p_y) {}
	int x;
	int y;
};

int sign(int val);
void string_to_upper(string& str);

// exclude boolean operation on two 1D lines, useful for refreshing a lot of ui rects.
void line_exclusion(int p1, int l1, int p2, int l2, int& out_p, int& out_length);
string note_number_to_letter(int note_number);
float get_float_from_string(string str_val, float min = 0.0, float max = 1.0);
string get_advisual_dir();

Gtk::ToggleButton create_image_button(string image_name);
