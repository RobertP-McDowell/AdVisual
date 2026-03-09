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
#include <common.h>

using namespace std;

struct OPLFM {
	uint8_t attack_rate = 0, decay_rate = 0, sustain_level = 0, release_rate = 0;
	uint8_t frequency_multiplier = 0, feedback = 0, output_level = 0, level_scaling = 0, waveform = 0;
	uint8_t sustain_sound = false, envelope_scaling = false, vibrato = false, tremelo = false;
};

class Instrument {
public:
	uint8_t percussion_mode = 0, voice_number = 0;
	bool additive_synth = false;
	OPLFM carrier;
	OPLFM modulator;
	uint8_t flags = 0; // 0 unused "record", 1 otherwise.
	char name[9] = {'\0'};
};

class Bank {
public:
	uint8_t file_version_major = 0;
	uint8_t file_version_minor = 0;
	vector<Instrument> instruments = {};
	void clear_bank_data() { instruments.clear(); }
	Instrument* GetInstrumentByName(char name[9]);
};

////////////////////////////////////////////////////////////////////////////////////
/////// Autocomplete for filtering and selecting bank instruments //////////////////
class BankControl : public wxControl {
private:
	wxListCtrl* list_ctrl;
	char filter_str[9];
	void on_list_item_selected(wxListEvent& event) {
		ProcessEvent(event); // Forward it.
	}
	Bank* current_bank = nullptr;
public:
	BankControl(wxWindow* parent, int id = wxID_ANY) : wxControl(parent, id) {
		list_ctrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL);
		list_ctrl->AppendColumn("Instruments");
		list_ctrl->Bind(wxEVT_LIST_ITEM_SELECTED, &BankControl::on_list_item_selected, this, wxID_ANY);
		SetWindowStyle(wxBORDER_NONE);
	}
	void SetBank(Bank* bank) {
		if (current_bank == bank) return;
		current_bank = bank;
		list_ctrl->DeleteAllItems();
		if (bank != nullptr) {
			for (int insi = 0; insi < current_bank->instruments.size(); insi++) {
				list_ctrl->InsertItem(insi, wxString(current_bank->instruments.at(insi).name));
			}
		}
	}
	void FilterString(const char new_filter[9]) {
		if (list_ctrl->GetItemCount() == 0 || current_bank == nullptr) {
			return;
		}
		memcpy(filter_str, new_filter, 9);
		int greatest_match_len = 0;
		int greatest_match_idx = 0;
		for (int insi = 0; insi < current_bank->instruments.size(); insi++) {
			Instrument& ins = current_bank->instruments.at(insi);
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
		list_ctrl->EnsureVisible(list_ctrl->GetItemCount() - 1); // Ensure visible the final item,
		list_ctrl->EnsureVisible(greatest_match_idx); // so the greatest match item will be at the top of the list!
	}
};










