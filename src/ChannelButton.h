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
	static void set_pressed_channel(int channel);
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