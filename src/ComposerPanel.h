#pragma once

#include <wx/wx.h>
#include <wx/utils.h>
#include <wx/filedlg.h> 
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <wx/scrolwin.h>
#include <wx/scrolbar.h>
#include <wx/popupwin.h>
#include <common.h>
#include <Track.h>
#include <Instrument.h>
#include <FileAccess.h>
#include <AdPlayer.h>
#include <memory.h>

using namespace std;

class EventPopup : public wxPopupTransientWindow {
public:
	int editing_event_tick;
	wxString current_track_file_path = wxEmptyString;
	BankControl* bank_ctrl;
	EventPopup(wxWindow* parent);
	void Popup(int at_tick);
protected:
	void init_event_field(wxGridSizer* sizer, wxTextCtrl*& event_field, int ID);
	void on_show(wxShowEvent& event);
	void on_text_entered(wxCommandEvent& event);
	wxTextCtrl* tempo_field;
	wxTextCtrl* instrument_field;
	wxTextCtrl* pitch_field;
	wxTextCtrl* volume_field;
};

class ComposerPanel : public wxWindow {
public:
	ComposerPanel(wxWindow *parent);
	void SetChannelIndex(int channel);
	int GetChannelIndex() const { return current_channel_idx; }
	void SetPreviewChannels(bool value);
	void SetAudioFeedback(bool value) { grid_audio_feedback = value; }
	EventPopup* event_popup;
	PianoControl* piano_ctrl;
protected:
	// grid_panel functions and events.
	void on_paint_grid(wxPaintEvent& event);
	void draw_notes();
	void draw_grid();
	void on_lmb_down(wxMouseEvent& event);
	void on_lmb_up(wxMouseEvent& event);
	void on_rmb_down(wxMouseEvent& event);
	void on_rmb_up(wxMouseEvent& event);
	void on_mouse_motion(wxMouseEvent& event);
	// header functions and events.
	void on_channel_button_pressed(wxCommandEvent& event);
	void on_preview_channels_checked(wxCommandEvent& event);
	// event_header and events.
	void on_paint_event_header(wxPaintEvent& event);
	void on_mouse_motion_event_header(wxMouseEvent& event);
	void on_lmb_down_event_header(wxMouseEvent& event);
	void on_lmb_up_event_header(wxMouseEvent& event);
	// Main window and misc.
	void on_mouse_wheel(wxMouseEvent& event);
	void on_key_down(wxKeyEvent& event);
	void on_resize(wxSizeEvent& event);
	void on_scroll_grid_horizontal(wxScrollEvent& event);
	void on_scroll_grid_vertical(wxScrollEvent& event);
	void update_scrollbars();
	void move_h_scrollbar(int new_pos);
	void move_v_scrollbar(int new_pos);
	// Helper functions.
	wxRect get_note_rect(const Note& note) const;

	wxFont event_font;

	wxPoint grid_offset = wxPoint(0, 0);
	wxSize note_size;
	wxSize cell_size;
	int scroll_multiplier = 4; // TODO: make this editable in settings.
	int cursor_end = -1; // Start of selection is cursor_tick in common.h
	double zoom = 1.0;
	wxPoint2DDouble mouse_down_start;
	int editing_event_tick = -1;
	bool preview_channels = false, grid_audio_feedback = false, follow_cursor = false;
	unique_ptr<Note> editing_note;
	void copy_notes();
	void paste_notes();
	vector<Note> copy_buffer;
	int copy_buffer_start_offset = 0, copy_buffer_length = 0;
	wxPanel* event_header;
	wxPanel* grid_panel;
	wxScrollBar* h_scrollbar;
	wxScrollBar* v_scrollbar;
};
