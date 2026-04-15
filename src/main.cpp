#include <wx/wx.h>
#include <wx/utils.h>
#include <wx/filedlg.h> 
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <wx/scrolwin.h>
#include <wx/scrolbar.h>
#include <wx/popupwin.h>
#include <wx/ribbon/toolbar.h>
#include <wx/artprov.h>
#include <common.h>
#include <Track.h>
#include <Instrument.h>
#include <FileAccess.h>
#include <ComposerPanel.h>
#include <InsmakerPanel.h>
#include <iostream>
#include <memory>
#include <deque>
#include <vector>


using namespace std;

#ifndef wxHAS_SVG
#define HAS_SVG false
#endif
#ifdef wxHAS_SVG
#define HAS_SVG true
#endif

class MainApp : public wxApp {
public:
	bool OnInit() override;
};

wxIMPLEMENT_APP(MainApp);

wxDECLARE_EVENT(EVT_TOGGLED, wxCommandEvent);
wxDEFINE_EVENT(EVT_TOGGLED, wxCommandEvent);
wxDECLARE_EVENT(EVT_TOGGLED_ENABLE, wxCommandEvent);
wxDEFINE_EVENT(EVT_TOGGLED_ENABLE, wxCommandEvent);

class AdVisualArtProvider : public wxArtProvider {
protected:
	wxBitmapBundle CreateBitmapBundle(const wxArtID& id, const wxArtClient& client, const wxSize& size) override;
};

wxBitmapBundle AdVisualArtProvider::CreateBitmapBundle(const wxArtID& id, const wxArtClient& client, const wxSize& size) {
	return wxNullBitmap;
}


class ChannelButton : public wxControl {
private:
	wxColour button_colour = *wxBLACK;
	bool pressed = false;
	bool hovered = false;
	bool enabled = true;
	void on_pressed(wxMouseEvent& event) {
		if (event.ShiftDown()) {
			enabled = !enabled;
			SendEnabledEvent();
		}
		else if (pressed != true) {
			pressed = true;
			SendToggledEvent();
		}
		Refresh();
		Update();
	}
	void on_mouse_enter(wxMouseEvent& event) {
		hovered = true;
		status_bar->SetStatusText("Select channel, Shift+Left Click to enable/disable playback");
		Refresh();
		Update();
	}
	void on_mouse_exit(wxMouseEvent& event) {
		hovered = false;
		Refresh();
		Update();
	}
	void on_paint(wxPaintEvent& event) {
		wxPaintDC dc(this);
		wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
		if (gc == nullptr) return;
		double outline_width = 4.0;
		wxPoint2DDouble center = (wxPoint2DDouble(GetSize().y, GetSize().y) / 2.0) - wxPoint2DDouble(outline_width / 2.0, outline_width / 2.0);
		double outline_radius = (GetSize().y / 2.0) - outline_width;
		double fill_radius = outline_radius - (outline_width * 2.0);
		wxGraphicsPen colored_pen = gc->CreatePen(wxGraphicsPenInfo(button_colour).Width(outline_width).Style(wxPENSTYLE_SOLID));
		wxGraphicsPen disabled_pen = gc->CreatePen(wxGraphicsPenInfo(wxColour(50, 50, 50)).Width(outline_width).Style(wxPENSTYLE_SOLID));
		gc->SetPen(enabled ? colored_pen : disabled_pen);
		wxGraphicsPath outline_path = gc->CreatePath();
		outline_path.AddCircle(center.m_x, center.m_y, outline_radius);
		outline_path.CloseSubpath();
		gc->StrokePath(outline_path);
		if (pressed || hovered) {
			wxColour fill_colour;
			if (pressed) {
				gc->SetPen(colored_pen);
				fill_colour = button_colour;
			}
			else { // TODO: decide if channels buttons look better without showing hover.
				gc->SetPen(wxNullPen);
				fill_colour = wxColour(button_colour.Red(), button_colour.Green(), button_colour.Blue(), 100);
			}
			gc->SetBrush(*wxTheBrushList->FindOrCreateBrush(fill_colour));
			wxGraphicsPath fill_path = gc->CreatePath();
			fill_path.AddCircle(center.m_x, center.m_y, fill_radius);
			outline_path.CloseSubpath();
			gc->DrawPath(fill_path);
		}
		wxGraphicsFont button_font = gc->CreateFont(GetSize().y, wxEmptyString, wxFONTFLAG_BOLD, *wxWHITE);
		gc->SetFont(button_font);
		gc->DrawText(GetLabel(), center.m_x * 2.0, 0);
		delete gc;
	}
	void Init() {
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		Bind(wxEVT_PAINT, &ChannelButton::on_paint, this);
		Bind(wxEVT_LEFT_DOWN, &ChannelButton::on_pressed, this);
		Bind(wxEVT_ENTER_WINDOW, &ChannelButton::on_mouse_enter, this);
		Bind(wxEVT_LEAVE_WINDOW, &ChannelButton::on_mouse_exit, this);
	}
public:
	ChannelButton() { Init(); }
	ChannelButton(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString &label = "", const wxColour &colour = *wxBLACK,
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxBORDER_NONE,
		const wxValidator& validator = wxDefaultValidator, const wxString& name = wxRadioButtonNameStr) :
			wxControl(parent, id, pos, size, style, validator, name) {
		button_colour = colour;
		Init();
		SetLabel(label);
	}
	void SetPressed(bool value) { pressed = value; } // Won't trigger event.
	void SetEnabled(bool value) { enabled = value; }
	bool GetPressed() { return pressed; }
	bool GetEnabled() const { return enabled; }
protected:
	virtual wxSize DoGetBestSize() const { return wxSize(20.0, 20.0); }
	void SendToggledEvent() {
		wxCommandEvent event(EVT_TOGGLED, GetId());
		event.SetInt(pressed);
		event.SetEventObject(this);
		ProcessWindowEvent(event);
	}
	void SendEnabledEvent() {
		wxCommandEvent event(EVT_TOGGLED_ENABLE, GetId());
		event.SetInt(enabled);
		event.SetEventObject(this);
		ProcessWindowEvent(event);
	}
private:
	wxDECLARE_DYNAMIC_CLASS(ChannelButton);
};

IMPLEMENT_DYNAMIC_CLASS(ChannelButton, wxControl)

class AdVisualToolBar : public wxToolBar {
public:
	AdVisualToolBar(wxWindow* parent, int id = wxID_ANY, wxPoint position = wxDefaultPosition,
		wxSize size = wxDefaultSize, long style = wxTB_HORIZONTAL, const wxString& name = wxToolBarNameStr);
	void CreateComposerTools(ComposerPanel* p_composer_panel);
	void ShowComposerTools(bool show);
	void CreateInsmakerTools(InsmakerPanel* p_insmaker_panel);
	void ShowInsmakerTools(bool show);
	void update_insmaker_toolbar();
	wxPopupWindow* bank_popup;
	BankControl* bank_ctrl;
	wxMenu* instrument_edit_menu;
	wxPopupTransientWindow* track_edit_popup;
private:
	void show_tools(bool show, vector<wxToolBarToolBase*> tools);
	// Composer events:
	void on_channel_button_pressed(wxCommandEvent& event);
	void on_channel_button_enabled(wxCommandEvent& event);
	void on_preview_channels_checked(wxCommandEvent& event);
	void on_grid_feedback_checked(wxCommandEvent& event);
	void on_follow_cursor_checked(wxCommandEvent& event);
	// ^-Popup signals.
	void on_popup_track_menu(wxCommandEvent& event);
	void on_toggle_percussion(wxCommandEvent& event);
	void on_enter_basic_tempo(wxCommandEvent& event);
	void on_enter_ticks_per_beat(wxCommandEvent& event);
	void on_enter_beats_per_measure(wxCommandEvent& event);
	// Insmaker events:
	void on_select_instrument_set_focus(wxFocusEvent& event);
	void on_select_instrument_kill_focus(wxFocusEvent& event);
	void on_select_instrument_text_changed(wxCommandEvent& event);
	void on_select_instrument_enter(wxCommandEvent& event);
	void on_bank_selector_select_item(wxCommandEvent& event);
	void on_toggle_additive_synth(wxCommandEvent& event);
	// ^-Menu signals.
	void on_popup_instrument_menu(wxCommandEvent& event);
	void on_create_instrument(wxCommandEvent& event);
	void on_copy_instrument(wxCommandEvent& event);
	void on_delete_instrument(wxCommandEvent& event);
	void on_set_instrument_percussion(wxCommandEvent& event);
	
	ComposerPanel* composer_panel;
	InsmakerPanel* insmaker_panel;
	vector<wxToolBarToolBase*> composer_tools;
	vector<wxToolBarToolBase*> insmaker_tools;
	vector<ChannelButton*> channel_buttons;
	wxSize tool_size;
	wxSize square_size;
};

AdVisualToolBar::AdVisualToolBar(wxWindow* parent, int id, wxPoint position, wxSize size, long style, const wxString& name) :
		wxToolBar(parent, id, position, size, style, name) {
	tool_size = wxSize(54, 36) / 1.5;
	square_size = wxSize(tool_size.y, tool_size.y);
	SetMargins(0, 0);

	bank_popup = new wxPopupWindow(this);
	bank_popup->SetSize(200, 500);
	bank_ctrl = new BankControl(bank_popup, wxID_ANY, wxPoint(0, 0), wxSize(200, 200));
	bank_ctrl->Bind(wxEVT_LISTBOX, &AdVisualToolBar::on_bank_selector_select_item, this, wxID_ANY);

	wxBoxSizer* popup_sizer = new wxBoxSizer(wxHORIZONTAL);
	popup_sizer->Add(bank_ctrl, 1, wxEXPAND);
	bank_popup->SetSizerAndFit(popup_sizer);

	SetToolBitmapSize(tool_size);

	AddTool(ID_FILE_MENU, "File Menu", GetAsset("FloppyDisk.svg", square_size), wxNullBitmap, wxITEM_NORMAL,
		"File", "Show File Menu");
	AddTool(ID_HELP_MENU, "Help Menu", GetAsset("HelpBook.svg", square_size), wxNullBitmap, wxITEM_NORMAL,
		"Help", "Show Help Menu");

	AddRadioTool(ID_COMPOSER, "Composer Panel", GetAsset("ComposerIcon.svg", tool_size),
		wxNullBitmap, "COMPOSER", "Edit Track in the COMPOSER panel.");

	AddRadioTool(ID_INSMAKER, "Insmaker Panel", GetAsset("InsmakerIcon.svg", tool_size),
		wxNullBitmap, "INSMAKER", "Edit Instrument in the INSMAKER panel.");

	AddSeparator();
	AddTool(ID_PLAY_TRACK, "Play", GetAsset("PlayButton.svg", square_size), wxNullBitmap, wxITEM_CHECK,
		"Start/Stop", "Start/Stop Track playback.");
	AddSeparator();
}

void AdVisualToolBar::CreateComposerTools(ComposerPanel* p_composer_panel) {
	tool_size = wxSize(54, 36) / 1.5;
	square_size = wxSize(tool_size.y, tool_size.y);
	composer_panel = p_composer_panel;
	// Start Composer ToolBar.
	// We add to composer tools, so we can hide and show them at will.
	for (int i = ID_VOICE_START; i < ID_VOICE_END; i++) {
		ChannelButton* new_button = new ChannelButton(this, i, to_string(i + 1 - ID_VOICE_START),
			current_track->channels[i - ID_VOICE_START].colour, wxDefaultPosition, wxSize(62, 36));
		new_button->Bind(EVT_TOGGLED, &AdVisualToolBar::on_channel_button_pressed, this, i);
		new_button->Bind(EVT_TOGGLED_ENABLE, &AdVisualToolBar::on_channel_button_enabled, this, i);
		composer_tools.push_back(AddControl(new_button, "CH" + to_string(i)));
		channel_buttons.push_back(new_button);
	}
	channel_buttons[0]->SetPressed(true);

	composer_tools.push_back( AddTool(ID_PREVIEW_CHANNELS, "Preview Channels", GetAsset("OpenedEye.svg", tool_size),
		wxNullBitmap, wxITEM_CHECK, "Preview Channels", "Preview Notes from other Channels on the grid.") );
	Bind(wxEVT_MENU, &AdVisualToolBar::on_preview_channels_checked, this, ID_PREVIEW_CHANNELS);

	composer_tools.push_back( AddTool(ID_TRACK_MENU, "Track Options", GetAsset("Cassete.svg", tool_size), wxNullBitmap, wxITEM_NORMAL,
		"Track Options", "Change base Track settings.") );

	composer_tools.push_back( AddTool(ID_AUDIO_FEEDBACK, "Audio Feedback", GetAsset("AudioFeedback.svg", tool_size), wxNullBitmap, wxITEM_CHECK,
		"Audio Feedback", "Sample newly created Notes on the grid.") );
	Bind(wxEVT_MENU, &AdVisualToolBar::on_grid_feedback_checked, this, ID_AUDIO_FEEDBACK);
	//composer_tools.push_back( AddTool(ID_FOLLOW_CURSOR, "Follow Cursor", GetAsset("SheetMusicBox.svg", tool_size), wxNullBitmap, wxITEM_CHECK,
	//	"Follow Cursor", "Follow the cursor during playback.") );
	//Bind(wxEVT_MENU, &AdVisualToolBar::on_follow_cursor_checked, this, ID_FOLLOW_CURSOR);

	// Track Popup.
	Bind(wxEVT_MENU, &AdVisualToolBar::on_popup_track_menu, this, ID_TRACK_MENU);

	track_edit_popup = new wxPopupTransientWindow(this, wxBORDER_RAISED);
	wxGridSizer* track_popup_sizer = new wxGridSizer(2, 10, 20);

	wxStaticText* percussion_name = new wxStaticText(track_edit_popup, wxID_ANY, "With Percussion");
	wxCheckBox* percussion_checkbox = new wxCheckBox(track_edit_popup, ID_RHYTHM_MODE, "");
	percussion_checkbox->Bind(wxEVT_CHECKBOX, &AdVisualToolBar::on_toggle_percussion, this);

	wxStaticText* tempo_name = new wxStaticText(track_edit_popup, wxID_ANY, "Beats per Minute");
	wxTextCtrl* tempo_ctrl = new wxTextCtrl(track_edit_popup, wxID_ANY, "120", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	tempo_ctrl->Bind(wxEVT_TEXT_ENTER, &AdVisualToolBar::on_enter_basic_tempo, this);

	wxStaticText* beat_name = new wxStaticText(track_edit_popup, wxID_ANY, "Ticks per Beat");
	wxTextCtrl* beat_ctrl = new wxTextCtrl(track_edit_popup, wxID_ANY, "4", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	beat_ctrl->Bind(wxEVT_TEXT_ENTER, &AdVisualToolBar::on_enter_ticks_per_beat, this);

	wxStaticText* measure_name = new wxStaticText(track_edit_popup, wxID_ANY, "Beats per Measure");
	wxTextCtrl* measure_ctrl = new wxTextCtrl(track_edit_popup, wxID_ANY, "4", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	measure_ctrl->Bind(wxEVT_TEXT_ENTER, &AdVisualToolBar::on_enter_beats_per_measure, this);

	track_popup_sizer->Add(percussion_name, 1, wxEXPAND);
	track_popup_sizer->Add(percussion_checkbox, 1, wxEXPAND);
	track_popup_sizer->Add(tempo_name, 1, wxEXPAND);
	track_popup_sizer->Add(tempo_ctrl, 1, wxEXPAND);
	track_popup_sizer->Add(beat_name, 1, wxEXPAND);
	track_popup_sizer->Add(beat_ctrl, 1, wxEXPAND);
	track_popup_sizer->Add(measure_name, 1, wxEXPAND);
	track_popup_sizer->Add(measure_ctrl, 1, wxEXPAND);
	track_edit_popup->SetSizerAndFit(track_popup_sizer);
}

void AdVisualToolBar::CreateInsmakerTools(InsmakerPanel* p_insmaker_panel) {
	wxSize tool_size = wxSize(54, 36) / 1.5;
	wxSize square_size = wxSize(tool_size.y, tool_size.y);
	insmaker_panel = p_insmaker_panel;
	// Start Composer ToolBar.
	// We add to composer tools, so we can hide and show them at will.
	wxTextCtrl* instrument_select_field = new wxTextCtrl(this, ID_INSTRUMENT_FIELD, wxEmptyString, wxDefaultPosition, wxSize(125, 1), wxTE_PROCESS_ENTER);
	instrument_select_field->Bind(wxEVT_SET_FOCUS, &AdVisualToolBar::on_select_instrument_set_focus, this, wxID_ANY);
	instrument_select_field->Bind(wxEVT_KILL_FOCUS, &AdVisualToolBar::on_select_instrument_kill_focus, this, wxID_ANY);
	instrument_select_field->Bind(wxEVT_TEXT, &AdVisualToolBar::on_select_instrument_text_changed, this, ID_INSTRUMENT_FIELD);
	instrument_select_field->Bind(wxEVT_TEXT_ENTER, &AdVisualToolBar::on_select_instrument_enter, this, ID_INSTRUMENT_FIELD);

	insmaker_tools.push_back(AddControl(instrument_select_field, "Select Instrument"));
	insmaker_tools.push_back(AddTool(ID_INSTRUMENT_MENU, "Edit Instrument", GetAsset("DropdownButton.svg", tool_size),
		wxNullBitmap, wxITEM_NORMAL, "Edit Bank", "Edit Instrument in Bank"));
	insmaker_tools.push_back(AddTool(ID_ADDITIVE_SYNTH, "Additive Synth", GetAsset("FMSynth.svg", tool_size),
		wxNullBitmap, wxITEM_CHECK, "AM Synthesis", "Toggle Between Amplitude Modulation and Frequency Modulation."));

	Bind(wxEVT_MENU, &AdVisualToolBar::on_toggle_additive_synth, this, ID_ADDITIVE_SYNTH);
	//Bind(wxEVT_MENU, &MainFrame::on_change_percussion_mode, this, ID_PERCUSSION_MODE);

	// Instrument Popup.
	Bind(wxEVT_MENU, &AdVisualToolBar::on_popup_instrument_menu, this, ID_INSTRUMENT_MENU);
	Bind(wxEVT_MENU, &AdVisualToolBar::on_create_instrument, this, ID_CREATE_INSTRUMENT);
	Bind(wxEVT_MENU, &AdVisualToolBar::on_copy_instrument, this, ID_COPY_INSTRUMENT);
	Bind(wxEVT_MENU, &AdVisualToolBar::on_delete_instrument, this, ID_DELETE_INSTRUMENT);
	Bind(wxEVT_MENU, &AdVisualToolBar::on_set_instrument_percussion, this, wxID_ANY);

	instrument_edit_menu = new wxMenu();
	instrument_edit_menu->Append(ID_CREATE_INSTRUMENT, "Create", "Create a new Instrument", wxITEM_NORMAL);
	// TODO: implement copying.
	instrument_edit_menu->Append(ID_DELETE_INSTRUMENT, "Delete", "Delete the selected Instrument", wxITEM_NORMAL);
	instrument_edit_menu->AppendSeparator();
	instrument_edit_menu->AppendRadioItem(ID_MELODIC_INSTRUMENT, "Melodic mode", "Melodic instruments use two operators.");
	instrument_edit_menu->AppendRadioItem(ID_BASS_INSTRUMENT, "Bass Drum mode", "Bass Drum instruments use one operator.");
	instrument_edit_menu->AppendRadioItem(ID_SNARE_INSTRUMENT, "Snare Drum mode", "Snare Drum instruments use one operator.");
	instrument_edit_menu->AppendRadioItem(ID_TOM_INSTRUMENT, "Tom mode", "Tom instruments use one operator.");
	instrument_edit_menu->AppendRadioItem(ID_CYMBAL_INSTRUMENT, "Cymbal mode", "Cymbal instruments use one operator.");
	instrument_edit_menu->AppendRadioItem(ID_HIHAT_INSTRUMENT, "Hi-Hat mode", "Hi-Hat instruments use one operator.");
}

void AdVisualToolBar::show_tools(bool show, vector<wxToolBarToolBase*> tools) {
	if (show) {
		for (wxToolBarToolBase*& tool_base : tools) {
			AddTool(tool_base);
		}
	}
	else {
		for (wxToolBarToolBase*& tool_base : tools) {
			RemoveTool(tool_base->GetId());
		}
	}
}

void AdVisualToolBar::ShowComposerTools(bool show) {
	show_tools(show, composer_tools);
}
void AdVisualToolBar::ShowInsmakerTools(bool show) {
	update_insmaker_toolbar();
	show_tools(show, insmaker_tools);
}

class MainFrame : public wxFrame {
public:
	MainFrame();
	ComposerPanel* composer_panel;
	InsmakerPanel* insmaker_panel;
	AdVisualToolBar* toolbar;
	wxMenu* file_menu;
	wxMenu* help_menu;
private:
	// Main signals
	void on_show_composer_panel(wxCommandEvent& event);
	void on_show_insmaker_panel(wxCommandEvent& event);
	void on_play_track(wxCommandEvent& event);
	void on_load_track(wxCommandEvent& event);
	void on_save_track(wxCommandEvent& event);
	void on_load_bank(wxCommandEvent& event);
	void on_save_bank(wxCommandEvent& event);
	void on_exit(wxCommandEvent& event);
	void on_about(wxCommandEvent& event);
	void on_popup_file_menu(wxCommandEvent& event);
	void on_popup_help_menu(wxCommandEvent& event);
};


bool MainApp::OnInit()
{
	wxInitAllImageHandlers();
	MainFrame *frame = new MainFrame();
	frame->Show(true);
	adplayer = make_unique<AdPlayer>();
	wxArtProvider::Push(new AdVisualArtProvider);
	return true;
}

MainFrame::MainFrame() :
	wxFrame(nullptr, wxID_ANY, "AdVisual", wxDefaultPosition, wxSize(1920, 1024))
{
	SetMinSize(wxSize(320, 180));

	wxColour bg_colour = wxColour(15, 10, 20);
	toolbar = new AdVisualToolBar(this, wxID_ANY, wxDefaultPosition, wxSize(100, 36));
	toolbar->SetBackgroundColour(bg_colour);
	SetToolBar(toolbar);

	status_bar = CreateStatusBar();
	status_bar->SetBackgroundColour(bg_colour);

	composer_panel = new ComposerPanel(this);
	toolbar->CreateComposerTools(composer_panel);

	insmaker_panel = new InsmakerPanel(this);
	insmaker_panel->Show(false);
	toolbar->CreateInsmakerTools(insmaker_panel);
	toolbar->ShowInsmakerTools(false);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(composer_panel, 1, wxEXPAND);
	sizer->Add(insmaker_panel, 1, wxEXPAND);
	SetSizer(sizer);

	Bind(wxEVT_MENU, &MainFrame::on_exit, this, wxID_EXIT);
	Bind(wxEVT_MENU, &MainFrame::on_about, this, wxID_ABOUT);
	Bind(wxEVT_MENU, &MainFrame::on_show_composer_panel, this, ID_COMPOSER);
	Bind(wxEVT_MENU, &MainFrame::on_show_insmaker_panel, this, ID_INSMAKER);
	Bind(wxEVT_MENU, &MainFrame::on_play_track, this, ID_PLAY_TRACK);
	Bind(wxEVT_MENU, &MainFrame::on_load_track, this, ID_LOAD_TRACK);
	Bind(wxEVT_MENU, &MainFrame::on_save_track, this, ID_SAVE_TRACK);
	Bind(wxEVT_MENU, &MainFrame::on_save_track, this, ID_SAVE_TRACK_AS);
	Bind(wxEVT_MENU, &MainFrame::on_load_bank, this, ID_LOAD_BANK);
	Bind(wxEVT_MENU, &MainFrame::on_save_bank, this, ID_SAVE_BANK);
	Bind(wxEVT_MENU, &MainFrame::on_popup_file_menu, this, ID_FILE_MENU);
	Bind(wxEVT_MENU, &MainFrame::on_popup_help_menu, this, ID_HELP_MENU);

	file_menu = new wxMenu();
	file_menu->Append(ID_LOAD_TRACK, "&Load Track...\tCtrl-L", "Load ROL files");
	file_menu->Append(ID_SAVE_TRACK, "&Save Track...\tCtrl-S", "Save ROL file to location");
	file_menu->Append(ID_SAVE_TRACK_AS, "&Save Track As...\tShift-Ctrl-S", "Save ROL file to location");
	file_menu->AppendSeparator();
	file_menu->Append(ID_PLAY_TRACK, "&Play Track...\tCtrl-P", "Play any Adlib song.");
	file_menu->AppendSeparator();
	file_menu->Append(ID_LOAD_BANK, "&Load Bank", "Load BNK file");
	file_menu->Append(ID_SAVE_BANK, "&Save Bank As", "Save BNK file to location");
	file_menu->AppendSeparator();
	file_menu->Append(wxID_EXIT);

	help_menu = new wxMenu();
	help_menu->Append(wxID_ABOUT);
}

void AdVisualToolBar::update_insmaker_toolbar() {
	ToggleTool(ID_ADDITIVE_SYNTH, insmaker_panel->GetAdditiveSynth());
	if (insmaker_panel->GetAdditiveSynth()) {
		SetToolNormalBitmap(ID_ADDITIVE_SYNTH, GetAsset("AMSynth.svg", tool_size));
	}
	else {
		SetToolNormalBitmap(ID_ADDITIVE_SYNTH, GetAsset("FMSynth.svg", tool_size));
	}
	instrument_edit_menu->Check(ID_MELODIC_INSTRUMENT + insmaker_panel->GetPercussionMode(), true);
	instrument_edit_menu->UpdateUI();
}

void MainFrame::on_popup_file_menu(wxCommandEvent& event) {
	PopupMenu(file_menu);
}
void MainFrame::on_popup_help_menu(wxCommandEvent& event) {
	PopupMenu(help_menu);
}

void MainFrame::on_exit(wxCommandEvent &event) {
	Close(true);
}

void MainFrame::on_about(wxCommandEvent &event) {
	wxMessageBox("AdVisual is meant to be a remake of the original AdLib Visual Composer packed with AdLib Soundcards.\n\n \
    Created and maintained by RobertP-McDowell, 2026.\n \
    Depending on 'wxWidgets', 'AdPlug', 'Libbinio', and miniaudio.h.",
		"About AdVisual", wxOK | wxICON_INFORMATION);
}

void MainFrame::on_show_composer_panel(wxCommandEvent& event) {
	composer_panel->Show(true);
	insmaker_panel->Show(false);
	toolbar->ShowComposerTools(true);
	toolbar->ShowInsmakerTools(false);
}

void MainFrame::on_show_insmaker_panel(wxCommandEvent& event) {
	composer_panel->Show(false);
	insmaker_panel->Show(true);
	toolbar->ShowComposerTools(false);
	toolbar->ShowInsmakerTools(true);
}

void MainFrame::on_play_track(wxCommandEvent& event) {
	if (event.IsChecked()) {
		adplayer->play((string)current_track->file_path, cursor_tick);
	}
	else {
		adplayer->stop();
	}
}

void MainFrame::on_load_track(wxCommandEvent& event) {
	wxString filename = wxFileSelector("Select file to load", "~/Desktop", "", "", "ROL Files(*.ROL)|*.ROL|Reality Adlib(*.RAD)|*.RAD");
	if (!filename.empty()) {
		current_track->load_file(filename);
	}
	Refresh();
	Update();
}

void MainFrame::on_save_track(wxCommandEvent& event) {
	if (event.GetId() != ID_SAVE_TRACK_AS && current_track->file_path != wxEmptyString) {
		current_track->save_file(wxEmptyString);
	}
	else {
		wxString new_path = wxSaveFileSelector("~/Desktop", "ROL Files(*.ROL)|*.ROL|Reality Adlib(*.RAD)|*.RAD");
		if (!new_path.empty()) {
			current_track->save_file(new_path);
		}
	}
	Refresh();
	Update();
}

void MainFrame::on_load_bank(wxCommandEvent& event) {
	wxString filename = wxFileSelector("Select file to load", "~/Desktop", "", "", "BNK Files(*.BNK)|*.bnk;*.BNK|Instrument Files(*.INS)|*.ins;*.INS");
	if (!filename.empty()) {
		current_bank->load_file(filename);
	}
	char new_name[9] = "";
	insmaker_panel->SetInstrumentByName(new_name);
	toolbar->bank_ctrl->SetBank(current_bank.get());
	composer_panel->event_popup->bank_ctrl->SetBank(current_bank.get());
	Refresh();
	Update();
}

void MainFrame::on_save_bank(wxCommandEvent& event) {
	wxString filename = wxFileSelector("Select file to Save as", "~/Desktop", "", "", "BNK Files(*.BNK)|*.bnk;*.BNK|Instrument Files(*.INS)|*.ins;*.INS");
	insmaker_panel->save_properties_to_opl();
	if (!filename.empty()) {
		current_bank->save_file(filename);
	}
	Refresh();
	Update();
}

// Track Popup.
void AdVisualToolBar::on_popup_track_menu(wxCommandEvent& event) {
	track_edit_popup->Position(ClientToScreen((wxPoint(GetSize().x, GetSize().y) / 2) - track_edit_popup->GetSize()),
		track_edit_popup->GetSize());
	track_edit_popup->Popup();
}
void AdVisualToolBar::on_toggle_percussion(wxCommandEvent& event) {
	current_track->rhythm_mode = (event.IsChecked() ? 0 : 1);
	Refresh();
	Update();
}
void AdVisualToolBar::on_enter_basic_tempo(wxCommandEvent& event) {
	current_track->basic_tempo = get_float_from_string((string)event.GetString(), 0.01, 1000.0);
	Refresh();
	Update();
}
void AdVisualToolBar::on_enter_ticks_per_beat(wxCommandEvent& event) {
	current_track->ticks_per_beat = (int)get_float_from_string((string)event.GetString(), 0.01, 1000.0);
	Refresh();
	Update();
}
void AdVisualToolBar::on_enter_beats_per_measure(wxCommandEvent& event) {
	current_track->beats_per_measure = (int)get_float_from_string((string)event.GetString(), 0.01, 1000.0);
	Refresh();
	Update();
}

// Instrument menu.
void AdVisualToolBar::on_popup_instrument_menu(wxCommandEvent& event) {
	bank_popup->Show(false);
	PopupMenu(instrument_edit_menu);
}
void AdVisualToolBar::on_create_instrument(wxCommandEvent& event) {
	Instrument new_instrument = Instrument::default_instrument; // Copy the default instrument.
	strcpy(new_instrument.name, bank_ctrl->GetText());
	current_bank->add_instrument(new_instrument);
	bank_ctrl->SetBank(current_bank.get()); // Update entries in bank control.
	bank_ctrl->FilterString(new_instrument.name); // Go to new entry in bank control.
	insmaker_panel->SetInstrumentByName(new_instrument.name); // Set it in the Insmaker panel.
	update_insmaker_toolbar();
}
void AdVisualToolBar::on_copy_instrument(wxCommandEvent& event) {
	
}
void AdVisualToolBar::on_delete_instrument(wxCommandEvent& event) {
	current_bank->delete_instrument(bank_ctrl->GetText());
	bank_ctrl->SetBank(current_bank.get()); // Update entries in bank control.
	insmaker_panel->SetInstrumentByName(bank_ctrl->GetText()); // Essentially clearing the insmaker editor.
	update_insmaker_toolbar();
}
void AdVisualToolBar::on_set_instrument_percussion(wxCommandEvent& event) {
	if (event.GetId() < ID_MELODIC_INSTRUMENT || event.GetId() > ID_HIHAT_INSTRUMENT) {
		event.Skip();
		return;
	}
	insmaker_panel->SetPercussionMode(event.GetId() - ID_MELODIC_INSTRUMENT);
}

// Composer toolbar.
void AdVisualToolBar::on_channel_button_pressed(wxCommandEvent& event) {
	channel_buttons[composer_panel->GetChannelIndex()]->SetPressed(false);
	composer_panel->SetChannelIndex(event.GetId() - ID_VOICE_START);
	Refresh();
	Update();
}
void AdVisualToolBar::on_channel_button_enabled(wxCommandEvent& event) {
	adplayer->set_channel_enable(event.GetId() - ID_VOICE_START, (bool)event.GetInt());
	Refresh();
	Update();
}

void AdVisualToolBar::on_preview_channels_checked(wxCommandEvent& event) {
	composer_panel->SetPreviewChannels(event.IsChecked());
	Refresh();
	Update();
}

void AdVisualToolBar::on_grid_feedback_checked(wxCommandEvent& event) {
	composer_panel->SetAudioFeedback(event.IsChecked());
	Refresh();
	Update();
}

void AdVisualToolBar::on_follow_cursor_checked(wxCommandEvent& event) {
	//composer_panel->follow_cursor = true;
	Refresh();
	Update();
}

// Insmaker toolbar.
void AdVisualToolBar::on_select_instrument_set_focus(wxFocusEvent& event) {
	wxTextCtrl* text_tool = static_cast<wxTextCtrl*>(FindControl(event.GetId()));
	wxSize popup_size = wxSize(100, 100);
	wxPoint popup_pos = text_tool->GetPosition();
	popup_pos.y += text_tool->GetSize().y;
	bank_popup->Position(popup_pos - popup_size, popup_size);
	bank_popup->Show(true);
}

void AdVisualToolBar::on_select_instrument_kill_focus(wxFocusEvent& event) {
	bank_popup->Show(false);
}

void AdVisualToolBar::on_select_instrument_text_changed(wxCommandEvent& event) {
	wxTextCtrl* text_tool = static_cast<wxTextCtrl*>(FindControl(event.GetId()));
	char ins_name[9];
	wxString evt_str = event.GetString().MakeUpper();
	evt_str.resize(8);
	memcpy(&ins_name, evt_str.c_str(), 9);
	bank_ctrl->FilterString(ins_name);
}

void AdVisualToolBar::on_select_instrument_enter(wxCommandEvent& event) {
	wxTextCtrl* text_tool = static_cast<wxTextCtrl*>(FindControl(event.GetId()));
	char ins_name[9];
	wxString evt_str = event.GetString().MakeUpper();
	evt_str.resize(8);
	memcpy(&ins_name, evt_str.c_str(), 9);
	insmaker_panel->SetInstrumentByName(ins_name);
	update_insmaker_toolbar();
	event.Skip();
}

void AdVisualToolBar::on_bank_selector_select_item(wxCommandEvent& event) {
	wxTextCtrl* text_tool = static_cast<wxTextCtrl*>(FindControl(ID_INSTRUMENT_FIELD));
	char ins_name[9];
	wxString evt_str = wxString(event.GetString()).MakeUpper();
	evt_str.resize(8);
	memcpy(&ins_name, evt_str.c_str(), 9);
	text_tool->ChangeValue(ins_name);
	insmaker_panel->SetInstrumentByName(ins_name);
	update_insmaker_toolbar();
}

void AdVisualToolBar::on_toggle_additive_synth(wxCommandEvent& event) {
	insmaker_panel->SetAdditiveSynth(event.IsChecked());
	update_insmaker_toolbar();
}
