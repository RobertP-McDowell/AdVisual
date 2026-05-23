#pragma once

struct UndoCommand {
	enum Reason {
		TYPE_INVALID,
		REASON_INVALID,
		TYPE_UNDO_NOTES,
		CREATE_NOTE,
		ERASE_NOTE,
		ERASE_SELECTION,
		PASTE_SELECTION,
		TYPE_UNDO_SELECTION,
		MOVE_PITCH,
		MOVE_TICKS
	};
	virtual void undo() = 0;
	virtual void redo() = 0;
	Reason get_reason() const {
		return reason;
	}
	Reason get_type() const {
		if (reason >= Reason::TYPE_UNDO_SELECTION) { return TYPE_UNDO_SELECTION; }
		if (reason >= Reason::TYPE_UNDO_NOTES) { return TYPE_UNDO_NOTES; }
		if (reason >= Reason::TYPE_INVALID) { return TYPE_INVALID; }
	}
	bool match_reason(Reason compare_reason) {
		return reason == compare_reason;
	}
protected:
	Reason reason;
	UndoCommand() {}
	UndoCommand(Reason p_reason) : reason(p_reason) {}
};
