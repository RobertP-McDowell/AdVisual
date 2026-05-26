#include <AdPlayer.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <adplug/fmopl.h>
#include <common.h>
#include <ComposerPanel.h>

#define RATE	44100   // Output frequency in Hz
#define BIT16	true    // true when 16bit samples should be used
#define BUFSIZE	512     // Sound buffer size in samples

unique_ptr<AdPlayer> adplayer = nullptr;

int AdPlayer::adplug_process(short* p_buffer, unsigned int p_frames, unsigned int p_buffer_offset) {
	unsigned int write = (towrite > p_frames ? p_frames : towrite);
	opl_device->update(p_buffer + p_buffer_offset, write);
	towrite -= write;
	if (towrite <= 0) { // When true we had to make the last buffer smaller, therefore we need to update.
		if (!opl_playback->update()) {
			if (loop) {
				seek(0.0);
			}
			else {
				thread stop_thr(&AdPlayer::stop, this);
				stop_thr.detach();
			}
			return write;
		}
		towrite = RATE / opl_playback->getrefresh(); // Important to refresh towrite after update.
		if (write < p_frames) { // If true, there are more p_frames left to process.
			write += adplug_process(p_buffer, p_frames - write, p_buffer_offset + write); // Finish the buffer since towrite ran out.
		}
	}
	return write;
}

void AdPlayer::mix_miniaudio(ma_device* p_device, void* p_output, const void* p_input, ma_uint32 frame_count) {
	if (!active || !opl_playback) {
		return; // Safety check.
	}
	unsigned long total_frames_processed = 0, frames_left = frame_count;
	int used_bufsize = (stereo == true ? BUFSIZE * 2 : BUFSIZE);
	int write = min((int)frames_left, used_bufsize);
	while (active && frames_left != 0) {
		long new_frames_processed = adplug_process(static_cast<short*>(p_output), write, total_frames_processed);
		total_frames_processed += new_frames_processed;
		if (new_frames_processed == 0) {
			thread stop_thr(&AdPlayer::stop, this);
			stop_thr.detach();
			return;
		}
		frames_left = frame_count - total_frames_processed;
		
		write = min((int)frames_left, used_bufsize);
	}
	int new_tick = opl_playback->getsubsong();
	if (playing_song && new_tick != cursor_tick) {
		ComposerPanel::set_cursor_tick(new_tick, new_tick);
	}
	return;
}

// Forward the callback to AdPlayer member function.
void mix_miniaudio_callback(ma_device* p_device, void* p_output, const void* p_input, ma_uint32 frame_count) {
	static_cast<AdPlayer*>(p_device->pUserData)->mix_miniaudio(p_device, p_output, p_input, frame_count);
}

AdPlayer::AdPlayer() {
	stereo = false;
	// Initialize mini_device.
	ma_device_config mini_config = ma_device_config_init(ma_device_type_playback);
	mini_config.playback.format = ma_format_s16;
	mini_config.playback.channels = (stereo == true ? 2 : 1);
	mini_config.sampleRate = RATE;
	mini_config.dataCallback = mix_miniaudio_callback;
	mini_config.pUserData = this;
	if (ma_device_init(NULL, &mini_config, &mini_device) != MA_SUCCESS) {
		cerr << "Failed to initialize miniaudio device!\n";
		return;
	}

	// Initialize opl_device.
	if (stereo == false) {
		unique_ptr<CEmuopl> new_opl = make_unique<CEmuopl>(RATE, BIT16, false);
		new_opl->settype(Copl::TYPE_OPL2);
		opl_device = move(new_opl);
	}
	else { // Nuked OPL.
		opl_device = make_unique<CNemuopl>(RATE);
	}
	if (!opl_device) {
		cerr << "Couldn't create the Adplug OPL device!\n";
		stop();
		return;
	}
	opl_device->init();
	opl_playback.reset(static_cast<CVisPlayer*>(CVisPlayer::factory( opl_device.get(), current_track, current_bank )));
	opl_playback->DisableAllChannels(); // So if we play a note in editor the song wont start.
	opl_playback->SetRhythmMode(1);
}

AdPlayer::~AdPlayer() {
	stop();
	if (opl_device) {
		opl_device = nullptr;
	}
	if (opl_playback) {
		opl_playback = nullptr;
	}
	ma_device_uninit(&mini_device);
}

void AdPlayer::play_note(int note_number, int channel, Instrument* instrument) {
	if (instrument != nullptr) {
		if (instrument->percussion_mode != 0 || channel >= 9) {
			opl_playback->SetRhythmMode(1);
			if (instrument->percussion_mode != 0) { channel = instrument->voice_number; }
		}
	}
	if (note_number != 0) { DBPRINT("Play dynamic note at channel: " << channel); }
	else { DBPRINT("Stop dynamic note at channel: " << channel); }
	opl_playback->DisableChannelAndPlayNote(channel, note_number, instrument);

	if (!active) {
		DBPRINT("Starting playback to play dynamic note");
		towrite = RATE / opl_playback->getrefresh();
		ma_device_start(&mini_device);
		active = true;
	}
}

void AdPlayer::set_channel_enable(int channel, bool enable) {
	enabled_channels[channel] = enable;
	if (enable == true) {
		opl_playback->EnableChannel(channel);
	}
	else {
		opl_playback->DisableChannelAndPlayNote(channel, 0, nullptr);
	}
}

void AdPlayer::toggle_play_song() {
	if (playing_song) {
		stop();
	}
	else {
		play("", cursor_tick);
	}
	common_action_group->change_action_state("toggle_play_song", Glib::Variant<bool>::create(playing_song));
}

bool AdPlayer::play(string file_path, int start_from) {
	opl_playback->SetTrack(current_track);
	opl_playback->SetBank(current_bank);
	opl_playback->EnableAllChannels();
	for (int v = 0; v < 11; v++) {
		if (enabled_channels[v] == false) {
			opl_playback->DisableChannelAndPlayNote(v, 0, nullptr);
		}
	}
	opl_playback->rewind(start_from);

	if (!opl_playback) {
		cerr << "Couldn't create Adplug playback for file! " << file_path << "\n";
		cerr << "Make sure 'standard.bnk' is in the same directory!\n";
		return false;
	}

	towrite = RATE / opl_playback->getrefresh();
	ma_device_start(&mini_device);
	active = true;
	playing_song = true;

	return true;
}

void AdPlayer::seek(unsigned long p_tick) {
	if (opl_playback) {
		opl_playback->seek(p_tick);
		if (!opl_playback->update()) {
			stop();
			return;
		}
		towrite = RATE / opl_playback->getrefresh();
	}
}

void AdPlayer::stop() {
	opl_playback->DisableAllChannels(); // So if we play a note in editor the song wont continue.
	ma_device_stop(&mini_device);
	towrite = 0;
	active = false;
	playing_song = false;
}