#pragma once

#include <iostream>
#include <memory>
#include <Track.h>
#include <Instrument.h>
#include <string>

using namespace std;


namespace FileAccess {
	extern void SaveTrack(string save_path, Track& track);
	extern void LoadTrack(string load_path, Track& track);
	extern void SaveBank(string save_path, Bank& bank);
	extern Bank LoadBank(string load_path);
	//extern void LoadInstrument(string load_path, Instrument& ins);
};

