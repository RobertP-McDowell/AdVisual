#pragma once
#include <gtkmm.h>
#include <common.h>
#include <Instrument.h>
#include <memory>

using namespace std;

class OPLFMPropertyControl : public Gtk::Box {
protected:
	uint8_t* base_value = nullptr;
	uint8_t* control_value = nullptr;
	bool is_valid() const { return base_value != nullptr && control_value != nullptr; }
public:
	virtual void set_new_value(int new_value) { // signed int as argument so it doesn't underflow.
		*control_value = (new_value < 0 ? uint8_t(0) : uint8_t(new_value));
		queue_draw();
	}
	int get_new_value() const { return *control_value; }
	void update_base_value() {
		if (!is_valid()) { return; }
		*base_value = *control_value;
		queue_draw();
	}
	void load_from_value(uint8_t* p_base_value, uint8_t* p_control_value) {
		if (p_control_value == nullptr) {
			set_visible(false);
			return;
		}
		base_value = p_base_value;
		control_value = p_control_value;
		set_visible(true);
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
	Gtk::CheckButton checkbox;
	Gtk::Picture asterisk_picture;
	void on_checkbox_toggled() {
		set_new_value((int)checkbox.get_active());
	}
public:
	virtual void set_new_value(int new_value) {
		if (!is_valid()) { return; }
		*control_value = (new_value < 0 ? uint8_t(0) : uint8_t(new_value));
		checkbox.set_active(*control_value);
		// We set opacity instead of showing/hiding it, so the box will measure its size.
		if (has_unsaved_change()) { asterisk_picture.set_opacity(1); }
		else { asterisk_picture.set_opacity(0); }
	}
	OPLFMCheckbox(uint8_t* p_base_value, uint8_t* p_control_value) : OPLFMPropertyControl(p_base_value, p_control_value) {
		add_css_class("insmaker-checkbox");
		set_expand(true);
		set_size_request(-1, -1);
		set_halign(Gtk::Align::CENTER);
		checkbox.signal_toggled().connect(sigc::mem_fun(*this, &OPLFMCheckbox::on_checkbox_toggled));
		append(checkbox);
		asterisk_picture = Gtk::Picture(ICON_PATH("UnsavedAsterisk.svg"));
		asterisk_picture.set_size_request(-1, -1);
		asterisk_picture.set_can_shrink(true);
		asterisk_picture.set_content_fit(Gtk::ContentFit::SCALE_DOWN);
		append(asterisk_picture);
		if (p_base_value != nullptr) {
			checkbox.set_active(*p_base_value);
		}
		if (has_unsaved_change()) { asterisk_picture.set_opacity(1); }
		else { asterisk_picture.set_opacity(0); }
		if (!is_valid()) {
			set_visible(false);
			return;
		}
		set_visible(true);
	}
};

class OPLFMSlider : public OPLFMPropertyControl {
protected:
	shared_ptr<Gtk::GestureClick> lmb_gesture;
	Gtk::DrawingArea draw_panel;
	uint8_t min_value = 0;
	uint8_t max_value = 100;
	bool lmb_down = false;
	void on_draw(const shared_ptr<Cairo::Context>& cr, int width, int height) {
		if (!is_valid()) {
			hide();
			return;
		}
		cr->set_line_width(1);
		Gdk::Cairo::set_source_rgba(cr, RGBA(0.5, 0.5, 0.5, 1.0));
		double increment_size = width / double(max_value - min_value);
		for (int i = min_value; i <= max_value; i++) {
			cr->move_to(i * increment_size, 0);
			cr->line_to(i * increment_size, height / 3.0);
			cr->stroke();
		}
		cr->set_line_width(1);
		Gdk::Cairo::set_source_rgba(cr, RGBA(1.0, 1.0, 1.0, 1.0));
		cr->move_to((*base_value - min_value) * increment_size, 0);
		cr->line_to((*base_value - min_value) * increment_size, (height / 2.0) - 2);
		cr->stroke();
		cr->set_line_width(2);
		if (has_unsaved_change()) {
			Gdk::Cairo::set_source_rgba(cr, RGBA(1.0, 1.0, 1.0, 1.0));
			cr->move_to((*control_value - min_value) * increment_size, (height / 2.0) + 2);
			cr->line_to((*control_value - min_value) * increment_size, height);
			cr->stroke();
			Gdk::Cairo::set_source_rgba(cr, RGBA(0.4, 0.4, 1.0, 0.5));
			cr->set_line_width(height);
			cr->move_to((*base_value - min_value) * increment_size, height / 2.0);
			cr->line_to((*control_value - min_value) * increment_size, height / 2.0);
			cr->stroke();
		}
	}
	void on_lmb_down(int n_press, double x, double y) {
		if (!is_valid()) { return; }
		set_new_value(min_value + ((x / get_width()) * (max_value - min_value)));
		draw_panel.queue_draw();
		lmb_down = true;
	}
	void on_lmb_up(int n_press, double x, double y) {
		lmb_down = false;
	}
	void on_mouse_motion(double x, double y) {
		if (!lmb_down) { return; }
		set_new_value(min_value + ((x / draw_panel.get_width()) * (max_value - min_value)));
		draw_panel.queue_draw();
	}
public:
	virtual void set_new_value(int new_value) {
		*control_value = clamp(new_value, int(min_value), int(max_value));
		queue_draw();
	}
	OPLFMSlider(uint8_t* p_base_value, uint8_t* p_control_value, uint8_t p_min_value = 0, uint8_t p_max_value = 100) :
				OPLFMPropertyControl(p_base_value, p_control_value), min_value(p_min_value), max_value(p_max_value) {
		using namespace Gtk;
		add_css_class("insmaker-slider");
		set_expand(true);
		draw_panel.set_draw_func(sigc::mem_fun(*this, &OPLFMSlider::on_draw));
		draw_panel.set_expand(true);
		
		lmb_gesture = GestureClick::create();
		lmb_gesture->set_button(GDK_BUTTON_PRIMARY);
		lmb_gesture->signal_pressed().connect(mem_fun(*this, &OPLFMSlider::on_lmb_down));
		lmb_gesture->signal_released().connect(mem_fun(*this, &OPLFMSlider::on_lmb_up));
		draw_panel.add_controller(lmb_gesture);
		
		auto motion_event = EventControllerMotion::create();
		motion_event->signal_motion().connect(mem_fun(*this, &OPLFMSlider::on_mouse_motion));
		draw_panel.add_controller(motion_event);
		
		Button* minus_button = Gtk::make_managed<Button>();
		Button* plus_button = Gtk::make_managed<Button>();
		Picture minus_pic = Gtk::Picture(ICON_PATH("MinusButton.svg"));
		minus_pic.set_size_request(-1, -1);
		minus_pic.set_can_shrink(true);
		minus_pic.set_content_fit(Gtk::ContentFit::SCALE_DOWN);
		minus_button->set_child(minus_pic);
		Picture plus_pic = Gtk::Picture(ICON_PATH("PlusButton.svg"));
		plus_pic.set_size_request(-1, -1);
		plus_pic.set_can_shrink(true);
		plus_pic.set_content_fit(Gtk::ContentFit::SCALE_DOWN);
		plus_button->set_child(plus_pic);
		
		append(*minus_button);
		append(*plus_button);
		append(draw_panel);
		
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
	void set_instrument(Instrument* p_new_instrument);
	void set_additive_synth(bool value);
	void save_current_instrument();
	void save_instruments();
private:
	Instrument* find_unsaved_instrument(char name[0]);
	int get_unsaved_count();
	void create_oplfm_editor(OPLFM* p_car, OPLFM* p_mod);
	void update_oplfm_editor(OPLFM* p_car, OPLFM* p_mod, OPLFM* n_car, OPLFM* n_mod);
	void update_oplfm_property(int idx, uint8_t* base_car_value_ptr, uint8_t* base_mod_value_ptr,
		uint8_t* new_car_value_ptr, uint8_t* new_mod_value_ptr);
	void add_slider_property(string name, uint8_t* p_car_value_ptr, uint8_t* p_mod_value_ptr,
		uint8_t p_min_value, uint8_t p_max_value);
	void add_checkbox_property(string name, uint8_t* p_car_value_ptr, uint8_t* p_mod_value_ptr);
	void on_value_event();
	int get_number_of_unsaved_changes();
	Instrument* instrument_ptr = nullptr;
	Instrument new_instrument;
	vector<unique_ptr<OPLFMPropertyControl>> carrier_properties;
	vector<unique_ptr<OPLFMPropertyControl>> modulator_properties;
	vector<Instrument> unsaved_instruments;
	
	unique_ptr<Gtk::Grid> property_grid;
	int number_of_unsaved_changes = 0;
};



