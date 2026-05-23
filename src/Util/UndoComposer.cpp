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
	UndoNotes::undo();
	for (Note& note : oldest_notes.notes) {
		channel->add_note(note);
	}
	new_cursor_start = ComposerPanel::selection_start();
	new_cursor_end = ComposerPanel::selection_end();
	cursor_tick = old_cursor_start;
	cursor_end = old_cursor_end;
}

void UndoSelection::redo() {
	UndoNotes::redo();
	if (redo_command & RedoCommand::ERASE_OLD_SELECTION) {
		channel->erase_notes(old_cursor_start, old_cursor_end - old_cursor_start);
	}
	cursor_tick = new_cursor_start;
	cursor_end = new_cursor_end;
}

void UndoSelection::setup_for_continue(int p_old_cursor_start, int p_old_cursor_end, NoteGroup p_old_notes) {
	if (oldest_notes.notes.empty()) {
		DBPRINT("Starting continuous undo.");
		old_cursor_start = p_old_cursor_start;
		old_cursor_end = p_old_cursor_end;
		oldest_notes = p_old_notes;
	}
	else {
		DBPRINT("Continuing continuous undo.");
		undo();
	}
	old_notes = p_old_notes;
}
