#pragma once

#include <wx/artprov.h>
using namespace std;

const int pitch_range = 108 - 12;

const wxString ASSETS_PATH = "/home/robert/Desktop/AdVisual/assets/";

wxBitmapBundle GetAsset(wxString asset_name, wxSize asset_size);

enum
{
	ID_COMPOSER = 1,
	ID_INSMAKER = 2,
	ID_LOAD = 3,
	ID_SAVE = 4,
	ID_PLAY = 5,
	ID_FILE_MENU = 6,
	ID_HELP_MENU = 7,
	ID_TRACK_START = 9, // Track specific enums start here.
	ID_TEMPO_EVENT = 10,
	ID_INSTRUMENT_EVENT = 11,
	ID_PITCH_EVENT = 12,
	ID_VOLUME_EVENT = 13,
	ID_VOICE_START = 20,
	ID_VOICE_END = 31,
	ID_PREVIEW_CHANNELS = 32,
	ID_TRACK_OPTIONS = 33
};
