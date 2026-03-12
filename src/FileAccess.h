#pragma once

#include <iostream>
#include <wx/wx.h>
#include <memory>
#include <string>
#include <cstdint>
#include <fstream>
#include <deque>
#include <vector>
#include <map>

using namespace std;


namespace FileAccess {
	extern void fieldcpy_write(void* object, int field_size);
	extern void fieldcpy_read(void* object, int field_size);
	// Checks and calls fieldcpy_read or write
	extern void fieldcpy(void* object, int field_size);
	// Same as fieldcpy, but ensures little endianness for integer types.
	extern void fieldcpyLE16(uint16_t* object, int field_size);
	extern void fieldcpyLE32(uint32_t* object, int field_size);
	extern void fieldzero(int field_size);
	extern void fieldcpy_float_events(map<int, float>& event_map, int loop_spacing);
	extern void access_file(wxString file_path, bool write);

	extern long file_pos;
	extern fstream file;
	extern bool writing;
};

