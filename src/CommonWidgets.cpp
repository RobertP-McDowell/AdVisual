#include <CommonWidgets.h>
#include <Track.h>
#include <common.h>

using namespace Gtk;

vector<ChannelButton*> ChannelButton::buttons = {};
ChannelButton* ChannelButton::currently_pressed = nullptr;

ChannelButton::ChannelButton(int p_channel_index) : channel_index(p_channel_index) {
	add_css_class("channel-button");
	set_name("channel-button" + to_string(channel_index + 1));
	if (current_track->melodic_mode == 0 && channel_index > 5) {
		add_css_class("channel-button-percussion");
	}
	set_draw_func(sigc::mem_fun(*this, &ChannelButton::on_draw));

	lmb_gesture = GestureClick::create();
	lmb_gesture->set_button(GDK_BUTTON_PRIMARY);
	lmb_gesture->signal_pressed().connect(mem_fun(*this, &ChannelButton::on_lmb_down));
	add_controller(lmb_gesture);

	shared_ptr<EventControllerMotion> motion_event = Gtk::EventControllerMotion::create();
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
	current_channel = current_track->get_channel(channel);
	currently_pressed->queue_draw();
	signal_channel_changed.emit();
}

void ChannelButton::update_melodic_mode() {
	if (!current_track->melodic_mode) {
		for (int i = 0; i < buttons.size(); i++) {
			if (buttons[i]->channel_index > 5) {
				buttons[i]->add_css_class("channel-button-percussion");
			}
			buttons[i]->set_visible(true);
			buttons[i]->queue_draw();
		}
	}
	else {
		for (int i = 6; i < 9; i++) { // We will need to redraw the text for BS and SD (now simply channel 8/9)
			if (buttons[i]->channel_index > 5) {
				buttons[i]->remove_css_class("channel-button-percussion");
			}
			buttons[i]->queue_draw();
		}
		for (int i = 9; i < buttons.size(); i++) {
			buttons[i]->set_visible(false);
			if (buttons[i]->pressed == true) {
				set_pressed_channel(0);
			}
		}
	}
}

void ChannelButton::on_lmb_down(int n_press, double x, double y) {
	if (bool(lmb_gesture->get_current_event_state() & Gdk::ModifierType::SHIFT_MASK)) {
		enabled_channels[channel_index] = !enabled_channels[channel_index];
		queue_draw();
		return;
	}
	set_pressed();
}

void ChannelButton::on_draw(const shared_ptr<Cairo::Context>& cr, int width, int height) {
	if (width == 0) { return; } // Probably cant happen, but just in case.
	RGBA ch_color = current_track->get_channel(channel_index)->color;
	if (!enabled_channels[channel_index]) {
		ch_color = RGBA(0.5, 0.5, 0.5, 1.0);
	}
	RGBA hover_color = ch_color;
	hover_color.set_alpha(0.5);
	double line_width = 4.0;
	double half_line_width = line_width / 2.0;
	cr->set_line_width(half_line_width * 2.0);
	double diameter = (height < width ? height : width) - (line_width);
	double x_offset = (diameter / 2.0);
	Gdk::Cairo::set_source_rgba(cr, ch_color);
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
	double text_scaler = ((double(width) - (diameter + half_line_width)) / double(width)) * 2.0;
	if (text_scaler <= 0.4 || width <= height) { return; } // Don't bother with writing text.
	Gdk::Cairo::set_source_rgba(cr, RGBA(1.0, 1.0, 1.0));
	Pango::FontDescription font;
	font.set_family("Monospace");
	font.set_weight(Pango::Weight::BOLD);
	font.set_size(height / 2.0 * PANGO_SCALE);
	shared_ptr<Pango::Layout> layout = create_pango_layout("");
	layout->set_font_description(font);
	string ch_txt = to_string(channel_index + 1);
	if (current_track->melodic_mode == 0 && channel_index > 5) {
		if (channel_index == 6)  ch_txt = "BD";
		if (channel_index == 7)  ch_txt = "SD";
		if (channel_index == 8)  ch_txt = "TD";
		if (channel_index == 9)  ch_txt = "CY";
		if (channel_index == 10) ch_txt = "HH";
	}
	layout->set_text(ch_txt);
	if (ch_txt.length() == 1) { text_scaler *= 2.0; }
	cr->move_to(diameter - half_line_width, 0);
	cr->scale(text_scaler, 1.0);
	layout->show_in_cairo_context(cr);
}

#include <Instrument.h>

BankCtrl::BankCtrl() : Box(Orientation::HORIZONTAL) {
	add_css_class("bank-ctrl");
	set_size_request(200, 400);
	draw_panel.set_size_request(-1, -1);
	draw_panel.set_expand(true);
	draw_panel.set_draw_func(mem_fun(*this, &BankCtrl::on_draw));
	append(draw_panel);
	draw_panel.add_css_class("drawing-area");
	vadjust = Adjustment::create(0.0, 0.0, item_height);
	vadjust->signal_value_changed().connect(mem_fun(draw_panel, &BankCtrl::queue_draw));
	vscrollbar = Scrollbar(vadjust, Orientation::VERTICAL);
	append(vscrollbar);
	signal_bank_changed.connect(mem_fun(*this, &BankCtrl::update));

	lmb_gesture = GestureClick::create();
	lmb_gesture->set_button(GDK_BUTTON_PRIMARY);
	lmb_gesture->signal_pressed().connect(mem_fun(*this, &BankCtrl::on_lmb_down));
	draw_panel.add_controller(lmb_gesture);

	scroll_controller = EventControllerScroll::create();
	scroll_controller->set_flags(EventControllerScroll::Flags::VERTICAL);
	scroll_controller->signal_scroll().connect(mem_fun(*this, &BankCtrl::on_mouse_scroll), true);
	add_controller(scroll_controller);
}

void BankCtrl::on_lmb_down(int n_press, double x, double y) {
	if (current_bank == nullptr || current_bank->instruments.empty()) { return; }
	double half_item_height = item_height / 2.0;
	int items_start = max(0, (int)floor(vadjust->get_value() / item_height) - 1);
	selected_item_idx = clamp(items_start + (int)floor((y - half_item_height) / item_height), 0, (int)current_bank->instruments.size() - 1);
	instrument_selected.emit(&current_bank->instruments[selected_item_idx]);
	draw_panel.queue_draw();
}

bool BankCtrl::on_mouse_scroll(double x, double y) {
	double add_y = y;
	if (scroll_controller->get_unit() == Gdk::ScrollUnit::WHEEL) {
		add_y = item_height * y;
	}
	vadjust->set_value(vadjust->get_value() + add_y);
	return true;
}

void BankCtrl::update() {
	vadjust->set_upper(current_bank->instruments.size() * item_height);
	draw_panel.queue_draw();
}

void BankCtrl::search(string search_string) {
	int best_match_idx = -1;
	int best_match_len = -1;
	for (int insi = 0; insi < current_bank->instruments.size(); insi++) {
		const Instrument& ins = current_bank->instruments.at(insi);
		int ins_strlen = strlen(ins.name);
		for (int stri = 0; stri < search_string.length(); stri++) {
			if (stri > ins_strlen || search_string[stri] != ins.name[stri]) { break; }
			if (stri > best_match_len) {
				first_match_idx = insi;
				best_match_idx = insi;
				best_match_len = stri;
			}
			if (stri == best_match_len) {
				last_match_idx = insi;
			}
		}
	}
	if (best_match_idx != -1) {
		ensure_range_visible(first_match_idx, last_match_idx);
	}
	else {
		first_match_idx = -1;
		last_match_idx = -1;
	}
	draw_panel.queue_draw();
}

void BankCtrl::ensure_range_visible(int first_item, int last_item) {
	int first_scroll_offset = (first_item + 1) * item_height;
	int last_scroll_offset = (last_item + 3) * item_height; // Plus 3, because of lower item height and a margin.
	if (first_scroll_offset < vadjust->get_value()) {
		vadjust->set_value(first_scroll_offset);
	}
	else if (last_scroll_offset > vadjust->get_value() + draw_panel.get_height()) {
		vadjust->set_value(last_scroll_offset - draw_panel.get_height());
	}
}

#include <InsmakerPanel.h>
void BankCtrl::on_draw(const shared_ptr<Cairo::Context>& cr, int width, int height) {
	if (current_bank == nullptr || current_bank->instruments.empty()) { return; }
	RGBA select_color = RGBA(0.4, 0.4, 0.4, 1.0);
	RGBA search_indicator_color = RGBA(0.7, 0.7, 0.7, 1.0);
	double left_gap = 4;
	double half_item_height = item_height / 2.0;
	int items_start = max(0, (int)floor(vadjust->get_value() / item_height) - 1);
	// Draw background for selected item.
	if (selected_item_idx != -1) {
		Gdk::Cairo::set_source_rgba(cr, select_color);
		int selected_start = ((selected_item_idx - items_start + 1) * item_height);
		cr->set_line_width(item_height + 1);
		cr->move_to(0, selected_start);
		cr->line_to(width, selected_start);
		cr->stroke();
	}
	// Draw matching search items indicator.
	if (first_match_idx != -1) {
		Gdk::Cairo::set_source_rgba(cr, search_indicator_color);
		cr->set_line_width(left_gap);
		cr->move_to(left_gap / 2.0, (half_item_height - item_padding) + (first_match_idx - items_start) * item_height);
		cr->line_to(left_gap / 2.0, (half_item_height - item_padding) + (last_match_idx - items_start + 1) * item_height);
		cr->stroke();
	}
	// Draw text for each item.
	Pango::FontDescription font;
	font.set_family("Monospace");
	font.set_weight(Pango::Weight::BOLD);
	font.set_size((item_height - item_padding) * PANGO_SCALE);
	shared_ptr<Pango::Layout> layout = create_pango_layout("");
	layout->set_font_description(font);
	int items_visible = (height / item_height) + 2;
	cr->scale(panel_stretch, 1.0);
	for (int i = items_start; i < min(int(current_bank->instruments.size()), items_visible + items_start); i++) {
		const Instrument& ins = current_bank->instruments[i];
		RGBA instrument_color = RGBA(1.0, 1.0, 1.0, 1.0);
		if (ins.percussion_mode == 1) {
			instrument_color = current_track->get_channel(ins.voice_number)->color;
		}
		Gdk::Cairo::set_source_rgba(cr, instrument_color);
		string ins_text = (string)ins.name;
		if (InsmakerPanel::find_unsaved_instrument(ins.name) != nullptr) {
			ins_text += "*";
		}
		layout->set_text(ins_text);
		cr->move_to(left_gap, (i - items_start) * item_height);
		layout->show_in_cairo_context(cr);
	}
}

#include <AdPlayer.h>

PianoCtrl::PianoCtrl(bool p_tall) : instrument(&Instrument::default_instrument), tall(p_tall) {
	add_css_class("piano-ctrl");
	set_draw_func(mem_fun(*this, &PianoCtrl::on_draw));
	if (tall) {
		set_size_request(deepness, -1);
	}
	else {
		set_size_request(-1, deepness);
	}
	lmb_gesture = GestureClick::create();
	lmb_gesture->set_button(GDK_BUTTON_PRIMARY);
	lmb_gesture->signal_pressed().connect(mem_fun(*this, &PianoCtrl::on_lmb_down));
	lmb_gesture->signal_released().connect(mem_fun(*this, &PianoCtrl::on_lmb_up));
	add_controller(lmb_gesture);
	motion_controller = Gtk::EventControllerMotion::create();
	motion_controller->signal_motion().connect(mem_fun(*this, &PianoCtrl::on_mouse_motion));
	add_controller(motion_controller);
}

int PianoCtrl::get_note_number_at_position(double x, double y) {
	if (tall == true) {
		return (y + scroll_offset) / key_width;
	}
	else {
		return (x + scroll_offset) / key_width;
	}
}

void PianoCtrl::on_lmb_down(int n_press, double x, double y) {
	int note_number = get_note_number_at_position(x, y);
	adplayer->play_note(note_number, current_channel->channel_number, instrument);
	playing_note = note_number;
}

void PianoCtrl::on_lmb_up(int n_press, double x, double y) {
	adplayer->play_note(0, current_channel->channel_number, instrument);
	playing_note = 0;
}

void PianoCtrl::on_mouse_motion(double x, double y) {
	int note_number = get_note_number_at_position(x, y);
	if (lmb_gesture->get_current_button() && playing_note != note_number) {
		adplayer->play_note(note_number, current_channel->channel_number, instrument);
		playing_note = note_number;
	}
	status->set_text(note_number_to_letter(note_number));
}

void PianoCtrl::on_draw(const shared_ptr<Cairo::Context>& cr, int width, int height) {
	RGBA sharp_color = RGBA(0.0, 0.0, 0.0, 1.0);
	Gdk::Cairo::set_source_rgba(cr, sharp_color);
	int max_offstep = key_width * full_octave;
	int draw_offstep = scroll_offset % max_offstep;
	int grid_sub = 0;
	int half_key = key_width / 2.0;
	if (tall == true) {
		for (int i = 0; i <= height / key_width; i++) {
			double y = (key_width * 2.0 * (i-(grid_sub/2.0))) - draw_offstep;
			if (i % 7 == 4 || i % 7 == 0)  {
				cr->move_to(0, y);
				cr->line_to(deepness, y);
				cr->stroke();
				grid_sub++;
				continue;
			}
			cr->move_to(0, y + half_key);
			cr->line_to(deepness, y + half_key);
			cr->stroke();
			cr->rectangle(0, y, deepness / 2.0, key_width);
			cr->fill();
		}
		// Draw middle C.
		for (int i = 0; i < 4; i++) {
			int y = (key_width * middle_c) - (key_width * 1.25) - scroll_offset;
			int x = deepness - (i * 6);
			cr->move_to(x, y);
			cr->line_to(x, y + key_width);
			cr->stroke();
		}
	}
	else {
		for (int i = 0; i <= width / key_width; i++) {
			double x = (key_width * 2.0 * (i-(grid_sub/2.0))) - draw_offstep;
			if (i % 7 == 4 || i % 7 == 0)  {
				cr->move_to(x, 0);
				cr->line_to(x, deepness);
				cr->stroke();
				grid_sub++;
				continue;
			}
			cr->move_to(x + half_key, 0);
			cr->line_to(x + half_key, deepness);
			cr->stroke();
			cr->rectangle(x, 0, key_width, deepness / 2.0);
			cr->fill();
		}
		// Draw middle C.
		for (int i = 0; i < 4; i++) {
			int x = (key_width * middle_c) - (key_width * 1.25) - scroll_offset;
			int y = deepness - (i * 6);
			cr->move_to(x, y);
			cr->line_to(x + key_width, y);
			cr->stroke();
		}
	}
}

