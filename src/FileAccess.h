#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <cstdint>
#include <fstream>
#include <deque>
#include <vector>
#include <map>
#include <binfile.h>
#include <binwrap.h>

using namespace std;

namespace FileAccess {
	extern int catch_libbinio_errors();
	// Checks and calls fieldcpy_read or write
	// leave delimiter empty for non null-terminated char arrays.
	extern void fieldcpy_char(char* object, int field_size);
	extern void fieldcpy_char(char* object, int field_size, char delimiter);
	extern void fieldcpy_uint8(uint8_t* object, int field_size);
	extern void fieldcpy_uint16(uint16_t* object, int field_size);
	extern void fieldcpy_uint32(uint32_t* object, int field_size);
	extern void fieldcpy_float(float* object, binio::FType floating_type = binio::Single);
	// Same as fieldcpy, but ensures little endianness for integer types.
	extern void fieldzero(int field_size);
	extern void fieldcpy_float_events(map<int, float>& event_map, int loop_spacing);
	// Opens file and initializes. returns false if opening file was unsuccessful.
	extern bool access_file(string file_path, bool write);

	extern binwstream file;
	extern fstream ios_file;
	extern bool writing;
	extern int file_length;
};

