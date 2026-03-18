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
#include <AdPlayer.h>
#include <ComposerPanel.h>
#include <InsmakerPanel.h>
#include <iostream>
#include <memory>
#include <deque>
#include <vector>


using namespace std;

unique_ptr<AdPlayer> adplayer = nullptr;

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

wxDECLARE_EVENT(EVT_PRESSED, wxCommandEvent);
wxDECLARE_EVENT(EVT_UNPRESSED, wxCommandEvent);
wxDEFINE_EVENT(EVT_PRESSED, wxCommandEvent);
wxDEFINE_EVENT(EVT_UNPRESSED, wxCommandEvent);

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
	void on_pressed(wxMouseEvent& event) {
		SendPressedEvent();
		pressed = true;
		Refresh();
		Update();
	}
	void on_mouse_enter(wxMouseEvent& event) {
		hovered = true;
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
		wxGraphicsPen solid_pen = gc->CreatePen(wxGraphicsPenInfo(button_colour).Width(outline_width).Style(wxPENSTYLE_SOLID));
		gc->SetPen(solid_pen);
		wxGraphicsPath outline_path = gc->CreatePath();
		outline_path.AddCircle(center.m_x, center.m_y, outline_radius);
		outline_path.CloseSubpath();
		gc->StrokePath(outline_path);
		if (pressed || hovered) {
			wxColour fill_colour = (pressed ? button_colour : *wxLIGHT_GREY);
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
	void SetPressed(bool value) { // Won't trigger event.
		pressed = value;
	}
protected:
	virtual wxSize DoGetBestSize() const { return wxSize(20.0, 20.0); }
	void SendPressedEvent() {
		wxCommandEvent event(EVT_PRESSED, GetId());
		event.SetEventObject(this);
		ProcessWindowEvent(event);
	}
	void SendUnpressedEvent() {
		wxCommandEvent event(EVT_UNPRESSED, GetId());
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
	void CreateComposerTools(ComposerPanel* composer_panel);
	void ShowComposerTools(bool show);
	void CreateInsmakerTools(InsmakerPanel* insmaker_panel);
	void ShowInsmakerTools(bool show);
	wxPopupWindow* bank_popup;
	BankControl* bank_ctrl;
private:
	void show_tools(bool show, vector<wxToolBarToolBase*> tools);
	// Composer events:
	void on_channel_button_pressed(wxCommandEvent& event);
	void on_preview_channels_checked(wxCommandEvent& event);
	// Insmaker events:
	void on_select_instrument_set_focus(wxFocusEvent& event);
	void on_select_instrument_kill_focus(wxFocusEvent& event);
	void on_select_instrument_text_changed(wxCommandEvent& event);
	void on_select_instrument_enter(wxCommandEvent& event);
	void on_bank_selector_select_item(wxCommandEvent& event);
	
	vector<wxToolBarToolBase*> composer_tools;
	vector<wxToolBarToolBase*> insmaker_tools;
	vector<ChannelButton*> channel_buttons;
	wxSize tool_size;
	wxSize square_size;
};

AdVisualToolBar::AdVisualToolBar(wxWindow* parent, int id, wxPoint position, wxSize size, long style, const wxString& name) :
		wxToolBar(parent, id, position, size, style, name) {
	wxSize tool_size = wxSize(54, 36) / 1.5;
	wxSize square_size = wxSize(tool_size.y, tool_size.y);
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
	// Start Playback options.
	AddSeparator();
	AddTool(ID_PLAY_TRACK, "Play", GetAsset("PlayButton.svg", square_size), wxNullBitmap, wxITEM_CHECK,
		"Start/Stop", "Start/Stop Track playback.");
}

void AdVisualToolBar::CreateComposerTools(ComposerPanel* composer_panel) {
	wxSize tool_size = wxSize(54, 36) / 1.5;
	wxSize square_size = wxSize(tool_size.y, tool_size.y);
	// Start Composer ToolBar.
	// We add to composer tools, so we can hide and show them at will.
	AddSeparator();

	for (int i = ID_VOICE_START; i < ID_VOICE_END; i++) {
		ChannelButton* new_button = new ChannelButton(this, i, to_string(i + 1 - ID_VOICE_START),
			composer_panel->current_track->channels[i - ID_VOICE_START]->colour, wxDefaultPosition, wxSize(62, 36));
		new_button->Bind(EVT_PRESSED, &AdVisualToolBar::on_channel_button_pressed, this, i);
		composer_tools.push_back(AddControl(new_button, "CH" + to_string(i)));
		channel_buttons.push_back(new_button);
	}
	
	composer_tools.push_back( AddTool(ID_PREVIEW_CHANNELS, "Preview Channels", GetAsset("OpenedEye.svg", tool_size),
		wxNullBitmap, wxITEM_CHECK, "Preview Channels", "Preview Notes from other Channels on the grid.") );
	
	Bind(wxEVT_MENU, &AdVisualToolBar::on_preview_channels_checked, this, ID_PREVIEW_CHANNELS);
	
	composer_tools.push_back( AddTool(ID_TRACK_OPTIONS, "Options", GetAsset("Cassete.svg", tool_size), wxNullBitmap, wxITEM_NORMAL,
		"Track Options", "Change base Track settings.") );
	AddSeparator();
	composer_tools.push_back( AddTool(ID_PIANO_GUIDE, "PianoGuide", GetAsset("PianoGuideButton.svg", tool_size), wxNullBitmap, wxITEM_NORMAL,
		"Piano Guide", "Show a piano on the grid.") );
}

void AdVisualToolBar::CreateInsmakerTools(InsmakerPanel* insmaker_panel) {
	wxSize tool_size = wxSize(54, 36) / 1.5;
	wxSize square_size = wxSize(tool_size.y, tool_size.y);
	// Start Composer ToolBar.
	// We add to composer tools, so we can hide and show them at will.
	AddSeparator();
	wxTextCtrl* instrument_select_field = new wxTextCtrl(this, ID_INSTRUMENT_FIELD, wxEmptyString, wxDefaultPosition, wxSize(125, 1), wxTE_PROCESS_ENTER);
	instrument_select_field->Bind(wxEVT_SET_FOCUS, &AdVisualToolBar::on_select_instrument_set_focus, this, wxID_ANY);
	instrument_select_field->Bind(wxEVT_KILL_FOCUS, &AdVisualToolBar::on_select_instrument_kill_focus, this, wxID_ANY);
	instrument_select_field->Bind(wxEVT_TEXT, &AdVisualToolBar::on_select_instrument_text_changed, this, ID_INSTRUMENT_FIELD);
	instrument_select_field->Bind(wxEVT_TEXT_ENTER, &AdVisualToolBar::on_select_instrument_enter, this, ID_INSTRUMENT_FIELD);
	insmaker_tools.push_back(AddControl(instrument_select_field, "Select Ins"));
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
	void on_resize(wxSizeEvent& event);
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

	toolbar = new AdVisualToolBar(this, wxID_ANY, wxDefaultPosition, wxSize(100, 36));
	SetToolBar(toolbar);

	CreateStatusBar();

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

	file_menu = new wxMenu("File");
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

	help_menu = new wxMenu("Help");
	help_menu->Append(wxID_ABOUT);
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
	wxMessageBox("AdVisual is meant to be a remake of the original AdLib Visual Composer for AdLib Soundcards.\n\n \
    Created and maintained by RobertP-McDowell, 2026.\n \
    Depending on 'wxWidgets', 'AdPlug' & 'Libbinio', and miniaudio.h.",
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
		adplayer->play((string)composer_panel->current_track->file_path);
	}
	else {
		adplayer->stop();
	}
}

void MainFrame::on_load_track(wxCommandEvent& event) {
	wxString filename = wxFileSelector("Select file to load", "~/Desktop", "", "", "ROL Files(*.ROL)|*.ROL|Reality Adlib(*.RAD)|*.RAD");
	if (!filename.empty()) {
		composer_panel->current_track->LoadFromFile(filename);
	}
	Refresh();
	Update();
}

void MainFrame::on_save_track(wxCommandEvent& event) {
	if (event.GetId() != ID_SAVE_TRACK_AS && composer_panel->current_track->file_path != wxEmptyString) {
		composer_panel->current_track->SaveToFile(wxEmptyString);
	}
	else {
		wxString new_path = wxSaveFileSelector("~/Desktop", "ROL Files(*.ROL)|*.ROL|Reality Adlib(*.RAD)|*.RAD");
		if (!new_path.empty()) {
			composer_panel->current_track->SaveToFile(new_path);
		}
	}
	Refresh();
	Update();
}

void MainFrame::on_load_bank(wxCommandEvent& event) {
	wxString filename = wxFileSelector("Select file to load", "~/Desktop", "", "", "BNK Files(*.BNK)|*.bnk;*.BNK|Instrument Files(*.INS)|*.ins;*.INS");
	if (!filename.empty()) {
		insmaker_panel->GetBank()->LoadFromFile(filename);
	}
	char new_name[9] = "ACCORDN";
	insmaker_panel->SetInstrumentByName(new_name);
	toolbar->bank_ctrl->SetBank(insmaker_panel->GetBank().get());
	composer_panel->event_popup->bank_ctrl->SetBank(insmaker_panel->GetBank().get());
	Refresh();
	Update();
}

void MainFrame::on_save_bank(wxCommandEvent& event) {
	wxString filename = wxFileSelector("Select file to Save as", "~/Desktop", "", "", "BNK Files(*.BNK)|*.bnk;*.BNK|Instrument Files(*.INS)|*.ins;*.INS");
	if (!filename.empty()) {
		insmaker_panel->GetBank()->SaveToFile(filename);
	}
	Refresh();
	Update();
}

void AdVisualToolBar::on_channel_button_pressed(wxCommandEvent& event) {
	ComposerPanel* composer_panel = static_cast<MainFrame*>(GetParent())->composer_panel;
	channel_buttons[composer_panel->GetChannelIndex()]->SetPressed(false);
	composer_panel->SetChannelIndex(event.GetId() - ID_VOICE_START);
	Refresh();
	Update();
}
void AdVisualToolBar::on_preview_channels_checked(wxCommandEvent& event) {
	ComposerPanel* composer_panel = static_cast<MainFrame*>(GetParent())->composer_panel;
	composer_panel->SetPreviewChannels(event.IsChecked());
	Refresh();
	Update();
}

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
	InsmakerPanel* insmaker_panel = static_cast<MainFrame*>(GetParent())->insmaker_panel;
	wxTextCtrl* text_tool = static_cast<wxTextCtrl*>(FindControl(event.GetId()));
	char ins_name[9];
	wxString evt_str = event.GetString().MakeUpper();
	evt_str.resize(8);
	memcpy(&ins_name, evt_str.c_str(), 9);
	bank_ctrl->FilterString(ins_name);
}

void AdVisualToolBar::on_select_instrument_enter(wxCommandEvent& event) {
	InsmakerPanel* insmaker_panel = static_cast<MainFrame*>(GetParent())->insmaker_panel;
	wxTextCtrl* text_tool = static_cast<wxTextCtrl*>(FindControl(event.GetId()));
	char ins_name[9];
	wxString evt_str = event.GetString().MakeUpper();
	evt_str.resize(8);
	memcpy(&ins_name, evt_str.c_str(), 9);
	insmaker_panel->SetInstrumentByName(ins_name);
	event.Skip();
}

void AdVisualToolBar::on_bank_selector_select_item(wxCommandEvent& event) {
	InsmakerPanel* insmaker_panel = static_cast<MainFrame*>(GetParent())->insmaker_panel;
	wxTextCtrl* text_tool = static_cast<wxTextCtrl*>(FindControl(ID_INSTRUMENT_FIELD));
	char ins_name[9];
	wxString evt_str = wxString(event.GetString()).MakeUpper();
	evt_str.resize(8);
	memcpy(&ins_name, evt_str.c_str(), 9);
	text_tool->ChangeValue(ins_name);
	insmaker_panel->SetInstrumentByName(ins_name);
}
