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
#include <FileAccess.h>
#include <AdPlayer.h>
#include <memory.h>

using namespace std;

class EventPopup : public wxPopupTransientWindow {
public:
	EventPopup(wxWindow* parent, shared_ptr<Track> track, shared_ptr<Channel> channel);
	int editing_event_tick;
	shared_ptr<Channel> current_channel;
	shared_ptr<Track> current_track;
	void Popup(int at_tick, shared_ptr<Track> track, shared_ptr<Channel> channel);
private:
	void init_event_field(wxGridSizer* sizer, wxTextCtrl*& event_field, int ID);
	void on_show(wxShowEvent& event);
	void on_text_entered(wxCommandEvent& event);
	wxTextCtrl* tempo_field;
	wxTextCtrl* instrument_field;
	wxTextCtrl* pitch_field;
	wxTextCtrl* volume_field;
};

class ComposerPanel : public wxPanel {
public:
	ComposerPanel(wxWindow *parent);
	shared_ptr<Track> current_track;
	void SetPreviewChannels(bool value);
	void SetChannelIndex(int value);
	int GetChannelIndex() const { return current_channel_idx; }
private:
	// grid _panelfunctions and events.
	void on_paint_grid(wxPaintEvent& event);
	void on_scroll_grid(wxScrollWinEvent& event);
	void draw_notes();
	void draw_grid();
	void on_lmb_down(wxMouseEvent& event);
	void on_lmb_up(wxMouseEvent& event);
//	void on_rmb_down();
//	void on_rmb_up();
	void on_mouse_motion(wxMouseEvent& event);
	// header functions and events.
	void on_channel_button_pressed(wxCommandEvent& event);
	void on_preview_channels_checked(wxCommandEvent& event);
	// event_header and events.
	void on_paint_event_header(wxPaintEvent& event);
	void on_mouse_motion_event_header(wxMouseEvent& event);
	void on_lmb_down_event_header(wxMouseEvent& event);
	void on_lmb_up_event_header(wxMouseEvent& event);
	wxFont event_font;

	wxPoint grid_offset = wxPoint(0, 0);
	wxSize note_size;
	double zoom = 1.0;
	wxPoint2DDouble mouse_down_start;
	bool preview_channels = false;
	unique_ptr<Note> editing_note;
	shared_ptr<Channel> current_channel;
	int current_channel_idx = 0;
	unique_ptr<wxPanel> event_header;
	unique_ptr<wxScrolledWindow> grid_panel;
	unique_ptr<EventPopup> event_popup;
};
