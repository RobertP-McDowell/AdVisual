#include <ComposerPanel.h>
#include <wx/scrolbar.h>
#include <wx/gbsizer.h>

////////////////////////////////////////////////////////////////////////////
// ComposerPanel Functions /////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

ComposerPanel::ComposerPanel(wxWindow *parent) : 
	wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
{
	SetBackgroundColour(*wxBLACK);
	event_font = wxFont(wxFontInfo(10).Bold());
	note_size = wxSize(20.0 / zoom, 19.0 / zoom);
	cell_size = wxSize(20.0 / zoom, 20.0 / zoom);

	current_channel_idx = 0;
	current_track = make_unique<Track>();
	current_channel = &(current_track->channels[0]);

	event_header = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(100, 30));
	event_header->SetBackgroundColour(panel_bg);
	event_header->Bind(wxEVT_PAINT, &ComposerPanel::on_paint_event_header, this);
	event_header->Bind(wxEVT_MOTION, &ComposerPanel::on_mouse_motion_event_header, this);
	event_header->Bind(wxEVT_LEFT_DOWN, &ComposerPanel::on_lmb_down_event_header, this);
	event_header->Bind(wxEVT_LEFT_UP, &ComposerPanel::on_lmb_up_event_header, this);

	grid_panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(100, 100));
	grid_panel->SetBackgroundColour(panel_bg);

	grid_panel->Bind(wxEVT_PAINT, &ComposerPanel::on_paint_grid, this);
	grid_panel->Bind(wxEVT_LEFT_DOWN, &ComposerPanel::on_lmb_down, this);
	grid_panel->Bind(wxEVT_LEFT_UP, &ComposerPanel::on_lmb_up, this);
	grid_panel->Bind(wxEVT_RIGHT_DOWN, &ComposerPanel::on_rmb_down, this);
	grid_panel->Bind(wxEVT_RIGHT_UP, &ComposerPanel::on_rmb_up, this);
	grid_panel->Bind(wxEVT_MOTION, &ComposerPanel::on_mouse_motion, this);

	wxGridBagSizer* sizer = new wxGridBagSizer();
	sizer->SetCols(3);
	sizer->SetRows(3);
	sizer->SetFlexibleDirection(wxBOTH);
	sizer->AddGrowableCol(0, 0);
	sizer->AddGrowableCol(1, 1);
	sizer->AddGrowableRow(0, 0);
	sizer->AddGrowableRow(1, 1);
	sizer->Add(1, 1, wxGBPosition(0, 0), wxGBSpan(1, 1), wxEXPAND);
	sizer->Add(event_header, wxGBPosition(0, 1), wxGBSpan(1, 1), wxEXPAND | wxBOTTOM, 4);
	piano_ctrl = new PianoControl(this, wxID_ANY, wxHORIZONTAL);
	sizer->Add(piano_ctrl, wxGBPosition(1, 0), wxGBSpan(1, 1), wxEXPAND);
	sizer->Add(grid_panel, wxGBPosition(1, 1), wxGBSpan(1, 1), wxEXPAND);

	h_scrollbar = new wxScrollBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_HORIZONTAL);
	v_scrollbar = new wxScrollBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);
	h_scrollbar->Bind(wxEVT_SCROLL_THUMBTRACK, &ComposerPanel::on_scroll_grid_horizontal, this);
	v_scrollbar->Bind(wxEVT_SCROLL_THUMBTRACK, &ComposerPanel::on_scroll_grid_vertical, this);
	h_scrollbar->SetBackgroundColour(scrollbar_bg);
	v_scrollbar->SetBackgroundColour(scrollbar_bg);
	h_scrollbar->SetForegroundColour(scrollbar_fg);
	v_scrollbar->SetForegroundColour(scrollbar_fg);

	sizer->Add(v_scrollbar, wxGBPosition(0, 2), wxGBSpan(2, 1), wxEXPAND);
	sizer->Add(h_scrollbar, wxGBPosition(2, 0), wxGBSpan(1, 3), wxEXPAND); // h_scroll_bar gets the extra cell here.

	Bind(wxEVT_SIZE, &ComposerPanel::on_resize, this);
	Bind(wxEVT_MOUSEWHEEL, &ComposerPanel::on_mouse_wheel, this);
	Bind(wxEVT_KEY_DOWN, &ComposerPanel::on_key_down, this);

	SetSizerAndFit(sizer);
	sizer->FitInside(GetParent());

	event_popup = new EventPopup(this);
}

void ComposerPanel::on_resize(wxSizeEvent& event) {
	update_scrollbars();
	event.Skip();
}

void ComposerPanel::on_scroll_grid_horizontal(wxScrollEvent& event) {
	grid_offset.x = cell_size.x * event.GetPosition();
	// Update piano_ctrl to play current instrument:
	int last_ins_event_tick = 0;
	string last_ins_event_value;
	current_channel->get_last_instrument_event(grid_offset.x, last_ins_event_tick, last_ins_event_value);
	piano_ctrl->SetInstrument(current_bank->find_instrument(wxString(last_ins_event_value)));
	grid_panel->Refresh();
	grid_panel->Update();
	event_header->Refresh();
	event_header->Update();
}

void ComposerPanel::on_scroll_grid_vertical(wxScrollEvent& event) {
	grid_offset.y = cell_size.y * event.GetPosition();
	piano_ctrl->SetScrollOffset(grid_offset.y);
	grid_panel->Refresh();
	grid_panel->Update();
}

void ComposerPanel::update_scrollbars() {
	int h_thumbsize = GetSize().x / cell_size.x;
	int h_range = current_track->get_tick_count() + (GetSize().x / cell_size.x);
	h_scrollbar->SetScrollbar(h_scrollbar->GetThumbPosition(), h_thumbsize, h_range, h_thumbsize, true);
	int v_thumbsize = grid_panel->GetSize().y / cell_size.y;
	v_scrollbar->SetScrollbar(v_scrollbar->GetThumbPosition(), v_thumbsize, pitch_range, v_thumbsize);
}
void ComposerPanel::move_h_scrollbar(int new_pos) {
	h_scrollbar->SetThumbPosition(new_pos);
	wxScrollEvent event(wxEVT_SCROLL_THUMBTRACK, h_scrollbar->GetId(), h_scrollbar->GetThumbPosition(), wxHORIZONTAL);
	on_scroll_grid_horizontal(event);
}
void ComposerPanel::move_v_scrollbar(int new_pos) {
	v_scrollbar->SetThumbPosition(new_pos);
	wxScrollEvent event(wxEVT_SCROLL_THUMBTRACK, v_scrollbar->GetId(), v_scrollbar->GetThumbPosition(), wxVERTICAL);
	on_scroll_grid_vertical(event);
}
void ComposerPanel::on_mouse_wheel(wxMouseEvent& event) {
	int wheel_difference = (event.GetWheelRotation() > 0 ? 1 : -1) * scroll_multiplier;
	if (event.ShiftDown()) {
		move_h_scrollbar(h_scrollbar->GetThumbPosition() - wheel_difference);
	}
	else {
		move_v_scrollbar(v_scrollbar->GetThumbPosition() - wheel_difference);
	}
}

void ComposerPanel::SetPreviewChannels(bool value) {
	preview_channels = value;
	grid_panel->Refresh();
	grid_panel->Update();
}

void ComposerPanel::SetChannelIndex(int channel) {
	current_channel_idx = channel;
	current_channel = current_track->GetChannel(current_channel_idx);
	grid_panel->Refresh();
	grid_panel->Update();
	event_header->Refresh();
	event_header->Update();
}

void ComposerPanel::on_paint_grid(wxPaintEvent& event) {
	draw_grid();
	draw_notes();
}

void ComposerPanel::draw_grid() {
	wxPaintDC dc(grid_panel);
	wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
	if (gc == nullptr) return;
// Create grid pens.
	double pen_width = 2.5;
	wxColour grid_color = wxColour(140, 140, 140);
	wxColour measure_colour = wxColour(190, 190, 190);
	wxDash vertical_dash = wxDash((((cell_size.y / 3.0) * 2.0) - pen_width) / 2.0); // Subtract pen_width because the rounded cap protrudes
	wxDash vertical_dashes[2] = {vertical_dash, wxDash(((cell_size.y / 2.0) - (vertical_dash)) - (pen_width / 2.0))};
	wxDash horizontal_dash = wxDash((((cell_size.x / 3.0) * 2.0) - pen_width) / 2.0); // Subtract pen_width because the rounded cap protrudes
	wxDash horizontal_dashes[2] = {horizontal_dash, wxDash(((cell_size.x / 2.0) - (horizontal_dash)) - (pen_width / 2.0))};
	wxGraphicsPen heavy_grid_pen = gc->CreatePen(wxGraphicsPenInfo(measure_colour).Width(3.5).Style(wxPENSTYLE_SOLID));
	wxGraphicsPen vgrid_pen = gc->CreatePen(wxGraphicsPenInfo(grid_color).Width(pen_width).Style(wxPENSTYLE_USER_DASH).Dashes(2, &vertical_dashes[0]));
	wxGraphicsPen hgrid_pen = gc->CreatePen(wxGraphicsPenInfo(grid_color).Width(pen_width).Style(wxPENSTYLE_USER_DASH).Dashes(2, &horizontal_dashes[0]));
// Specify for loop/grid measurements.
	int ticks_per_measure = current_track->beats_per_measure * current_track->ticks_per_beat;
	wxPoint max_offstep = wxPoint(cell_size.x * ticks_per_measure, cell_size.y * full_octave);
	wxPoint draw_offstep = wxPoint(grid_offset.x % max_offstep.x, grid_offset.y % max_offstep.y);
	int panel_width;
	int panel_height;
	GetSize(&panel_width, &panel_height);
	int columns = (panel_width / cell_size.x);
	int rows = min(pitch_range, panel_height / cell_size.y);
	int grid_sub = 0;
	double middle_c_y = cell_size.y * 50;
// Horizontal grid.
	gc->SetPen(hgrid_pen);
	for (int i = 0; i <= rows; i++) {
		if (i % 7 == 4 || i % 7 == 0)  {
			grid_sub++;
			continue;
		}
		double y = (cell_size.y * 2.0 * (i-(grid_sub/2.0))) + (cell_size.y / 2.0);
		gc->StrokeLine(horizontal_dash - (pen_width / 2.0), y - draw_offstep.y, panel_width, y - draw_offstep.y);
	}
	gc->SetPen(heavy_grid_pen);
	gc->StrokeLine(0, grid_offset.y + middle_c_y, panel_width, grid_offset.y + middle_c_y);
	for (int measure = 0; measure <= columns; measure += ticks_per_measure) {
		// Draw solid line per measure.
		gc->SetPen(heavy_grid_pen);
		double x = cell_size.x * measure;
		gc->StrokeLine(x - draw_offstep.x, 0, x - draw_offstep.x, panel_height);
		// Draw dashed line per beat.
		for (int beat = current_track->ticks_per_beat; beat < ticks_per_measure; beat += current_track->ticks_per_beat) {
			gc->SetPen(vgrid_pen);
			x = cell_size.x * (measure + beat);
			gc->StrokeLine(	x - draw_offstep.x, (vertical_dash - (pen_width / 2.0)) - draw_offstep.y,
							x - draw_offstep.x, (panel_height + max_offstep.y) - draw_offstep.y);
		}
	}
	delete gc;
}

void ComposerPanel::draw_notes() {
	wxPaintDC dc(grid_panel);
	wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
	if (gc == nullptr) return;
	int ticks_per_measure = current_track->beats_per_measure * current_track->ticks_per_beat;
	int panel_width;
	int panel_height;
	GetSize(&panel_width, &panel_height);
	wxSize middle_of_cell = cell_size / 2;
	double note_pen_width = note_size.y / 4.0;
	wxSize size_diff = cell_size - note_size;
	wxGraphicsPen note_pen = gc->CreatePen(wxGraphicsPenInfo(
	current_channel->colour).Width(note_pen_width).Style(wxPENSTYLE_SOLID).Cap(wxCAP_BUTT).Join(wxJOIN_BEVEL));
	gc->SetPen(note_pen);
	gc->SetBrush(*wxBLACK_BRUSH);
	for (Note& note : current_channel->notes) {
		gc->DrawRoundedRectangle((note.offset * cell_size.x) + 1 - grid_offset.x, ((note.pitch * cell_size.y) + 1) - grid_offset.y,
			(note.length * note_size.x) - 2, note_size.y, note_size.y / 4.0);
	}
	wxGraphicsPen ghost_pen = gc->CreatePen(wxGraphicsPenInfo(
	wxColour(215, 200, 255, 100)).Width(note_size.y + note_pen_width).Style(wxPENSTYLE_SOLID).Cap(wxCAP_BUTT).Join(wxJOIN_BEVEL));
	if (editing_note != nullptr) {
		gc->SetPen(ghost_pen);
		wxRect cell_rect = get_note_rect(*editing_note);
		gc->StrokeLine(cell_rect.x, cell_rect.y + (cell_rect.height / 2), cell_rect.x + cell_rect.width, cell_rect.y + (cell_rect.height / 2));
	}

	// Draw Preview notes.
	if (preview_channels) {
		for (int i = 0; i < current_track->get_channel_count(); i++) {
			if (i == current_channel_idx) {
				continue;
			}
			Channel* channel = current_track->GetChannel(i);
			gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(
			channel->colour).Width(note_size.y / 8.0).Style(wxPENSTYLE_SOLID).Cap(wxCAP_BUTT).Join(wxJOIN_BEVEL)));
			int line_y_offset = ((note_size.y / 8.0) * i) + note_pen_width + 1;
			for (Note& note : channel->notes) {
				gc->StrokeLine((note.offset * cell_size.x) + note_pen_width, (note.pitch * cell_size.y) + line_y_offset - grid_offset.y,
				((note.offset + note.length) * cell_size.x) - note_pen_width, (note.pitch * cell_size.y) + line_y_offset - grid_offset.y);
			}
		}
	}

	// Draw Cursor/Selection.
	gc->SetBrush(wxColour(100, 100, 100, 100));
	gc->SetPen(wxNullPen);
	gc->DrawRectangle((cursor_tick * cell_size.x) - grid_offset.x, 0, ((cursor_end - cursor_tick) * cell_size.x) - grid_offset.x, panel_height);
	wxPen heavy_cursor_pen = *wxWHITE_PEN;
	heavy_cursor_pen.SetWidth(3.5);
	gc->SetPen(heavy_cursor_pen); // Make cursor_tick heavier to differentiate from cursor_end.
	gc->StrokeLine((cursor_tick * cell_size.x) - grid_offset.x, 0, (cursor_tick * cell_size.x) - grid_offset.x, panel_height);
	gc->SetPen(*wxWHITE_PEN);
	gc->StrokeLine((cursor_end * cell_size.x) - grid_offset.x, 0, (cursor_end * cell_size.x) - grid_offset.x, panel_height);

	delete gc;
}

void ComposerPanel::on_lmb_down(wxMouseEvent& event) {
	SetFocus();
	mouse_down_start = event.GetPosition() + grid_offset;
	editing_note = make_unique<Note>();
	editing_note->offset = mouse_down_start.x / cell_size.x;
	editing_note->pitch = mouse_down_start.y / cell_size.y;
	editing_note->length = 1;
	if (grid_audio_feedback) {
		adplayer->play_note(editing_note->pitch, current_channel_idx, piano_ctrl->GetInstrument());
	}
	refresh_ticks(editing_note->offset, editing_note->offset + editing_note->length, 0);
	grid_panel->Update();
}

void ComposerPanel::on_lmb_up(wxMouseEvent& event) {
	if (editing_note != nullptr) {
		current_channel->add_note(*editing_note);
	}
	refresh_ticks(editing_note->offset, editing_note->offset + editing_note->length);
	editing_note = nullptr;
	update_scrollbars();
	adplayer->play_note(0, current_channel_idx, piano_ctrl->GetInstrument());
	grid_panel->Update();
}

void ComposerPanel::on_rmb_down(wxMouseEvent& event) {
	SetFocus();
	mouse_down_start = event.GetPosition() + grid_offset;
	editing_note = make_unique<Note>();
	int new_cursor_pos = mouse_down_start.x / cell_size.x;
	// If shift is down then the user just wants to move cursor_end, so skip cursor_tick.
	if (!event.ShiftDown()) {
		cursor_tick = new_cursor_pos;
	}
	cursor_end = new_cursor_pos;
	refresh_selected_ticks();
	grid_panel->Update();
}

void ComposerPanel::on_rmb_up(wxMouseEvent& event) {
	refresh_selected_ticks();
	grid_panel->Update();
}

void ComposerPanel::refresh_selected_ticks(int margin) {
	int selection_start = (cursor_tick <= cursor_end ? cursor_tick : cursor_end);
	int selection_end = (cursor_tick <= cursor_end ? cursor_end : cursor_tick);
	wxRect selection_rect(selection_start * cell_size.x, 0, (selection_end - selection_start) * cell_size.x, grid_panel->GetSize().y);
	selection_rect.x -= margin * cell_size.x;
	selection_rect.width += (margin * cell_size.x) + wxWHITE_PEN->GetWidth();
	grid_panel->RefreshRect(selection_rect, true);
}

void ComposerPanel::refresh_ticks(int start_tick, int end_tick, int margin) {
	wxRect refresh_rect(start_tick * cell_size.x, 0, (end_tick - start_tick) * cell_size.x, grid_panel->GetSize().y);
	refresh_rect.x -= margin * cell_size.x;
	refresh_rect.width += margin * cell_size.x;
	grid_panel->RefreshRect(refresh_rect, true);
}

void ComposerPanel::on_mouse_motion(wxMouseEvent& event) {
	wxPoint local_mouse_position = event.GetPosition() + grid_offset;
	if (event.LeftIsDown() && editing_note != nullptr) {
		int start_offset = (mouse_down_start.x / cell_size.x);
		int end_offset = (local_mouse_position.x / cell_size.x);
		wxRect old_note_rect = get_note_rect(*editing_note); // We will need to update this if the note gets shorter.
		if (end_offset >= start_offset) {
			editing_note->length = (end_offset - start_offset) + 1;
			editing_note->offset = start_offset;
		}
		else {
			editing_note->length = (start_offset - end_offset) + 1;
			editing_note->offset = end_offset;
		}
		wxRect note_rect = get_note_rect(*editing_note);
		if (note_rect.x == old_note_rect.x && note_rect.width == old_note_rect.width) {
			return; // There is no change, dont update.
		}
		wxRect final_rect(0, note_rect.y, 0, note_rect.height);
		line_exclusion(note_rect.x, note_rect.width, old_note_rect.x, old_note_rect.width, final_rect.x, final_rect.width);
		grid_panel->RefreshRect(final_rect, true);
		grid_panel->Update();
	}
	else if (event.RightIsDown() && cursor_end != -1) {
		int new_cursor_end = (local_mouse_position.x / cell_size.x);
		int selection_start = (cursor_tick <= cursor_end ? cursor_tick : cursor_end);
		int selection_end = (cursor_tick <= cursor_end ? cursor_end : cursor_tick);
		int new_selection_start = (cursor_tick <= new_cursor_end ? cursor_tick : new_cursor_end);
		int new_selection_end = (cursor_tick <= new_cursor_end ? new_cursor_end : cursor_tick);
		wxRect selection_rect(0, 0, 0, grid_panel->GetSize().y);
		line_exclusion(selection_start, selection_end - selection_start, new_selection_start,
			new_selection_end - new_selection_start, selection_rect.x, selection_rect.width);
		cursor_end = new_cursor_end;
		selection_rect.x = (selection_rect.x * cell_size.x);
		selection_rect.width = (selection_rect.width * cell_size.x) + wxWHITE_PEN->GetWidth();
		grid_panel->RefreshRect(selection_rect, true);
		grid_panel->Update();
	}
	status_bar->SetStatusText(note_number_to_letter(local_mouse_position.y / cell_size.y));
}

void ComposerPanel::copy_notes() {
	copy_buffer.clear();
	if (cursor_tick - cursor_end == 0) {
		return; // Return if no range is selected.
	}
	int selection_start = (cursor_tick <= cursor_end ? cursor_tick : cursor_end);
	int selection_end = (cursor_tick <= cursor_end ? cursor_end : cursor_tick);
	for (int i = 0; i < current_channel->notes.size(); i++) {
		Note& note = current_channel->notes[i];
		if (note.offset < selection_end && note.offset + note.length > selection_start) {
			Note copy_note = note;
			copy_note.offset -= selection_start;
			if (copy_note.offset < 0) {
				copy_note.offset = 0;
			}
			if (copy_note.length > selection_end - selection_start) {
				copy_note.length = selection_end - selection_start;
			}
			copy_buffer.push_back(copy_note);
		}
	}
	if (!copy_buffer.empty()) {
		copy_buffer_start_offset = selection_start - copy_buffer[0].offset;
		copy_buffer_length = selection_end - selection_start;
	}
}

void ComposerPanel::paste_notes() {
	int ins_pos;
	int selection_start = (cursor_tick <= cursor_end ? cursor_tick : cursor_end);
	int selection_length = (cursor_tick <= cursor_end ? cursor_end - cursor_tick : cursor_tick - cursor_end);
	// Erase notes before pasting.
	current_channel->erase_notes(selection_start, selection_length, ins_pos);
	for (Note new_note : copy_buffer) {
		new_note.offset += selection_start;
		current_channel->add_note(new_note);
	}
	refresh_ticks(selection_start, selection_start + selection_length);
}

void ComposerPanel::on_key_down(wxKeyEvent& event) {
	int selection_start = (cursor_tick <= cursor_end ? cursor_tick : cursor_end);
	int selection_length = (cursor_tick <= cursor_end ? cursor_end - cursor_tick : cursor_tick - cursor_end);
	int ins_pos; // generic used for erasing notes.
	switch (event.GetKeyCode()) {
	case 67: // 'c' keycode. TODO: temporary solution, should be in on_char event function.
		copy_notes();
		break;
	case 86: // 'v' keycode.
		paste_notes();
		break;
	case 88: // 'x' keycode.
		copy_notes();
		current_channel->erase_notes(selection_start, selection_length, ins_pos);
		refresh_selected_ticks();
		break;
	case WXK_BACK: // Erase contents of selection.
		current_channel->erase_notes(selection_start, selection_length, ins_pos);
		refresh_selected_ticks();
		break;
	case WXK_RETURN: // Chop selection (selection is erased, and everything past cursor end gets repositioned back to cursor start).
		break;
	case WXK_ESCAPE:
		if (cursor_tick != cursor_end) { // Unset selection if there is one;
			cursor_end = cursor_tick;
		}
		else {                            // Otherwise move cusor back to beginning.
			cursor_tick = 0;
			cursor_end = 0;
		}
		refresh_selected_ticks();
		break;
	case WXK_LEFT:
		cursor_end -= 1;
		break;
	case WXK_RIGHT:
		cursor_end += 1;
		break;
	}
	grid_panel->Update();
}

void ComposerPanel::on_paint_event_header(wxPaintEvent& event) {
	wxPaintDC dc(event_header);
	wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
	if (gc == nullptr) return;
	int panel_width;
	int panel_height;
	GetSize(&panel_width, &panel_height);
	int event_header_width;
	int event_header_height;
	int draw_offstep = grid_offset.x % note_size.x;
	event_header->GetSize(&event_header_width, &event_header_height);
	wxSize event_bar_size = wxSize(cell_size.x, event_header_height / 4.0);
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
	editing_event_tick = (event.GetPosition().x + grid_offset.x) / cell_size.x;
	wxPoint popup_pos = wxPoint(editing_event_tick * cell_size.x, event_header->GetSize().y) + event_header->GetPosition();
	event_popup->Position(ClientToScreen(popup_pos) - event_popup->GetSize(), event_popup->GetSize());
	event_popup->Popup(editing_event_tick);
	event_header->Refresh();
	event_header->Update();
}

///////////////////////////////////////////////////////////////////////////////////
// EventPopup Functions ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

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

EventPopup::EventPopup(wxWindow* parent) : 
		wxPopupTransientWindow(parent, wxBORDER_DEFAULT | wxPU_CONTAINS_CONTROLS) {
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

void EventPopup::Popup(int at_tick) {
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

wxRect ComposerPanel::get_note_rect(const Note& note) const {
	return wxRect((editing_note->offset * cell_size.x) - grid_offset.x,
	((editing_note->pitch * cell_size.y) - grid_offset.y) - ((cell_size.y - note_size.y) + 1),
	editing_note->length * cell_size.x,
	cell_size.y + ((cell_size.y - note_size.y) * 2) + 2);
}
