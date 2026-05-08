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
	shared_ptr<Gio::SimpleActionGroup> action_group;
	void show_track_settings();
	TrackSettings track_settings;
protected:
	void on_show() override;
	void on_lmb_down();
	void on_hscroll();
	void on_vscroll();
	bool on_mouse_scroll(double x, double y);
	// Actions.
	void cut_selection();
	void copy_selection();
	void paste_selection();
	void delete_selection();
	void move_selection_semitone(int relative_semitones);
	void move_selection_tick(int relative_offset);
	shared_ptr<Gtk::EventControllerScroll> scroll_controller;
	vector<Note> copy_buffer;
	int copy_buffer_start_offset = 0, copy_buffer_length = 0;

	EventHeader* event_header;
	GridPanel* grid_panel;
	PianoCtrl* piano_ctrl;
	Gtk::Scrollbar* hscrollbar;
	Gtk::Scrollbar* vscrollbar;

	double zoom = 1.0;
};
