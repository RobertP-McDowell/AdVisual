#pragma once
#include <Util/UndoCommand.h>
#include <Track.h>

struct UndoNotes : public UndoCommand {
	enum RedoCommand {
		ERASE_OLD = 1,
		WRITE_NEW = 2,
		ERASE_NEW = 4,
		ERASE_OLD_GAP = 8,
		MAKE_NEW_GAP = 16
	};
	UndoNotes(Reason p_reason, int p_redo_command = RedoCommand::WRITE_NEW) :
			redo_command(p_redo_command), UndoCommand(p_reason) {}
	NoteGroup new_notes;
	NoteGroup old_notes;
	int redo_command;
	bool insert_mode = false;
	void set_insert_mode(bool value) { insert_mode = value; }
	bool get_insert_mode() const { return insert_mode; }
	virtual void undo() override;
	virtual void redo() override;
};

struct UndoSelection : public UndoNotes {
	UndoSelection(Reason p_reason, int p_redo_command = RedoCommand::WRITE_NEW) :
			UndoNotes(p_reason, p_redo_command) {}
	NoteGroup oldest_notes;
	int old_selection_start = 0;
	int old_selection_end = 0;
	int new_selection_start = 0;
	int new_selection_end = 0;
	void set_selection(int p_old_start, int p_old_end, int p_new_offset) {
		old_selection_start = p_old_start;
		old_selection_end = p_old_end;
		new_selection_start = p_old_start + p_new_offset;
		new_selection_end = p_old_end + p_new_offset;
	}
	void set_old_selection(int p_old_start, int p_old_end) {
		old_selection_start = p_old_start;
		old_selection_end = p_old_end;
	}
	void set_new_selection(int p_new_start, int p_new_end) {
		new_selection_start = p_new_start;
		new_selection_end = p_new_end;
	}
	int get_tick_offset() const { return new_selection_start - old_selection_start; }
	virtual void undo() override;
	virtual void redo() override;
	void setup_for_continue(int p_old_selection_start, int p_old_selection_end, NoteGroup p_old_notes);
protected:
};
