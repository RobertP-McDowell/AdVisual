#pragma once
#include <gtkmm.h>
#include <iostream>
#include <vector>
using namespace std;

class ChannelButton : public Gtk::DrawingArea {
public:
	ChannelButton(int p_channel_index);
	virtual ~ChannelButton();
	static vector<ChannelButton*> buttons;
	static ChannelButton* currently_pressed;
	static int get_pressed_channel() { return currently_pressed->channel_index; }
	static void set_pressed_channel(int channel);
	static void update_melodic_mode();
	void set_pressed();
protected:
	void on_draw(const shared_ptr<Cairo::Context>& cr, int width, int height);
	void on_lmb_down(int n_press, double x, double y);
	void on_mouse_entered(double x, double y);
	void on_mouse_exited();
	int channel_index;
	bool pressed = false, hovered = false;
	shared_ptr<Gtk::GestureClick> lmb_gesture;
};

#include <Instrument.h>
class BankCtrl : public Gtk::Box {
public:
	BankCtrl();
	void update();
	sigc::signal<void(Instrument*)> instrument_selected;
protected:
	void on_draw(const shared_ptr<Cairo::Context>& cr, int width, int height);
	void on_lmb_down(int n_press, double x, double y);
	bool on_mouse_scroll(double x, double y);
	Gtk::DrawingArea draw_panel;
	Gtk::Scrollbar vscrollbar;
	shared_ptr<Gtk::Adjustment> vadjust;
	shared_ptr<Gtk::GestureClick> lmb_gesture;
	shared_ptr<Gtk::EventControllerScroll> scroll_controller;
	int selected_item_idx = -1;
	double item_height = 10, item_gap = 6, panel_stretch = 1.2;
};

class PianoCtrl : public Gtk::DrawingArea {
protected:
	int key_width = 20, deepness = 80, scroll_offset = 0, playing_note = 0, orientation;
	bool tall;
	shared_ptr<Gtk::GestureClick> lmb_gesture;
	shared_ptr<Gtk::GestureClick> rmb_gesture;
	shared_ptr<Gtk::EventControllerMotion> motion_controller;
	Instrument* instrument;
	int get_note_number_at_position(double x, double y);
	void on_lmb_down(int n_press, double x, double y);
	void on_lmb_up(int n_press, double x, double y);
	void on_mouse_motion(double x, double y);
	void on_rmb_down(int n_press, double x, double y); // when rmb is clicked, it instantly stops the note.
	void on_draw(const shared_ptr<Cairo::Context>& cr, int width, int height);
public:
	PianoCtrl(bool p_tall);
	void set_key_width(int value) { key_width = value; queue_draw(); }
	void set_deepness(int value) { deepness = value; queue_draw(); }
	void set_scroll_offset(int value) { scroll_offset = value; queue_draw(); }
	void set_instrument(Instrument* value) { instrument = value; }
	int get_key_width() const { return key_width; }
	int get_deepness() const { return deepness; }
	int get_scroll_offset() const { return scroll_offset; }
	Instrument* get_instrument() const { return instrument; }
};
