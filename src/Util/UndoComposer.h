#pragma once
#include <Util/UndoCommand.h>
#include <Track.h>

struct UndoNotes : public UndoCommand {
	enum RedoCommand {
		ERASE_OLD = 1,
		WRITE_NEW = 2,
		ERASE_NEW = 4,
		ERASE_OLD_SELECTION = 8 // UndoSelection only.
	};
	UndoNotes(Channel* p_channel, Reason p_reason, int p_redo_command = RedoCommand::WRITE_NEW) :
			channel(p_channel), redo_command(p_redo_command), UndoCommand(p_reason) {}
	NoteGroup new_notes;
	NoteGroup old_notes;
	Channel* channel;
	int redo_command;
	virtual void undo() override;
	virtual void redo() override;
};

struct UndoSelection : public UndoNotes {
	UndoSelection(Channel* p_channel, Reason p_reason, int p_redo_command = RedoCommand::WRITE_NEW) :
			UndoNotes(p_channel, p_reason, p_redo_command) {}
	NoteGroup oldest_notes;
	int old_cursor_start = -1;
	int old_cursor_end = -1;
	int new_gap_start = 0;
	int new_gap_length = 0;
	virtual void undo() override;
	virtual void redo() override;
	void setup_for_continue(int p_old_cursor_start, int p_old_cursor_end, NoteGroup p_old_notes);
	int new_cursor_start = -1;
	int new_cursor_end = -1;
protected:
};
