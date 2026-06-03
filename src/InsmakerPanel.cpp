#include <InsmakerPanel.h>

using namespace Gtk;

vector<Instrument> InsmakerPanel::unsaved_instruments;

enum {
	ATTACK_RATE = 0,
	DECAY_RATE,
	SUSTAIN_LEVEL,
	RELEASE_RATE,
	SUSTAIN_SOUND,
	KEY_SCALING_RATE,
	FREQUENCY_MULTIPLIER,
	MODULATION_FEEDBACK,
	VIBRATO,
	OUTPUT_LEVEL,
	KEY_SCALING_LEVEL,
	TREMELO,
	WAVEFORM
};


#define get_opl_prop(oplfm_ptr, property_name) (oplfm_ptr != nullptr ? &(oplfm_ptr->property_name) : nullptr)
#define upd_prop_macro(idx, base_car, base_mod, new_car, new_mod, property_name) update_oplfm_property(idx, \
get_opl_prop(base_car, property_name), get_opl_prop(base_mod, property_name), \
get_opl_prop(new_car, property_name), get_opl_prop(new_mod, property_name)) \

int current_property_row = 0;

void InsmakerPanel::create_oplfm_editor() {
	add_slider_property("Attack Rate", 0, 15);
	add_slider_property("Decay Rate", 0, 15);
	add_slider_property("Sustain Level", 0, 15, true); // Inverted.
	add_slider_property("Release Rate", 0, 15);
	add_checkbox_property("Sustain Sound");
	add_checkbox_property("Envelope Scaling");
	add_slider_property("Frequency Multiplier", 0, 15); // 0.5
	add_slider_property("Modulation Feedback", 0, 7);
	add_checkbox_property("Pitch Vibrato");
	add_slider_property("Output Level", 0, 63, true); // Inverted.
	add_slider_property("Level Scaling", 0, 3);
	add_checkbox_property("Amplitude Tremelo");
	add_radio_property("Wave Form");
}

void InsmakerPanel::update_oplfm_property(int idx, uint8_t* car_value_ptr, uint8_t* mod_value_ptr,
		uint8_t* new_car_value_ptr, uint8_t* new_mod_value_ptr) {
	carrier_properties.at(idx)->load_from_value(car_value_ptr, new_car_value_ptr);
	modulator_properties.at(idx)->load_from_value(mod_value_ptr, new_mod_value_ptr);
}

void InsmakerPanel::update_oplfm_editor(OPLFM* car, OPLFM* mod, OPLFM* ncar, OPLFM* nmod) {
	OPLFM* sudo_nullptr = nullptr; // Used for macro.
	upd_prop_macro(ATTACK_RATE,          car, mod, ncar, nmod, attack_rate);
	upd_prop_macro(DECAY_RATE,           car, mod, ncar, nmod, decay_rate);
	upd_prop_macro(SUSTAIN_LEVEL,        car, mod, ncar, nmod, sustain_level);
	upd_prop_macro(RELEASE_RATE,         car, mod, ncar, nmod, release_rate);
	upd_prop_macro(SUSTAIN_SOUND,        car, mod, ncar, nmod, sustain_sound);
	upd_prop_macro(KEY_SCALING_RATE,     car, mod, ncar, nmod, ksr);
	upd_prop_macro(FREQUENCY_MULTIPLIER, car, mod, ncar, nmod, frequency_multiplier);
	upd_prop_macro(MODULATION_FEEDBACK,  sudo_nullptr, mod, sudo_nullptr, nmod, feedback);
	upd_prop_macro(VIBRATO,              car, mod, ncar, nmod, vibrato);
	upd_prop_macro(OUTPUT_LEVEL,         car, mod, ncar, nmod, output_level);
	upd_prop_macro(KEY_SCALING_LEVEL,    car, mod, ncar, nmod, ksl);
	upd_prop_macro(TREMELO,              car, mod, ncar, nmod, tremelo);
	upd_prop_macro(WAVEFORM,             car, mod, ncar, nmod, waveform);
}

void InsmakerPanel::add_checkbox_property(string name) {
	// Property name.
	Label* property_label = make_managed<Label>(name);
	property_label->set_vexpand(true);
	property_grid->attach(*property_label, 0, current_property_row);
	// Carrier.
	unique_ptr<OPLFMCheckbox> car_checkbox = make_unique<OPLFMCheckbox>(nullptr, nullptr);
	property_grid->attach(*car_checkbox, 1, current_property_row);
	carrier_properties.push_back(move(car_checkbox));
	// Modulator.
	unique_ptr<OPLFMCheckbox> mod_checkbox = make_unique<OPLFMCheckbox>(nullptr, nullptr);
	property_grid->attach(*mod_checkbox, 2, current_property_row);
	modulator_properties.push_back(move(mod_checkbox));

	current_property_row++;
}

void InsmakerPanel::add_radio_property(string name) {
	// Property name.
	auto property_label = make_managed<Label>(name);
	property_label->set_vexpand(true);
	property_grid->attach(*property_label, 0, current_property_row);
	// Carrier.
	unique_ptr<OPLFMRadio> car_slider = make_unique<OPLFMRadio>(nullptr, nullptr);
	property_grid->attach(*car_slider, 1, current_property_row);
	carrier_properties.push_back(move(car_slider));
	// Modulator.
	unique_ptr<OPLFMRadio> mod_slider = make_unique<OPLFMRadio>(nullptr, nullptr);
	property_grid->attach(*mod_slider, 2, current_property_row);
	modulator_properties.push_back(move(mod_slider));

	current_property_row++;
}

void InsmakerPanel::add_slider_property(string name, uint8_t p_min_value, uint8_t p_max_value, bool p_inverted) {
	// Property name.
	auto property_label = make_managed<Label>(name);
	property_label->set_vexpand(true);
	property_grid->attach(*property_label, 0, current_property_row);
	// Carrier.
	unique_ptr<OPLFMSlider> car_slider = make_unique<OPLFMSlider>(nullptr, nullptr, p_min_value, p_max_value, p_inverted);
	property_grid->attach(*car_slider, 1, current_property_row);
	carrier_properties.push_back(move(car_slider));
	// Modulator.
	unique_ptr<OPLFMSlider> mod_slider = make_unique<OPLFMSlider>(nullptr, nullptr, p_min_value, p_max_value, p_inverted);
	property_grid->attach(*mod_slider, 2, current_property_row);
	modulator_properties.push_back(move(mod_slider));

	current_property_row++;
}

InsmakerPanel::InsmakerPanel() : Box(Orientation::VERTICAL) {
	current_bank = new Bank();
	set_name("insmaker");
	set_expand(true);
	property_grid = make_unique<Grid>();
	for (int i = 0; i < 3; i++) {
		property_grid->insert_column(0);
	}
	for (int i = 0; i < 10; i++) {
		property_grid->insert_row(0);
	}
	instrument_ptr = nullptr;
	new_instrument = Instrument::default_instrument;
	
	property_grid->set_row_spacing(8);
	property_grid->set_column_spacing(8);
	property_grid->set_expand(true);
	property_grid->set_row_homogeneous(true);
	append(*property_grid);
// Property, Sliders and Checkboxes init.
	Label* car_label = make_managed<Label>("Carrier");
	car_label->set_expand(true);
	car_label->set_halign(Align::CENTER);
	property_grid->attach(*car_label, 1, 0);
	Label* mod_label = make_managed<Label>("Modulator");
	mod_label->set_expand(true);
	mod_label->set_halign(Align::CENTER);
	property_grid->attach(*mod_label, 2, 0);
	current_property_row += 1;
	
	create_oplfm_editor();
	update_oplfm_editor(nullptr, nullptr, nullptr, nullptr);
// PianoCtrl init.
	piano_ctrl = make_managed<PianoCtrl>(false);
	append(*piano_ctrl);
// Scrollbar init.
	shared_ptr<Adjustment> hadjust = Adjustment::create(0, 0, pitch_range, 1, 10, 10);
	hscrollbar = make_managed<Scrollbar>(hadjust, Orientation::HORIZONTAL);
	hadjust->signal_value_changed().connect(mem_fun(*this, &InsmakerPanel::on_scroll_piano));
	hadjust->set_value(middle_c - ((1024 / piano_ctrl->get_key_width()) / 2.0)); // Go to center of piano.
	hadjust->set_page_size(1024 / piano_ctrl->get_key_width());
	append(*hscrollbar);

	scroll_controller = EventControllerScroll::create();
	scroll_controller->set_flags(EventControllerScroll::Flags::VERTICAL);
	scroll_controller->signal_scroll().connect(mem_fun(*this, &InsmakerPanel::on_mouse_scroll_piano), true);
	piano_ctrl->add_controller(scroll_controller); // the piano_ctrl owns the scroll_controller.
	hscrollbar->get_adjustment()->set_page_size(piano_ctrl->get_width() / piano_ctrl->get_key_width());
// Setup actions.
	action_group = Gio::SimpleActionGroup::create();
	action_group->add_action_bool("toggle_additive_synth", mem_fun(*this, &InsmakerPanel::toggle_additive_synth), false);
	action_group->add_action_radio_integer("set_rhythm_mode", mem_fun(*this, &InsmakerPanel::set_rhythm_mode), 0);
}

void InsmakerPanel::on_scroll_piano() {
	piano_ctrl->set_scroll_offset(hscrollbar->get_adjustment()->get_value() * piano_ctrl->get_key_width());
	hscrollbar->get_adjustment()->set_page_size(piano_ctrl->get_width() / piano_ctrl->get_key_width());
}
bool InsmakerPanel::on_mouse_scroll_piano(double x, double y) {
	hscrollbar->get_adjustment()->set_value(hscrollbar->get_adjustment()->get_value() + y);
	return true;
}

int InsmakerPanel::get_number_of_unsaved_changes() const {
	int ret = 0;
	for (auto&& prop : carrier_properties) {
		ret += (prop->has_unsaved_change() ? 1 : 0);
	}
	for (auto&& prop : modulator_properties) {
		ret += (prop->has_unsaved_change() ? 1 : 0);
	}
	return ret;
}

void InsmakerPanel::toggle_additive_synth() {
	new_instrument.modulator.additive_synth = uint8_t(!bool(new_instrument.modulator.additive_synth));
	action_group->change_action_state("toggle_additive_synth",
				Glib::Variant<bool>::create(new_instrument.modulator.additive_synth));
}

void InsmakerPanel::set_rhythm_mode(int value) {
	if (value >= 6) {
		new_instrument.voice_number = value;
		new_instrument.percussion_mode = 1;
	}
	else {
		new_instrument.voice_number = 0; 
		new_instrument.percussion_mode = 0;
	}
	if (instrument_ptr) {
		if (new_instrument.percussion_mode == 0 || new_instrument.voice_number == 6) {
			update_oplfm_editor(&(instrument_ptr->carrier), &(instrument_ptr->modulator),
				&(new_instrument.carrier), &(new_instrument.modulator));
		}
		else {
			update_oplfm_editor(nullptr, &(instrument_ptr->modulator), nullptr, &(new_instrument.modulator));
		}
	}
	action_group->change_action_state("set_rhythm_mode", Glib::Variant<int>::create(new_instrument.voice_number));
}

void InsmakerPanel::rename_instrument(const char new_name[9]) {
	if (instrument_ptr == nullptr) {
		cerr << "Can't rename instrument.\n";
		return;
	}
	if (current_bank->find_instrument(new_name)) {
		cerr << "The name '" << new_name << "' is already taken.\n";
		return;
	}
	char old_name[9];
	memcpy(old_name, instrument_ptr->name, 9);
	memcpy(new_instrument.name, new_name, 9);
	current_bank->delete_instrument(old_name); // We need to delete and add to reorder it in the bank.
	current_bank->add_instrument(new_instrument);
	instrument_ptr = current_bank->find_instrument(new_name);
	Instrument* unsaved_ins = find_unsaved_instrument(old_name);
	if (unsaved_ins) {
		memcpy(unsaved_ins->name, new_name, 9);
	}
}

Instrument* InsmakerPanel::find_unsaved_instrument(const char name[9]) {
	for (Instrument& ins : unsaved_instruments) {
		if (strcmp(ins.name, name) == 0) {
			return &ins;
		}
	}
	return nullptr;
}

void InsmakerPanel::set_instrument(Instrument* p_new_instrument)  {
	if (current_bank) {
		if (get_number_of_unsaved_changes() != 0) {
			Instrument* unsaved_instrument = find_unsaved_instrument(instrument_ptr->name);
			if (unsaved_instrument != nullptr) {
				// If we alread have an unsaved instrument in the buffer, update that directly!
				*unsaved_instrument = new_instrument;
			}
			else {
				unsaved_instruments.push_back(new_instrument);
			}
		}
		instrument_ptr = p_new_instrument;
		if (instrument_ptr == nullptr) {
			update_oplfm_editor(nullptr, nullptr, nullptr, nullptr);
			number_of_unsaved_changes = 0;
			piano_ctrl->set_instrument(&(Instrument::default_instrument));
			return;
		}
		Instrument* unsaved_instrument = find_unsaved_instrument(instrument_ptr->name);
		if (unsaved_instrument != nullptr) { // Check if we were already editing this instrument with unsaved changes.
			new_instrument = *unsaved_instrument;
		}
		else { // If not just copy instrument_ptr.
			new_instrument = *instrument_ptr;
		}
		if (new_instrument.percussion_mode == 0 || new_instrument.voice_number == 6) {
			update_oplfm_editor(&(instrument_ptr->carrier), &(instrument_ptr->modulator),
				&(new_instrument.carrier), &(new_instrument.modulator));
		}
		else {
			update_oplfm_editor(nullptr, &(instrument_ptr->modulator), nullptr, &(new_instrument.modulator));
		}
		action_group->change_action_state("set_rhythm_mode", Glib::Variant<int>::create(new_instrument.voice_number));
		action_group->change_action_state("toggle_additive_synth", Glib::Variant<bool>::create(new_instrument.modulator.additive_synth));
		number_of_unsaved_changes = get_number_of_unsaved_changes(); // Might not be 0 if using unsaved instrument.
		piano_ctrl->set_instrument(&new_instrument);
		queue_draw();
	}
}

void InsmakerPanel::save_current_instrument() {
	if (instrument_ptr != nullptr) {
		*instrument_ptr = new_instrument;
	}
	update_oplfm_editor(&instrument_ptr->carrier, &instrument_ptr->modulator, &new_instrument.carrier, &new_instrument.modulator);
}

void InsmakerPanel::save_instruments() {
	save_current_instrument();
	for (Instrument& ins : unsaved_instruments) {
		Instrument* base_ins = current_bank->find_instrument(ins.name);
		if (base_ins == nullptr) {
			cerr << "Orphan instrument with name '" << ins.name << "' is not going to be saved\n";
			continue;
		}
		*base_ins = ins;
	}
	unsaved_instruments.clear();
}

void InsmakerPanel::on_volume_changed(shared_ptr<Adjustment> volume_adjust) {
	piano_ctrl->set_volume_multiplier(volume_adjust->get_value());
}