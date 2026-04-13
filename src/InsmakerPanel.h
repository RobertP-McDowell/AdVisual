#pragma once

#include <wx/wx.h>
#include <wx/utils.h>
#include <wx/filedlg.h> 
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <wx/scrolwin.h>
#include <wx/scrolbar.h>
#include <wx/popupwin.h>
#include <wx/artprov.h>
#include <common.h>
#include <Track.h>
#include <Instrument.h>
#include <FileAccess.h>
#include <memory.h>
#include <map>

using namespace std;

enum {
	ID_MINUS = 1,
	ID_PLUS = 2,
};

wxDECLARE_EVENT(EVT_SPIN_BOX_SLIDER, wxCommandEvent);

class OPLFMPropertyControl : public wxControl {
protected:
	uint8_t* value_ptr = nullptr;
	uint8_t* new_value_ptr = nullptr;
	bool is_valid() const { return value_ptr != nullptr && new_value_ptr; }
public:
	virtual void SetNewValue(uint8_t new_value) {
		if (new_value_ptr == nullptr) return;
		*new_value_ptr = (new_value < 0 ? uint8_t(0) : uint8_t(new_value));
		Refresh();
		Update();
	}
	int GetNewValue() const { return *new_value_ptr; }
	void SaveCurrentValue() {
		if (is_valid()) {
			*value_ptr = *new_value_ptr;
		}
	}
	void LoadFromValue(uint8_t* p_value_ptr, uint8_t* p_new_value_ptr) {
		if (p_new_value_ptr == nullptr) {
			Show(false);
			return;
		}
		value_ptr = p_value_ptr;
		new_value_ptr = p_new_value_ptr;
		Show(true);
		Refresh();
		Update();
	}
	bool HasUnsavedChange() const { return is_valid() && *new_value_ptr != *value_ptr; }
	OPLFMPropertyControl(wxWindow* parent, int id, uint8_t* p_value_ptr, uint8_t* p_new_value_ptr) : 
			wxControl(parent, id), value_ptr(p_value_ptr), new_value_ptr(p_value_ptr) {
		if (!is_valid()) {
			Show(false);
			return;
		}
		*new_value_ptr = *value_ptr;
	}
};

class OPLFMCheckbox : public OPLFMPropertyControl {
protected:
	void on_set_focus(wxFocusEvent& event) {
		Refresh();
		Update();
	}
	void on_kill_focus(wxFocusEvent& event) {
		Refresh();
		Update();
	}
	void on_key_down(wxKeyEvent& event) {
		if (event.IsKeyInCategory(WXK_CATEGORY_ARROW)) {
			switch (event.GetKeyCode()) {
			case WXK_LEFT:
				SetNewValue(0);
				return;
			case WXK_RIGHT:
				SetNewValue(1);
				return;
			}
		}
		event.Skip();
	}
	void on_lmb_down(wxMouseEvent& event) {
		SetFocus();
		Refresh();
		Update();
	}
	void on_lmb_up(wxMouseEvent& event) {
		SetNewValue(uint8_t(!(*new_value_ptr)));
		Refresh();
		Update();
	}
	void on_paint(wxPaintEvent& event) {
		wxPaintDC dc(this);
		wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
		wxSize bitmap_size = wxSize(32, 32);
		if (*new_value_ptr == 1) {
			wxBitmap checked_bitmap = GetAsset((HasFocus() ? "CheckboxCheckedFocused.svg" : "CheckboxChecked.svg"), GetSize()).GetBitmap(bitmap_size);
			gc->DrawBitmap(checked_bitmap, 0, 0, bitmap_size.x, bitmap_size.y);
		}
		if (*new_value_ptr == 0) {
			wxBitmap checked_bitmap = GetAsset((HasFocus() ? "CheckboxUncheckedFocused.svg" : "CheckboxUnchecked.svg"), GetSize()).GetBitmap(bitmap_size);
			gc->DrawBitmap(checked_bitmap, 0, 0, bitmap_size.x, bitmap_size.y);
		}
		if (*new_value_ptr != *value_ptr) {
			wxBitmap checked_bitmap = GetAsset("UnsavedAsterisk.svg", GetSize()).GetBitmap(bitmap_size);
			gc->DrawBitmap(checked_bitmap, bitmap_size.x, 0, bitmap_size.x, bitmap_size.y);
		}
		delete gc;
	}
	void Init() {
		wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
		wxColour bg_colour(10, 10, 10);
		wxSize increment_button_size(GetSize().y, GetSize().y);
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		SetWindowStyle(wxBORDER_NONE);
		Bind(wxEVT_KILL_FOCUS, &OPLFMCheckbox::on_kill_focus, this);
		Bind(wxEVT_SET_FOCUS, &OPLFMCheckbox::on_set_focus, this);
		Bind(wxEVT_KEY_DOWN, &OPLFMCheckbox::on_key_down, this);
		Bind(wxEVT_LEFT_DOWN, &OPLFMCheckbox::on_lmb_down, this);
		Bind(wxEVT_LEFT_UP, &OPLFMCheckbox::on_lmb_up, this);
		Bind(wxEVT_PAINT, &OPLFMCheckbox::on_paint, this);
	}
	wxSize DoGetBestSize() const override {
		return wxSize(66, 33);
	}
public:
	OPLFMCheckbox(wxWindow* parent, int id, uint8_t* p_value_ptr, uint8_t* p_new_value_ptr) :
			OPLFMPropertyControl(parent, id, p_value_ptr, p_new_value_ptr) {
		Init();
	}
};

class SpinBoxSlider : public OPLFMPropertyControl {
protected:
	void on_increment_button_pressed(wxCommandEvent& event) {
		int new_control_value = *new_value_ptr + (event.GetId() == ID_PLUS ? 1 : -1);
		// Make sure unsigned int isn't negative.
		SetNewValue(new_control_value);
	}
	void on_set_focus(wxFocusEvent& event) {
		Refresh();
		Update();
	}
	void on_kill_focus(wxFocusEvent& event) {
		Refresh();
		Update();
	}
	void on_child_focus(wxChildFocusEvent& event) {
		SetFocus();
	}
	void on_key_down(wxKeyEvent& event) {
		if (event.IsKeyInCategory(WXK_CATEGORY_ARROW)) {
			switch (event.GetKeyCode()) {
			case WXK_LEFT:
				SetNewValue(*new_value_ptr - 1);
				return;
			case WXK_RIGHT:
				SetNewValue(*new_value_ptr + 1);
				return;
			}
		}
		event.Skip();
	}
	void on_lmb_down(wxMouseEvent& event) {
		SetFocus();
		Refresh();
		Update();
	}
	void on_mouse_motion(wxMouseEvent& event) {
		if (event.Dragging()) {
			int slider_width = slider_panel->GetSize().x;
			float new_slider_valuef = (event.GetPosition().x / float(slider_width)) * max_value;
			// First we use int, than cast to uint8_t, just so we have a higher and lower bounds limit and it won't wrap negative numbers.
			int int_new_slider_value = (new_slider_valuef > *value_ptr ? floor(new_slider_valuef) : ceil(new_slider_valuef));
			int_new_slider_value = clamp(int_new_slider_value, int(min_value), int(max_value));
			uint8_t new_slider_value = (int_new_slider_value < 0 ? uint8_t(0) : uint8_t(int_new_slider_value)); // Keep unsigned positive.
			if (*new_value_ptr != new_slider_value) {
				SetNewValue(new_slider_value);
				Refresh();
				Update();
			}
		}
	}
	void on_lmb_up(wxMouseEvent& event) {
		SetNewValue(*new_value_ptr);
		Refresh();
		Update();
	}
	void paint_text_on_slider(wxGraphicsContext* gc, int slider_width, int slider_height, string text,
			int offset, int offset_pad, bool draw_high = true, wxBrush bg_brush = wxNullBrush) {
		// Just a helper function to keep value_ptr and new_value_ptr text within slider width bounds.
		wxDouble text_width, text_height, text_descent, text_leading_space;
		gc->GetTextExtent(text, &text_width, &text_height, &text_descent, &text_leading_space);
		int difference = (text_width + offset) - slider_width;
		if (difference > 0) { // text overran slider, show draw before end of it.
			gc->DrawText(text, offset - (text_width + offset_pad), 0, gc->CreateBrush(bg_brush));
		}
		else {
			gc->DrawText(text, offset + offset_pad, 0, gc->CreateBrush(bg_brush));
		}
	}
	void on_paint(wxPaintEvent& event) {
		wxPaintDC dc(slider_panel);
		wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
		
		wxGraphicsPen tick_pen = gc->CreatePen(wxGraphicsPenInfo(wxColour(140, 140, 140)).Width(1.0).Style(wxPENSTYLE_SOLID).Cap(wxCAP_BUTT));
		gc->SetPen(tick_pen);
		int slider_width, slider_height;
		slider_panel->GetSize(&slider_width, &slider_height);
		int tick_count = max_value - min_value;
		double tick_spacing = slider_width / double(tick_count != 0 ? tick_count : 1); // Use double so it size nicely.
		int tick_height = slider_height / 3.0;
		for (int i = 0; i <= tick_count; i++) {
			int x_offset = (i * tick_spacing);
			gc->StrokeLine(x_offset, 0, x_offset, tick_height);
		}
		int bold_tick_width = 3.0;
		wxGraphicsPen bold_pen = gc->CreatePen(wxGraphicsPenInfo(*wxWHITE).Width(bold_tick_width).Style(wxPENSTYLE_SOLID).Cap(wxCAP_BUTT));
		wxGraphicsFont number_font = gc->CreateFont(slider_height, wxEmptyString, wxFONTFLAG_DEFAULT, *wxWHITE);
		gc->SetFont(number_font);
		int current_tick_offset = *value_ptr * tick_spacing;
		int slider_tick_offset = *new_value_ptr * tick_spacing;
		slider_tick_offset += (*new_value_ptr > *value_ptr ? 0 : 1); // Consistently draw brush before ticks.
		if (HasUnsavedChange()) { // Draw Slider Value and Text.
			wxGraphicsBrush highlight_brush;
			if (HasFocus()) {
				highlight_brush = gc->CreateBrush(*(wxTheBrushList->FindOrCreateBrush(wxColour(20, 20, 255, 150))));
			}
			else {
				highlight_brush = gc->CreateBrush(*(wxTheBrushList->FindOrCreateBrush(wxColour(100, 100, 100, 100))));
			}
			gc->SetPen(wxNullPen);
			gc->SetBrush(highlight_brush);
			gc->DrawRectangle(current_tick_offset, 0, slider_tick_offset - current_tick_offset, slider_height);
			paint_text_on_slider(gc, slider_width, slider_height, to_string(*new_value_ptr), slider_tick_offset, bold_tick_width, false);
			paint_text_on_slider(gc, slider_width, slider_height, to_string(*value_ptr), current_tick_offset, bold_tick_width, true);
		}
		else { // We draw text with a black background when not actively editing, for better text clarity.
			paint_text_on_slider(gc, slider_width, slider_height, to_string(*value_ptr),
				current_tick_offset, bold_tick_width, true, *wxBLACK_BRUSH);
		}
		wxGraphicsPen focus_bold_pen = gc->CreatePen(
			wxGraphicsPenInfo(wxColour(100, 100, 255)).Width(bold_tick_width * 2).Style(wxPENSTYLE_SOLID).Cap(wxCAP_BUTT));
		if (HasFocus() || HasUnsavedChange()) {
			gc->SetPen(bold_pen); // Draw Slider Value Tick
			gc->StrokeLine(slider_tick_offset, slider_height, slider_tick_offset, slider_height - tick_height);
		}
		gc->SetPen(bold_pen); // Draw Value Tick
		gc->StrokeLine(current_tick_offset, 0, current_tick_offset, slider_height / 2);
		
		delete gc;
	}
	void Init() {
		wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
		wxColour bg_colour(10, 10, 10);
		wxSize increment_button_size(GetSize().y, GetSize().y);
		SetBackgroundColour(bg_colour);
		wxBitmapButton* minus_button = new wxBitmapButton(this, ID_MINUS, GetAsset("MinusButton.svg", increment_button_size));
		minus_button->Bind(wxEVT_BUTTON, &SpinBoxSlider::on_increment_button_pressed, this, ID_MINUS);
		minus_button->SetBackgroundColour(bg_colour);
		minus_button->SetWindowStyle(wxBORDER_NONE);
		sizer->Add(minus_button, 0);
		wxBitmapButton* plus_button = new wxBitmapButton(this, ID_PLUS, GetAsset("PlusButton.svg", increment_button_size));
		plus_button->Bind(wxEVT_BUTTON, &SpinBoxSlider::on_increment_button_pressed, this, ID_PLUS);
		plus_button->SetBackgroundColour(bg_colour);
		plus_button->SetWindowStyle(wxBORDER_NONE);
		sizer->Add(plus_button, 0);
		slider_panel = new wxPanel(this, wxID_ANY);
		sizer->Add(slider_panel, 1, wxEXPAND);
		Bind(wxEVT_KILL_FOCUS, &SpinBoxSlider::on_kill_focus, this);
		Bind(wxEVT_SET_FOCUS, &SpinBoxSlider::on_set_focus, this);
		Bind(wxEVT_KEY_DOWN, &SpinBoxSlider::on_key_down, this);
		Bind(wxEVT_CHILD_FOCUS, &SpinBoxSlider::on_child_focus, this);
		slider_panel->Bind(wxEVT_LEFT_DOWN, &SpinBoxSlider::on_lmb_down, this);
		slider_panel->Bind(wxEVT_MOTION, &SpinBoxSlider::on_mouse_motion, this);
		slider_panel->Bind(wxEVT_LEFT_UP, &SpinBoxSlider::on_lmb_up, this);
		slider_panel->Bind(wxEVT_PAINT, &SpinBoxSlider::on_paint, this);
		SetSizerAndFit(sizer);
	}
	wxSize DoGetBestSize() const override {
		return wxSize(32, 32);
	}
	wxPanel* slider_panel;
	bool lmb_down = false;
	uint8_t min_value = 0;
	uint8_t max_value = 100;
public:
	void SetNewValue(uint8_t p_new_value) override;
	SpinBoxSlider(wxWindow* parent, int id, uint8_t* p_value_ptr, uint8_t* p_new_value_ptr, uint8_t p_min_value = 0, uint8_t p_max_value = 100);
};

class InsmakerPanel : public wxScrolledWindow {
public:
	InsmakerPanel(wxWindow* parent, int id = wxID_ANY);
	void SetAdditiveSynth(bool value);
	void SetPercussionMode(uint8_t value); // 0 for melodic, 1 for bass, 2 for snare ...
	bool GetAdditiveSynth() const { return new_instrument.modulator.additive_synth; }
	uint8_t GetPercussionMode() const;
	void save_properties_to_opl();
	void SetInstrumentByName(char name[9]);
private:
	Instrument* find_unsaved_instrument(char name[9]);
	int get_number_of_unsaved_changes();
	void create_oplfm_editor(OPLFM* p_car, OPLFM* p_mod);
	void update_oplfm_editor(OPLFM* p_car, OPLFM* p_mod, OPLFM* n_car, OPLFM* n_mod);
	void update_oplfm_property(int idx, uint8_t* car_value_ptr, uint8_t* mod_value_ptr,
		uint8_t* new_car_value_ptr, uint8_t* new_mod_value_ptr);
	void add_slider_property(string name, uint8_t* p_car_value_ptr, uint8_t* p_mod_value_ptr,
		uint8_t p_min_value, uint8_t p_max_value);
	void add_checkbox_property(string name, uint8_t* p_car_value_ptr, uint8_t* p_mod_value_ptr);
	void on_slider_event(wxCommandEvent& event);
	void on_checkbox_event(wxCommandEvent& event);
	void on_resize_piano(wxSizeEvent& event);
	void on_scroll_piano(wxScrollEvent& event);
	//void add_radio_property();
	Instrument* instrument_ptr = nullptr;
	Instrument  new_instrument;
	vector<OPLFMPropertyControl*> carrier_properties;
	vector<OPLFMPropertyControl*> modulator_properties;
	vector<Instrument> unsaved_instruments;
	wxFlexGridSizer* property_sizer;
	PianoControl* piano_ctrl;
	wxScrollBar* h_scroll_bar;
	//map<string, variant> unsaved_properties;
	int number_of_unsaved_changes = 0; // Is more than 0 if a Slider or Checkbox is not their saved value.
};
