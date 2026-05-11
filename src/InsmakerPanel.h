#pragma once
#include <gtkmm.h>
#include <common.h>
#include <CommonWidgets.h>
#include <Instrument.h>
#include <memory>

using namespace std;

class OPLFMPropertyControl : public Gtk::Box {
protected:
	uint8_t* base_value = nullptr;
	uint8_t* control_value = nullptr;
public:
	bool is_valid() const { return base_value != nullptr && control_value != nullptr; }
	virtual void set_new_value(int new_value) { // signed int as argument so it doesn't underflow.
		*control_value = (new_value < 0 ? uint8_t(0) : uint8_t(new_value));
		queue_draw();
	}
	int get_new_value() const { return *control_value; }
	void load_from_value(uint8_t* p_base_value, uint8_t* p_control_value) {
		base_value = p_base_value;
		control_value = p_control_value;
		if (!is_valid()) {
			set_visible(false);
			return;
		}
		set_visible(true);
		set_new_value(*p_control_value); // Call this to draw again.
	}
	void save_base_value() {
		if (!is_valid()) { return; }
		*base_value = *control_value;
		queue_draw();
	}
	bool has_unsaved_change() const { return is_valid() && *control_value != *base_value; }
	OPLFMPropertyControl(uint8_t* p_base_value, uint8_t* p_control_value) :
			base_value(p_base_value), control_value(p_control_value) {
		if (!is_valid()) {
			set_visible(false);
			return;
		}
		*control_value = *base_value;
	}
};

class OPLFMCheckbox : public OPLFMPropertyControl {
protected:
	Gtk::Button checkbox;
	Gtk::Picture check_picture;
	Gtk::Picture asterisk_picture;
	void on_checkbox_toggled() {
		set_new_value(int(!bool(*control_value)));
	}
public:
	virtual void set_new_value(int new_value) {
		if (!is_valid()) { return; }
		*control_value = (new_value < 0 ? uint8_t(0) : uint8_t(new_value));
		check_picture.set_opacity(*control_value);
		// We set opacity instead of showing/hiding it, so the box will measure its size.
		if (has_unsaved_change()) { asterisk_picture.set_opacity(1); }
		else { asterisk_picture.set_opacity(0); }
		queue_draw();
	}
	OPLFMCheckbox(uint8_t* p_base_value, uint8_t* p_control_value) : OPLFMPropertyControl(p_base_value, p_control_value) {
		set_expand(true);
		set_size_request(-1, -1);
		set_halign(Gtk::Align::CENTER);
		checkbox.set_name("insmaker-checkbox");
		checkbox.signal_clicked().connect(sigc::mem_fun(*this, &OPLFMCheckbox::on_checkbox_toggled));
		append(checkbox);
		asterisk_picture = Gtk::Picture(ICON_PATH("UnsavedAsterisk.svg"));
		asterisk_picture.set_size_request(-1, -1);
		asterisk_picture.set_content_fit(Gtk::ContentFit::COVER);
		append(asterisk_picture);
		check_picture = Gtk::Picture(ICON_PATH("CheckButton.svg"));
		check_picture.set_size_request(-1, -1);
		check_picture.set_content_fit(Gtk::ContentFit::COVER);
		checkbox.set_child(check_picture);
		if (has_unsaved_change()) { asterisk_picture.set_opacity(1); }
		else { asterisk_picture.set_opacity(0); }
		if (!is_valid()) {
			set_visible(false);
			return;
		}
		check_picture.set_opacity(*control_value);
		set_visible(true);
	}
};

class OPLFMRadio : public OPLFMPropertyControl {
protected:
	vector<unique_ptr<Gtk::ToggleButton>> buttons;
	Gtk::Picture asterisk_picture;
	uint8_t min_value;
	uint8_t max_value;
	void on_radio_button_clicked(int idx) {
		set_new_value(idx);
	}
	void add_radio_button(string img_name, int idx) {
		unique_ptr<Gtk::ToggleButton> bttn = make_unique<Gtk::ToggleButton>();
		Gtk::Picture pic = Gtk::Picture(ICON_PATH(img_name));
		pic.set_size_request(-1, -1);
		pic.set_content_fit(Gtk::ContentFit::COVER);
		bttn->set_name("insmaker-radio");
		bttn->set_child(pic);
		bttn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &OPLFMRadio::on_radio_button_clicked), idx));
		if (!buttons.empty()) { bttn->set_group(*buttons[0]); }
		append(*bttn);
		buttons.push_back(move(bttn));
	}
public:
	virtual void set_new_value(int new_value) {
		if (!is_valid()) { return; }
		*control_value = clamp(new_value, int(min_value), int(max_value));
		buttons[*control_value]->set_active(true);
		// We set opacity instead of showing/hiding it, so the box will measure its size.
		if (has_unsaved_change()) { asterisk_picture.set_opacity(1); }
		else { asterisk_picture.set_opacity(0); }
		queue_draw();
	}
	OPLFMRadio(uint8_t* p_base_value, uint8_t* p_control_value) :
				OPLFMPropertyControl(p_base_value, p_control_value), min_value(0), max_value(3) {
		set_expand(true);
		set_size_request(-1, -1);
		set_halign(Gtk::Align::CENTER);
		add_radio_button("FullSine.svg", 0);
		add_radio_button("HalfSine.svg", 1);
		add_radio_button("AbsSine.svg", 2);
		add_radio_button("PulseSine.svg", 3);
		
		asterisk_picture = Gtk::Picture(ICON_PATH("UnsavedAsterisk.svg"));
		asterisk_picture.set_size_request(-1, -1);
		asterisk_picture.set_content_fit(Gtk::ContentFit::COVER);
		append(asterisk_picture);
		if (has_unsaved_change()) { asterisk_picture.set_opacity(1); }
		else { asterisk_picture.set_opacity(0); }
		if (!is_valid()) {
			set_visible(false);
			return;
		}
		buttons[0]->set_active(*p_base_value);
		set_visible(true);
	}
};

class OPLFMSlider : public OPLFMPropertyControl {
protected:
	shared_ptr<Gtk::GestureClick> lmb_gesture;
	shared_ptr<Gtk::EventControllerKey> key_gesture;
	Gtk::DrawingArea draw_panel;
	Gtk::Button minus_button;
	Gtk::Button plus_button;
	uint8_t min_value;
	uint8_t max_value;
	bool inverted;
	bool lmb_down = false;
	int get_inverted(int v) const { return (inverted ? max_value + (v * -1.0) : v); }
	//int draw_text_on_slider(shared_ptr<Pango::Layout> layout; 
	void on_draw(const shared_ptr<Cairo::Context>& cr, int width, int height) {
		if (!is_valid()) {
			hide();
			return;
		}
		Pango::FontDescription font;
		font.set_family("Monospace");
		font.set_weight(Pango::Weight::BOLD);
		font.set_size((height / 4.0) * PANGO_SCALE);
		shared_ptr<Pango::Layout> layout = create_pango_layout("");
		layout->set_font_description(font);

		uint8_t disp_ctrl_value = get_inverted(*control_value), disp_base_value = get_inverted(*base_value);
		int layout_width, layout_height;
		cr->set_line_width(1);
		Gdk::Cairo::set_source_rgba(cr, RGBA(0.5, 0.5, 0.5, 1.0));
		double increment_size = width / double(max_value - min_value);
		for (int i = min_value; i <= max_value; i++) {
			cr->move_to(i * increment_size, 0);
			cr->line_to(i * increment_size, height / 3.0);
			cr->stroke();
		}
		if (has_unsaved_change() || draw_panel.is_focus()) {
			// Draw difference between base_value and control_value.
			cr->set_line_width(height);
			if (draw_panel.is_focus()) {
				Gdk::Cairo::set_source_rgba(cr, RGBA(1.0, 0.4, 0.4, 0.5));
			}
			else {
				Gdk::Cairo::set_source_rgba(cr, RGBA(0.4, 0.4, 0.4, 0.5));
			}
			cr->move_to((disp_base_value - min_value) * increment_size, height / 2.0);
			cr->line_to((disp_ctrl_value - min_value) * increment_size, height / 2.0);
			cr->stroke();
			// Draw control_value increment.
			cr->set_line_width(3);
			Gdk::Cairo::set_source_rgba(cr, RGBA(1.0, 1.0, 1.0, 1.0));
			cr->move_to((disp_ctrl_value - min_value) * increment_size, (height / 2.0) + 2);
			cr->line_to((disp_ctrl_value - min_value) * increment_size, height);
			cr->stroke();
			// Draw control_value number.
			layout->set_text(to_string(disp_ctrl_value));
			int ctrl_value_offset = (disp_ctrl_value - min_value) * increment_size;
			layout->get_pixel_size(layout_width, layout_height);
			cr->move_to(ctrl_value_offset + (ctrl_value_offset + layout_width > width ? -layout_width - 3 : 3), (height / 2.0));
			layout->show_in_cairo_context(cr);
		}
		// Draw base_value increment.
		cr->set_line_width(3);
		Gdk::Cairo::set_source_rgba(cr, RGBA(1.0, 1.0, 1.0, 1.0));
		cr->move_to((disp_base_value - min_value) * increment_size, 0);
		cr->line_to((disp_base_value - min_value) * increment_size, (height / 2.0) - 2);
		cr->stroke();
		// Draw base_value number.
		layout->set_text(to_string(disp_base_value));
		layout->get_pixel_size(layout_width, layout_height);
		int base_value_offset = (disp_base_value - min_value) * increment_size;
		cr->move_to(base_value_offset + (base_value_offset + layout_width > width ? -layout_width - 3 : 3), 0);
		layout->show_in_cairo_context(cr);
	}
	void on_lmb_down(int n_press, double x, double y) {
		if (!is_valid()) { return; }
		lmb_down = true;
		draw_panel.grab_focus();
		on_mouse_motion(x, y);
	}
	void on_lmb_up(int n_press, double x, double y) {
		lmb_down = false;
		draw_panel.queue_draw();
	}
	void on_mouse_motion(double x, double y) {
		if (!lmb_down || !is_valid()) { return; }
		double offset = double((x / draw_panel.get_width()) * (max_value - min_value)) + 0.5;
		set_new_value(get_inverted(min_value + offset));
		draw_panel.queue_draw();
	}
	void on_minus_button_clicked() {
		set_new_value(*control_value + (inverted ? 1 : -1));
		draw_panel.queue_draw();
	}
	void on_plus_button_clicked() {
		set_new_value(*control_value + (inverted ? -1 : 1));
		draw_panel.queue_draw();
	}
	void on_draw_state_flags_changed(Gtk::StateFlags previous_flags) {
		draw_panel.queue_draw();
	}
	bool on_key_pressed(guint keyval, guint, Gdk::ModifierType state) {
		if (keyval == GDK_KEY_Left) {
			on_minus_button_clicked();
			return true;
		}
		if (keyval == GDK_KEY_Right) {
			on_plus_button_clicked();
			return true;
		}
		return false;
	}
public:
	virtual void set_new_value(int new_value) {
		*control_value = clamp(new_value, int(min_value), int(max_value));
		draw_panel.queue_draw();
	}
	OPLFMSlider(uint8_t* p_base_value, uint8_t* p_control_value, uint8_t p_min_value = 0, uint8_t p_max_value = 100, bool p_inverted = false) :
				OPLFMPropertyControl(p_base_value, p_control_value), min_value(p_min_value), max_value(p_max_value), inverted(p_inverted) {
		using namespace Gtk;
		set_expand(true);
		set_focusable(false);
		draw_panel.set_focusable(true);
		draw_panel.set_can_focus(true);
		draw_panel.set_sensitive(true);
		draw_panel.set_focus_on_click(true);
		draw_panel.set_can_target(true);
		draw_panel.set_receives_default(true);
		draw_panel.signal_state_flags_changed().connect(mem_fun(*this, &OPLFMSlider::on_draw_state_flags_changed));
		draw_panel.set_draw_func(sigc::mem_fun(*this, &OPLFMSlider::on_draw));
		draw_panel.set_expand(true);
		draw_panel.set_name("insmaker-slider");
		
		lmb_gesture = GestureClick::create();
		lmb_gesture->set_button(GDK_BUTTON_PRIMARY);
		lmb_gesture->signal_pressed().connect(mem_fun(*this, &OPLFMSlider::on_lmb_down));
		lmb_gesture->signal_released().connect(mem_fun(*this, &OPLFMSlider::on_lmb_up));
		draw_panel.add_controller(lmb_gesture);
		key_gesture = EventControllerKey::create();
		key_gesture->signal_key_pressed().connect(mem_fun(*this, &OPLFMSlider::on_key_pressed), false);
		draw_panel.add_controller(key_gesture);
		
		auto motion_event = EventControllerMotion::create();
		motion_event->signal_motion().connect(mem_fun(*this, &OPLFMSlider::on_mouse_motion));
		draw_panel.add_controller(motion_event);
		
		Picture minus_pic = Gtk::Picture(ICON_PATH("MinusButton.svg"));
		minus_pic.set_size_request(-1, -1);
		minus_pic.set_content_fit(Gtk::ContentFit::COVER);
		minus_button.set_can_focus(false);
		minus_button.set_child(minus_pic);
		minus_button.set_name("insmaker-minus");
		Picture plus_pic = Gtk::Picture(ICON_PATH("PlusButton.svg"));
		plus_pic.set_size_request(-1, -1);
		plus_pic.set_content_fit(Gtk::ContentFit::COVER);
		plus_button.set_can_focus(false);
		plus_button.set_child(plus_pic);
		plus_button.set_name("insmaker-plus");
		
		minus_button.signal_clicked().connect(sigc::mem_fun(*this, &OPLFMSlider::on_minus_button_clicked));
		plus_button.signal_clicked().connect(sigc::mem_fun(*this, &OPLFMSlider::on_plus_button_clicked));
		
		append(draw_panel);
		append(minus_button);
		append(plus_button);
		
		if (!is_valid()) {
			hide();
			return;
		}
		show();
	}
};

class InsmakerPanel : public Gtk::Box {
public:
	InsmakerPanel();
	Instrument* get_instrument_ptr() { return instrument_ptr; }
	Instrument* get_instrument() { return &new_instrument; }
	void set_instrument(Instrument* p_new_instrument);
	void set_additive_synth(bool value);
	void toggle_additive_synth();
	void set_rhythm_mode(int value);
	void save_current_instrument();
	void save_instruments();
	void rename_instrument(const char new_name[9]);
	static Instrument* find_unsaved_instrument(const char name[9]);
	shared_ptr<Gio::SimpleActionGroup> action_group;
private:
	int get_unsaved_count();
	void create_oplfm_editor();
	void update_oplfm_editor(OPLFM* p_car, OPLFM* p_mod, OPLFM* n_car, OPLFM* n_mod);
	void update_oplfm_property(int idx, uint8_t* base_car_value_ptr, uint8_t* base_mod_value_ptr,
		uint8_t* new_car_value_ptr, uint8_t* new_mod_value_ptr);
	void add_checkbox_property(string name);
	void add_radio_property(string name);
	void add_slider_property(string name, uint8_t p_min_value, uint8_t p_max_value, bool p_inverted = false);
	void on_value_event();
	void on_scroll_piano();
	bool on_mouse_scroll_piano(double x, double y);
	int get_number_of_unsaved_changes();
	Instrument* instrument_ptr = nullptr;
	Instrument new_instrument;
	vector<unique_ptr<OPLFMPropertyControl>> carrier_properties;
	vector<unique_ptr<OPLFMPropertyControl>> modulator_properties;
	static vector<Instrument> unsaved_instruments;

	unique_ptr<Gtk::Grid> property_grid;
	PianoCtrl* piano_ctrl;
	Gtk::Scrollbar* hscrollbar;
	shared_ptr<Gtk::EventControllerScroll> scroll_controller;

	int number_of_unsaved_changes = 0;
};
