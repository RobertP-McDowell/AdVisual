#include <ComposerPanel.h>
#include <math.h>

using namespace Gtk;

int cursor_tick = 0;
int cursor_end = 0;

static vec2 note_size;
static vec2 cell_size;

ComposerPanel::ComposerPanel() {
	current_track = new Track();
	set_name("composer");
	insert_row(0);
	insert_row(0);
	insert_row(0);
	insert_column(0);
	insert_column(0);
	insert_column(0);
	note_size = vec2(20.0 / zoom, 17.0 / zoom);
	cell_size = vec2(20.0 / zoom, 20.0 / zoom);

	event_header = make_unique<EventHeader>();
	event_header->show();
	attach(*event_header, 1, 0);
	grid_panel = make_unique<GridPanel>();
	grid_panel->set_expand(true);
	grid_panel->show();
	attach(*grid_panel, 1, 1);
}

void EventPopup::init_spin_field(Grid& grid, string text, int entry_idx, SpinButton& spinbox) {
	Label label(text, Align::START);
	grid.attach(label, 0, entry_idx);
	spinbox.set_range(0.0, 10.0);
	spinbox.set_increments(0.1, 0.5);
	spinbox.add_css_class("field");
	grid.attach(spinbox, 1, entry_idx);
}

EventPopup::EventPopup() {
	set_size_request(100, 100);
	set_name("event-popup");
	signal_closed().connect(mem_fun(*this, &EventPopup::on_closed));
	Box main_box(Orientation::HORIZONTAL, 4);
	set_child(main_box);
	Grid grid;
	grid.insert_row(0);
	grid.insert_column(0);
	grid.insert_column(0);
	main_box.append(grid);
	grid.set_vexpand(true);
	// Initialize each event field.
	tempo_field = SpinButton(0.2, 2);
	tempo_field.set_name("field-tempo");
	init_spin_field(grid, "Tempo", 0, tempo_field);
	// Initialize Instrument event field.
	Label ins_label("Instrument", Align::START);
	grid.attach(ins_label, 0, 1);
	instrument_field.set_name("field-instrument");
	instrument_field.add_css_class("field");
	grid.attach(instrument_field, 1, 1);
	pitch_field = SpinButton(0.2, 2);
	volume_field = SpinButton(0.2, 2);
	pitch_field.set_name("field-pitch");
	volume_field.set_name("field-volume");
	init_spin_field(grid, "Pitch", 2, pitch_field);
	init_spin_field(grid, "Volume", 3, volume_field);
}

#define update_event_field(event_field, get_last_event_callable, t_type, type_conversion_func, max_text_length) do { \
	int last_event_tick; \
	t_type last_event_value; \
	get_last_event_callable(editing_tick, last_event_tick, last_event_value); \
	if (last_event_tick == editing_tick) { \
		string new_text = type_conversion_func(last_event_value); \
		new_text.resize(max_text_length); \
		event_field.set_text(new_text); \
		get_last_event_callable(editing_tick - 1, last_event_tick, last_event_value); \
	} \
	else { \
		event_field.set_text(""); \
	} \
	string new_hint = type_conversion_func(last_event_value); \
	new_hint.resize(max_text_length); \
	/* event_field.set_placeholder_text(new_hint); */ \
} while(0)

void EventPopup::popup(int at_tick) {
	editing_tick = at_tick;
	update_event_field(tempo_field, current_track->get_last_tempo_event, float, to_string, 4);
	update_event_field(instrument_field, current_channel->get_last_instrument_event, string, , 9);
	update_event_field(pitch_field, current_channel->get_last_pitch_event, float, to_string, 4);
	update_event_field(volume_field, current_channel->get_last_volume_event, float, to_string, 4);
	Popover::popup();
}

void EventPopup::on_closed() {
	if (!tempo_field.get_text().empty())
	current_track->set_tempo_event(editing_tick, clamp(tempo_field.get_value(), 0.01, 10.0));
	if (!instrument_field.get_text().empty())
	current_channel->set_instrument_event(editing_tick, (string)instrument_field.get_text());
	if (!pitch_field.get_text().empty())
	current_channel->set_pitch_event(editing_tick, clamp(pitch_field.get_value(), 0.0, 2.0));
	if (!volume_field.get_text().empty())
	current_channel->set_volume_event(editing_tick, clamp(volume_field.get_value(), 0.0, 1.0));
	get_parent()->queue_draw();
}

EventHeader::EventHeader() {
	set_name("event-header");
	set_draw_func(sigc::mem_fun(*this, &EventHeader::on_draw));
	set_size_request(-1, cell_size.y * 1.5);
	rmb_gesture = GestureClick::create();
	rmb_gesture->set_button(GDK_BUTTON_SECONDARY);
	rmb_gesture->signal_pressed().connect(mem_fun(*this, &EventHeader::on_rmb_down));
	//rmb_gesture->signal_released().connect(mem_fun(*this, &EventHeader::on_rmb_up));
	add_controller(rmb_gesture);
	event_popup.set_parent(*this);
	signal_channel_changed.connect(mem_fun(*this, &EventHeader::queue_draw));
}


void EventHeader::on_rmb_down(int n_press, double x, double y) {
	int tick = int(x / cell_size.x);
	event_popup.popup(tick);
	event_popup.set_pointing_to(Gdk::Rectangle(tick * cell_size.x, 0, cell_size.x, get_height()));
}

GridPanel::GridPanel() {
	set_name("grid-panel");
	lmb_gesture = Gtk::GestureClick::create();
	lmb_gesture->set_button(GDK_BUTTON_PRIMARY);
	lmb_gesture->signal_pressed().connect(mem_fun(*this, &GridPanel::on_lmb_down));
	lmb_gesture->signal_released().connect(mem_fun(*this, &GridPanel::on_lmb_up));
	add_controller(lmb_gesture);

	rmb_gesture = Gtk::GestureClick::create();
	rmb_gesture->set_button(GDK_BUTTON_SECONDARY);
	rmb_gesture->signal_pressed().connect(mem_fun(*this, &GridPanel::on_rmb_down));
	rmb_gesture->signal_released().connect(mem_fun(*this, &GridPanel::on_rmb_up));
	add_controller(rmb_gesture);
	set_draw_func(sigc::mem_fun(*this, &GridPanel::on_draw));
	auto motion_event = Gtk::EventControllerMotion::create();
	motion_event->signal_motion().connect(mem_fun(*this, &GridPanel::on_mouse_motion));
	add_controller(motion_event);
	signal_channel_changed.connect(mem_fun(*this, &GridPanel::queue_draw));
}

void GridPanel::on_lmb_down(int n_press, double x, double y) {
	mouse_down_start = vec2(x, y);
	ghost_note = make_unique<Note>(x / cell_size.x, y / cell_size.x, 1);
	ghost_note->offset = mouse_down_start.x / cell_size.x;
	ghost_note->pitch = mouse_down_start.y / cell_size.y;
	ghost_note->length = 1;
	queue_draw();
}

void GridPanel::on_lmb_up(int n_press, double x, double y) {
	if (ghost_note != nullptr) {
		current_channel->add_note(*ghost_note);
	}
	ghost_note = nullptr;
	queue_draw();
}

void GridPanel::on_rmb_down(int n_press, double x, double y) {
	cursor_tick = x / cell_size.x;
	cursor_end = cursor_tick;
	queue_draw();
}

void GridPanel::on_rmb_up(int n_press, double x, double y) {
	cursor_end = x / cell_size.x;
	queue_draw();
}

void GridPanel::on_mouse_motion(double x, double y) {
	vec2 local_mouse_position = vec2(x + scroll_offset.x, y + scroll_offset.y);
	if (lmb_gesture->get_current_button() == GDK_BUTTON_PRIMARY) {
		int start_offset = (mouse_down_start.x / cell_size.x);
		int end_offset = (local_mouse_position.x / cell_size.x);
		//wxRect old_note_rect = get_note_rect(*editing_note); // We will need to update this if the note gets shorter.
		if (end_offset >= start_offset) {
			ghost_note->length = (end_offset - start_offset) + 1;
			ghost_note->offset = start_offset;
		}
		else {
			ghost_note->length = (start_offset - end_offset) + 1;
			ghost_note->offset = end_offset;
		}
		//wxRect note_rect = get_note_rect(*editing_note);
		//if (note_rect.x == old_note_rect.x && note_rect.width == old_note_rect.width) {
		//	return; // There is no change, dont update.
		//}
		//wxRect final_rect(0, note_rect.y, 0, note_rect.height);
		//line_exclusion(note_rect.x, note_rect.width, old_note_rect.x, old_note_rect.width, final_rect.x, final_rect.width);
		//grid_panel->RefreshRect(final_rect, true);
		queue_draw();
	}
	if (rmb_gesture->get_current_button() == GDK_BUTTON_SECONDARY) {
		cursor_end = x / cell_size.x;
		queue_draw();
	}
	//status_bar->SetStatusText(note_number_to_letter(local_mouse_position.y / cell_size.y));
}

void cairo_round_rect(const shared_ptr<Cairo::Context>& cr, int x, int y, int width, int height, int radius) {
	cr->arc(x+radius,       y+radius,        radius, M_PI,         3.0*M_PI/2.0);
	cr->arc(x+width-radius, y+radius,        radius, 3.0*M_PI/2.0, 0);
	cr->arc(x+width-radius, y+height-radius, radius, 0,            M_PI/2.0);
	cr->arc(x+radius,       y+height-radius, radius, M_PI/2.0,     M_PI);
	cr->close_path();
}

void GridPanel::on_draw(const shared_ptr<Cairo::Context>& cr, int width, int height) {
	//vec2 cell_size(20, 20);
	double grid_width = 2.5;
	double measure_width = 3.5;
	RGBA grid_color = RGBA(.5, .5, .5, 1.0);
	RGBA measure_color = RGBA(.8, .8, .8, 1.0);

	// Start drawing Grid.
	double vertical_dash = (cell_size.y / 3.0) * 2.0; // Subtract pen_width because the rounded cap protrudes
	vector<double> vertical_dashes = {vertical_dash, cell_size.y - vertical_dash};
	double horizontal_dash = (cell_size.x / 3.0) * 2.0; // Subtract pen_width because the rounded cap protrudes
	vector<double> horizontal_dashes = {horizontal_dash, cell_size.x - horizontal_dash};

	int ticks_per_measure = current_track->beats_per_measure * current_track->ticks_per_beat;
	vec2 max_offstep = vec2(cell_size.x * ticks_per_measure, cell_size.y * full_octave);
	vec2 draw_offstep = vec2(scroll_offset.x % max_offstep.x, scroll_offset.y % max_offstep.y);

	int columns = (get_width() / cell_size.x);
	int rows = min(pitch_range, get_height() / cell_size.y);
	int grid_sub = 0;
	double middle_c_y = cell_size.y * 50;
	Gdk::Cairo::set_source_rgba(cr, grid_color);
	cr->set_dash(horizontal_dashes, 0);
	cr->set_line_width(grid_width);
	// Draw dashed line for every sharp note.
	for (int i = 0; i <= rows; i++) {
		if (i % 7 == 4 || i % 7 == 0) {
			grid_sub++;
			continue;
		}
		int y = (cell_size.y * 2.0 * (i-(grid_sub/2.0))) + (cell_size.y /2.0);
		cr->move_to(0, y);
		cr->line_to(get_width(), y);
		cr->stroke();
	}
	// Draw solid line per measure.
	for (int measure = 0; measure <= columns; measure += ticks_per_measure) {
		cr->unset_dash();
		cr->set_line_width(measure_width);
		Gdk::Cairo::set_source_rgba(cr, measure_color);
		double x = measure * cell_size.x;
		cr->move_to(x - draw_offstep.x, 0);
		cr->line_to(x - draw_offstep.x, get_height());
		cr->stroke();
		// Draw dashed line per beat.
		for (int beat = current_track->ticks_per_beat; beat < ticks_per_measure; beat += current_track->ticks_per_beat) {
			cr->set_dash(vertical_dashes, 0);
			cr->set_line_width(grid_width);
			Gdk::Cairo::set_source_rgba(cr, grid_color);
			x = (measure + beat) * cell_size.x;
			cr->move_to(x - draw_offstep.x, -draw_offstep.y);
			cr->line_to(x - draw_offstep.x, get_height());
			cr->stroke();
		}
	}

	RGBA ghost_note_color = RGBA(0.5, 0.5, 0.5, 0.7);
	cr->unset_dash();
	// Note pen setup.
	cr->set_line_width(note_size.y / 4.0);
	//cr->set_fill_rule(Cairo::FillRule::EVEN
	vec2 note_visdiff = vec2((note_size.x - cell_size.x) / 2.0, (note_size.y - cell_size.x) / 2.0);
	// Notes.
	for (Note& note : current_channel->notes) {
		cairo_round_rect(cr, note.offset * cell_size.x, (note.pitch * cell_size.y) - note_visdiff.y,
						note.length * cell_size.x, note_size.y, note_size.y / 4.0);
		Gdk::Cairo::set_source_rgba(cr, RGBA(0, 0, 0, 1));
		cr->fill_preserve();
		Gdk::Cairo::set_source_rgba(cr, current_channel->color);
		cr->stroke();
	}
	// Start drawing Notes.
	// Ghost Note.
	if (ghost_note != nullptr) {
		Gdk::Cairo::set_source_rgba(cr, ghost_note_color);
		cr->set_line_width(cell_size.y);
		int y = (ghost_note->pitch * cell_size.y) + (cell_size.y / 2.0);
		cr->move_to(ghost_note->offset * cell_size.x, y);
		cr->line_to((ghost_note->offset + ghost_note->length) * cell_size.x, y);
		cr->stroke();
	}
	// Start drawing Cursor & Selection.
	int cursor_real_x = cursor_tick * cell_size.x;
	int cursor_end_x = cursor_end * cell_size.x;
	Gdk::Cairo::set_source_rgba(cr, RGBA(0.6, 0.4, 0.4, 0.5));
	cr->rectangle(cursor_real_x, 0, cursor_end_x - cursor_real_x, height);
	cr->fill();
	
	Gdk::Cairo::set_source_rgba(cr, RGBA(1.0, 1.0, 1.0, 1.0));
	cr->set_line_width(cell_size.x / 4);
	cr->move_to(cursor_real_x, 0);
	cr->line_to(cursor_real_x, height);
	cr->stroke();
	Gdk::Cairo::set_source_rgba(cr, RGBA(0.7, 0.7, 0.7, 1.0));
	cr->set_line_width(cell_size.x / 4);
	cr->move_to(cursor_end_x, 0);
	cr->line_to(cursor_end_x, height);
	cr->stroke();
}

void EventHeader::on_draw(const shared_ptr<Cairo::Context>& cr, int width, int height) {
	using Gdk::Cairo::set_source_rgba;
	int draw_offstep = scroll_offset % note_size.x;
	vec2 event_bar_size = vec2(cell_size.x, height / 4.0);
	vector<int16_t> event_ticks;
	
	cr->set_line_width(1.25);
	set_source_rgba(cr, RGBA(0.4, 0.4, 0.4));
	
	for (int column = 0; column <= width / note_size.x; column++) {
		int x = (column * note_size.x) - draw_offstep;
		cr->move_to(x, 0);
		cr->line_to(x, height);
		cr->stroke();
	}
	// Draw bottom line.
	cr->move_to(0, height);
	cr->line_to(width, height);
	cr->stroke();

	// Tempo events.
	for (auto event = current_track->tempo_events.begin(); event != current_track->tempo_events.end(); event++) {
		int event_on_grid = (event->first * note_size.x);
		if (event_on_grid > width) break;
		if (event_on_grid < -(note_size.x * 20)) continue;
		cr->rectangle(event_on_grid, 0, event_bar_size.x, event_bar_size.y);
		set_source_rgba(cr, RGBA(1.0, 1.0, 1.0));
		cr->fill_preserve();
		set_source_rgba(cr, RGBA(0.0, 0.0, 0.0));
		cr->stroke();
		event_ticks.push_back(event->first);
	}
	// Instruments event.
	for (auto event = current_channel->instrument_events.begin(); event != current_channel->instrument_events.end(); event++) {
		int event_on_grid = (event->first * note_size.x) - scroll_offset;
		if (event_on_grid > width) break;
		if (event_on_grid < -(note_size.x * 20)) continue;
		cr->rectangle(event_on_grid, event_bar_size.y, event_bar_size.x, event_bar_size.y);
		set_source_rgba(cr, RGBA(1.0, 0.15, 0.15));
		cr->fill_preserve();
		set_source_rgba(cr, RGBA(0.0, 0.0, 0.0));
		cr->stroke();
		event_ticks.push_back(event->first);
	}
	for (auto event = current_channel->pitch_events.begin(); event != current_channel->pitch_events.end(); event++) {
		int event_on_grid = (event->first * note_size.x) - scroll_offset;
		if (event_on_grid > width) break;
		if (event_on_grid < -(note_size.x * 20)) continue;
		cr->rectangle(event_on_grid, event_bar_size.y * 2, event_bar_size.x, event_bar_size.y);
		set_source_rgba(cr, RGBA(0.15, 1.0, 0.15));
		cr->fill_preserve();
		set_source_rgba(cr, RGBA(0.0, 0.0, 0.0));
		cr->stroke();
		event_ticks.push_back(event->first);
	}
	for (auto event = current_channel->volume_events.begin(); event != current_channel->volume_events.end(); event++) {
		int event_on_grid = (event->first * note_size.x) - scroll_offset;
		if (event_on_grid > width) break;
		if (event_on_grid < -(note_size.x * 20)) continue;
		cr->rectangle(event_on_grid, event_bar_size.y * 3, event_bar_size.x, event_bar_size.y);
		set_source_rgba(cr, RGBA(0.15, 0.15, 1.0));
		cr->fill_preserve();
		set_source_rgba(cr, RGBA(0.0, 0.0, 0.0));
		cr->stroke();
		event_ticks.push_back(event->first);
	}
}