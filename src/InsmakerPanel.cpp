#include <InsmakerPanel.h>

void InsmakerPanel::add_slider_property(string name, int default_value, int min_value, int max_value, bool modulator_only) {
	// Property name:
	wxStaticText* property_name = new wxStaticText(this, wxID_ANY, name);
	property_name->SetWindowStyle(wxALIGN_LEFT);
	property_sizer->Add(property_name, 1, wxALIGN_LEFT);

	// Carrier:
	if (!modulator_only) {
		SpinBoxSlider* carrier_slider = new SpinBoxSlider(this, wxID_ANY, default_value, min_value, max_value);
		//carrier_slider->Bind(wxEVT_SLIDER, &InsmakerPanel::on_carrier_slider_event, this, wxID_ANY);
		property_sizer->Add(carrier_slider, 1, wxEXPAND);
	}
	else { // Add Spacer.
		property_sizer->Add(0, 0, 1, wxEXPAND);
	}

	// Modulator:
	SpinBoxSlider* modulator_slider = new SpinBoxSlider(this, wxID_ANY, default_value, min_value, max_value);
	//modulator_slider->Bind(wxEVT_SLIDER, &InsmakerPanel::on_modulator_slider_event, this, wxID_ANY);
	property_sizer->Add(modulator_slider, 1, wxEXPAND | wxALIGN_LEFT);
}

InsmakerPanel::InsmakerPanel(wxWindow* parent, int id) : wxScrolledWindow(parent, id) {
	//SetBackgroundColour(*wxBLACK);
	property_sizer = new wxFlexGridSizer(3, 10, 10);
	property_sizer->AddGrowableCol(0, 1); // Make StaticText growable.
	property_sizer->AddGrowableCol(1, 3); // Make Carrier Slider growable.
	property_sizer->AddGrowableCol(2, 3); // Make Modulator Slider growable.

	property_sizer->Add(0, 0, 1, wxEXPAND); // Add Spacer.
	wxStaticText* carrier_header = new wxStaticText(this, wxID_ANY, "Carrier");
	property_sizer->Add(carrier_header, 0, wxALIGN_CENTER);
	wxStaticText* modulator_header = new wxStaticText(this, wxID_ANY, "Modulator");
	property_sizer->Add(modulator_header, 0, wxALIGN_CENTER);

	property_sizer->SetFlexibleDirection(wxALL);
	add_slider_property("Attack Rate", 0, 0, 15);
	add_slider_property("Decay Rate", 15, 0, 15);
	add_slider_property("Sustain Level", 7, 0, 15);
	add_slider_property("Release Rate", 3, 0, 15);
	add_slider_property("Frequency Multiplier", 5, 0, 15, true); // 0.5
	add_slider_property("Modulation Feedback", 4, 0, 7);
	// Debug: for loop just to see what multiple rows will look like.
	for (int i = 0; i <= 10; i++) {
		
	}
	SetSizerAndFit(property_sizer);
}

void InsmakerPanel::on_carrier_slider_event(wxCommandEvent& event) {
	cout << "Carry\n";
	
}

void InsmakerPanel::on_modulator_slider_event(wxCommandEvent& event) {
	cout << "Modulator\n";
}

void InsmakerPanel::on_carrier_spinbox_event(wxSpinEvent& event) {
	cout << "Spin Carry\n";
}
void on_modulator_spinbox_event(wxSpinEvent& event) {
	cout << "Spin Modulator\n";
}

SpinBoxSlider::SpinBoxSlider(wxWindow* parent, int id, int p_value, int p_min_value, int p_max_value) :
		wxControl(parent, id), min_value(p_min_value), max_value(p_max_value), value(p_value) {
	Init();
}
