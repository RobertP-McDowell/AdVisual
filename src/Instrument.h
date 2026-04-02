#pragma once



#include <wx/wx.h>
#include <wx/utils.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <wx/listctrl.h>
#include <cstdint>
#include <memory>
#include <deque>
#include <vector>
#include <map>

using namespace std;

struct OPLFM {
	OPLFM() {}
	OPLFM(uint8_t atrt, uint8_t dcrt, uint8_t stlvl, uint8_t rsrt, bool stbool, bool p_ksr,
		uint8_t frqmul, uint8_t fdbk, bool vib, uint8_t outlvl, uint8_t p_ksl, bool trem, uint8_t wave) : 
		attack_rate(atrt), decay_rate(dcrt), sustain_level(stlvl), release_rate(rsrt), sustain_sound(stbool), ksr(p_ksr),
		frequency_multiplier(frqmul), feedback(fdbk), vibrato(vib), output_level(outlvl), ksl(p_ksl), tremelo(trem), waveform(wave) {}

	uint8_t attack_rate = 0, decay_rate = 0, sustain_level = 0, release_rate = 0;
	uint8_t frequency_multiplier = 0, feedback = 0, output_level = 0, ksl = 0, waveform = 0;
	uint8_t sustain_sound = false, ksr = false, vibrato = false, tremelo = false, additive_synth = false;

	// the number at the end of these function is in hex!
	uint8_t reg20()  const {
		return tremelo << 7 | vibrato << 6 | sustain_sound << 5 | ksr << 4 | frequency_multiplier;
	}
	uint8_t reg40() const {
		return ksl << 6 | output_level;
	}
	uint8_t reg60() const {
		return attack_rate << 4 | decay_rate;
	}
	uint8_t reg80() const {
		return sustain_level << 4 | release_rate;
	}
	uint8_t regC0() const {
		return feedback << 1 | additive_synth;
	}
	uint8_t regE0() const {
		return 0 << 3 | waveform;
	}
};

class Instrument {
public:
	Instrument() {}
	Instrument(OPLFM p_carrier, OPLFM p_modulator) : carrier(p_carrier), modulator(p_modulator) {}
	uint8_t percussion_mode = 0; // 0=Melodic, 1=percussive.
	uint8_t voice_number = 0; // Starts from 6 (percussive only).
	OPLFM carrier;
	OPLFM modulator;
	uint8_t flags = 0; // 0 unused "record", 1 otherwise.
	char name[9] = {'\0'};

	static OPLFM default_carrier;
	static OPLFM default_modulator;
	static Instrument default_instrument;
};

class Bank {
public:
	void load_file(wxString filename);
	void save_file(wxString filename);
	uint8_t file_version_major = 0;
	uint8_t file_version_minor = 0;
	vector<Instrument> instruments = {};
	void add_instrument(Instrument new_instrument);
	void delete_instrument(char name[9]);
	void clear_bank_data() { instruments.clear(); }
	Instrument* find_instrument(wxString name);
	Instrument* find_instrument(char name[9]);
	wxString file_path = wxEmptyString;
protected:
	void bnk_move_fields();
};

////////////////////////////////////////////////////////////////////////////////////
// Autocomplete for filtering and selecting bank instruments ///////////////////////
////////////////////////////////////////////////////////////////////////////////////
class BankControl : public wxControl {
private:
	wxListBox* list_box;
	char filter_str[9] = "";
	void on_item_selected(wxCommandEvent& event) {
		ProcessEvent(event); // Forward it.
	}
	Bank* bank;
public:
	BankControl(wxWindow* parent, int id = wxID_ANY, wxPoint pos = wxDefaultPosition, wxSize size = wxDefaultSize) :
			wxControl(parent, id, pos, size) {
		list_box = new wxListBox(this, wxID_ANY, wxPoint(0, 0), size, 0, NULL, wxBORDER_NONE | wxLB_SINGLE);
		list_box->Bind(wxEVT_LISTBOX, &BankControl::on_item_selected, this, wxID_ANY);
		SetWindowStyle(wxBORDER_NONE);
		wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(list_box, 1, wxEXPAND);
		SetSizerAndFit(sizer);
	}
	char* GetText() { return filter_str; }
	void SetBank(Bank* new_bank) {
		bank = new_bank;
		list_box->Clear();
		if (bank != nullptr) {
			for (int insi = 0; insi < bank->instruments.size(); insi++) {
				list_box->Append(wxString(bank->instruments.at(insi).name));
			}
		}
		Refresh();
		Update();
	}
	void FilterString(const char new_filter[9]) {
		memcpy(filter_str, new_filter, 9);
		if (list_box->GetCount() == 0 || bank == nullptr) {
			return;
		}
		int greatest_match_len = 0;
		int greatest_match_idx = 0;
		for (int insi = 0; insi < bank->instruments.size(); insi++) {
			Instrument& ins = bank->instruments.at(insi);
			for (int i = 0; i <= 9; i++) {
				if (filter_str[i] == '\0') {
					if (greatest_match_len < i) {
						greatest_match_len = i;
						greatest_match_idx = insi;
					}
					break;
				}
				if (filter_str[i] != ins.name[i]) {
					if (greatest_match_len < i) {
						greatest_match_len = i;
						greatest_match_idx = insi;
					}
					break;
				}
			}
		}
		list_box->EnsureVisible(list_box->GetCount() - 1); // Ensure visible the final item,
		list_box->EnsureVisible(greatest_match_idx); // so the greatest match item will be at the top of the list!
	}
};






