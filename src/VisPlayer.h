/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2008 Simon Peter, <dn.tlp@gmx.net>, et al.
 * Copyright (C) 2026 Robert McDowell, GitHub(@RobertP-McDowell)
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Forked from adplug:
 * composer.h - AdLib Visual Composer synth class by OPLx <oplx@yahoo.com>
 *              with improvements by Stas'M <binarymaster@mail.ru> and Jepael
 * Forked to AdVisual:
 * VisPlayer.h - AdVisual synth class, modifications by GitHub(@RobertP-McDowell).
 *
 * Source references ADLIB.C from Adlib MSC SDK.
 */

#pragma once

#include <vector>
#include <string>

#include <adplug/player.h>
#include <Track.h>
#include <Instrument.h>

// These are here since Visual C 6 doesn't support statics declared and defined in class.
#define INS_MAX_NAME_SIZE  9U
#define BNK_SIGNATURE_SIZE 6U
#define MAX_VOICES         11
#define ADLIB_OPER_LEN     13		/* operator length, sizeof(SFMOperator) */
#define ADLIB_INST_LEN     (ADLIB_OPER_LEN * 2 + 2)	/* modulator, carrier, mod/car wave select */

#include <stdint.h> // for uintxx_t

#ifdef __x86_64__
	typedef signed   int      int32;
#else
	typedef signed long int   int32;
#endif

using namespace std;

class CVisPlayer: public CPlayer
{
public:
	static CPlayer* factory(Copl* p_opl, Track* p_track, Bank* p_bank);

	CVisPlayer(Copl* p_opl, Track* p_track, Bank* p_bank);

	~CVisPlayer() {};

	virtual bool load(const std::string &filename, const CFileProvider &fp)
	{
		return false;
	};
	virtual bool update();
	virtual void rewind(int subsong);		// rewinds to specified subsong
	virtual float getrefresh() {
		return refresh_rate;
	};
	virtual unsigned int getsubsongs() override { return track->get_tick_count(); }
	virtual unsigned int getsubsong() override { return tick; }

	virtual string gettype() { return string("AdVisual, AdLib Visual Composer"); }
	virtual unsigned int getinstruments() { return 0; };
	virtual string getinstrument(unsigned int n) { return string(); };
	virtual string getdesc() { return string(); };

	static const int      kNumMelodicVoices;
	static const int      kNumPercussiveVoices;
	static const uint32_t kMidPitch;
	static const uint8_t  kMaxVolume;

	void set_bank(Bank* p_bank) { bank = p_bank; }
	void set_track(Track* p_track) { track = p_track; set_rhythm_mode(!track->melodic_mode); }
	void enable_channel(int v);
	void channel_play_note(int channel, int note_pitch, Instrument* instrument = nullptr, float pitch_mult = 1.0, float volume_mult = 1.0);
	void enable_all_channels();
	void disable_all_channels();
	void set_rhythm_mode(const int mode);
	int get_rhythm_mode() const { return mRhythmMode; }
	void set_pitch_range(uint8_t pitchRange);
	void change_pitch(int voice, const uint16_t pitchBend);
	uint8_t get_ksltl(const int voice, const uint8_t volume, const int carrier_ksltl);
	void set_volume(const int voice, const uint8_t volume);
protected:
	void refresh_track_events();
	void refresh_voice_events(const int voice);
	void update_voice(const int voice);
	bool update_track();
	void note_on(const int voice, const int note);
	void note_off(const int voice);
	void set_note(const int voice, const int note);
	void set_note_melodic(const int voice, const int note);
	void set_note_percussive(const int voice, const int note);
	void set_freq(const int voice, const int note, const bool keyOn=false);
	void set_instrument(const int voice, const Instrument* instrument);
	int get_channel_count() const;

	typedef const uint16_t*              TUint16ConstPtr;
	typedef std::vector<TUint16ConstPtr> TUint16PtrVector;
	typedef std::vector<int16_t>         TInt16Vector;
	typedef std::vector<uint8_t>         TUInt8Vector;
	typedef std::vector<bool>            TBoolVector;

	TUint16ConstPtr   mpOldFNumFreqPtr;
	TUint16PtrVector  mFNumFreqPtrList;
	TInt16Vector      mHalfToneOffset;
	TUInt8Vector      mVolumeCache, mKSLTLCache, mNoteCache, mKOnOctFNumCache;
	TBoolVector       mKeyOnCache;
	uint8_t           mRhythmMode, mAMVibRhythmCache;
	int32_t           mOldPitchBendLength;
	uint16_t          mPitchRangeStep, mOldHalfToneOffset;

	static const int kSizeofDataRecord, kSilenceNote, kBassDrumChannel, kSnareDrumChannel,
		kTomtomChannel, kTomTomNote, kTomTomToSnare, kSnareNote;
	Bank* bank;
	Track* track;
	array<int, 11> dynamic_notes; // 1 per channel.
	unsigned int tick;
	float refresh_rate;
};
