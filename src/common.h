#pragma once

#include <wx/artprov.h>


#define DEBUG_MODE

#ifdef DEBUG_MODE
#define DBPRINT(p_output) do { cout << p_output << "\n"; } while(0)
// unlike DBPRINT, DBBREAKPOINT's are meant to be temporarily used (with a tool like gdb),
// and should be left out of pr's/commits. Prefer assert otherwise.
#define DBBREAKPOINT(p_output, p_name) do { \
	raise(SIGTRAP); \
	cout << p_output << " Breakpoint name: " << p_name << "\n"; \
} while(0)
#else
#define DBPRINT(p_output) do {} while(0)
#define DBBREAKPOINT(p_output, p_name) do { \
	cout << p_output << " This breakpoint shouldn't exist in release/shared builds, Breakpoint name: " << p_name << "\n"; \
} while(0)
#endif


using namespace std;

const int pitch_range = 108 - 12;

const wxString ASSETS_PATH = "/home/robert/Desktop/AdVisual/assets/";

wxBitmapBundle GetAsset(wxString asset_name, wxSize asset_size);

enum
{
	ID_COMPOSER = 1,
	ID_INSMAKER,
	ID_LOAD_TRACK,
	ID_SAVE_TRACK,
	ID_PLAY_TRACK,
	ID_LOAD_BANK,
	ID_SAVE_BANK,
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
	ID_TRACK_OPTIONS,
	ID_INSMAKER_START, // INSMAKER specific enums start here.
	ID_INSTRUMENT_FIELD
};



