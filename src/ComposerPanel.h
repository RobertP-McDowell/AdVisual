#pragma once
#include <gtkmm.h>
#include <common.h>
#include <cairomm/context.h>
#include <Track.h>

using namespace std;

class GridPanel : public Gtk::DrawingArea {
public:
	GridPanel();
protected:
	void on_draw(const shared_ptr<Cairo::Context>& cr, int width, int height);
	void draw_notes();
	void draw_grid();
	void on_lmb_down(int n_press, double x, double y);
	void on_lmb_up(int n_press, double x, double y);
	void on_rmb_down(int n_press, double x, double y);
	void on_rmb_up(int n_press, double x, double y);
	void on_mouse_motion(double x, double y);
	vec2 scroll_offset = vec2(0);
	shared_ptr<Gtk::GestureClick> lmb_gesture;
	shared_ptr<Gtk::GestureClick> rmb_gesture;
	unique_ptr<Note> ghost_note = nullptr;
	vec2 note_size;
	vec2 cell_size;
	vec2 mouse_down_start;
};

class EventHeader : public Gtk::DrawingArea {
public:
	EventHeader();
protected:
	void on_draw(const shared_ptr<Cairo::Context>& cr, int width, int height);
};

class ComposerPanel : public Gtk::Grid {
public:
	ComposerPanel();
protected:
	void on_lmb_down();
	std::unique_ptr<EventHeader> event_header;
	std::unique_ptr<GridPanel> grid_panel;
	double zoom = 1.0;
	vec2 note_size;
	vec2 cell_size;
};
