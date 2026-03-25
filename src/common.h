#pragma once

#include <wx/wx.h>
#include <wx/utils.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <wx/artprov.h>
#include <memory>
class AdPlayer; // Declare these, since they're mutually dependant!
class Track;
class Channel;
class Bank;
class Instrument;
struct OPLFM;
#include <AdPlayer.h>
#include <Track.h>
#include <Instrument.h>

using namespace std;
extern unique_ptr<AdPlayer> adplayer;
extern unique_ptr<Track> current_track;
extern unique_ptr<Bank> current_bank;
extern Channel* current_channel;
extern int current_channel_idx;

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


const int pitch_range = 108 - 12;
const int middle_c = pitch_range / 2;
const int full_octave = 12;

const wxString ASSETS_PATH = "/home/robert/Desktop/AdVisual/assets/";

wxBitmapBundle GetAsset(wxString asset_name, wxSize asset_size);

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
	ID_TRACK_OPTIONS,
	ID_PIANO_GUIDE,
	ID_FOLLOW_CURSOR,
	ID_INSMAKER_START, // INSMAKER specific enums start here.
	ID_INSTRUMENT_FIELD
};



class PianoControl : public wxControl {
private:
	int key_width = 20;
	int deepness = 80;
	int scroll_offset = 0;
	int orientation;
	Instrument* instrument;
	void on_lmb_down(wxMouseEvent& event);
	void on_mouse_motion(wxMouseEvent& event);
	void on_rmb_down(wxMouseEvent& event); // when rmb is clicked, it instantly stops the note.
	void on_paint(wxPaintEvent& event) {
		wxPaintDC dc(this);
		wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
		int max_offstep = key_width * full_octave;
		int draw_offstep = scroll_offset % max_offstep;
		gc->SetPen(*wxBLACK_PEN);
		gc->SetBrush(*wxBLACK_BRUSH);
		int grid_sub = 0;
		int half_key = (key_width / 2.0);
		wxGraphicsFont text_font = gc->CreateFont(key_width * 2, wxEmptyString, wxFONTFLAG_DEFAULT, *wxWHITE);
		gc->SetFont(text_font);
		if (orientation == wxHORIZONTAL) {
			for (int i = 0; i <= GetSize().y / key_width; i++) {
				double y = (key_width * 2.0 * (i-(grid_sub/2.0))) - draw_offstep;
				if (i % 7 == 4 || i % 7 == 0)  {
					gc->StrokeLine(0, y, deepness, y);
					grid_sub++;
					continue;
				}
				gc->StrokeLine(0, y + half_key, deepness, y + half_key);
				gc->DrawRectangle(0, y, deepness / 2.0, key_width);
			}
			// Draw middle C.
			for (int i = 0; i < 4; i++) {
				int y = (key_width * middle_c) - (key_width * 1.25) - scroll_offset;
				int x = deepness - (i * 6);
				gc->StrokeLine(x, y, x, y + key_width);
			}
		}
		else {
			for (int i = 0; i <= GetSize().x / key_width; i++) {
				double x = (key_width * 2.0 * (i-(grid_sub/2.0))) - draw_offstep;
				if (i % 7 == 4 || i % 7 == 0)  {
					gc->StrokeLine(x, 0, x, deepness);
					grid_sub++;
					continue;
				}
				gc->StrokeLine(x + half_key, 0, x + half_key, deepness);;
				gc->DrawRectangle(x, 0, key_width, deepness / 2.0);
			}
			// Draw middle C.
			for (int i = 0; i < 4; i++) {
				int x = (key_width * middle_c) - (key_width * 1.25) - scroll_offset;
				int y = deepness - (i * 6);
				gc->StrokeLine(x, y, x + key_width, y);
			}
		}
		
		delete gc;
	}
public:
	PianoControl(wxWindow* parent, int id = wxID_ANY, int orient = wxHORIZONTAL) : wxControl(parent, id), orientation(orient) {
		Bind(wxEVT_PAINT, &PianoControl::on_paint, this);
		Bind(wxEVT_LEFT_DOWN, &PianoControl::on_lmb_down, this);
		if (orientation == wxHORIZONTAL) {
			SetMinSize(wxSize(deepness, key_width));
		}
		else {
			SetMinSize(wxSize(key_width, deepness));
		}
		SetBackgroundColour(wxColour(200, 190, 190));
		SetWindowStyle(wxBORDER_NONE);
	}
	void SetKeyWidth(int value) { key_width = value; Refresh(); Update(); }
	void SetDeepness(int value) { deepness = value; Refresh(); Update(); }
	void SetScrollOffset(int value) { scroll_offset = value; Refresh(); Update(); }
	void SetInstrument(Instrument* value) { instrument = value; }
	int GetKeyWidth() const { return key_width; }
	int GetDeepness() const { return deepness; }
	int GetScrollOffset() const { return scroll_offset; }
	Instrument* GetInstrument() const { return instrument; }
};




