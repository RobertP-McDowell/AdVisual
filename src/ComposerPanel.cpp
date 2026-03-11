#include <ComposerPanel.h>

////////////////////////////////////////////////////////////////////////////
// ComposerPanel Functions /////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

ComposerPanel::ComposerPanel(wxWindow *parent) : 
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
{
	wxColour bg_colour = wxColour(40, 40, 40);
	SetBackgroundColour(*wxBLACK);
	event_font = wxFont(wxFontInfo(10).Bold());
	note_size = wxSize(20.0, 20.0);
	
	event_header = make_unique<wxPanel>(this, wxID_ANY, wxDefaultPosition, wxSize(100, 30));
	event_header->SetBackgroundColour(bg_colour);
	event_header->Bind(wxEVT_PAINT, &ComposerPanel::on_paint_event_header, this);
	event_header->Bind(wxEVT_MOTION, &ComposerPanel::on_mouse_motion_event_header, this);
	event_header->Bind(wxEVT_LEFT_DOWN, &ComposerPanel::on_lmb_down_event_header, this);
	event_header->Bind(wxEVT_LEFT_UP, &ComposerPanel::on_lmb_up_event_header, this);
	
	grid_panel = make_unique<wxScrolledWindow>(this, wxID_ANY, wxDefaultPosition, wxSize(100, 100));
	grid_panel->SetBackgroundColour(bg_colour);
	grid_panel->EnableScrolling(true, true);
	grid_panel->ShowScrollbars(wxSHOW_SB_ALWAYS, wxSHOW_SB_ALWAYS);
	grid_panel->SetScrollbars(note_size.x, note_size.y, 0, pitch_range, 0, pitch_range/2.0);
	
	grid_panel->Bind(wxEVT_PAINT, &ComposerPanel::on_paint_grid, this);
	grid_panel->Bind(wxEVT_SCROLLWIN_THUMBTRACK, &ComposerPanel::on_scroll_grid, this);
	grid_panel->Bind(wxEVT_LEFT_DOWN, &ComposerPanel::on_lmb_down, this);
	grid_panel->Bind(wxEVT_LEFT_UP, &ComposerPanel::on_lmb_up, this);
	grid_panel->Bind(wxEVT_MOTION, &ComposerPanel::on_mouse_motion, this);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(event_header.get(), 0, wxEXPAND | wxBOTTOM, 4);
	sizer->Add(grid_panel.get(), 1, wxEXPAND);
	SetSizerAndFit(sizer);
	current_channel_idx = 0;
	current_track = make_shared<Track>();
	current_channel = (current_track->channels[0]);
	wxPoint control_offset(30, 0);
	
	event_popup = make_unique<EventPopup>(this, current_track, current_channel);
}

void ComposerPanel::SetPreviewChannels(bool value) {
	preview_channels = value;
	Refresh();
	Update();
}

void ComposerPanel::SetChannelIndex(int value) {
	current_channel_idx = value;
	current_channel = current_track->GetChannel(current_channel_idx);
	Refresh();
	Update();
}

void ComposerPanel::on_scroll_grid(wxScrollWinEvent& event) {
	if (event.GetOrientation() == wxHORIZONTAL) {
		grid_offset.x = note_size.x * event.GetPosition();
	}
	else {
		grid_offset.y = note_size.x * event.GetPosition();
	}
	Refresh();
	Update();
}

void ComposerPanel::on_paint_grid(wxPaintEvent& event) {
	draw_grid();
	draw_notes();
}

void ComposerPanel::draw_grid() {
	wxPaintDC dc(grid_panel.get());
	wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
	if (gc == nullptr) return;
	wxGraphicsPen heavy_grid_pen = gc->CreatePen(wxGraphicsPenInfo(*wxColour(190, 190, 190)).Width(3.5).Style(wxPENSTYLE_SOLID));
	wxGraphicsPen dashed_grid_pen = gc->CreatePen(wxGraphicsPenInfo(*wxColour(140, 140, 140)).Width(2.5).Style(wxPENSTYLE_SHORT_DASH));
	wxGraphicsPen dotted_grid_pen = gc->CreatePen(wxGraphicsPenInfo(*wxColour(140, 140, 140)).Width(2.5).Style(wxPENSTYLE_DOT));
	int ticks_per_measure = current_track->beats_per_measure * current_track->ticks_per_beat;
	wxPoint draw_offstep = wxPoint(grid_offset.x % (note_size.x * ticks_per_measure),
			(grid_offset.y % (note_size.y * 11)));
	int panel_width;
	int panel_height;
	GetSize(&panel_width, &panel_height);
	wxSize cell_size(note_size.x / zoom, (note_size.y) / zoom);
	int columns = (panel_width / cell_size.x);
	int rows = min(pitch_range, panel_height / cell_size.y);
	int grid_sub = 0;
	gc->SetPen(dotted_grid_pen);
	for (int i = 0; i <= rows; i++) {
		if (i % 7 == 4 || i % 7 == 0)  {
			grid_sub++;
			continue;
		}
		double y = (cell_size.y * 2.0 * (i-(grid_sub/2.0)));
		gc->StrokeLine(0, y - draw_offstep.y, panel_width, y - draw_offstep.y);
	}
	gc->SetPen(heavy_grid_pen);
	double middle_c_y = cell_size.y * 50;
	gc->StrokeLine(0, grid_offset.y + middle_c_y, panel_width, grid_offset.y + middle_c_y);
	for (int measure = 0; measure <= columns; measure += ticks_per_measure) {
		gc->SetPen(heavy_grid_pen);
		double x = cell_size.x * measure;
		gc->StrokeLine(x - draw_offstep.x, 0, x - draw_offstep.x, panel_height);
		for (int beat = current_track->ticks_per_beat; beat < ticks_per_measure; beat += current_track->ticks_per_beat) {
			gc->SetPen(dashed_grid_pen);
			x = cell_size.x * (measure + beat);
			gc->StrokeLine(x - draw_offstep.x, -panel_height - draw_offstep.y, x - draw_offstep.x, panel_height);
		}
	}
	delete gc;
}

void ComposerPanel::draw_notes() {
	wxPaintDC dc(grid_panel.get());
	wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
	if (gc == nullptr) return;
	int ticks_per_measure = current_track->beats_per_measure * current_track->ticks_per_beat;
	int panel_width;
	int panel_height;
	wxSize middle_of_cell = note_size / 2;
	double note_pen_width = note_size.y / 4.0;
	wxGraphicsPen note_pen = gc->CreatePen(wxGraphicsPenInfo(
	current_channel->colour).Width(note_pen_width).Style(wxPENSTYLE_SOLID).Cap(wxCAP_BUTT).Join(wxJOIN_BEVEL));
	gc->SetPen(note_pen);
	gc->SetBrush(*wxBLACK_BRUSH);
	for (Note& note : current_channel->notes) {
		gc->DrawRoundedRectangle((note.offset * note_size.x) + 1 - grid_offset.x, (note.pitch * note_size.y) + 2 - grid_offset.y,
			(note.length * note_size.x) - 2, note_size.y - 4, note_size.y / 4.0);
	}
	wxGraphicsPen ghost_pen = gc->CreatePen(wxGraphicsPenInfo(
	*wxColour(215, 200, 255, 100)).Width(note_size.y - 2).Style(wxPENSTYLE_SOLID).Cap(wxCAP_BUTT).Join(wxJOIN_BEVEL));
	if (editing_note != nullptr) {
		gc->SetPen(ghost_pen);
		gc->StrokeLine((editing_note->offset * note_size.x) - grid_offset.x, (editing_note->pitch * note_size.y) + middle_of_cell.y - grid_offset.y,
		((editing_note->offset + editing_note->length) * note_size.x) - grid_offset.x, (editing_note->pitch * note_size.y) + middle_of_cell.y - grid_offset.y);
	}

	if (preview_channels) {
		for (int i = 0; i < current_track->get_channel_count(); i++) {
			if (i == current_channel_idx) {
				continue;
			}
			shared_ptr<Channel> channel = current_track->GetChannel(i);
			gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(
			channel->colour).Width(note_size.y / 8.0).Style(wxPENSTYLE_SOLID).Cap(wxCAP_BUTT).Join(wxJOIN_BEVEL)));
			int line_y_offset = ((note_size.y / 8.0) * i) + note_pen_width + 1;
			for (Note& note : channel->notes) {
				gc->StrokeLine((note.offset * note_size.x) + note_pen_width - grid_offset.x, (note.pitch * note_size.y) + line_y_offset - grid_offset.y,
				((note.offset + note.length) * note_size.x) - note_pen_width - grid_offset.x, (note.pitch * note_size.y) + line_y_offset - grid_offset.y);
			}
		}
	}

	delete gc;
}

void ComposerPanel::on_lmb_down(wxMouseEvent& event) {
	mouse_down_start = event.GetPosition() + grid_offset;
	editing_note = make_unique<Note>();
	editing_note->offset = mouse_down_start.m_x / note_size.x;
	editing_note->pitch = mouse_down_start.m_y / note_size.y;
	editing_note->length = 1;
}

void ComposerPanel::on_lmb_up(wxMouseEvent& event) {
	if (editing_note != nullptr) {
		current_channel->add_note(*editing_note);
	}
	editing_note = nullptr;
	grid_panel->SetVirtualSize((current_channel->get_tick_count() * note_size.x) + grid_panel->GetSize().x, note_size.y * pitch_range);
	Refresh();
	Update();
}

void ComposerPanel::on_mouse_motion(wxMouseEvent& event) {
	if (event.LeftIsDown() && editing_note != nullptr) {
		wxPoint2DDouble local_mouse_position = event.GetPosition() + grid_offset;
		int start_offset = (mouse_down_start.m_x / note_size.x);
		int end_offset = (local_mouse_position.m_x / note_size.x);
		if (end_offset >= start_offset) {
			editing_note->length = (end_offset - start_offset) + 1;
			editing_note->offset = start_offset;
		}
		else {
			editing_note->length = (start_offset - end_offset) + 1;
			editing_note->offset = end_offset;
		}
		Refresh();
		Update();
	}
}

void ComposerPanel::on_paint_event_header(wxPaintEvent& event) {
	wxPaintDC dc(event_header.get());
	wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
	if (gc == nullptr) return;
	int panel_width;
	int panel_height;
	GetSize(&panel_width, &panel_height);
	int event_header_width;
	int event_header_height;
	int draw_offstep = grid_offset.x % note_size.x;
	event_header->GetSize(&event_header_width, &event_header_height);
	wxSize event_bar_size = wxSize(note_size.x, event_header_height / 4.0);
	vector<int16_t> event_ticks;
	wxGraphicsPen grid_pen = gc->CreatePen(wxGraphicsPenInfo(
	wxColour(100, 100, 100)).Width(1.25).Style(wxPENSTYLE_SOLID).Cap(wxCAP_BUTT).Join(wxJOIN_BEVEL));
	gc->SetPen(grid_pen);
	gc->SetFont(event_font, *wxBLACK);
	
	for (int column = 0; column <= panel_width / note_size.x; column++) {
		gc->StrokeLine((column * note_size.x) - draw_offstep, 0, (column * note_size.x) - draw_offstep, panel_height);
	}
	
	wxGraphicsPen event_pen = gc->CreatePen(wxGraphicsPenInfo(
	wxColour(0, 0, 0)).Width(2.0).Style(wxPENSTYLE_SOLID).Cap(wxCAP_BUTT).Join(wxJOIN_BEVEL));
	gc->SetPen(event_pen);
	gc->SetBrush(*wxWHITE_BRUSH);
	for (auto event = current_track->tempo_events.begin(); event != current_track->tempo_events.end(); event++) {
		int event_on_grid = (event->first * note_size.x) - grid_offset.x;
		if (event_on_grid > panel_width) break;
		if (event_on_grid < -(note_size.x * 20)) continue;
		gc->DrawRectangle(event_on_grid, 0, event_bar_size.x, event_bar_size.y);
		event_ticks.push_back(event->first);
		//string text = to_string(event->second);
		//text.resize(3);
		//gc->DrawText(text, event_on_grid - grid_offset.x, 0);
	}
	gc->SetBrush(wxColour(200, 10, 10));
	for (auto event = current_channel->instrument_events.begin(); event != current_channel->instrument_events.end(); event++) {
		int event_on_grid = (event->first * note_size.x) - grid_offset.x;
		if (event_on_grid > panel_width) break;
		if (event_on_grid < -(note_size.x * 20)) continue;
		gc->DrawRectangle(event_on_grid, event_bar_size.y, event_bar_size.x, event_bar_size.y);
		event_ticks.push_back(event->first);
	}
	gc->SetBrush(wxColour(10, 200, 10));
	for (auto event = current_channel->pitch_events.begin(); event != current_channel->pitch_events.end(); event++) {
		int event_on_grid = (event->first * note_size.x) - grid_offset.x;
		if (event_on_grid > panel_width) break;
		if (event_on_grid < -(note_size.x * 20)) continue;
		gc->DrawRectangle(event_on_grid, event_bar_size.y * 2, event_bar_size.x, event_bar_size.y);
		event_ticks.push_back(event->first);
	}
	gc->SetBrush(wxColour(10, 10, 200));
	for (auto event = current_channel->volume_events.begin(); event != current_channel->volume_events.end(); event++) {
		int event_on_grid = (event->first * note_size.x) - grid_offset.x;
		if (event_on_grid > panel_width) break;
		if (event_on_grid < -(note_size.x * 20)) continue;
		gc->DrawRectangle(event_on_grid, event_bar_size.y * 3, event_bar_size.x, event_bar_size.y);
		event_ticks.push_back(event->first);
	}
	if (editing_event_tick != -1) {
		gc->SetBrush(wxColour(200, 200, 255, 100));
		int event_on_grid = (editing_event_tick * note_size.x) - grid_offset.x;
		gc->DrawRectangle(event_on_grid, 0, event_bar_size.x, event_header_height);
	}
	delete gc;
}

void ComposerPanel::on_mouse_motion_event_header(wxMouseEvent& event) {
	//wxPoint2DDouble local_mouse_position = event.GetPosition() + grid_offset;
}

void ComposerPanel::on_lmb_down_event_header(wxMouseEvent& event) {
	
}

void ComposerPanel::on_lmb_up_event_header(wxMouseEvent& event) {
	editing_event_tick = (event.GetPosition().x + grid_offset.x) / note_size.x;
	wxPoint popup_pos = wxPoint(editing_event_tick * note_size.x, event_header->GetSize().y) + event_header->GetPosition();
	event_popup->Position(ClientToScreen(popup_pos) - event_popup->GetSize(), event_popup->GetSize());
	event_popup->Popup(editing_event_tick, current_track, current_channel);
	event_header->Refresh();
	event_header->Update();
}

///////////////////////////////////////////////////////////////////////////////////
// EventPopup Functions ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

float get_float_from_string(string str_val, float min = 0.0, float max = 1.0) {
	if (str_val.empty()) return -1.0;
	float ret_float = 1.0;
	try {
		ret_float = stof(str_val);
		ret_float = clamp(ret_float, min, max);
	}
	catch (invalid_argument e) {
		cerr << str_val << " Not a float!\n";
		ret_float = -1.0;
	}
	catch (out_of_range e) {
		cerr << str_val << " Float out of range\n";
		ret_float = -1.0;
	}
	return ret_float;
}

void EventPopup::init_event_field(wxGridSizer* sizer, wxTextCtrl*& event_field, int ID) {
	event_field = new wxTextCtrl(this, ID);
	const string field_names[4] = {"Tempo", "Instrument", "Pitch", "Volume"};
	wxStaticText* field_name = new wxStaticText(this, ID, field_names[ID-ID_TEMPO_EVENT]);
	sizer->Add(field_name, 0, wxEXPAND | wxLEFT, 12);
	event_field->SetHint(field_names[ID-ID_TEMPO_EVENT]);
	event_field->SetWindowStyle(wxTE_PROCESS_ENTER);
	event_field->Bind(wxEVT_TEXT_ENTER, &EventPopup::on_text_entered, this, ID);
	sizer->Add(event_field, 0, wxEXPAND);
}

EventPopup::EventPopup(wxWindow* parent, shared_ptr<Track> track, shared_ptr<Channel> channel) : 
	wxPopupTransientWindow(parent, wxBORDER_DEFAULT | wxPU_CONTAINS_CONTROLS), current_track(track), current_channel(channel)
{
	Bind(wxEVT_SHOW, &EventPopup::on_show, this);
	wxGridSizer* field_sizer = new wxGridSizer(2);
	
	wxStaticText* popup_header = new wxStaticText(this, wxID_ANY, "Set Events");
	field_sizer->Add(popup_header, 0, wxLEFT, 6); // Add Event Header.
	field_sizer->Add(1, 1, wxEXPAND); // Add Spacer.
	
	init_event_field(field_sizer, tempo_field, ID_TEMPO_EVENT);
	init_event_field(field_sizer, instrument_field, ID_INSTRUMENT_EVENT);
	init_event_field(field_sizer, pitch_field, ID_PITCH_EVENT);
	init_event_field(field_sizer, volume_field, ID_VOLUME_EVENT);
	wxBoxSizer* main_sizer = new wxBoxSizer(wxHORIZONTAL);
	main_sizer->Add(field_sizer, 0);
	bank_ctrl = new BankControl(this, wxID_ANY, wxDefaultPosition, wxSize(200, 200));
	main_sizer->Add(bank_ctrl, 1, wxEXPAND);
	SetSizerAndFit(main_sizer);
}

#define update_event_field(event_field, get_last_event_callable, t_type, type_conversion_func, max_text_length) do { \
	int last_event_tick; \
	t_type last_event_value; \
	get_last_event_callable(editing_event_tick, last_event_tick, last_event_value); \
	if (last_event_tick == editing_event_tick) { \
		string new_text = type_conversion_func(last_event_value); \
		new_text.resize(max_text_length); \
		event_field->ChangeValue(new_text); \
		get_last_event_callable(editing_event_tick - 1, last_event_tick, last_event_value); \
	} \
	else { \
		event_field->ChangeValue(wxEmptyString); \
	} \
	string new_hint = type_conversion_func(last_event_value); \
	new_hint.resize(max_text_length); \
	event_field->SetHint( new_hint ); \
} while(0)

void EventPopup::Popup(int at_tick, shared_ptr<Track> track, shared_ptr<Channel> channel) {
	current_track = track;
	current_channel = channel;
	editing_event_tick = at_tick;
	update_event_field(tempo_field, current_track->get_last_tempo_event, float, to_string, 4);
	update_event_field(instrument_field, current_channel->get_last_instrument_event, string, , 9);
	update_event_field(pitch_field, current_channel->get_last_pitch_event, float, to_string, 4);
	update_event_field(volume_field, current_channel->get_last_volume_event, float, to_string, 4);
	wxPopupTransientWindow::Popup();
}

void EventPopup::on_text_entered(wxCommandEvent& event) {
	Dismiss();
}

void EventPopup::on_show(wxShowEvent& event) {
	if (event.IsShown() == false) {
		current_track->set_tempo_event(editing_event_tick, get_float_from_string((string)tempo_field->GetValue(), 0.01, 10.0));
		current_channel->set_instrument_event(editing_event_tick, (string)instrument_field->GetValue());
		current_channel->set_pitch_event(editing_event_tick, get_float_from_string((string)pitch_field->GetValue(), 0.0, 2.0));
		current_channel->set_volume_event(editing_event_tick, get_float_from_string((string)volume_field->GetValue(), 0.0, 1.0));
		GetParent()->Refresh();
		GetParent()->Update();
	}
}

