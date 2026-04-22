#pragma once

#include <gtkmm.h>
#include <memory>
#include <string>
#include <Track.h>
#include <Instrument.h>
#include <AdPlayer.h>
#include <debug.h>

using namespace std;
using Gdk::RGBA;
using sigc::mem_fun;

extern unique_ptr<AdPlayer> adplayer;
extern unique_ptr<Track> current_track;
extern unique_ptr<Bank> current_bank;
extern Channel* current_channel;
extern Gtk::Statusbar* status_bar;
extern int current_channel_idx;
extern int cursor_tick;


#define ICON_PATH(file_name) (string(ICONS_PATH) + string(file_name))

// UI colours.
const RGBA bg_colour = RGBA(0.2, 0.1, 0.25);
const RGBA fg_colour = RGBA(0.3, 0.2, 0.35);
const RGBA scrollbar_bg = RGBA(0.2, 0.1, 0.25); // Til we implement custom themeing, all fg and bg will just be the default.
const RGBA scrollbar_fg = RGBA(0.3, 0.2, 0.35);

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

// exclude boolean operation on two 1D lines, useful for refreshing a lot of ui rects.
void line_exclusion(int p1, int l1, int p2, int l2, int& out_p, int& out_length);

//wxBitmapBundle GetAsset(wxString asset_name, wxSize asset_size);
string note_number_to_letter(int note_number);
float get_float_from_string(string str_val, float min = 0.0, float max = 1.0);

