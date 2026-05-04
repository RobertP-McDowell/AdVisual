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
	void init_spin_field(Gtk::Grid& grid, string text, int entry_idx, Gtk::SpinButton& entry);
	int editing_tick = 0;
	Gtk::SpinButton tempo_field;
	Gtk::Entry instrument_field;
	Gtk::SpinButton pitch_field;
	Gtk::SpinButton volume_field;
	BankCtrl bank_ctrl;
};

class GridPanel : public Gtk::DrawingArea {
public:
	GridPanel();
	vec2 scroll_offset = vec2(0);
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

class ComposerPanel : public Gtk::Grid {
public:
	ComposerPanel();
protected:
	void on_lmb_down();
	void on_hscroll();
	void on_vscroll();
	EventHeader* event_header;
	GridPanel* grid_panel;
	PianoCtrl* piano_ctrl;
	Gtk::Scrollbar* hscrollbar;
	Gtk::Scrollbar* vscrollbar;
	
	double zoom = 1.0;
};
