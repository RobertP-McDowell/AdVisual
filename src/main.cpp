// Start of wxWidgets "Hello World" Program
#include <wx/wx.h>
#include <wx/utils.h>
#include <wx/filedlg.h> 
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <wx/scrolwin.h>
#include <wx/scrolbar.h>
#include <wx/popupwin.h>
#include <Track.h>
#include <FileAccess.h>
#include <iostream>
#include <memory>
#include <deque>
#include <vector>


using namespace std;

const int pitch_range = 95;

unique_ptr<wxPopupWindow> c = nullptr;

class MainApp : public wxApp
{
public:
	bool OnInit() override;
};

wxIMPLEMENT_APP(MainApp);

wxDECLARE_EVENT(EVT_PRESSED, wxCommandEvent);
wxDECLARE_EVENT(EVT_UNPRESSED, wxCommandEvent);
wxDEFINE_EVENT(EVT_PRESSED, wxCommandEvent);
wxDEFINE_EVENT(EVT_UNPRESSED, wxCommandEvent);

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
		double outline_width = 2.0;
		wxPoint2DDouble center = (wxPoint2DDouble(GetSize().x, GetSize().y) / 2.0) - wxPoint2DDouble(outline_width / 2.0, outline_width / 2.0);
		double outline_radius = (GetSize().x / 2) - outline_width;
		double fill_radius = outline_radius - 4;
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
	const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0,
	const wxValidator& validator = wxDefaultValidator, const wxString& name = wxRadioButtonNameStr)
			 {
		button_colour = colour;
		Init();
		Create(parent, id, pos, size, style, validator, name);
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

class EventPopup : public wxPopupTransientWindow {
public:
	EventPopup(wxWindow* parent, shared_ptr<Track> track, shared_ptr<Channel> channel);
	int editing_event_tick;
	shared_ptr<Channel> current_channel;
	shared_ptr<Track> current_track;
	void Popup(int at_tick);
private:
	void init_event_field(wxGridSizer* sizer, wxTextCtrl*& event_field, int ID);
	void on_show(wxShowEvent& event);
	void on_text_entered(wxCommandEvent& event);
	wxTextCtrl* tempo_field;
	wxTextCtrl* instrument_field;
	wxTextCtrl* pitch_field;
	wxTextCtrl* volume_field;
};

class ComposerPanel : public wxPanel {
public:
	ComposerPanel(wxWindow *parent);
	shared_ptr<Track> current_track;
private:
	// grid _panelfunctions and events.
	void on_paint_grid(wxPaintEvent& event);
	void on_scroll_grid(wxScrollWinEvent& event);
	void draw_notes();
	void draw_grid();
	void on_lmb_down(wxMouseEvent& event);
	void on_lmb_up(wxMouseEvent& event);
//	void on_rmb_down();
//	void on_rmb_up();
	void on_mouse_motion(wxMouseEvent& event);
	// header functions and events.
	void on_channel_button_pressed(wxCommandEvent& event);
	void on_preview_channels_checked(wxCommandEvent& event);
	// event_header and events.
	void on_paint_event_header(wxPaintEvent& event);
	void on_mouse_motion_event_header(wxMouseEvent& event);
	void on_lmb_down_event_header(wxMouseEvent& event);
	void on_lmb_up_event_header(wxMouseEvent& event);
	wxFont event_font;
	

	wxPoint grid_offset = wxPoint(0, 0);
	wxSize note_size;
	double zoom = 1.0;
	wxPoint2DDouble mouse_down_start;
	bool preview_channels = false;
	unique_ptr<Note> editing_note;
	shared_ptr<Channel> current_channel;
	int current_channel_idx;
	unique_ptr<wxPanel> header;
	unique_ptr<wxPanel> event_header;
	unique_ptr<wxScrolledWindow> grid_panel;
	unique_ptr<EventPopup> event_field;
	vector<unique_ptr<ChannelButton>> channel_buttons;
};

class MainFrame : public wxFrame
{
public:
	MainFrame();

private:
	void on_load(wxCommandEvent &event);
	void on_play(wxCommandEvent &event);
	void on_exit(wxCommandEvent &event);
	void on_about(wxCommandEvent &event);
	unique_ptr<ComposerPanel> panel;
};

enum
{
	ID_COMPOSER = 1,
	ID_PLAY = 2,
	ID_LOAD = 3,
	ID_TEMPO_EVENT = 10,
	ID_INSTRUMENT_EVENT = 11,
	ID_PITCH_EVENT = 12,
	ID_VOLUME_EVENT = 13
};

bool MainApp::OnInit()
{
	wxInitAllImageHandlers();
	MainFrame *frame = new MainFrame();
	frame->Show(true);
	return true;
}

ComposerPanel::ComposerPanel(wxWindow *parent) : 
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
{
	wxColour bg_colour = wxColour(40, 40, 40);
	SetBackgroundColour(*wxBLACK);
	event_font = wxFont(wxFontInfo(10).Bold());
	note_size = wxSize(20.0, 20.0);
	header = make_unique<wxPanel>(this, wxID_ANY, wxDefaultPosition, wxSize(100, 30));
	header->SetBackgroundColour(bg_colour);
	event_header = make_unique<wxPanel>(this, wxID_ANY, wxDefaultPosition, wxSize(100, 30));
	event_header->SetBackgroundColour(bg_colour);
	event_header->Bind(wxEVT_PAINT, &ComposerPanel::on_paint_event_header, this);
	event_header->Bind(wxEVT_MOTION, &ComposerPanel::on_mouse_motion_event_header, this);
	event_header->Bind(wxEVT_LEFT_DOWN, &ComposerPanel::on_lmb_down_event_header, this);
	event_header->Bind(wxEVT_LEFT_UP, &ComposerPanel::on_lmb_up_event_header, this);
	
	grid_panel = make_unique<wxScrolledWindow>(this, wxID_ANY, wxDefaultPosition, wxSize(100, 100));
	grid_panel->SetBackgroundColour(bg_colour);
	grid_panel->EnableScrolling(true, true);
	grid_panel->ShowScrollbars(wxSHOW_SB_ALWAYS, wxSHOW_SB_DEFAULT);
	grid_panel->SetVirtualSize(wxSize(10000.0 / zoom, (note_size.y * pitch_range ) / zoom));
	grid_panel->SetScrollRate(4, 5);
	
	grid_panel->Bind(wxEVT_PAINT, &ComposerPanel::on_paint_grid, this);
	grid_panel->Bind(wxEVT_SCROLLWIN_THUMBTRACK, &ComposerPanel::on_scroll_grid, this);
	grid_panel->Bind(wxEVT_LEFT_DOWN, &ComposerPanel::on_lmb_down, this);
	grid_panel->Bind(wxEVT_LEFT_UP, &ComposerPanel::on_lmb_up, this);
	grid_panel->Bind(wxEVT_MOTION, &ComposerPanel::on_mouse_motion, this);
	
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(header.get(), 0, wxEXPAND | wxBOTTOM, 4);
	sizer->Add(event_header.get(), 0, wxEXPAND | wxBOTTOM, 4);
	sizer->Add(grid_panel.get(), 1, wxEXPAND);
	SetSizerAndFit(sizer);
	
	current_channel_idx = 0;
	current_track = make_shared<Track>();
	current_channel = (current_track->channels[0]);
	wxPoint control_offset(30, 0);
	for (int i = 0; i < 6; i++) {
		control_offset.x += 40;
		unique_ptr<ChannelButton> new_button = make_unique<ChannelButton>(header.get(), i, "", current_track->channels[i]->colour,
			control_offset, wxSize(30, 30));
		new_button->Bind(EVT_PRESSED, &ComposerPanel::on_channel_button_pressed, this, i);
		channel_buttons.push_back(move(new_button));
	}
	control_offset.x += 40;
	wxCheckBox* preview_channels_button = new wxCheckBox(this, wxID_ANY, "Channel Previews", control_offset);
	preview_channels_button->Bind(wxEVT_CHECKBOX, &ComposerPanel::on_preview_channels_checked, this);
	event_field = make_unique<EventPopup>(this, current_track, current_channel);
}

void EventPopup::init_event_field(wxGridSizer* sizer, wxTextCtrl*& event_field, int ID) {
	event_field = new wxTextCtrl(this, ID);
	const string field_names[4] = {"Tempo", "Instrument", "Pitch", "Volume"};
	event_field->SetHint(field_names[ID-ID_TEMPO_EVENT]);
	event_field->SetWindowStyle(wxTE_PROCESS_ENTER);
	event_field->Bind(wxEVT_TEXT_ENTER, &EventPopup::on_text_entered, this, ID);
	sizer->Add(event_field, 0, wxEXPAND | wxLEFT, 10);
	wxStaticText* field_name = new wxStaticText(this, ID, field_names[ID-ID_TEMPO_EVENT]);
	sizer->Add(field_name, 0, wxEXPAND | wxRIGHT, 10);
}

EventPopup::EventPopup(wxWindow* parent, shared_ptr<Track> track, shared_ptr<Channel> channel) : 
	wxPopupTransientWindow(parent, wxBORDER_NONE | wxPU_CONTAINS_CONTROLS), current_track(track), current_channel(channel)
{
	Bind(wxEVT_SHOW, &EventPopup::on_show, this);
	wxGridSizer* sizer = new wxGridSizer(2);
	init_event_field(sizer, tempo_field, ID_TEMPO_EVENT);
	init_event_field(sizer, instrument_field, ID_INSTRUMENT_EVENT);
	init_event_field(sizer, pitch_field, ID_PITCH_EVENT);
	init_event_field(sizer, volume_field, ID_VOLUME_EVENT);
	SetSizerAndFit(sizer);
}

#define update_event_field(event_field, get_last_event_callable, t_type, type_conversion_func, max_text_length) do { \
	int last_event_tick; \
	t_type last_event_value; \
	get_last_event_callable(editing_event_tick, last_event_tick, last_event_value); \
	if (last_event_tick == editing_event_tick) { \
		string new_text = type_conversion_func(last_event_value); \
		new_text.resize(max_text_length); \
		event_field->ChangeValue(new_text); \
		get_last_event_callable(editing_event_tick - 1, last_event_tick, last_event_value); \
	} \
	else { \
		event_field->ChangeValue(wxEmptyString); \
	} \
	string new_hint = type_conversion_func(last_event_value); \
	new_hint.resize(max_text_length); \
	event_field->SetHint( new_hint ); \
} while(0)

void EventPopup::Popup(int at_tick) {
	editing_event_tick = at_tick;
	update_event_field(tempo_field, current_track->get_last_tempo_event, float, to_string, 4);
	update_event_field(instrument_field, current_channel->get_last_instrument_event, string, , 9);
	update_event_field(pitch_field, current_channel->get_last_pitch_event, float, to_string, 4);
	update_event_field(volume_field, current_channel->get_last_volume_event, float, to_string, 4);
	wxPopupTransientWindow::Popup();
}


MainFrame::MainFrame() : 
	wxFrame(nullptr, wxID_ANY, "AdVisual", wxDefaultPosition, wxSize(1920, 1024))
{
	SetMinSize(wxSize(250, 250));
	
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(ID_PLAY, "&PLAY...\tCtrl-P", "Play any Adlib song.");
	menuFile->Append(ID_LOAD, "&Load...\tCtrl-L", "Load ROL files");
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);

	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT);

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(menuFile, "&File");
	menuBar->Append(menuHelp, "&Help");

	SetMenuBar( menuBar );

	CreateStatusBar();
	panel = make_unique<ComposerPanel>(this);

	Bind(wxEVT_MENU, &MainFrame::on_exit, this, wxID_EXIT);
	Bind(wxEVT_MENU, &MainFrame::on_about, this, wxID_ABOUT);
	Bind(wxEVT_MENU, &MainFrame::on_play, this, ID_PLAY);
	Bind(wxEVT_MENU, &MainFrame::on_load, this, ID_LOAD);
}

void MainFrame::on_exit(wxCommandEvent &event) {
	Close(true);
}

void MainFrame::on_about(wxCommandEvent &event) {
	wxMessageBox("AdVisual is meant to be a remake of the original AdLib Visual Composer for AdLib Soundcards.\n\n \
    Created and maintained by RobertP-McDowell, 2026.\n \
    Depending on 'wxWidgets', 'AdPlug', and 'libbinio' libraries.",
		"About AdVisual", wxOK | wxICON_INFORMATION);
}

void MainFrame::on_play(wxCommandEvent &event) {
	wxFileSelector("Select file to play", "~/Desktop", "", "", "*.ROL|*.RAD");
}

void MainFrame::on_load(wxCommandEvent &event) {
	wxString filename = wxFileSelector("Select file to play", "~/Desktop", "", "", "ROL Files(*.ROL)|*.ROL|Reality Adlib(*.RAD)|*.ROL");
	if (!filename.empty()) {
		FileAccess::LoadTrack((string)filename, *(panel->current_track.get()));
	}
	Refresh();
	Update();
}

void ComposerPanel::on_channel_button_pressed(wxCommandEvent& event) {
	channel_buttons[current_channel_idx]->SetPressed(false);
	current_channel_idx = event.GetId();
	current_channel = current_track->GetChannel(current_channel_idx);
	Refresh();
	Update();
}

void ComposerPanel::on_preview_channels_checked(wxCommandEvent& event) {
	preview_channels = event.IsChecked();
	Refresh();
	Update();
}

void ComposerPanel::on_scroll_grid(wxScrollWinEvent& event) {
	if (event.GetOrientation() == wxHORIZONTAL) {
		grid_offset.x = event.GetPosition();
	}
	else {
		grid_offset.y = event.GetPosition();
	}
	Refresh();
	Update();
}

void ComposerPanel::on_paint_grid(wxPaintEvent& event) {
	draw_grid();
	draw_notes();
}

void ComposerPanel::draw_grid() {
	wxPaintDC dc(grid_panel.get());
	wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
	if (gc == nullptr) return;
	wxGraphicsPen heavy_grid_pen = gc->CreatePen(wxGraphicsPenInfo(*wxColour(190, 190, 190)).Width(3.5).Style(wxPENSTYLE_SOLID));
	wxGraphicsPen dashed_grid_pen = gc->CreatePen(wxGraphicsPenInfo(*wxColour(140, 140, 140)).Width(2.5).Style(wxPENSTYLE_SHORT_DASH));
	wxGraphicsPen dotted_grid_pen = gc->CreatePen(wxGraphicsPenInfo(*wxColour(140, 140, 140)).Width(2.5).Style(wxPENSTYLE_DOT));
	int ticks_per_measure = current_track->beats_per_measure * current_track->ticks_per_beat;
	wxPoint draw_offstep = wxPoint(grid_offset.x % (note_size.x * ticks_per_measure), grid_offset.y % (note_size.y * 11));
	int panel_width;
	int panel_height;
	GetSize(&panel_width, &panel_height);
	wxSize cell_size(note_size.x / zoom, (note_size.y) / zoom);
	int columns = (panel_width / cell_size.x);
	int rows = min(pitch_range, panel_height / cell_size.y);
	int grid_sub = 0;
	gc->SetPen(dotted_grid_pen);
	for (int i = 0; i <= rows; i++) {
		if (i % 7 == 4 || i % 7 == 0)  {
			grid_sub++;
			continue;
		}
		double y = (cell_size.y * 2.0 * (i-(grid_sub/2.0)));
		gc->StrokeLine(0, y - draw_offstep.y, panel_width, y - draw_offstep.y);
	}
	gc->SetPen(heavy_grid_pen);
	double middle_c_y = cell_size.y * 50;
	gc->StrokeLine(0, grid_offset.y + middle_c_y, panel_width, grid_offset.y + middle_c_y);
	for (int measure = 0; measure <= columns; measure += ticks_per_measure) {
		gc->SetPen(heavy_grid_pen);
		double x = cell_size.x * measure;
		gc->StrokeLine(x - draw_offstep.x, 0, x - draw_offstep.x, panel_height);
		for (int beat = current_track->ticks_per_beat; beat < ticks_per_measure; beat += current_track->ticks_per_beat) {
			gc->SetPen(dashed_grid_pen);
			x = cell_size.x * (measure + beat);
			gc->StrokeLine(x - draw_offstep.x, -panel_height - draw_offstep.y, x - draw_offstep.x, panel_height);
		}
	}
	delete gc;
}

void ComposerPanel::draw_notes() {
	wxPaintDC dc(grid_panel.get());
	wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
	if (gc == nullptr) return;
	int ticks_per_measure = current_track->beats_per_measure * current_track->ticks_per_beat;
	int panel_width;
	int panel_height;
	wxSize middle_of_cell = note_size / 2;
	double note_pen_width = note_size.y / 4.0;
	wxGraphicsPen note_pen = gc->CreatePen(wxGraphicsPenInfo(
	current_channel->colour).Width(note_pen_width).Style(wxPENSTYLE_SOLID).Cap(wxCAP_BUTT).Join(wxJOIN_BEVEL));
	gc->SetPen(note_pen);
	gc->SetBrush(*wxBLACK_BRUSH);
	for (Note& note : current_channel->notes) {
		gc->DrawRoundedRectangle((note.offset * note_size.x) + 1 - grid_offset.x, (note.pitch * note_size.y) + 2 - grid_offset.y,
			(note.length * note_size.x) - 2, note_size.y - 4, note_size.y / 4.0);
	}
	wxGraphicsPen ghost_pen = gc->CreatePen(wxGraphicsPenInfo(
	*wxColour(215, 200, 255, 100)).Width(note_size.y - 2).Style(wxPENSTYLE_SOLID).Cap(wxCAP_BUTT).Join(wxJOIN_BEVEL));
	if (editing_note != nullptr) {
		gc->SetPen(ghost_pen);
		gc->StrokeLine((editing_note->offset * note_size.x) - grid_offset.x, (editing_note->pitch * note_size.y) + middle_of_cell.y - grid_offset.y,
		((editing_note->offset + editing_note->length) * note_size.x) - grid_offset.x, (editing_note->pitch * note_size.y) + middle_of_cell.y - grid_offset.y);
	}

	if (preview_channels) {
		for (int i = 0; i < current_track->get_channel_count(); i++) {
			if (i == current_channel_idx) {
				continue;
			}
			shared_ptr<Channel> channel = current_track->GetChannel(i);
			gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(
			channel->colour).Width(note_size.y / 8.0).Style(wxPENSTYLE_SOLID).Cap(wxCAP_BUTT).Join(wxJOIN_BEVEL)));
			int line_y_offset = ((note_size.y / 8.0) * i) + note_pen_width + 1;
			for (Note& note : channel->notes) {
				gc->StrokeLine((note.offset * note_size.x) + note_pen_width - grid_offset.x, (note.pitch * note_size.y) + line_y_offset - grid_offset.y,
				((note.offset + note.length) * note_size.x) - note_pen_width - grid_offset.x, (note.pitch * note_size.y) + line_y_offset - grid_offset.y);
			}
		}
	}

	delete gc;
}

void ComposerPanel::on_lmb_down(wxMouseEvent& event) {
	mouse_down_start = event.GetPosition() + grid_offset;
	editing_note = make_unique<Note>();
	editing_note->offset = mouse_down_start.m_x / note_size.x;
	editing_note->pitch = mouse_down_start.m_y / note_size.y;
	editing_note->length = 1;
}

void ComposerPanel::on_lmb_up(wxMouseEvent& event) {
	if (editing_note != nullptr) {
		current_channel->add_note(*editing_note);
	}
	editing_note = nullptr;
	Refresh();
	Update();
}

void ComposerPanel::on_mouse_motion(wxMouseEvent& event) {
	if (event.LeftIsDown() && editing_note != nullptr) {
		wxPoint2DDouble local_mouse_position = event.GetPosition() + grid_offset;
		int start_offset = (mouse_down_start.m_x / note_size.x);
		int end_offset = (local_mouse_position.m_x / note_size.x);
		if (end_offset >= start_offset) {
			editing_note->length = (end_offset - start_offset) + 1;
			editing_note->offset = start_offset;
		}
		else {
			editing_note->length = (start_offset - end_offset) + 1;
			editing_note->offset = end_offset;
		}
		Refresh();
		Update();
	}
}

void ComposerPanel::on_paint_event_header(wxPaintEvent& event) {
	wxPaintDC dc(event_header.get());
	wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
	if (gc == nullptr) return;
	int panel_width;
	int panel_height;
	GetSize(&panel_width, &panel_height);
	int event_header_width;
	int event_header_height;
	event_header->GetSize(&event_header_width, &event_header_height);
	wxSize event_bar_size = wxSize(note_size.x, event_header_height);
	vector<int16_t> event_ticks;
	wxGraphicsPen note_pen = gc->CreatePen(wxGraphicsPenInfo(
	current_channel->colour).Width(1.25).Style(wxPENSTYLE_SOLID).Cap(wxCAP_BUTT).Join(wxJOIN_BEVEL));
	gc->SetPen(note_pen);
	gc->SetFont(event_font, *wxWHITE);
	gc->SetBrush(*wxBLACK_BRUSH);
	for (auto event = current_track->tempo_events.begin(); event != current_track->tempo_events.end(); event++) {
		int event_on_grid = (event->first * note_size.x) - grid_offset.x;
		if (event_on_grid > panel_width) break;
		if (event_on_grid < -(note_size.x * 20)) continue;
		gc->DrawRectangle(event_on_grid, 0, event_bar_size.x, event_bar_size.y);
		event_ticks.push_back(event->first);
		//string text = to_string(event->second);
		//text.resize(3);
		//gc->DrawText(text, event_on_grid - grid_offset.x, 0);
	}
	for (auto event = current_channel->instrument_events.begin(); event != current_channel->instrument_events.end(); event++) {
		int event_on_grid = (event->first * note_size.x) - grid_offset.x;
		if (event_on_grid > panel_width) break;
		if (event_on_grid < -(note_size.x * 20) || count(event_ticks.begin(), event_ticks.end(), event->first)) continue;
		gc->DrawRectangle(event_on_grid, 0, event_bar_size.x, event_bar_size.y);
		event_ticks.push_back(event->first);
	}
	for (auto event = current_channel->pitch_events.begin(); event != current_channel->pitch_events.end(); event++) {
		int event_on_grid = (event->first * note_size.x) - grid_offset.x;
		if (event_on_grid > panel_width) break;
		if (event_on_grid < -(note_size.x * 20) || count(event_ticks.begin(), event_ticks.end(), event->first)) continue;
		gc->DrawRectangle(event_on_grid, 0, event_bar_size.x, event_bar_size.y);
		event_ticks.push_back(event->first);
	}
	for (auto event = current_channel->volume_events.begin(); event != current_channel->volume_events.end(); event++) {
		int event_on_grid = (event->first * note_size.x) - grid_offset.x;
		if (event_on_grid > panel_width) break;
		if (event_on_grid < -(note_size.x * 20) || count(event_ticks.begin(), event_ticks.end(), event->first)) continue;
		gc->DrawRectangle(event_on_grid, 0, event_bar_size.x, event_bar_size.y);
		event_ticks.push_back(event->first);
	}
	delete gc;
}

void ComposerPanel::on_mouse_motion_event_header(wxMouseEvent& event) {
	//wxPoint2DDouble local_mouse_position = event.GetPosition() + grid_offset;
}

void ComposerPanel::on_lmb_down_event_header(wxMouseEvent& event) {
	
}

void ComposerPanel::on_lmb_up_event_header(wxMouseEvent& event) {
	int mouse_tick = (event.GetPosition().x + grid_offset.x) / note_size.x;
	for (auto event = current_track->tempo_events.begin(); event != current_track->tempo_events.end(); event++) {
		if (event->first == mouse_tick) {
			
		}
		else if (event->first > mouse_tick) break;
	}
	event_field->Position(ClientToScreen(wxPoint(0, 0)), (GetSize() / 2.0) - (event_field->GetSize() / 2.0));
	event_field->Popup(mouse_tick);
}

float get_float_from_string(string str_val, float min = 0.0, float max = 1.0) {
	if (str_val.empty()) return -1.0;
	float ret_float = 1.0;
	try {
		ret_float = stof(str_val);
		ret_float = clamp(ret_float, min, max);
	}
	catch (invalid_argument e) {
		cerr << str_val << " Not a float!\n";
		ret_float = -1.0;
	}
	catch (out_of_range e) {
		cerr << str_val << " Float out of range\n";
		ret_float = -1.0;
	}
	return ret_float;
}

void EventPopup::on_text_entered(wxCommandEvent& event) {
	cout << "Text entered" << "\n";
}

void EventPopup::on_show(wxShowEvent& event) {
	if (event.IsShown() == false) {
		current_track->set_tempo_event(editing_event_tick, get_float_from_string((string)tempo_field->GetValue(), 0.01, 10.0));
		current_channel->set_instrument_event(editing_event_tick, (string)instrument_field->GetValue());
		cout << "Pitch: " << pitch_field->GetValue() << " " << (string)pitch_field->GetValue() << "\n";
		current_channel->set_pitch_event(editing_event_tick, get_float_from_string((string)pitch_field->GetValue(), 0.0, 2.0));
		current_channel->set_volume_event(editing_event_tick, get_float_from_string((string)volume_field->GetValue(), 0.0, 1.0));
		for (auto it = current_channel->pitch_events.begin(); it != current_channel->pitch_events.end(); it++) {
			cout << "Tick: " << it->first << " Value: " << it->second << "\n";
		}
	}
}

