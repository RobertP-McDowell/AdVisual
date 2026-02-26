#include <AdPlayer.h>
#include <iostream>

#define RATE	44100   // Output frequency in Hz
#define BIT16	true    // true when 16bit samples should be used
#define BUFSIZE	512     // Sound buffer size in samples


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
				stop();
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
	//cout << "Mini 1\n";
	if (!active || !opl_playback) return; // Safety check.
	unsigned long total_frames_processed = 0, frames_left = frame_count;
	int used_bufsize = (stereo == true ? BUFSIZE * 2 : BUFSIZE);
	int write = min((int)frames_left, used_bufsize);
	//cout << "Mini 2\n";
	while (active && frames_left != 0) {
		long new_frames_processed = adplug_process(static_cast<short*>(p_output), write, total_frames_processed);
		total_frames_processed += new_frames_processed;
		if (new_frames_processed == 0) {
			stop();
			return;
		}
		frames_left = frame_count - total_frames_processed;
		
		write = min((int)frames_left, used_bufsize);
	}
	return;
}
// cant seem to convert my member function ptr to a normal function ptr, so just forward it ig.
void mix_miniaudio_callback(ma_device* p_device, void* p_output, const void* p_input, ma_uint32 frame_count) {
	//cout << "Mini 0\n";
	static_cast<AdPlayer*>(p_device->pUserData)->mix_miniaudio(p_device, p_output, p_input, frame_count);
}


AdPlayer::AdPlayer() {
	stereo = true;
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
}

AdPlayer::~AdPlayer() {
	stop();
	ma_device_uninit(&mini_device);
}

bool AdPlayer::play(string file_path) {
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
		return false;
	}
	opl_device->init();
	opl_playback.reset(CAdPlug::factory(file_path, opl_device.get()));

	if (!opl_playback) {
		cerr << "Couldn't create Adplug playback for file! " << file_path << "\n";
		cerr << "Make sure 'standard.bnk' is in the same directory!\n";
		return false;
	}
	towrite = RATE / opl_playback->getrefresh();

	ma_device_start(&mini_device);
	active = true;

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
	if (opl_device) {
		opl_device = nullptr;
	}
	if (opl_playback) {
		opl_playback = nullptr;
	}
	ma_device_stop(&mini_device);
	towrite = 0;
	active = false;
}