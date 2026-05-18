#pragma once
#include <gtkmm.h>
#include <common.h>
#include <cairomm/context.h>
#include <Track.h>
#include <CommonWidgets.h>

using namespace std;

extern int cursor_tick;
extern int cursor_end;

class EventPopup : public Gtk::Popover {
public:
	EventPopup();
	void popup(int at_tick);
protected:
	void on_closed();
	void on_instrument_field_text_changed();
	void on_bank_ctrl_instrument_selected(Instrument* ins);
	void init_spin_field(Gtk::Grid& grid, string text, int entry_idx, Gtk::Entry& entry);
	int editing_tick = 0;
	Gtk::Entry tempo_field;
	Gtk::Entry instrument_field;
	Gtk::Entry pitch_field;
	Gtk::Entry volume_field;
	BankCtrl bank_ctrl;
};

class GridPanel : public Gtk::DrawingArea {
public:
	GridPanel();
	vec2 scroll_offset = vec2(0);
	void set_preview_channels(bool value) { preview_channels = value; queue_draw(); }
	bool get_preview_channels() const { return preview_channels; }
	void set_audio_feedback(bool value) { audio_feedback = value; }
	bool get_audio_feedback() const { return audio_feedback; }
protected:
	void on_draw(const shared_ptr<Cairo::Context>& cr, int width, int height);
	void on_lmb_down(int n_press, double x, double y);
	void on_lmb_up(int n_press, double x, double y);
	void on_rmb_down(int n_press, double x, double y);
	void on_rmb_up(int n_press, double x, double y);
	void on_mouse_motion(double x, double y);
	shared_ptr<Gtk::GestureClick> lmb_gesture;
	shared_ptr<Gtk::GestureClick> rmb_gesture;
	unique_ptr<Note> ghost_note = nullptr;
	vec2 mouse_down_start;
	bool preview_channels = true;
	bool audio_feedback = true;
};

class EventHeader : public Gtk::DrawingArea {
public:
	EventHeader();
	int scroll_offset = 0;
protected:
	void on_draw(const shared_ptr<Cairo::Context>& cr, int width, int height);
	void on_rmb_down(int n_press, double x, double y);
	void on_rmb_up(int n_press, double x, double y);
	shared_ptr<Gtk::GestureClick> rmb_gesture;
	EventPopup event_popup;
};

class TrackSettings : public Gtk::Window {
public:
	TrackSettings();
	sigc::signal<void()> signal_visible_change;
protected:
	void on_track_changed();
	void on_tempo_set();
	void on_beats_per_measure_set();
	void on_ticks_per_beat_set();
	void on_percussion_toggled();
	void add_spinbox_property(string name, Gtk::SpinButton& spinner, double val, double min, double max, double step, double page, int idx);
	Gtk::SpinButton tempo_spinner;
	Gtk::SpinButton beats_per_measure_spinner;
	Gtk::SpinButton ticks_per_beat_spinner;
	Gtk::CheckButton percussion_checkbox;
	Gtk::Grid grid;
};

class ComposerPanel : public Gtk::Grid {
public:
	ComposerPanel();
	void set_preview_channels(bool value) { grid_panel->set_preview_channels(value); }
	bool get_preview_channels() const { return grid_panel->get_preview_channels(); }
	void set_audio_feedback(bool value) { grid_panel->set_audio_feedback(value); }
	bool get_audio_feedback() const { return grid_panel->get_audio_feedback(); }
	shared_ptr<Gio::SimpleActionGroup> action_group;
	void show_track_settings();
	TrackSettings track_settings;
	enum PasteMode {
		OVERWRITE_SELECTION,
		OVERWRITE_BUFFER_LENGTH,
		OVERWRITE_BUFFER_NOTES
	};
protected:
	// Signals.
	void on_show() override;
	void on_lmb_down();
	void on_hscroll();
	void on_vscroll();
	bool on_mouse_scroll(double x, double y);
	void on_track_changed();
	void on_channel_changed();
	// Actions.
	void cut_selection();
	void copy_selection(vector<Note>* p_copy_buffer, bool remove_whitespace);
	void paste_selection(vector<Note>* p_copy_buffer, PasteMode paste_mode = OVERWRITE_BUFFER_LENGTH);
	void erase_selection();
	void move_selection_semitone(int pitch_offset);
	void move_selection_tick(int tick_offset);
	void unselect();

	vector<Note> copy_buffer;

	EventHeader* event_header;
	GridPanel* grid_panel;
	PianoCtrl* piano_ctrl;
	Gtk::Label* instrument_hint;
	Gtk::Scrollbar* hscrollbar;
	Gtk::Scrollbar* vscrollbar;
	shared_ptr<Gtk::EventControllerScroll> scroll_controller;

	double zoom = 1.0;
};
