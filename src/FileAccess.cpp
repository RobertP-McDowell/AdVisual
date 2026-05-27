#include <FileAccess.h>
#include <cstring>
#include <fstream>
#include <vector>
#include <common.h>

namespace FileAccess {
	fstream ios_file;
	binwstream file(&ios_file);
	bool writing = false;
	int file_length = 0;
};

int FileAccess::catch_libbinio_errors() {
	int error = file.error();
	if (error) {
		cerr << "Libbinio error code: " << error << ", at file positon: " << file.pos() << "\n";
		if (error == binio::Fatal) cerr << "Fatal error: an unspecified libbinio error occured.\n";
		else if (error == binio::Unsupported) cerr << "Fatal error: unsupported libbinio conversion.\n";
		else if (error == binio::NotOpen) cerr << "The file you tried to acces is not open yet\n";
		else if (error == binio::Denied) cerr << "Fatal error: denied access to file.\n";
		else if (error == binio::NotFound) cerr << "Fatal error: could not find file.\n";
		else if (error == binio::Eof) cerr << "Fatal error: reached end of file.\n";
		else cerr << "An unkown fatal error occured.\n";
		DBBREAKPOINT("Libbinio error");
		ios_file.close();
	}
	return error;
}

// Checks and calls fieldcpy_ functions will set object from read file, or get object and write to file.
void FileAccess::fieldcpy_char(char* object, int field_size) {
	if (writing == true) file.writeString(object, field_size);
	else file.readString(object, field_size);
	catch_libbinio_errors();
}

void FileAccess::fieldcpy_char(char* object, int field_size, char delimeter) {
	if (writing == true) file.writeString(object, field_size);
	else file.readString(object, field_size, delimeter);
	catch_libbinio_errors();
}

void FileAccess::fieldcpy_uint8(uint8_t* object, int field_size) {
	if (writing == true) file.writeInt(*object, field_size);
	else *object = file.readInt(1);
	catch_libbinio_errors();
}

void FileAccess::fieldcpy_uint16(uint16_t* object, int field_size) {
	if (writing == true) file.writeInt(*object, field_size);
	else *object = file.readInt(2);
	catch_libbinio_errors();
}

void FileAccess::fieldcpy_uint32(uint32_t* object, int field_size) {
	if (writing == true) file.writeInt(*object, field_size);
	else *object = file.readInt(field_size);
	catch_libbinio_errors();
}

void FileAccess::fieldcpy_float(float* object, binio::FType floating_type) {
	if (writing == true) {
		// BUG: Can't use binfstream::writeFloat() because it's not working for some reason.
		// Should be fine for ROL files, since most hardware is IEEE 754.
		// file.writeFloat(*object, floating_type);
		int field_size = 4;
		ios_file.write(reinterpret_cast<char*>(object), field_size);
	}
	else *object = file.readFloat(floating_type);
	catch_libbinio_errors();
}

void FileAccess::fieldzero(int field_size) {
	if (writing == true) {
		vector<char> buffer = {};
		buffer.resize(field_size + 1, 0);
		file.writeString(buffer.data(), field_size);
	}
	else {
		file.seek(file.pos() + field_size, binio::Set);
	}
	catch_libbinio_errors();
}

void FileAccess::seek(int p_pos) {
	file.seek(p_pos, binio::Set);
}

void FileAccess::fieldcpy_float_events(map<int, float>& event_map, int loop_spacing) {
	uint16_t event_count = event_map.size();
	fieldcpy_uint16(&event_count, 2);
	DBPRINT("process " << event_count << " float events");
	if (writing) {
		for (auto it = event_map.begin(); it != event_map.end(); it++) {
			uint16_t event_tick = it->first;
			float event_value = it->second;
			fieldcpy_uint16(&event_tick, 2);
			fieldcpy_float(&event_value, binio::Single);
			fieldzero(loop_spacing);
		}
	}
	else {
		for (int i = 0; i < event_count; i++) {
			uint16_t event_tick;
			float event_value;
			fieldcpy_uint16(&event_tick, 2);
			fieldcpy_float(&event_value, binio::Single);
			fieldzero(loop_spacing);

			event_map.insert({event_tick, event_value});
		}
	}
}

bool FileAccess::access_file(string file_path, bool write) {
	writing = write;
	if (writing) {
		ios_file = fstream(file_path, ios::out | ios::binary);
	}
	else {
		ios_file = fstream(file_path, ios::in | ios::binary);
	}
	if (!ios_file.is_open()) {
		cerr << "Could not open file for " << (writing ? "write" : "read") << " operation: " << file_path << "\n";
		if (ios_file.bad()) cerr << "Fatal error: badbit is set.\n";
		if (ios_file.fail()) cerr << strerror(errno) << "\n";
		return false;
	}
	file = binwstream(&ios_file);
	int error = file.error();
	if (error) {
		cerr << "Libbinio could not open file for " << (writing ? "write" : "read") << " operation: " << file_path << "\n";
		if (error == binio::Fatal) cerr << "Fatal error: an unspecified libbinio error occured.\n";
		if (error == binio::Denied) cerr << "Fatal error: denied access to file.\n";
		if (error == binio::NotFound) cerr << "Fatal error: could not find file.\n";
		if (error == binio::Eof) cerr << "Fatal error: reached end of file immediately.\n";
		if (error == binio::Unsupported) cerr << "Fatal error: unsupported libbinio conversion.\n";
		ios_file.close();
		return false;
	}
	file.setFlag(binio::BigEndian, false); // Setting BigEndian flag will reset FloatIEEE flag, so do it beforehand.
	file.setFlag(binio::FloatIEEE, true);
	file.seek(0, binio::End); // Go to end of file to get length.
	file_length = file.pos();
	file.seek(0, binio::Set);
	DBPRINT("Access file, size: " << file_length << ", Starting from: " << file.pos() <<
		", BigEndian: " << file.getFlag(binio::BigEndian) <<
		", FloatIEEE: " << file.getFlag(binio::FloatIEEE));
	return true;
}

void FileAccess::close_file() {
	ios_file.close();
}