#include <Util/UndoComposer.h>
#include <ComposerPanel.h>

void UndoNotes::undo() {
	for (Note& note : new_notes.notes) {
		current_channel->erase_notes(note.offset, note.length);
	}
	for (Note& note : old_notes.notes) {
		current_channel->add_note(note);
	}
}
void UndoNotes::redo() {
	if (redo_command & RedoCommand::ERASE_OLD) {
		for (Note& note : old_notes.notes) {
			current_channel->erase_notes(note.offset, note.length);
		}
	}
	if (redo_command & RedoCommand::WRITE_NEW) {
		for (Note& note : new_notes.notes) {
			current_channel->add_note(note);
		}
	}
	else if (redo_command & RedoCommand::ERASE_NEW) {
		for (Note& note : new_notes.notes) {
			current_channel->erase_notes(note.offset, note.length);
		}
	}
}

void UndoSelection::undo() {
	int old_selection_length = old_selection_end - old_selection_start;
	int new_selection_length = new_selection_end - new_selection_start;
	for (Note& note : new_notes.notes) {
		current_channel->erase_notes(note.offset, note.length);
	}
	if (insert_mode) {
		if (redo_command & RedoCommand::ERASE_OLD_GAP) {
			current_channel->notes.make_gap(old_selection_start, old_selection_length);
		}
		if (redo_command & RedoCommand::MAKE_NEW_GAP) {
			current_channel->notes.erase_gap(new_selection_start, new_selection_length);
		}
	}
	for (Note& note : old_notes.notes) {
		current_channel->add_note(note);
	}
	for (Note& note : oldest_notes.notes) {
		current_channel->add_note(note);
	}
	selection1 = old_selection_start;
	selection2 = old_selection_end;
}

void UndoSelection::redo() {
	int old_selection_length = old_selection_end - old_selection_start;
	int new_selection_length = new_selection_end - new_selection_start;
	if (insert_mode) {
		if (redo_command & RedoCommand::ERASE_OLD_GAP) {
			current_channel->notes.erase_gap(old_selection_start, old_selection_length);
		}
		if (redo_command & RedoCommand::MAKE_NEW_GAP) {
			current_channel->notes.make_gap(new_selection_start, new_selection_length);
		}
	}
	else { // We still need to at least erase where a closed gap would be.
		if (redo_command & RedoCommand::ERASE_OLD_GAP) {
			current_channel->notes.erase(old_selection_start, old_selection_length);
		}
	}
	UndoNotes::redo();
	selection1 = new_selection_start;
	selection2 = new_selection_end;
}

void UndoSelection::setup_for_continue(int p_old_selection_start, int p_old_selection_end, NoteGroup p_old_notes) {
	if (oldest_notes.notes.empty()) {
		DBPRINT("Starting continuous undo.");
		old_selection_start = p_old_selection_start;
		old_selection_end = p_old_selection_end;
		new_selection_start = p_old_selection_start;
		new_selection_end = p_old_selection_end;
		oldest_notes = p_old_notes;
	}
	else {
		DBPRINT("Continuing continuous undo.");
		//undo();
	}
	old_notes = p_old_notes;
}
