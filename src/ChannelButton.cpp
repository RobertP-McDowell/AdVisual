#include <ChannelButton.h>
#include <Track.h>
#include <common.h>

using namespace Gtk;

vector<ChannelButton*> ChannelButton::buttons = {};
ChannelButton* ChannelButton::currently_pressed = nullptr;

ChannelButton::ChannelButton(int p_channel_index) : channel_index(p_channel_index) {
	set_size_request(60, -1);
	set_draw_func(sigc::mem_fun(*this, &ChannelButton::on_draw));

	lmb_gesture = GestureClick::create();
	lmb_gesture->set_button(GDK_BUTTON_PRIMARY);
	lmb_gesture->signal_pressed().connect(mem_fun(*this, &ChannelButton::on_lmb_down));
	add_controller(lmb_gesture);

	auto motion_event = Gtk::EventControllerMotion::create();
	motion_event->signal_enter().connect(mem_fun(*this, &ChannelButton::on_mouse_entered));
	motion_event->signal_leave().connect(mem_fun(*this, &ChannelButton::on_mouse_exited));
	add_controller(motion_event);

	buttons.push_back(this);
}

ChannelButton::~ChannelButton() {
}

void ChannelButton::on_mouse_entered(double x, double y) {
	hovered = true;
	queue_draw();
}

void ChannelButton::on_mouse_exited() {
	hovered = false;
	queue_draw();
}

void ChannelButton::set_pressed() {
	set_pressed_channel(channel_index);
}

void ChannelButton::set_pressed_channel(int channel) {
	if (currently_pressed != nullptr) {
		currently_pressed->pressed = false;
		currently_pressed->queue_draw();
	}
	currently_pressed = buttons[channel];
	currently_pressed->pressed = true;
	current_channel = current_track->GetChannel(channel);
	currently_pressed->queue_draw();
	signal_channel_changed.emit();
}

void ChannelButton::on_lmb_down(int n_press, double x, double y) {
	set_pressed();
}

void ChannelButton::on_draw(const shared_ptr<Cairo::Context>& cr, int width, int height) {
	RGBA ch_color = current_track->GetChannel(channel_index)->color;
	RGBA hover_color = ch_color;
	hover_color.set_alpha(0.5);
	double half_line_width = 2.0;
	cr->set_line_width(half_line_width * 2.0);
	Gdk::Cairo::set_source_rgba(cr, ch_color);
	double diameter = height * 0.8;
	double x_offset = min(diameter / 2.0, width - diameter);
	cr->arc(x_offset, height / 2, (diameter / 2) - half_line_width, 0.0, M_PI * 2);
	cr->stroke();
	if (pressed) {
		cr->arc(x_offset, height / 2, (diameter / 4), 0.0, M_PI * 2);
		cr->fill();
	}
	else if (hovered) {
		Gdk::Cairo::set_source_rgba(cr, hover_color);
		cr->arc(x_offset, height / 2, (diameter / 4), 0.0, M_PI * 2);
		cr->fill();
		Gdk::Cairo::set_source_rgba(cr, ch_color);
	}
	Gdk::Cairo::set_source_rgba(cr, RGBA(1.0, 1.0, 1.0));
	Pango::FontDescription font;
	font.set_family("Monospace");
	font.set_weight(Pango::Weight::HEAVY);
	font.set_size(height / 2.0 * PANGO_SCALE);
	font.set_stretch(Pango::Stretch::ULTRA_CONDENSED);
	shared_ptr<Pango::Layout> layout = create_pango_layout("");
	layout->set_font_description(font);
	string ch_txt = to_string(channel_index + 1);
	if (channel_index > 5) {
		if (channel_index == 6)  ch_txt = "BD";
		if (channel_index == 7)  ch_txt = "SD";
		if (channel_index == 8)  ch_txt = "TD";
		if (channel_index == 9)  ch_txt = "CY";
		if (channel_index == 10) ch_txt = "HH";
	}
	layout->set_text(ch_txt);
	cr->move_to(diameter + (half_line_width * 2.0), 0);
	if (ch_txt.length() == 2) {
		cr->move_to(diameter, 0);
		cr->scale(0.75, 1.0);
	}
	layout->show_in_cairo_context(cr);
}
