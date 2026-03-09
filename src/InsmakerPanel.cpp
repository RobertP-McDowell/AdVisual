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

#define get_opl_prop(oplfm_ptr, property_name) (oplfm_ptr != nullptr ? &(oplfm_ptr->property_name) : nullptr)

void InsmakerPanel::create_oplfm_editor(OPLFM* car, OPLFM* mod) {
	add_slider_property("Attack Rate", get_opl_prop(car, attack_rate), get_opl_prop(mod, attack_rate), 0, 15);
	add_slider_property("Decay Rate", get_opl_prop(car, decay_rate), get_opl_prop(mod, decay_rate), 0, 15);
	add_slider_property("Sustain Level", get_opl_prop(car, sustain_level), get_opl_prop(mod, sustain_level), 0, 15);
	add_slider_property("Release Rate", get_opl_prop(car,release_rate), get_opl_prop(mod, release_rate), 0, 15);
	add_checkbox_property("Sustain Sound", get_opl_prop(car, sustain_sound), get_opl_prop(mod, sustain_sound));
	add_checkbox_property("Envelope Scaling", get_opl_prop(car,envelope_scaling), get_opl_prop(mod,envelope_scaling));
	add_slider_property("Frequency Multiplier", get_opl_prop(car, frequency_multiplier), get_opl_prop(mod, frequency_multiplier), 0, 15); // 0.5
	add_slider_property("Modulation Feedback", nullptr, get_opl_prop(mod, feedback), 0, 7);
	add_checkbox_property("Pitch Vibrato", get_opl_prop(car, vibrato), get_opl_prop(mod, vibrato));
	add_slider_property("Output Level", get_opl_prop(car, output_level), get_opl_prop(mod, output_level), 0, 63);
	add_slider_property("Level Scaling", get_opl_prop(car, level_scaling), get_opl_prop(mod, level_scaling), 0, 3);
	add_checkbox_property("Amplitude Tremelo", get_opl_prop(car, tremelo), get_opl_prop(mod, tremelo));
	add_slider_property("Wave Form", get_opl_prop(car, waveform), get_opl_prop(mod, waveform), 0, 3);
}

void InsmakerPanel::update_oplfm_property(int idx, uint8_t* car_value_ptr, uint8_t* mod_value_ptr) {
	carrier_properties.at(idx)->LoadFromValue(car_value_ptr);
	modulator_properties.at(idx)->LoadFromValue(mod_value_ptr);
}
void InsmakerPanel::update_oplfm_editor(OPLFM* car, OPLFM* mod) {
	update_oplfm_property(ATTACK_RATE, get_opl_prop(car, attack_rate), get_opl_prop(mod, attack_rate));
	update_oplfm_property(DECAY_RATE, get_opl_prop(car, decay_rate), get_opl_prop(mod, decay_rate));
	update_oplfm_property(SUSTAIN_LEVEL, get_opl_prop(car, sustain_level), get_opl_prop(mod, sustain_level));
	update_oplfm_property(RELEASE_RATE, get_opl_prop(car,release_rate), get_opl_prop(mod, release_rate));
	update_oplfm_property(SUSTAIN_SOUND, get_opl_prop(car, sustain_sound), get_opl_prop(mod, sustain_sound));
	update_oplfm_property(ENVELOPE_SCALING, get_opl_prop(car,envelope_scaling), get_opl_prop(mod,envelope_scaling));
	update_oplfm_property(FREQUENCY_MULTIPLIER, get_opl_prop(car, frequency_multiplier), get_opl_prop(mod, frequency_multiplier));
	update_oplfm_property(MODULATION_FEEDBACK, nullptr, get_opl_prop(mod, feedback));
	update_oplfm_property(VIBRATO, get_opl_prop(car, vibrato), get_opl_prop(mod, vibrato));
	update_oplfm_property(OUTPUT_LEVEL, get_opl_prop(car, output_level), get_opl_prop(mod, output_level));
	update_oplfm_property(LEVEL_SCALING, get_opl_prop(car, level_scaling), get_opl_prop(mod, level_scaling));
	update_oplfm_property(TREMELO, get_opl_prop(car, tremelo), get_opl_prop(mod, tremelo));
	update_oplfm_property(WAVEFORM, get_opl_prop(car, waveform), get_opl_prop(mod, waveform));
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
	property_sizer->AddSpacer(0);
	wxStaticText* carrier_text = new wxStaticText(this, wxID_ANY, "Carrier");
	property_sizer->Add(carrier_text, 1, wxALIGN_CENTER);
	wxStaticText* modulator_text = new wxStaticText(this, wxID_ANY, "Modulator");
	property_sizer->Add(modulator_text, 1, wxALIGN_CENTER);

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
	SpinBoxSlider* car_slider = new SpinBoxSlider(this, wxID_ANY, p_car_value_ptr, p_min_value, p_max_value);
	car_slider->Bind(EVT_SPIN_BOX_SLIDER, &InsmakerPanel::on_slider_event, this, wxID_ANY);
	property_sizer->Add(car_slider, 1, wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN);
	carrier_properties.push_back(car_slider);
	// Modulator: // Same as last time.
	SpinBoxSlider* mod_slider = new SpinBoxSlider(this, wxID_ANY, p_mod_value_ptr, p_min_value, p_max_value);
	mod_slider->Bind(EVT_SPIN_BOX_SLIDER, &InsmakerPanel::on_slider_event, this, wxID_ANY);
	property_sizer->Add(mod_slider, 1, wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN);
	modulator_properties.push_back(mod_slider);
}

void InsmakerPanel::add_checkbox_property(string name, uint8_t* p_car_value_ptr, uint8_t* p_mod_value_ptr) {
	// Property name:
	wxStaticText* property_name = new wxStaticText(this, wxID_ANY, name);
	property_name->SetWindowStyle(wxALIGN_LEFT);
	property_sizer->Add(property_name, 0, wxALIGN_LEFT);

	OPLFMCheckbox* car_checkbox = new OPLFMCheckbox(this, wxID_ANY, p_car_value_ptr);
	car_checkbox->Bind(wxEVT_CHECKBOX, &InsmakerPanel::on_checkbox_event, this, wxID_ANY);
	property_sizer->Add(car_checkbox, 0, wxALIGN_CENTER | wxRESERVE_SPACE_EVEN_IF_HIDDEN);
	carrier_properties.push_back(car_checkbox);

	OPLFMCheckbox* mod_checkbox = new OPLFMCheckbox(this, wxID_ANY, p_mod_value_ptr);
	mod_checkbox->Bind(wxEVT_CHECKBOX, &InsmakerPanel::on_checkbox_event, this, wxID_ANY);
	property_sizer->Add(mod_checkbox, 0, wxALIGN_CENTER | wxRESERVE_SPACE_EVEN_IF_HIDDEN);
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
