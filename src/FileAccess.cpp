#include <FileAccess.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <common.h>
#include <boost/endian/conversion.hpp>

using namespace boost::endian;

namespace FileAccess {
	long file_pos = 0;
	fstream file;
	bool writing = false;
};

void FileAccess::fieldcpy_write(void* object, int field_size) {
	file.write(static_cast<char*>(object), field_size);
	file_pos += field_size;
	file.seekg(file_pos);
}

void FileAccess::fieldcpy_read(void* object, int field_size) {
	file.read(static_cast<char*>(object), field_size);
	file_pos += field_size;
	file.seekg(file_pos);
}
// Checks and calls fieldcpy_read or write
void FileAccess::fieldcpy(void* object, int field_size) {
	if (writing == true) fieldcpy_write(object, field_size);
	else fieldcpy_read(object, field_size);
}

// Same as fieldcpy, but ensures little endianness for integer types.
void FileAccess::fieldcpyLE16(uint16_t* object, int field_size) {
	if (writing == true) fieldcpy_write(object, field_size);
	else fieldcpy_read(object, field_size);
	native_to_little(*object);
}
void FileAccess::fieldcpyLE32(uint32_t* object, int field_size) {
	if (writing == true) fieldcpy_write(object, field_size);
	else fieldcpy_read(object, field_size);
	native_to_little(*object);
}

void FileAccess::fieldzero(int field_size) {
	if (writing == true) {
		vector<char> buffer = {};
		buffer.resize(field_size, 0);
		file.write(buffer.data(), field_size);
	}
	file_pos += field_size;
	file.seekg(file_pos);
}

void FileAccess::fieldcpy_float_events(map<int, float>& event_map, int loop_spacing) {
	if (writing) {
		int16_t event_count = event_map.size();
		fieldcpy_write(&event_count, 2);
		for (auto it = event_map.begin(); it != event_map.end(); it++) {
			int16_t event_tick = it->first;
			float event_value = it->second;
			fieldcpy_write(&event_tick, 2);
			fieldcpy_write(&event_value, 4);
			fieldzero(loop_spacing);
		}
	}
	else {
		int16_t event_count;
		fieldcpy_read(&event_count, 2);
		for (int i = 0; i < event_count; i++) {
			int16_t event_tick;
			float event_value;
			fieldcpy_read(&event_tick, 2);
			fieldcpy_read(&event_value, 4);
			event_map.insert({event_tick, event_value});
			fieldzero(loop_spacing);
		}
	}
}

bool FileAccess::access_file(wxString file_path, bool write) {
	writing = write;
	if (writing) {
		file = fstream(file_path, ios::out);
	}
	else {
		file = fstream(file_path, ios::in);
	}
	if (!file.is_open()) {
		cerr << "Could not open file for " << (writing ? "write" : "read") << " operation: " << file_path << "\n";
		if (file.bad()) cerr << "Fatal error: badbit is set.\n";
		if (file.fail()) cerr << strerror(errno) << "\n";
		return false;
	}
	file.seekg(0); // Initializing.
	file_pos = 0;
	return true;
}