#include <ComposerPanel.h>
#include <AdPlayer.h>
#include <Util/MetaDataWindow.h>
#include <math.h>

using namespace Gtk;

int cursor_tick = 0;
int selection1 = 0;
int selection2 = 0;

static const vec2 default_note_size = vec2(20.0, 17.0);
static const vec2 default_cell_size = vec2(20.0, 20.0);

static vec2 note_size;
static vec2 cell_size;
sigc::signal<void()> ComposerPanel::signal_cursor_moved;
array<UndoBuffer<UndoNotes*>, 11> ComposerPanel::composer_undo;

ComposerPanel::ComposerPanel() {
	current_track = new Track();
	set_name("composer");
	insert_row(0);
	insert_row(0);
	insert_row(0);
	insert_column(0);
	insert_column(0);
	insert_column(0);
	note_size = vec2(default_note_size.x / zoom, default_note_size.y / zoom);
	cell_size = vec2(default_cell_size.x / zoom, default_cell_size.y / zoom);
// Init event_header.
	event_header = make_managed<EventHeader>();
	attach(*event_header, 1, 0);
// Init grid_panel.
	grid_panel = make_managed<GridPanel>();
	grid_panel->set_expand(true);
	attach(*grid_panel, 1, 1);
// Init instrument_hint.
	instrument_hint = make_managed<Label>("piano1");
	attach(*instrument_hint, 0, 0);
// Init piano_ctrl.
	piano_ctrl = make_managed<PianoCtrl>(true);
	attach(*piano_ctrl, 0, 1);
// Init scrollbars.
	grid_panel->hadjust = Adjustment::create(0, 0, 100, 1, 10, 10);
	grid_panel->hadjust->signal_value_changed().connect(mem_fun(*this, &ComposerPanel::on_hscroll));
	hscrollbar = make_managed<Scrollbar>(grid_panel->hadjust, Orientation::HORIZONTAL);

	shared_ptr<Adjustment> vadjust = Adjustment::create(0, 0, pitch_range, 1, 1, 0);
	vscrollbar = make_managed<Scrollbar>(vadjust, Orientation::VERTICAL);
	vadjust->signal_value_changed().connect(mem_fun(*this, &ComposerPanel::on_vscroll));
	vadjust->set_value(middle_c - ((720 / cell_size.y) / 2.0)); // Go to center of grid.
	vadjust->set_page_size(700 / cell_size.y);
	attach(*hscrollbar, 0, 2, 3, 1);
	attach(*vscrollbar, 2, 0, 1, 2);

	scroll_controller = EventControllerScroll::create();
	scroll_controller->set_flags(EventControllerScroll::Flags::BOTH_AXES);
	scroll_controller->signal_scroll().connect(mem_fun(*this, &ComposerPanel::on_mouse_scroll), true);
	add_controller(scroll_controller);
// Setup action group.
	action_group = Gio::SimpleActionGroup::create();
	action_group->add_action_bool("toggle_insert_mode", mem_fun(*this, &ComposerPanel::toggle_insert_mode), false);
	action_group->add_action("cut_selection", mem_fun(*this, &ComposerPanel::cut));
	action_group->add_action("copy_selection", mem_fun(*this, &ComposerPanel::copy));
	action_group->add_action("paste_selection", mem_fun(*this, &ComposerPanel::paste));
	action_group->add_action("erase_selection", mem_fun(*this, &ComposerPanel::erase_selection));
	action_group->add_action("move_selection_semitone_up", bind(mem_fun(*this, &ComposerPanel::move_selection_semitone), -1));
	action_group->add_action("move_selection_semitone_down", bind(mem_fun(*this, &ComposerPanel::move_selection_semitone), 1));
	action_group->add_action("move_selection_tick_left", bind(mem_fun(*this, &ComposerPanel::move_selection_tick), -1));
	action_group->add_action("move_selection_tick_right", bind(mem_fun(*this, &ComposerPanel::move_selection_tick), 1));
	action_group->add_action("move_selection_measure_left", bind(mem_fun(*this, &ComposerPanel::move_selection_tick), -16));
	action_group->add_action("move_selection_measure_right", bind(mem_fun(*this, &ComposerPanel::move_selection_tick), 16));
	action_group->add_action("unselect", mem_fun(*this, &ComposerPanel::unselect));
	action_group->add_action("undo", mem_fun(*this, &ComposerPanel::undo));
	action_group->add_action("redo", mem_fun(*this, &ComposerPanel::redo));
	action_group->add_action("show_track_settings", mem_fun(*this, &ComposerPanel::show_track_settings));
// Set shortcuts.
	app->set_accel_for_action("composer.cut_selection", "<Ctrl>x");
	app->set_accel_for_action("composer.copy_selection", "<Ctrl>c");
	app->set_accel_for_action("composer.paste_selection", "<Ctrl>v");
	app->set_accels_for_action("composer.erase_selection", {"Delete", "BackSpace"});
	app->set_accel_for_action("composer.move_selection_semitone_up", "Up");
	app->set_accel_for_action("composer.move_selection_semitone_down", "Down");
	app->set_accel_for_action("composer.move_selection_tick_left", "Left");
	app->set_accel_for_action("composer.move_selection_tick_right", "Right");
	app->set_accel_for_action("composer.move_selection_measure_left", "<Ctrl>Left");
	app->set_accel_for_action("composer.move_selection_measure_right", "<Ctrl>Right");
	app->set_accel_for_action("composer.unselect", "Escape");
	app->set_accel_for_action("composer.undo", "<Ctrl>z");
	app->set_accel_for_action("composer.redo", "<Ctrl><Shift>z");
	track_settings.signal_visible_change.connect(mem_fun(*grid_panel, &GridPanel::queue_draw));
	signal_track_changed.connect(mem_fun(*this, &ComposerPanel::on_track_changed));
	signal_channel_changed.connect(mem_fun(*this, &ComposerPanel::on_channel_changed));
	signal_bank_changed.connect(mem_fun(*this, &ComposerPanel::on_channel_changed));
}

void ComposerPanel::set_cursor_tick(int p_new_tick) {
	cursor_tick = p_new_tick;
	signal_cursor_moved.emit();
}

void ComposerPanel::toggle_insert_mode() {
	insert_mode = !insert_mode;
	current_undo()->set_continuous(false);
	action_group->change_action_state("toggle_insert_mode", Glib::Variant<bool>::create(insert_mode));
}

void ComposerPanel::on_track_changed() {
	for (UndoBuffer<UndoNotes*>& i : composer_undo) {
		i.clear(); // clear undo buffers.
	}
	current_undo()->set_continuous(false);
	on_channel_changed();
}

void ComposerPanel::on_channel_changed() {
	update_piano_ctrl();
	current_undo()->set_continuous(false);
}

bool ComposerPanel::is_unsaved() {
	bool saved = true;
	for (UndoBuffer<UndoNotes*>& i : composer_undo) {
		saved &= i.is_saved();
	}
	return saved;
}

void ComposerPanel::on_track_saved() {
	for (UndoBuffer<UndoNotes*>& i : composer_undo) {
		i.set_saved();
	}
}

void ComposerPanel::cut() {
	if (selection_length() == 0) { return; }
	copy();
	erase_selection();
}
void ComposerPanel::copy() {
	if (selection_length() == 0) { return; }
	copy_buffer = current_channel->copy(selection_start(), selection_end());
	copy_buffer.trim(selection_start(), selection_end());
	copy_buffer_start_tick = selection_start();
	copy_buffer_length = selection_length();
}
void ComposerPanel::paste() {
	if (copy_buffer.notes.empty()) { return; }
	UndoSelection* new_undo = new UndoSelection(UndoCommand::Reason::PASTE_SELECTION,
		UndoNotes::RedoCommand::WRITE_NEW | UndoNotes::RedoCommand::MAKE_NEW_GAP);
	new_undo->old_notes = current_channel->copy(selection_start(), selection_end());
	new_undo->set_insert_mode(insert_mode);
	new_undo->set_old_selection(selection_start(), selection_end());
	new_undo->set_new_selection(selection_start(), selection_length() != 0 ? selection_end() : selection_start() + copy_buffer_length);
	new_undo->new_notes = copy_buffer;
	new_undo->new_notes.offset_tick(selection_start() - copy_buffer_start_tick);
	new_undo->new_notes.trim(selection_start(), new_undo->new_selection_end);
	new_undo->redo();
	current_undo()->add_undo(new_undo);
	grid_panel->update_grid();
}

void ComposerPanel::erase_selection() {
	UndoSelection* new_undo = new UndoSelection(UndoCommand::Reason::ERASE_SELECTION,
		UndoNotes::RedoCommand::ERASE_OLD_GAP);
	new_undo->set_insert_mode(insert_mode);
	new_undo->set_old_selection(selection_start(), selection_end());
	new_undo->set_new_selection(selection_start(), selection_start());
	new_undo->old_notes = current_channel->copy(selection_start(), selection_end());
	new_undo->redo();
	current_undo()->add_undo(new_undo);
	grid_panel->update_grid();
}
void ComposerPanel::move_selection_semitone(int pitch_offset) {
	UndoNotes* new_undo = new UndoNotes(UndoCommand::Reason::MOVE_PITCH, UndoNotes::RedoCommand::WRITE_NEW);
	new_undo->old_notes = current_channel->copy(selection_start(), selection_end());
	if (selection_length() == 0 || new_undo->old_notes.notes.empty()) { return; }
	new_undo->new_notes = new_undo->old_notes;
	new_undo->new_notes.trim(selection_start(), selection_end());
	new_undo->new_notes.offset_pitch(pitch_offset);
	current_channel->paste(&new_undo->new_notes, selection_start(), selection_length(), selection_start());
	current_undo()->add_undo(new_undo);
	grid_panel->update_grid();
}
void ComposerPanel::move_selection_tick(int tick_offset) {
	if (selection_length() == 0) {
		selection1 = max(0, selection1 + tick_offset); selection2 = selection1;
		grid_panel->update_grid(); return;
	}
	UndoSelection* new_undo = new UndoSelection(UndoCommand::Reason::MOVE_TICKS,
		UndoNotes::RedoCommand::WRITE_NEW | UndoNotes::RedoCommand::MAKE_NEW_GAP | UndoNotes::RedoCommand::ERASE_OLD_GAP);
	if (current_undo()->get_continuous() && new_undo->match_reason(current_undo()->get_last_undo_reason())) {
		UndoSelection* prior_undo = static_cast<UndoSelection*>(current_undo()->get_current_undo());
		*new_undo = *prior_undo;
		undo();
	}
	new_undo->set_insert_mode(insert_mode);
	tick_offset += new_undo->get_tick_offset();
	int range_affected_start = (tick_offset < 0 ? selection_start() + tick_offset - 1 : selection_start() - 1);
	int range_affected_end = (tick_offset > 0 ? selection_end() + tick_offset + 1 : selection_end() + 1);
	new_undo->setup_for_continue(selection_start(), selection_end(), current_channel->copy(range_affected_start, range_affected_end));
	int sel_start = selection_start(); // We need to make sure selection doesn't go negative.
	selection1 = (selection1 + tick_offset) - min(0, sel_start + tick_offset);
	selection2 = (selection2 + tick_offset) - min(0, sel_start + tick_offset);
	new_undo->set_new_selection(selection_start(), selection_end());
	new_undo->new_notes = new_undo->oldest_notes;
	new_undo->new_notes.trim(new_undo->old_selection_start, new_undo->old_selection_end);
	new_undo->new_notes.offset_tick(selection_start() - new_undo->old_selection_start);
	new_undo->redo();
	current_undo()->add_undo(new_undo, true);
	grid_panel->update_grid();
}
void ComposerPanel::unselect() {
	if (selection_length() == 0) {
		selection1 = 0;
	}
	selection2 = selection1;
	grid_panel->update_grid();
}

void ComposerPanel::undo() {
	current_undo()->undo();
	grid_panel->update_grid();
}
void ComposerPanel::redo() {
	current_undo()->redo();
	grid_panel->update_grid();
}

void ComposerPanel::show_track_settings() {
	track_settings.set_transient_for(*(app->get_run_window()));
	track_settings.show();
}

void ComposerPanel::on_show() {
	Gtk::Grid::on_show();
	vscrollbar->get_adjustment()->set_page_size(grid_panel->get_height() / cell_size.y);
}

void ComposerPanel::update_piano_ctrl() {
	int scroll_tick = hscrollbar->get_adjustment()->get_value();
	Instrument* last_ins = current_channel->get_instrument_at_tick(scroll_tick);
	instrument_hint->set_text((string)last_ins->name);
	piano_ctrl->set_instrument(last_ins);
	piano_ctrl->set_pitch_precision(current_channel->get_pitch_at_tick(scroll_tick));
	piano_ctrl->set_volume_multiplier(current_channel->get_volume_at_tick(scroll_tick));
	piano_ctrl->set_key_width(cell_size.y);
}

void ComposerPanel::on_hscroll() {
	int new_x = hscrollbar->get_adjustment()->get_value() * cell_size.x;
	event_header->scroll_offset = new_x;
	event_header->queue_draw();
	grid_panel->scroll_offset.x = new_x;
	grid_panel->queue_draw();
	update_piano_ctrl();
}

void ComposerPanel::on_vscroll() {
	int new_y = vscrollbar->get_adjustment()->get_value() * cell_size.y;
	grid_panel->scroll_offset.y = new_y;
	grid_panel->queue_draw();
	piano_ctrl->set_scroll_offset(new_y);
	vscrollbar->get_adjustment()->set_page_size(grid_panel->get_height() / cell_size.y);
}

bool ComposerPanel::on_mouse_scroll(double x, double y) {
	if (bool(scroll_controller->get_current_event_state() & Gdk::ModifierType::CONTROL_MASK)) {
		zoom += y * 0.1;
		note_size = vec2(default_note_size.x / zoom, default_note_size.y / zoom);
		cell_size = vec2(default_cell_size.x / zoom, default_cell_size.y / zoom);
		grid_panel->update_grid();
		update_piano_ctrl();
		return true;
	}
	if (bool(scroll_controller->get_current_event_state() & Gdk::ModifierType::SHIFT_MASK)) {
		// Need to setup Shift+Scroll for horizontal scrolling... and potentially vertical from horizontal?
		double old_x = x;
		x = y;
		y = old_x;
	}
	if (bool(scroll_controller->get_current_event_state() & Gdk::ModifierType::ALT_MASK)) {
		x *= current_track->ticks_per_beat;
		y *= full_octave;
	}
	vscrollbar->get_adjustment()->set_value(vscrollbar->get_adjustment()->get_value() + y);
	hscrollbar->get_adjustment()->set_value(hscrollbar->get_adjustment()->get_value() + x);
	return true;
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
	event_field.set_placeholder_text(new_hint); \
} while(0)

void EventPopup::init_spin_field(Grid& grid, string text, int entry_idx, Entry& spinbox) {
	Label label(text, Align::START);
	grid.attach(label, 0, entry_idx);
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
	tempo_field.set_name("field-tempo");
	init_spin_field(grid, "Tempo (mul)", 0, tempo_field);
	// Initialize Instrument event field.
	Label ins_label("Instrument", Align::START);
	grid.attach(ins_label, 0, 1);
	instrument_field.set_name("field-instrument");
	instrument_field.add_css_class("field");
	instrument_field.signal_changed().connect(mem_fun(*this, &EventPopup::on_instrument_field_text_changed));
	grid.attach(instrument_field, 1, 1);
	pitch_field.set_name("field-pitch");
	volume_field.set_name("field-volume");
	init_spin_field(grid, "Pitch (prec)", 2, pitch_field);
	init_spin_field(grid, "Volume (mul)", 3, volume_field);
	bank_ctrl.instrument_selected.connect(mem_fun(*this, &EventPopup::on_bank_ctrl_instrument_selected));
	main_box.append(bank_ctrl);
}

void EventPopup::popup(int at_tick) {
	editing_tick = at_tick;
	update_event_field(tempo_field, current_track->get_last_tempo_event, float, to_string, 4);
	update_event_field(instrument_field, current_channel->get_last_instrument_event, string, , 9);
	update_event_field(pitch_field, current_channel->get_last_pitch_event, float, to_string, 4);
	update_event_field(volume_field, current_channel->get_last_volume_event, float, to_string, 4);
	Popover::popup();
}

void EventPopup::on_closed() {
	current_track->set_tempo_event(editing_tick, get_float_from_string(tempo_field.get_text(), 0.0, 10.0));
	current_channel->set_instrument_event(editing_tick, instrument_field.get_text());
	current_channel->set_pitch_event(editing_tick, get_float_from_string(pitch_field.get_text(), 0.0, 2.0));
	current_channel->set_volume_event(editing_tick, get_float_from_string(volume_field.get_text(), 0.0, 1.0));
	get_parent()->queue_draw();
}

void EventPopup::on_instrument_field_text_changed() {
	string text = instrument_field.get_text();
	string_to_upper(text);
	bank_ctrl.search(text);
	if (text == instrument_field.get_text()) {
		return; // Don't change text if it's already uppercase.
	}
	int caret_pos = instrument_field.get_position();
	instrument_field.set_text(text);
	instrument_field.set_position(caret_pos);
}

void EventPopup::on_bank_ctrl_instrument_selected(Instrument* ins) {
	instrument_field.set_text((string)ins->name);
}

EventHeader::EventHeader() {
	set_name("event-header");
	set_draw_func(sigc::mem_fun(*this, &EventHeader::on_draw));
	set_size_request(-1, cell_size.y * 1.5);
	lmb_gesture = GestureClick::create();
	lmb_gesture->set_button(GDK_BUTTON_PRIMARY);
	lmb_gesture->signal_pressed().connect(mem_fun(*this, &EventHeader::on_lmb_down));
	add_controller(lmb_gesture);
	event_popup.set_parent(*this);
	signal_channel_changed.connect(mem_fun(*this, &EventHeader::queue_draw));
	signal_track_changed.connect(mem_fun(*this, &EventHeader::queue_draw));
}

void EventHeader::on_lmb_down(int n_press, double x, double y) {
	int tick = int((x + scroll_offset) / cell_size.x);
	event_popup.popup(tick);
	event_popup.set_pointing_to(Gdk::Rectangle((tick * cell_size.x) - scroll_offset, 0, cell_size.x, get_height()));
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
	signal_channel_changed.connect(mem_fun(*this, &GridPanel::update_grid));
	signal_track_changed.connect(mem_fun(*this, &GridPanel::update_grid));
	ComposerPanel::signal_cursor_moved.connect(mem_fun(*this, &GridPanel::update_grid));
}

void GridPanel::on_lmb_down(int n_press, double x, double y) {
	mouse_down_start = vec2(x + scroll_offset.x, y + scroll_offset.y);
	ghost_note = make_unique<Note>(x / cell_size.x, y / cell_size.x, 1);
	ghost_note->offset = mouse_down_start.x / cell_size.x;
	ghost_note->pitch = clamp(mouse_down_start.y / cell_size.y, 0, pitch_range-1);
	ghost_note->length = 1;
	if (audio_feedback) {
		int down_tick = x / cell_size.x;
		Instrument* last_ins = current_channel->get_instrument_at_tick(down_tick);
		adplayer->play_note(ghost_note->pitch, current_channel->channel_number, last_ins,
		current_channel->get_pitch_at_tick(down_tick), current_channel->get_volume_at_tick(down_tick));
	}
	status->set_text(note_number_to_letter(ghost_note->pitch) + " " +
		to_string(ghost_note->offset) + ":" + "1" + ", Shift+lmb to erase.");
	update_grid();
}

void GridPanel::on_lmb_up(int n_press, double x, double y) {
	if (ghost_note != nullptr) {
		if (bool(lmb_gesture->get_current_event_state() & Gdk::ModifierType::SHIFT_MASK)) {
			UndoNotes* undo = new UndoNotes(UndoNotes::Reason::ERASE_NOTE, UndoNotes::RedoCommand::ERASE_NEW);
			undo->old_notes = current_channel->copy(ghost_note->offset, ghost_note->get_end_offset());
			current_channel->erase_notes(ghost_note->offset, ghost_note->length);
			undo->new_notes = NoteGroup({*ghost_note});
			ComposerPanel::current_undo()->add_undo(undo);
		}
		else {
			UndoNotes* undo = new UndoNotes(UndoNotes::Reason::CREATE_NOTE, UndoNotes::RedoCommand::WRITE_NEW);
			undo->old_notes = current_channel->copy(ghost_note->offset, ghost_note->get_end_offset());
			current_channel->add_note(*ghost_note);
			undo->new_notes = NoteGroup({*ghost_note});
			ComposerPanel::current_undo()->add_undo(undo);
		}
	}
	ghost_note = nullptr;
	if (audio_feedback) {
		adplayer->play_note(-1, current_channel->channel_number, nullptr);
	}
	update_grid();
}

void GridPanel::on_rmb_down(int n_press, double x, double y) {
	ComposerPanel::current_undo()->set_continuous(false);
	selection1 = (x + scroll_offset.x) / cell_size.x;
	selection2 = selection1;
	update_grid();
}

void GridPanel::on_rmb_up(int n_press, double x, double y) {
	selection2 = (x + scroll_offset.x) / cell_size.x;
	update_grid();
}

void GridPanel::on_mouse_motion(double x, double y) {
	vec2 mouse_position = vec2(x + scroll_offset.x, y + scroll_offset.y);
	if (lmb_gesture->get_current_button() == GDK_BUTTON_PRIMARY) {
		int start_offset = max(0, mouse_down_start.x / cell_size.x);
		int end_offset = max(0, mouse_position.x / cell_size.x);
		if (end_offset >= start_offset) {
			ghost_note->length = (end_offset - start_offset) + 1;
			ghost_note->offset = start_offset;
		}
		else {
			ghost_note->length = (start_offset - end_offset) + 1;
			ghost_note->offset = end_offset;
		}
		queue_draw();
		status->set_text(note_number_to_letter(ghost_note->pitch) + " " +
			to_string(ghost_note->offset) + ":" + to_string(ghost_note->length) + ", Shift+lmb to erase.");
		return;
	}
	if (rmb_gesture->get_current_button() == GDK_BUTTON_SECONDARY) {
		selection2 = (x + scroll_offset.x) / cell_size.x;
		queue_draw();
		return;
	}
	status->set_text(note_number_to_letter(mouse_position.y / cell_size.y) + " " + to_string(mouse_position.x / cell_size.x));
}

void GridPanel::update_grid() {
	hadjust->set_upper(current_track->get_tick_count() +
		max(20, current_track->beats_per_measure * current_track->ticks_per_beat));
	queue_draw();
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
	double vdash_gap = cell_size.y - vertical_dash;
	vector<double> vertical_dashes = {vertical_dash, vdash_gap};
	double horizontal_dash = (cell_size.x / 3.0) * 2.0; // Subtract pen_width because the rounded cap protrudes
	double hdash_gap = cell_size.x - horizontal_dash;
	vector<double> horizontal_dashes = {horizontal_dash, hdash_gap};

	int ticks_per_measure = current_track->beats_per_measure * current_track->ticks_per_beat;
	vec2 max_offstep = vec2(cell_size.x * ticks_per_measure, cell_size.y * full_octave);
	vec2 draw_offstep = vec2(scroll_offset.x % max_offstep.x, scroll_offset.y % max_offstep.y);

	int columns = (width / cell_size.x);
	int rows = min(pitch_range, get_height() / cell_size.y);
	int grid_sub = 0;
	double middle_c_y = cell_size.y * 50;
	Gdk::Cairo::set_source_rgba(cr, grid_color);
	cr->set_dash(horizontal_dashes, -hdash_gap / 2.0);
	cr->set_line_width(grid_width);
	// Draw dashed line for every sharp note.
	for (int i = 0; i <= rows; i++) {
		if (i % 7 == 4 || i % 7 == 0) {
			grid_sub++;
			continue;
		}
		int y = ((cell_size.y * 2.0 * (i-(grid_sub/2.0))) + (cell_size.y /2.0)) - draw_offstep.y;
		cr->move_to(-draw_offstep.x, y);
		cr->line_to(width, y);
		cr->stroke();
	}
	// Draw solid line per measure.
	for (int measure = 0; measure <= columns + ticks_per_measure; measure += ticks_per_measure) {
		cr->unset_dash();
		cr->set_line_width(measure_width);
		Gdk::Cairo::set_source_rgba(cr, measure_color);
		double x = measure * cell_size.x;
		cr->move_to(x - draw_offstep.x, 0);
		cr->line_to(x - draw_offstep.x, height);
		cr->stroke();
		// Draw dashed line per beat.
		for (int beat = current_track->ticks_per_beat; beat < ticks_per_measure; beat += current_track->ticks_per_beat) {
			cr->set_dash(vertical_dashes, -vdash_gap / 2.0);
			cr->set_line_width(grid_width);
			Gdk::Cairo::set_source_rgba(cr, grid_color);
			x = (measure + beat) * cell_size.x;
			cr->move_to(x - draw_offstep.x, -draw_offstep.y);
			cr->line_to(x - draw_offstep.x, get_height());
			cr->stroke();
		}
	}
	// Draw selection.
	cr->set_dash(vertical_dashes, -vdash_gap / 2.0);
	cr->set_line_width(grid_width);
	int sel_start_x = (ComposerPanel::selection_start() * cell_size.x) - scroll_offset.x;
	int sel_end_x = (ComposerPanel::selection_end() * cell_size.x) - scroll_offset.x;
	Gdk::Cairo::set_source_rgba(cr, current_channel->color);
	cr->set_line_width(cell_size.x / 4);
	cr->move_to(sel_end_x, 0);
	cr->line_to(sel_end_x, height);
	cr->stroke();
	cr->move_to(sel_start_x, 0);
	cr->line_to(sel_start_x, height);
	cr->stroke();
	// Note pen setup.
	cr->unset_dash();
	double note_line_width = note_size.y / 4.0;
	cr->set_line_width(note_line_width);
	vec2 note_visdiff = vec2((note_size.x - cell_size.x) / 2.0, (note_size.y - cell_size.x) / 2.0);
	// Notes.
	for (Note& note : current_channel->notes) {
		cairo_round_rect(cr, (note.offset * cell_size.x) - scroll_offset.x, (note.pitch * cell_size.y) - note_visdiff.y - scroll_offset.y,
						note.length * cell_size.x, note_size.y, note_size.y / 4.0);
		Gdk::Cairo::set_source_rgba(cr, RGBA(0, 0, 0, 1));
		cr->fill_preserve();
		Gdk::Cairo::set_source_rgba(cr, current_channel->color);
		cr->stroke();
	}
	// Preview Notes.
	if (preview_channels) {
		for (int i = 0; i < current_track->get_channel_count(); i++) {
			if (i == current_channel->channel_number) {
				continue;
			}
			Channel* channel = current_track->get_channel(i);
			Gdk::Cairo::set_source_rgba(cr, channel->color);
			double preview_line_width = note_size.y / 8.0;
			cr->set_line_width(preview_line_width);
			int line_y_offset = (preview_line_width * i) + note_line_width + 1;
			for (Note& note : channel->notes) {
				cr->move_to((note.offset * cell_size.x) + note_line_width - scroll_offset.x,
						(note.pitch * cell_size.y) + line_y_offset - scroll_offset.y);
				cr->line_to(((note.offset + note.length) * cell_size.x) - note_line_width - scroll_offset.x,
						(note.pitch * cell_size.y) + line_y_offset - scroll_offset.y);
				cr->stroke();
			}
		}
	}
	// Ghost Note.
	RGBA ghost_note_color = RGBA(0.5, 0.5, 0.5, 0.7);
	if (ghost_note != nullptr) {
		Gdk::Cairo::set_source_rgba(cr, ghost_note_color);
		cr->set_line_width(cell_size.y);
		int y = ((ghost_note->pitch * cell_size.y) + (cell_size.y / 2.0)) - scroll_offset.y;
		cr->move_to((ghost_note->offset * cell_size.x) - scroll_offset.x, y);
		cr->line_to(((ghost_note->offset + ghost_note->length) * cell_size.x) - scroll_offset.x, y);
		cr->stroke();
	}
	// Draw selection overlay.
	if (ComposerPanel::selection_length() != 0) {
		Gdk::Cairo::set_source_rgba(cr, RGBA(0.4, 0.4, 0.4, 0.5));
		cr->rectangle(sel_start_x - (cell_size.x / 8) - 1, 0, (sel_end_x - sel_start_x) + (cell_size.x / 4) + 1, height);
		cr->fill();
	}
	// Draw the cursor.
	Gdk::Cairo::set_source_rgba(cr, RGBA(1.0, 1.0, 1.0, 1.0));
	cr->set_line_width(cell_size.x / 4);
	int cursor_start_x = (cursor_tick * cell_size.x) - scroll_offset.x;
	cr->move_to(cursor_start_x, 0);
	cr->line_to(cursor_start_x, height);
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
		int event_on_grid = (event->first * note_size.x) - scroll_offset;
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

TrackSettings::TrackSettings() {
	set_size_request(300, 100);
	set_name("track-settings");
	set_hide_on_close(true);
	set_modal(true);
	grid.insert_row(0);
	grid.insert_row(0);
	grid.insert_column(0);
	grid.insert_column(0);
	grid.set_expand(true);
	signal_track_changed.connect(mem_fun(*this, &TrackSettings::on_track_changed));

	tempo_spinner = SpinButton(0.2, 2);
	add_spinbox_property("Tempo (bpm)", tempo_spinner, 120, 0.1, 1000.0, 1, 10, 0);
	tempo_spinner.signal_value_changed().connect(mem_fun(*this, &TrackSettings::on_tempo_set));

	beats_per_measure_spinner = SpinButton(0.2, 0);
	add_spinbox_property("Beats per Measure", beats_per_measure_spinner, 4, 1, 1000.0, 1, 10, 1);
	beats_per_measure_spinner.signal_value_changed().connect(mem_fun(*this, &TrackSettings::on_beats_per_measure_set));

	ticks_per_beat_spinner = SpinButton(0.2, 0);
	add_spinbox_property("Ticks per Beat", ticks_per_beat_spinner, 4, 1, 1000.0, 1, 10, 2);
	ticks_per_beat_spinner.signal_value_changed().connect(mem_fun(*this, &TrackSettings::on_ticks_per_beat_set));

	Label percussion_label("Percussion");
	grid.attach(percussion_label, 0, 3);
	grid.attach(percussion_checkbox, 1, 3);
	percussion_checkbox.set_active(true);
	percussion_checkbox.signal_toggled().connect(mem_fun(*this, &TrackSettings::on_percussion_toggled));

	Button* meta_data_button = make_managed<Button>("Edit Meta");
	grid.attach(*meta_data_button, 0, 4, 2, 1);
	meta_data_button->signal_clicked().connect(mem_fun(*this, &TrackSettings::on_edit_meta_data));

	set_child(grid);
}

void TrackSettings::add_spinbox_property(string name, SpinButton& spinner, double val, double min, double max, double step, double page, int idx) {
	Label label = Label(name);
	spinner.set_range(min, max);
	spinner.set_increments(step, page);
	spinner.set_value(val);
	spinner.set_hexpand(true);
	grid.attach(label, 0, idx);
	grid.attach(spinner, 1, idx);
}

void TrackSettings::on_track_changed() {
	tempo_spinner.set_value(current_track->basic_tempo);
	beats_per_measure_spinner.set_value(current_track->beats_per_measure);
	ticks_per_beat_spinner.set_value(current_track->ticks_per_beat);
	percussion_checkbox.set_active(!current_track->melodic_mode);
}

void TrackSettings::on_tempo_set() {
	current_track->basic_tempo = tempo_spinner.get_value();
	signal_visible_change.emit();
}
void TrackSettings::on_beats_per_measure_set() {
	current_track->beats_per_measure = beats_per_measure_spinner.get_value();
	signal_visible_change.emit();
}
void TrackSettings::on_ticks_per_beat_set() {
	current_track->ticks_per_beat = ticks_per_beat_spinner.get_value();
	signal_visible_change.emit();
}
void TrackSettings::on_percussion_toggled() {
	current_track->melodic_mode = !percussion_checkbox.get_active();
	signal_visible_change.emit();
}

void TrackSettings::on_edit_meta_data() {
	if (meta_data_window.is_visible()) {
		meta_data_window.close();
		return;
	}
	meta_data_window = MetaDataWindow();
	meta_data_window.set_transient_for(*this);
	meta_data_window.set_title("Track Meta Data");
	meta_data_window.set_base_string(&current_track->meta_data);
	meta_data_window.set_max_length(39);
	meta_data_window.show();
}
