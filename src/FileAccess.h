#pragma once

#include <iostream>
#include <memory>
#include <Track.h>
#include <string>

using namespace std;


namespace FileAccess {
	extern void SaveTrack(string save_path, Track& track);
	extern void LoadTrack(const string& load_path, Track& track);
};

