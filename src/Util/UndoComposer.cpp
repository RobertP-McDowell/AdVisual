#include <Util/UndoComposer.h>
#include <ComposerPanel.h>

void UndoNotes::undo() {
	for (Note& note : new_notes.notes) {
		channel->erase_notes(note.offset, note.length);
	}
	for (Note& note : old_notes.notes) {
		channel->add_note(note);
	}
}
void UndoNotes::redo() {
	if (redo_command & RedoCommand::ERASE_OLD) {
		for (Note& note : old_notes.notes) {
			channel->erase_notes(note.offset, note.length);
		}
	}
	if (redo_command & RedoCommand::WRITE_NEW) {
		for (Note& note : new_notes.notes) {
			channel->add_note(note);
		}
	}
	else if (redo_command & RedoCommand::ERASE_NEW) {
		for (Note& note : new_notes.notes) {
			channel->erase_notes(note.offset, note.length);
		}
	}
}

void UndoSelection::undo() {
	int old_selection_length = old_cursor_end - old_cursor_start;
	int new_selection_length = new_cursor_end - new_cursor_start;
	for (Note& note : new_notes.notes) {
		channel->erase_notes(note.offset, note.length);
	}
	if (insert_mode) {
		if (redo_command & RedoCommand::ERASE_OLD_GAP) {
			channel->notes.make_gap(old_cursor_start, old_selection_length);
		}
		if (redo_command & RedoCommand::MAKE_NEW_GAP) {
			channel->notes.erase_gap(new_cursor_start, new_selection_length);
		}
	}
	for (Note& note : old_notes.notes) {
		channel->add_note(note);
	}
	for (Note& note : oldest_notes.notes) {
		channel->add_note(note);
	}
	cursor_tick = old_cursor_start;
	cursor_end = old_cursor_end;
}

void UndoSelection::redo() {
	int old_selection_length = old_cursor_end - old_cursor_start;
	int new_selection_length = new_cursor_end - new_cursor_start;
	if (insert_mode) {
		if (redo_command & RedoCommand::ERASE_OLD_GAP) {
			current_channel->notes.erase_gap(old_cursor_start, old_selection_length);
		}
		if (redo_command & RedoCommand::MAKE_NEW_GAP) {
			current_channel->notes.make_gap(new_cursor_start, new_selection_length);
		}
	}
	else { // We still need to at least erase where a closed gap would be.
		if (redo_command & RedoCommand::ERASE_OLD_GAP) {
			cout << "AAA\n";
			current_channel->notes.erase(old_cursor_start, old_selection_length);
		}
	}
	UndoNotes::redo();
	cursor_tick = new_cursor_start;
	cursor_end = new_cursor_end;
}

void UndoSelection::setup_for_continue(int p_old_cursor_start, int p_old_cursor_end, NoteGroup p_old_notes) {
	if (oldest_notes.notes.empty()) {
		DBPRINT("Starting continuous undo.");
		old_cursor_start = p_old_cursor_start;
		old_cursor_end = p_old_cursor_end;
		new_cursor_start = p_old_cursor_start;
		new_cursor_end = p_old_cursor_end;
		oldest_notes = p_old_notes;
	}
	else {
		DBPRINT("Continuing continuous undo.");
		//undo();
	}
	old_notes = p_old_notes;
}
