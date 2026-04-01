#include <InsmakerPanel.h>

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

void InsmakerPanel::create_oplfm_editor(OPLFM* car, OPLFM* mod) {
	add_slider_property("Attack Rate", get_opl_prop(car, attack_rate), get_opl_prop(mod, attack_rate), 0, 15);
	add_slider_property("Decay Rate", get_opl_prop(car, decay_rate), get_opl_prop(mod, decay_rate), 0, 15);
	add_slider_property("Sustain Level", get_opl_prop(car, sustain_level), get_opl_prop(mod, sustain_level), 0, 15);
	add_slider_property("Release Rate", get_opl_prop(car, release_rate), get_opl_prop(mod, release_rate), 0, 15);
	add_checkbox_property("Sustain Sound", get_opl_prop(car, sustain_sound), get_opl_prop(mod, sustain_sound));
	add_checkbox_property("Envelope Scaling", get_opl_prop(car, ksr), get_opl_prop(mod, ksr));
	add_slider_property("Frequency Multiplier", get_opl_prop(car, frequency_multiplier), get_opl_prop(mod, frequency_multiplier), 0, 15); // 0.5
	add_slider_property("Modulation Feedback", nullptr, get_opl_prop(mod, feedback), 0, 7);
	add_checkbox_property("Pitch Vibrato", get_opl_prop(car, vibrato), get_opl_prop(mod, vibrato));
	add_slider_property("Output Level", get_opl_prop(car, output_level), get_opl_prop(mod, output_level), 0, 63);
	add_slider_property("Level Scaling", get_opl_prop(car, ksl), get_opl_prop(mod, ksl), 0, 3);
	add_checkbox_property("Amplitude Tremelo", get_opl_prop(car, tremelo), get_opl_prop(mod, tremelo));
	add_slider_property("Wave Form", get_opl_prop(car, waveform), get_opl_prop(mod, waveform), 0, 3);
}

void InsmakerPanel::update_oplfm_property(int idx, uint8_t* car_value_ptr, uint8_t* mod_value_ptr,
		uint8_t* new_car_value_ptr, uint8_t* new_mod_value_ptr) {
	carrier_properties.at(idx)->LoadFromValue(car_value_ptr, new_car_value_ptr);
	modulator_properties.at(idx)->LoadFromValue(mod_value_ptr, new_mod_value_ptr);
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

InsmakerPanel::InsmakerPanel(wxWindow* parent, int id) : wxScrolledWindow(parent, id) {
	current_bank = make_unique<Bank>();
	instrument_ptr = nullptr;
	unsaved_instruments = {};
	property_sizer = new wxFlexGridSizer(3, 10, 10);
	property_sizer->AddGrowableCol(0, 1);
	property_sizer->AddGrowableCol(1, 3);
	property_sizer->AddGrowableCol(2, 3);
	property_sizer->SetFlexibleDirection(wxALL);
	property_sizer->AddSpacer(0);
	wxStaticText* carrier_text = new wxStaticText(this, wxID_ANY, "Carrier");
	property_sizer->Add(carrier_text, 1, wxALIGN_CENTER);
	wxStaticText* modulator_text = new wxStaticText(this, wxID_ANY, "Modulator");
	property_sizer->Add(modulator_text, 1, wxALIGN_CENTER);

	create_oplfm_editor(nullptr, nullptr);

	// Create Piano and ScrollBar.
	piano_ctrl = new PianoControl(this, wxID_ANY, wxVERTICAL);
	piano_ctrl->Bind(wxEVT_SIZE, &InsmakerPanel::on_resize_piano, this);
	h_scroll_bar = new wxScrollBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_HORIZONTAL);
	int scroll_thumb_size = (piano_ctrl->GetSize().x / piano_ctrl->GetKeyWidth());
	h_scroll_bar->SetScrollbar(middle_c, scroll_thumb_size, pitch_range, scroll_thumb_size);
	h_scroll_bar->Bind(wxEVT_SCROLL_THUMBTRACK, &InsmakerPanel::on_scroll_piano, this);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(property_sizer, 1, wxEXPAND);
	sizer->Add(piano_ctrl, 0, wxEXPAND | wxUP, 2);
	sizer->Add(h_scroll_bar, 0, wxEXPAND);
	SetSizerAndFit(sizer);
}

int InsmakerPanel::get_number_of_unsaved_changes() {
	int ret = 0;
	for (OPLFMPropertyControl*& prop : carrier_properties) {
		ret += (prop->HasUnsavedChange() ? 1 : 0);
	}
	for (OPLFMPropertyControl*& prop : modulator_properties) {
		ret += (prop->HasUnsavedChange() ? 1 : 0);
	}
	return ret;
}
Instrument* InsmakerPanel::find_unsaved_instrument(char name[9]) {
	for (Instrument& ins : unsaved_instruments) {
		if (strcmp(ins.name, name) == 0) {
			return &ins;
		}
	}
	return nullptr;
}

void InsmakerPanel::SetInstrumentByName(char name[9])  {
	if (current_bank) {
		if (number_of_unsaved_changes != 0) {
			Instrument* unsaved_instrument = find_unsaved_instrument(instrument_ptr->name);
			if (unsaved_instrument != nullptr) {
				// If we alread have an unsaved instrument in the buffer, update that directly!
				*unsaved_instrument = new_instrument;
			}
			else {
				unsaved_instruments.push_back(new_instrument);
			}
		}
		instrument_ptr = current_bank->find_instrument(name);
		if (instrument_ptr == &(Instrument::default_instrument)) {
			DBPRINT("Instrument of name " << name << " Could not be found.");
			instrument_ptr = nullptr;
			update_oplfm_editor(nullptr, nullptr, nullptr, nullptr);
			number_of_unsaved_changes = 0;
			piano_ctrl->SetInstrument(&(Instrument::default_instrument));
			return;
		}
		Instrument* unsaved_instrument = find_unsaved_instrument(name);
		if (unsaved_instrument != nullptr) { // Check if we were already editing this instrument with unsaved changes.
			new_instrument = *unsaved_instrument;
		}
		else {
			new_instrument = *instrument_ptr;
		}
		new_additive_synth = new_instrument.additive_synth;
		new_percussion_mode = new_instrument.percussion_mode;
		if (new_instrument.percussion_mode == 0) {
			update_oplfm_editor(&(instrument_ptr->carrier), &(instrument_ptr->modulator),
				&(new_instrument.carrier), &(new_instrument.modulator));
		}
		else {
			update_oplfm_editor(nullptr, &(instrument_ptr->modulator), nullptr, &(new_instrument.modulator));
		}
		number_of_unsaved_changes = get_number_of_unsaved_changes(); // Might not be 0 if using unsaved instrument.
		piano_ctrl->SetInstrument(&new_instrument);
	}
}

void InsmakerPanel::SetAdditiveSynth(uint8_t value) {
	if (new_additive_synth == value) {
		return;
	}
	new_additive_synth = value;
	number_of_unsaved_changes += (new_additive_synth != new_instrument.additive_synth ? 1 : -1);
}

void InsmakerPanel::SetPercussionMode(uint8_t value) {
	if (new_percussion_mode == value) {
		return;
	}
	new_percussion_mode = value;
	number_of_unsaved_changes += (new_percussion_mode != new_instrument.percussion_mode ? 1 : -1);
}

void InsmakerPanel::add_slider_property(string name, uint8_t* p_car_value_ptr, uint8_t* p_mod_value_ptr,
		uint8_t p_min_value, uint8_t p_max_value) {
	// Property name.
	wxStaticText* property_name = new wxStaticText(this, wxID_ANY, name);
	property_name->SetWindowStyle(wxALIGN_LEFT);
	property_sizer->Add(property_name, 0, wxALIGN_LEFT);
	// Carrier.
	SpinBoxSlider* car_slider = new SpinBoxSlider(this, wxID_ANY, p_car_value_ptr, nullptr, p_min_value, p_max_value);
	car_slider->Bind(EVT_SPIN_BOX_SLIDER, &InsmakerPanel::on_slider_event, this, wxID_ANY);
	property_sizer->Add(car_slider, 1, wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN);
	carrier_properties.push_back(car_slider);
	// Modulator.
	SpinBoxSlider* mod_slider = new SpinBoxSlider(this, wxID_ANY, p_mod_value_ptr, nullptr, p_min_value, p_max_value);
	mod_slider->Bind(EVT_SPIN_BOX_SLIDER, &InsmakerPanel::on_slider_event, this, wxID_ANY);
	property_sizer->Add(mod_slider, 1, wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN);
	modulator_properties.push_back(mod_slider);
}

void InsmakerPanel::add_checkbox_property(string name, uint8_t* p_car_value_ptr, uint8_t* p_mod_value_ptr) {
	// Property name.
	wxStaticText* property_name = new wxStaticText(this, wxID_ANY, name);
	property_name->SetWindowStyle(wxALIGN_LEFT);
	property_sizer->Add(property_name, 0, wxALIGN_LEFT);
	// Carrier.
	OPLFMCheckbox* car_checkbox = new OPLFMCheckbox(this, wxID_ANY, p_car_value_ptr, nullptr);
	car_checkbox->Bind(wxEVT_CHECKBOX, &InsmakerPanel::on_checkbox_event, this, wxID_ANY);
	property_sizer->Add(car_checkbox, 0, wxALIGN_CENTER | wxRESERVE_SPACE_EVEN_IF_HIDDEN);
	carrier_properties.push_back(car_checkbox);
	// Modulator.
	OPLFMCheckbox* mod_checkbox = new OPLFMCheckbox(this, wxID_ANY, p_mod_value_ptr, nullptr);
	mod_checkbox->Bind(wxEVT_CHECKBOX, &InsmakerPanel::on_checkbox_event, this, wxID_ANY);
	property_sizer->Add(mod_checkbox, 0, wxALIGN_CENTER | wxRESERVE_SPACE_EVEN_IF_HIDDEN);
	modulator_properties.push_back(mod_checkbox);
}

void InsmakerPanel::save_properties_to_opl() {
	for (OPLFMPropertyControl*& prop_ctrl : carrier_properties) {
		prop_ctrl->SaveCurrentValue();
	}
	for (OPLFMPropertyControl*& prop_ctrl : modulator_properties) {
		prop_ctrl->SaveCurrentValue();
	}
	new_instrument.additive_synth = new_additive_synth;
	new_instrument.percussion_mode = new_percussion_mode;
	unsaved_instruments.clear();
}

void InsmakerPanel::on_slider_event(wxCommandEvent& event) {
	number_of_unsaved_changes += event.GetInt();
}
void InsmakerPanel::on_checkbox_event(wxCommandEvent& event) {
	cout << event.IsChecked() << "\n";
}

void InsmakerPanel::on_resize_piano(wxSizeEvent& event) {
	int scroll_thumb_size = (piano_ctrl->GetSize().x / piano_ctrl->GetKeyWidth());
	h_scroll_bar->SetScrollbar(piano_ctrl->GetScrollOffset() / piano_ctrl->GetKeyWidth(), scroll_thumb_size, pitch_range, scroll_thumb_size);
	event.Skip();
}

void InsmakerPanel::on_scroll_piano(wxScrollEvent& event) {
	piano_ctrl->SetScrollOffset(piano_ctrl->GetKeyWidth() * event.GetPosition());
}

wxDEFINE_EVENT(EVT_SPIN_BOX_SLIDER, wxCommandEvent);

SpinBoxSlider::SpinBoxSlider(wxWindow* parent, int id, uint8_t* p_value_ptr, uint8_t* p_new_value_ptr, uint8_t p_min_value, uint8_t p_max_value) :
		OPLFMPropertyControl(parent, id, p_value_ptr, p_new_value_ptr), min_value(p_min_value), max_value(p_max_value) {
	Init();
}

void SpinBoxSlider::SetNewValue(uint8_t p_new_value) {
	uint8_t new_value = (p_new_value < 0 ? uint8_t(0) : uint8_t(p_new_value)); // First we need to check that this uint isn't negative.
	new_value = clamp(new_value, min_value, max_value);
	wxCommandEvent event(EVT_SPIN_BOX_SLIDER, GetId());
	event.SetInt(0); // If event int is 0, no change to 'HasUnsavedChange' has happened, i;e value still = / still != slider_value.
	if (*new_value_ptr != new_value) {
		if (*new_value_ptr != *value_ptr && new_value == *value_ptr) {
			event.SetInt(-1); // -1 = SpinBox had unsaved value, but now equals its original value therefore it is saved.
		}
		if (*new_value_ptr == *value_ptr && new_value != *value_ptr) {
			event.SetInt(1); // 1 = SpinBox had saved value, but is now unsaved.
		}
	}
	*new_value_ptr = new_value;
	ProcessWindowEvent(event);
	Refresh();
	Update();
}
