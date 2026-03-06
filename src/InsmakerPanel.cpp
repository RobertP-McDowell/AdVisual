#include <InsmakerPanel.h>

enum {
	ATTACK_RATE = 0,
	DECAY_RATE,
	SUSTAIN_LEVEL,
	RELEASE_RATE,
	SUSTAIN_SOUND,
	ENVELOPE_SCALING,
	FREQUENCY_MULTIPLIER,
	MODULATION_FEEDBACK,
	VIBRATO,
	OUTPUT_LEVEL,
	LEVEL_SCALING,
	TREMELO,
	WAVEFORM
};

void InsmakerPanel::create_oplfm_editor(OPLFM* car, OPLFM* mod) {
	add_slider_property("Attack Rate", &(car->attack_rate), &(mod->attack_rate), 0, 15);
	add_slider_property("Decay Rate", &(car->decay_rate), &(mod->decay_rate), 0, 15);
	add_slider_property("Sustain Level", &(car->sustain_level), &(mod->sustain_level), 0, 15);
	add_slider_property("Release Rate", &(car->release_rate), &(mod->release_rate), 0, 15);
	add_checkbox_property("Sustain Sound", &(car->sustain_sound), &(mod->sustain_sound));
	add_checkbox_property("Envelope Scaling", &(car->envelope_scaling), &(mod->envelope_scaling));
	add_slider_property("Frequency Multiplier", &(car->frequency_multiplier), &(mod->frequency_multiplier), 0, 15); // 0.5
	add_slider_property("Modulation Feedback", nullptr, &(mod->feedback), 0, 7);
	add_checkbox_property("Pitch Vibrato", &(car->vibrato), &(mod->vibrato));
	add_slider_property("Output Level", &(car->output_level), &(mod->output_level), 0, 63);
	add_slider_property("Level Scaling", &(car->level_scaling), &(mod->level_scaling), 0, 3);
	add_checkbox_property("Amplitude Tremelo", &(car->tremelo), &(mod->tremelo));
	add_slider_property("Wave Form", &(car->waveform), &(mod->waveform), 0, 3);
}

void InsmakerPanel::update_oplfm_property(int idx, uint8_t* car_value_ptr, uint8_t* mod_value_ptr) {
	carrier_properties.at(idx)->LoadFromValue(car_value_ptr);
	modulator_properties.at(idx)->LoadFromValue(mod_value_ptr);
}
void InsmakerPanel::update_oplfm_editor(OPLFM* p_car, OPLFM* p_mod) {
	update_oplfm_property(ATTACK_RATE, &(p_car->attack_rate), &(p_mod->attack_rate));
	update_oplfm_property(DECAY_RATE, &(p_car->decay_rate), &(p_mod->decay_rate));
	update_oplfm_property(SUSTAIN_LEVEL, &(p_car->sustain_level), &(p_mod->sustain_level));
	update_oplfm_property(RELEASE_RATE, &(p_car->release_rate), &(p_mod->release_rate));
	update_oplfm_property(SUSTAIN_SOUND, &(p_car->sustain_sound), &(p_mod->sustain_sound));
	update_oplfm_property(ENVELOPE_SCALING, &(p_car->envelope_scaling), &(p_mod->envelope_scaling));
	update_oplfm_property(FREQUENCY_MULTIPLIER, &(p_car->frequency_multiplier), &(p_mod->frequency_multiplier));
	update_oplfm_property(MODULATION_FEEDBACK, &(p_car->feedback), &(p_mod->feedback));
	update_oplfm_property(VIBRATO, &(p_car->vibrato), &(p_mod->vibrato));
	update_oplfm_property(OUTPUT_LEVEL, &(p_car->output_level), &(p_mod->output_level));
	update_oplfm_property(LEVEL_SCALING, &(p_car->level_scaling), &(p_mod->level_scaling));
	update_oplfm_property(TREMELO, &(p_car->tremelo), &(p_mod->tremelo));
	update_oplfm_property(WAVEFORM, &(p_car->waveform), &(p_mod->waveform));
}

InsmakerPanel::InsmakerPanel(wxWindow* parent, int id) : wxScrolledWindow(parent, id) {
	//SetBackgroundColour(*wxBLACK);
	current_bank = make_unique<Bank>();
	current_instrument = new Instrument;
	property_sizer = new wxFlexGridSizer(3, 10, 10);
	property_sizer->AddGrowableCol(0, 1);
	property_sizer->AddGrowableCol(1, 3);
	property_sizer->AddGrowableCol(2, 3);
	property_sizer->SetFlexibleDirection(wxALL);

	create_oplfm_editor(&(current_instrument->carrier), &(current_instrument->modulator));

	SetSizerAndFit(property_sizer);
}

void InsmakerPanel::SaveChanges() {
	
}

void InsmakerPanel::add_slider_property(string name, uint8_t* p_car_value_ptr, uint8_t* p_mod_value_ptr,
		uint8_t p_min_value, uint8_t p_max_value) {
	// Property name:
	wxStaticText* property_name = new wxStaticText(this, wxID_ANY, name);
	property_name->SetWindowStyle(wxALIGN_LEFT);
	property_sizer->Add(property_name, 0, wxALIGN_LEFT);
	// Carrier:
	if (p_car_value_ptr != nullptr) {
		SpinBoxSlider* slider = new SpinBoxSlider(this, wxID_ANY, p_car_value_ptr, p_min_value, p_max_value);
		slider->Bind(EVT_SPIN_BOX_SLIDER, &InsmakerPanel::on_slider_event, this, wxID_ANY);
		property_sizer->Add(slider, 1, wxEXPAND);
		carrier_properties.push_back((slider));
	}
	else { // Add Spacer if property doesn't exist.
		property_sizer->Add(0, 0, 1, wxEXPAND);
	}
	// Modulator: // Same as last time.
	if (p_mod_value_ptr != nullptr) {
		SpinBoxSlider* slider = new SpinBoxSlider(this, wxID_ANY, p_mod_value_ptr, p_min_value, p_max_value);
		slider->Bind(EVT_SPIN_BOX_SLIDER, &InsmakerPanel::on_slider_event, this, wxID_ANY);
		property_sizer->Add(slider, 1, wxEXPAND);
		modulator_properties.push_back((slider));
	}
	else {
		property_sizer->Add(0, 0, 1, wxEXPAND);
	}
}

void InsmakerPanel::add_checkbox_property(string name, uint8_t* p_car_value_ptr, uint8_t* p_mod_value_ptr) {
	// Property name:
	wxStaticText* property_name = new wxStaticText(this, wxID_ANY, name);
	property_name->SetWindowStyle(wxALIGN_LEFT);
	property_sizer->Add(property_name, 0, wxALIGN_LEFT);

	OPLFMCheckbox* car_checkbox = new OPLFMCheckbox(this, wxID_ANY, p_car_value_ptr);
	car_checkbox->Bind(wxEVT_CHECKBOX, &InsmakerPanel::on_checkbox_event, this, wxID_ANY);
	property_sizer->Add(car_checkbox, 0, wxALIGN_CENTER);
	carrier_properties.push_back(car_checkbox);

	OPLFMCheckbox* mod_checkbox = new OPLFMCheckbox(this, wxID_ANY, p_mod_value_ptr);
	mod_checkbox->Bind(wxEVT_CHECKBOX, &InsmakerPanel::on_checkbox_event, this, wxID_ANY);
	property_sizer->Add(mod_checkbox, 0, wxALIGN_CENTER);
	modulator_properties.push_back(mod_checkbox);
}

void InsmakerPanel::on_slider_event(wxCommandEvent& event) {
	number_of_unsaved_changes += event.GetInt();
}
void InsmakerPanel::on_checkbox_event(wxCommandEvent& event) {
	cout << event.IsChecked() << "\n";
}

wxDEFINE_EVENT(EVT_SPIN_BOX_SLIDER, wxCommandEvent);

SpinBoxSlider::SpinBoxSlider(wxWindow* parent, int id, uint8_t* p_value_ptr, uint8_t p_min_value, uint8_t p_max_value) :
		OPLFMPropertyControl(parent, id, p_value_ptr), min_value(p_min_value), max_value(p_max_value) {
	Init();
}


void SpinBoxSlider::SetControlValue(int new_control_value) {
	uint8_t new_value = (new_control_value < 0 ? uint8_t(0) : uint8_t(new_control_value)); // First we need to check that this uint isn't negative.
	new_value = clamp(new_value, min_value, max_value);
	wxCommandEvent event(EVT_SPIN_BOX_SLIDER, GetId());
	event.SetInt(0); // If event int is 0, no change to 'HasUnsavedChange' has happened, i;e value still = / still != slider_value.
	if (control_value != new_value) {
		if (control_value != *value_ptr && new_value == *value_ptr) {
			event.SetInt(-1); // -1 = SpinBox had unsaved value, but now equals its original value therefore it is saved.
		}
		if (control_value == *value_ptr && new_value != *value_ptr) {
			event.SetInt(1); // 1 = SpinBox had saved value, but is now unsaved.
		}
	}
	control_value = new_value;
	ProcessWindowEvent(event);
	Refresh();
	Update();
}
