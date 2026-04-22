#include <ComposerPanel.h>
#include <math.h>

using namespace Gtk;

ComposerPanel::ComposerPanel() {
	set_name("composer");
	insert_row(0);
	insert_row(0);
	insert_row(0);
	insert_column(0);
	insert_column(0);
	insert_column(0);
	note_size = vec2(20.0 / zoom, 19.0 / zoom);
	cell_size = vec2(20.0 / zoom, 20.0 / zoom);
	
	current_channel_idx = 0;
	current_track = make_unique<Track>();
	current_channel = &(current_track->channels[0]);
	
	event_header = make_unique<EventHeader>();
	event_header->show();
	event_header->queue_draw();
	event_header->set_expand(true);
	attach(*event_header, 1, 0);
	grid_panel = make_unique<GridPanel>();
	grid_panel->show();
	grid_panel->queue_draw();
	grid_panel->set_expand(true);
	attach(*grid_panel, 1, 1);
	current_track = make_unique<Track>();
}

EventHeader::EventHeader() {
	set_draw_func(sigc::mem_fun(*this, &EventHeader::on_draw));
}

void EventHeader::on_draw(const shared_ptr<Cairo::Context>& cr, int width, int height) {
using namespace Gtk;
	cr->set_source_rgba(0.8, 0.0, 0.0, 0.5);
	cr->fill();
	cr->move_to(0, 0);
	cr->line_to(1000, 500);
	cr->stroke();
}

GridPanel::GridPanel() {
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
	note_size = vec2(20.0, 19.0);
	cell_size = vec2(20.0, 20.0);
	auto motion_event = Gtk::EventControllerMotion::create();
	motion_event->signal_motion().connect(mem_fun(*this, &GridPanel::on_mouse_motion));
	add_controller(motion_event);
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
		current_channel->notes = {};
		//current_channel->add_note(*ghost_note);
	}
	ghost_note = nullptr;
	//queue_draw();
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
		
	}
	//status_bar->SetStatusText(note_number_to_letter(local_mouse_position.y / cell_size.y));
}

void GridPanel::on_rmb_down(int n_press, double x, double y) {
}

void GridPanel::on_rmb_up(int n_press, double x, double y) {
}

void stroke_round_rect(const shared_ptr<Cairo::Context>& cr, int x, int y, int width, int height, int radius) {
	//cr->arc(x+r, y+r, r, M_PI
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

	RGBA ghost_note_color = RGBA(0.5, 0.5, 0.0, 1.0);
	cr->unset_dash();
	cr->set_line_width(1.0);
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
	// Notes.
}

