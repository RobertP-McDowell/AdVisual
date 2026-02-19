#include <Track.h>


const wxColour ChannelColours[11] = { wxColour(255, 100, 100), wxColour(100, 255, 100), wxColour(100, 100, 255), wxColour(140, 40, 208),
		wxColour(255, 100, 255), wxColour(255, 255, 100), wxColour(100, 255, 255), wxColour(255, 255, 255),
		wxColour(255, 255, 255), wxColour(255, 255, 255), wxColour(255, 255, 255) };

Track::Track() {
	for (int i = 0; i < 11; i++) {
		shared_ptr<Channel> new_channel = make_shared<Channel>();
		new_channel->colour = ChannelColours[i];
		channels.push_back(new_channel);
	}
}

void Channel::erase_notes(int eraser_offset, int eraser_length, int& insert_position) {
	insert_position = notes.size();
	for (int i = notes.size() - 1; i >= 0; i--) {
		Note& note = notes[i];
		if (note.offset < eraser_offset + eraser_length && note.offset + note.length > eraser_offset) {
			insert_position = i;
			if (note.offset < eraser_offset) {
				Note note_slice1(note.offset, note.pitch, eraser_offset - note.offset);
				notes.insert(notes.begin() + i+1, note_slice1);
				insert_position = i + 1;
			}
			if (note.offset + note.length > eraser_offset + eraser_length) {
				Note note_slice2(eraser_offset + eraser_length, note.pitch, (note.offset + note.length) - (eraser_offset + eraser_length));
				notes.insert(notes.begin() + insert_position + 1, note_slice2);
			}
			notes.erase(notes.begin() + i);
		}
	}
}

void Channel::add_note(Note new_note) {
	int insert_position = notes.size();
	erase_notes(new_note.offset, new_note.length, insert_position);
	notes.insert(notes.begin() + insert_position, new_note);
}

Note::Note() {}
Note::Note(int _offset, int _pitch, int _length) : offset(_offset), pitch(_pitch), length(_length) {}

