#pragma once
#include <miniaudio.h>
#include <adplug/adplug.h>
#include <adplug/emuopl.h>
#include <adplug/player.h>
#include <adplug/nemuopl.h> // Nuked OPL.
#include <string>
#include <memory>
#include <Instrument.h>
#include <VisPlayer.h>

using namespace std;

class AdPlayer {
public:
	AdPlayer();
	~AdPlayer();
	bool play(string file_path, int start_from = 0); // Returns false if failed to play.
	void play_note(int note_number, int channel, Instrument* instrument);
	void stop();
	void seek(unsigned long p_tick);
	void mix_miniaudio(ma_device* p_device, void* p_output, const void* p_input, ma_uint32 frame_count);
	void set_channel_enable(int channel, bool enable);
	bool get_channel_enable(int channel) const { return enabled_channels[channel]; }
protected:
	long towrite;
	ma_device mini_device;
	unique_ptr<CVisPlayer> opl_playback;
	unique_ptr<Copl> opl_device;
	bool stereo = false, loop = false, active = false;
	int adplug_process(short* p_buffer, unsigned int p_frames, unsigned int p_buffer_offset);
	friend class ma_device;
};

extern unique_ptr<AdPlayer> adplayer;