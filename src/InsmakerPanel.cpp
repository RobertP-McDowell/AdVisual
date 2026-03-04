#include <InsmakerPanel.h>

void InsmakerPanel::add_oplfm_editor(OPLFM* opl, bool carrier, wxBoxSizer* sizer) {
	add_slider_property("Attack Rate", &(opl->attack_rate), 0, 15, sizer);
	add_slider_property("Decay Rate", &(opl->decay_rate), 0, 15, sizer);
	add_slider_property("Sustain Level", &(opl->sustain_level), 0, 15, sizer);
	add_slider_property("Release Rate", &(opl->release_rate), 0, 15, sizer);
	add_checkbox_property("Sustaining Sound", false, sizer);
	add_checkbox_property("Envelope Scaling", false, sizer);
	add_slider_property("Frequency Multiplier", &(opl->frequency_multiplier), 0, 15, sizer); // 0.5
	add_slider_property("Modulation Feedback", &(opl->feedback), 0, 7, sizer);
	add_checkbox_property("Pitch Vibrato", false, sizer);
	add_slider_property("Output Level", &(opl->output_level), 0, 63, sizer);
	add_slider_property("Level Scaling", &(opl->level_scaling), 0, 3, sizer);
	add_checkbox_property("Amplitude Vibrato", false, sizer);
	add_slider_property("Wave Form", &(opl->waveform), 0, 3, sizer);
}

InsmakerPanel::InsmakerPanel(wxWindow* parent, int id) : wxScrolledWindow(parent, id) {
	//SetBackgroundColour(*wxBLACK);
	current_bank = make_unique<Bank>();
	current_instrument = make_unique<Instrument>();
	wxBoxSizer* h_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* vname_sizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* vcarrier_sizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* vmodulator_sizer = new wxBoxSizer(wxVERTICAL);

	add_property_name("Attack Rate", vname_sizer);
	add_property_name("Decay Rate", vname_sizer);
	add_property_name("Sustain Level", vname_sizer);
	add_property_name("Release Rate", vname_sizer);
	add_property_name("Sustaining Sound", vname_sizer);
	add_property_name("Envelope Scaling", vname_sizer);
	add_property_name("Frequency Multiplier", vname_sizer);
	add_property_name("Modulation Feedback", vname_sizer);
	add_property_name("Pitch Vibrato", vname_sizer);
	add_property_name("Output Level",vname_sizer);
	add_property_name("Level Scaling", vname_sizer);
	add_property_name("Amplitude Vibrato", vname_sizer);
	add_property_name("Wave Form", vname_sizer);

	add_oplfm_editor(&(current_instrument->carrier), true, vcarrier_sizer);
	add_oplfm_editor(&(current_instrument->modulator), false, vmodulator_sizer);

	h_sizer->Add(vname_sizer, 1, wxEXPAND);
	h_sizer->Add(vcarrier_sizer, 3, wxEXPAND);
	h_sizer->Add(vmodulator_sizer, 3, wxEXPAND);

	SetSizerAndFit(h_sizer);
}

void InsmakerPanel::SaveChanges() {
	
}

void InsmakerPanel::add_property_name(string name, wxBoxSizer* sizer) {
	// Property name:
	wxStaticText* property_name = new wxStaticText(this, wxID_ANY, name);
	property_name->SetWindowStyle(wxALIGN_LEFT);
	sizer->Add(property_name, 1, wxALIGN_LEFT);
}

void InsmakerPanel::add_slider_property(string name, uint8_t* p_value_ptr, uint8_t p_min_value, uint8_t p_max_value, wxBoxSizer* sizer) {
	// Carrier:
	if (p_value_ptr != nullptr) {
		SpinBoxSlider* carrier_slider = new SpinBoxSlider(this, wxID_ANY, p_value_ptr, p_min_value, p_max_value);
		carrier_slider->Bind(EVT_SPIN_BOX_SLIDER, &InsmakerPanel::on_carrier_slider_event, this, wxID_ANY);
		sizer->Add(carrier_slider, 1, wxEXPAND);
	}
	else { // Add Spacer.
		sizer->Add(0, 0, 1, wxEXPAND);
	}
}

void InsmakerPanel::add_checkbox_property(string name, bool p_value_ptr, wxBoxSizer* sizer) {
	wxCheckBox* checkbox = new wxCheckBox(this, wxID_ANY, wxEmptyString);
	checkbox->Bind(wxEVT_CHECKBOX, &InsmakerPanel::on_carrier_checkbox_event, this, wxID_ANY);
	sizer->Add(checkbox, 1, wxALIGN_CENTER);
}


void InsmakerPanel::on_carrier_slider_event(wxCommandEvent& event) {
	number_of_unsaved_changes += event.GetInt();
}
void InsmakerPanel::on_modulator_slider_event(wxCommandEvent& event) {
	number_of_unsaved_changes += event.GetInt();
}
void InsmakerPanel::on_carrier_checkbox_event(wxCommandEvent& event) {
	cout << event.IsChecked() << "\n";
}
void InsmakerPanel::on_modulator_checkbox_event(wxCommandEvent& event) {
	cout << event.IsChecked() << "\n";
}

wxDEFINE_EVENT(EVT_SPIN_BOX_SLIDER, wxCommandEvent);

SpinBoxSlider::SpinBoxSlider(wxWindow* parent, int id, uint8_t* p_value_ptr, uint8_t p_min_value, uint8_t p_max_value) :
		wxControl(parent, id), min_value(p_min_value), max_value(p_max_value), value_ptr(p_value_ptr) {
	Init();
}

void SpinBoxSlider::SetNewValue(int new_value) {
	new_value = clamp(new_value, min_value, max_value);
	wxCommandEvent event(EVT_SPIN_BOX_SLIDER, GetId());
	event.SetInt(0); // If event int is 0, no change to 'HasUnsavedChange' has happened, i;e value still = / still != slider_value.
	if (slider_value != new_value) {
		if (slider_value != *value_ptr && new_value == *value_ptr) {
			event.SetInt(-1); // -1 = SpinBox had unsaved value, but now equals its original value therefore it is saved.
		}
		if (slider_value == *value_ptr && new_value != *value_ptr) {
			event.SetInt(1); // 1 = SpinBox had saved value, but is now unsaved.
		}
	}
	slider_value = new_value;
	ProcessWindowEvent(event);
	Refresh();
	Update();
}

