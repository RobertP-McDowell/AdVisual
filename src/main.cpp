#include <gtkmm.h>
#include <common.h>
#include <ComposerPanel.h>
#include <InsmakerPanel.h>
#include <CommonWidgets.h>
#include <AdPlayer.h>
#include <Util/HelpWindow.h>

class MainWindow;

Gtk::MenuButton create_image_menu_button(string image_name) {
using namespace Gtk;
	MenuButton bttn;
	Image bttn_img(ICON_PATH(image_name));
	bttn.set_child(bttn_img);
	shared_ptr<Gio::Menu> menu = Gio::Menu::create();
	bttn.set_menu_model(menu);
	bttn_img.set_icon_size(IconSize::LARGE);
	return bttn;
}

class Toolbar : public Gtk::Box {
public:
	Toolbar();
	void create_composer_tools(shared_ptr<ComposerPanel> p_composer_panel);
	void create_insmaker_tools(shared_ptr<InsmakerPanel> p_insmaker_panel);
	enum InsEntryMode {
		FIND_INS,
		CREATE_INS,
		DELETE_INS,
		RENAME_INS,
		DUPLICATE_INS
	};
	// Insmaker actions.
	void set_instrument_entry_mode(InsEntryMode mode = InsEntryMode::FIND_INS) {
		instrument_entry_mode = mode;
		instrument_entry.grab_focus();
		switch (instrument_entry_mode) {
		case InsEntryMode::FIND_INS:
			status->set_text("Find instrument to edit.");
			break;
		case InsEntryMode::CREATE_INS:
			status->set_text("Press enter to create new instrument with name.");
			break;
		case InsEntryMode::DELETE_INS:
			status->set_text("Press enter to confirm deletion!");
			break;
		case InsEntryMode::RENAME_INS:
			status->set_text("Press enter to rename instrument with new name.");
			break;
		case InsEntryMode::DUPLICATE_INS:
			status->set_text("Press enter to duplicate instrument with new name.");
			break;
		}
	}
	Gtk::Box composer_toolbar;
	Gtk::Box insmaker_toolbar;
private:
	InsEntryMode instrument_entry_mode = InsEntryMode::FIND_INS;
	// General signals.
	void on_file_pressed();
	void on_help_pressed();
	void on_play_track();
	// Composer signals.
	void on_preview_channels_toggled();
	void on_audio_feedback_toggled();
	// Insmaker signals.
	void on_instrument_entry_text_changed() {
		bank_ctrl_popover.popup();
		string text = instrument_entry.get_text();
		string_to_upper(text);
		bank_ctrl.search(text);
		if (text == instrument_entry.get_text()) {
			return; // Don't change text if it's already uppercase.
		}
		int caret_pos = instrument_entry.get_position();
		instrument_entry.set_text(text);
		instrument_entry.set_position(caret_pos);
	}
	void on_instrument_entry_text_entered() {
		bank_ctrl_popover.set_visible(false);
		switch (instrument_entry_mode) {
		case InsEntryMode::FIND_INS: {
			Instrument* ins = current_bank->find_instrument(instrument_entry.get_text());
			if (ins == &(Instrument::default_instrument)) {
				cout << "Couldn't find an instrument with the name '" << instrument_entry.get_text() << "'\n";
				return;
			}
			insmaker_panel->set_instrument(ins);
			return; } // Only FIND_INS will return immediately, everything else has to break.
		case InsEntryMode::CREATE_INS: {
			cout << "Create instrument\n";
			if (instrument_entry.get_text().empty()) { break; }
			Instrument new_ins = Instrument::default_instrument;
			memcpy(new_ins.name, instrument_entry.get_text().c_str(), 9);
			current_bank->add_instrument(new_ins);
			insmaker_panel->set_instrument(current_bank->find_instrument(new_ins.name));
			break; }
		case InsEntryMode::DELETE_INS: {
			cout << "Delete instrument\n";
			current_bank->delete_instrument(instrument_entry.get_text().c_str());
			insmaker_panel->set_instrument(nullptr);
			break; }
		case InsEntryMode::RENAME_INS: {
			cout << "Rename instrument\n";
			if (instrument_entry.get_text().empty()) { break; }
			insmaker_panel->rename_instrument(instrument_entry.get_text().c_str());
			break; }
		case InsEntryMode::DUPLICATE_INS: {
			cout << "Duplicate instrument\n";
			if (instrument_entry.get_text().empty()) { break; }
			Instrument new_ins = *(insmaker_panel->get_instrument());
			memcpy(new_ins.name, instrument_entry.get_text().c_str(), 9);
			current_bank->add_instrument(new_ins);
			insmaker_panel->set_instrument(current_bank->find_instrument(new_ins.name));
			break; }
		}
		instrument_entry_mode = InsEntryMode::FIND_INS;
		status->set_text("");
	}
	void on_bank_ctrl_instrument_selected(Instrument* instrument) {
		instrument_entry.set_text((string)instrument->name);
		insmaker_panel->set_instrument(instrument);
		instrument_entry.set_position(instrument_entry.get_text().length());
	}
	void on_instrument_entry_dropdown_pressed(Gtk::Entry::IconPosition icon_pos) {
		if (icon_pos == Gtk::Entry::IconPosition::PRIMARY) {
			instrument_entry.grab_focus();
			bank_ctrl_popover.popup();
		}
		else {
			instrument_edit_menu.set_pointing_to(instrument_entry.get_icon_area(Gtk::Entry::IconPosition::SECONDARY));
			instrument_edit_menu.popup();
		}
	}
	void on_global_lmb_down(int n_press, double x, double y) {
		Gtk::Widget* focused = get_root()->get_focus();
		if (!focused) {
			return;
		}
		// Hide bank_ctrl completion_popover on click away.
		if (focused->is_ancestor(instrument_entry) && !int(instrument_entry.get_state_flags() & Gtk::StateFlags::PRELIGHT)) {
			int caret_pos = instrument_entry.get_position();
			if (insmaker_panel->get_instrument_ptr()) { instrument_entry.set_text((string)insmaker_panel->get_instrument_ptr()->name); }
			else { instrument_entry.set_text(""); }
			instrument_entry.set_position(caret_pos);
			bank_ctrl_popover.set_visible(false);
		}
	}
	// General widgets.
	Gtk::ToggleButton composer_button;
	Gtk::ToggleButton play_button;
	// Composer widgets.
	Gtk::ToggleButton preview_channels_button;
	Gtk::ToggleButton track_settings_button;
	Gtk::ToggleButton audio_feedback_button;
	// Insmaker widgets.
	Gtk::Entry instrument_entry;
	Gtk::PopoverMenu instrument_edit_menu;
	Gtk::Popover bank_ctrl_popover;
	BankCtrl bank_ctrl;
	shared_ptr<ComposerPanel> composer_panel;
	shared_ptr<InsmakerPanel> insmaker_panel;
};

void Toolbar::on_preview_channels_toggled() {
	composer_panel->set_preview_channels(preview_channels_button.get_active());
}
void Toolbar::on_audio_feedback_toggled() {
	composer_panel->set_audio_feedback(audio_feedback_button.get_active());
}

Toolbar::Toolbar() : Gtk::Box(Gtk::Orientation::HORIZONTAL, 0) {
using namespace Gtk;
	set_css_classes({"toolbar"});
	MenuButton file_button = create_image_menu_button("FloppyDisk.svg");
	shared_ptr<Gio::Menu> file_menu = static_pointer_cast<Gio::Menu>(file_button.get_menu_model());
	file_menu->append("Load", "actions.load");
	file_menu->append("Save", "actions.save");
	file_menu->append("Save Panel", "actions.save_panel");
	file_menu->append("Save As", "actions.save_as");
	file_menu->append("Help", "actions.show_help");
	file_menu->append("Settings", "actions.open_settings");
	file_menu->append("Quit", "actions.quit");
	file_button.set_can_focus(false);
	append(file_button);

	composer_button = create_image_button("ComposerIcon.svg");
	composer_button.set_active();
	composer_button.set_action_name("actions.open_panel");
	composer_button.set_action_target_value(Glib::create_variant(0));
	composer_button.set_can_focus(false);
	append(composer_button);
	ToggleButton insmaker_button = create_image_button("InsmakerIcon.svg");
	insmaker_button.set_action_name("actions.open_panel");
	insmaker_button.set_action_target_value(Glib::create_variant(1));
	insmaker_button.set_can_focus(false);
	append(insmaker_button);
	play_button = create_image_button("PlayButton.svg");
	play_button.set_action_name("actions.toggle_play_song");
	play_button.set_can_focus(false);
	append(play_button);
	append(composer_toolbar);
	append(insmaker_toolbar);
	insmaker_toolbar.hide();
}

void Toolbar::create_composer_tools(shared_ptr<ComposerPanel> p_composer_panel) {
using namespace Gtk;
	composer_panel = p_composer_panel;
	composer_toolbar.set_can_focus(false);
	for (int i = 0; i < 11; i++) {
		string channel_name = "CH" + to_string(i + 1);
		ChannelButton* channel_button = make_managed<ChannelButton>(i);
		channel_button->set_name(channel_name);
		composer_toolbar.append(*channel_button);
	}
	Grid* grid = make_managed<Grid>();
	ChannelButton::set_pressed_channel(0);
	preview_channels_button = create_image_button("OpenedEye.svg", false);
	preview_channels_button.set_active(true);
	preview_channels_button.signal_clicked().connect(mem_fun(*this, &Toolbar::on_preview_channels_toggled));
	grid->attach(preview_channels_button, 0, 0);

	audio_feedback_button = create_image_button("AudioFeedback.svg", false);
	audio_feedback_button.set_active(true);
	audio_feedback_button.signal_clicked().connect(mem_fun(*this, &Toolbar::on_audio_feedback_toggled));
	grid->attach(audio_feedback_button, 1, 0);

	ToggleButton insert_mode_button = create_image_button("InsertMode.svg", false);
	insert_mode_button.set_action_name("composer.toggle_insert_mode");
	grid->attach(insert_mode_button, 0, 1);

	grid->set_row_spacing(0);
	grid->set_column_spacing(0);
	composer_toolbar.append(*grid);

	track_settings_button = create_image_button("Cassete.svg");
	track_settings_button.set_action_name("composer.show_track_settings");
	composer_toolbar.append(track_settings_button);

	composer_panel->track_settings.signal_visible_change.connect(sigc::ptr_fun(&ChannelButton::update_melodic_mode));
	signal_track_changed.connect(sigc::ptr_fun(&ChannelButton::update_melodic_mode));
}

void Toolbar::on_play_track() {
using namespace Gtk;
	if (play_button.get_active()) {
		adplayer->play(current_track->file_path, cursor_tick);
	}
	else {
		adplayer->stop();
	}
}

class MainWindow : public Gtk::Window {
public:
	MainWindow();
	double opacity = 0.5;
	shared_ptr<Toolbar> toolbar;
	shared_ptr<Gtk::CssProvider> css_provider = Gtk::CssProvider::create();
	shared_ptr<ComposerPanel> composer_panel;
	shared_ptr<InsmakerPanel> insmaker_panel;
	static shared_ptr<Gtk::GestureClick> global_lmb_gesture;
protected:
	HelpWindow help_window;
	void show_help();
	void open_panel(int p_panel);
	void load();
	void save();
	void load_panel();
	void save_panel();
	void save_as();
	void on_select_file(bool loading_bank, bool saving);
	void on_file_selected(const shared_ptr<Gio::AsyncResult>& result, const shared_ptr<Gtk::FileDialog>& dialog, bool saving);
	bool on_close_request();
	void on_close_dialog_choose(const shared_ptr<Gio::AsyncResult>& result, const shared_ptr<Gtk::AlertDialog>& dialog);
};

shared_ptr<Gtk::GestureClick> MainWindow::global_lmb_gesture = nullptr;

static void menu_append_radio(shared_ptr<Gio::Menu> menu, string name, string action, int value) {
	shared_ptr<Gio::MenuItem> rhythm_item = Gio::MenuItem::create(name, action);
	rhythm_item->set_action_and_target(action, Glib::Variant<int>::create(value));
	menu->append_item(rhythm_item);
}

void Toolbar::create_insmaker_tools(shared_ptr<InsmakerPanel> p_insmaker_panel) {
using namespace Gtk;
	insmaker_panel = p_insmaker_panel;
	instrument_entry.set_max_length(8);
	instrument_entry.signal_changed().connect(mem_fun(*this, &Toolbar::on_instrument_entry_text_changed));
	instrument_entry.signal_activate().connect(mem_fun(*this, &Toolbar::on_instrument_entry_text_entered));
	shared_ptr<Gdk::Texture> dropdown_texture = Gdk::Texture::create_from_filename(ICON_PATH("MagnifyingGlass.svg"));
	instrument_entry.set_icon_from_paintable(dropdown_texture, Entry::IconPosition::PRIMARY);
	shared_ptr<Gdk::Texture> settings_texture = Gdk::Texture::create_from_filename(ICON_PATH("ThreeDots.svg"));
	instrument_entry.set_icon_from_paintable(settings_texture, Entry::IconPosition::SECONDARY);
	instrument_entry.set_icon_activatable(true);
	instrument_entry.signal_icon_press().connect(mem_fun(*this, &Toolbar::on_instrument_entry_dropdown_pressed));
	insmaker_toolbar.append(instrument_entry);
// Setup instrument select popover (bank_ctrl_popover).
	MainWindow::global_lmb_gesture->signal_pressed().connect(mem_fun(*this, &Toolbar::on_global_lmb_down));
	bank_ctrl.instrument_selected.connect(mem_fun(*this, &Toolbar::on_bank_ctrl_instrument_selected));
	bank_ctrl_popover.set_parent(instrument_entry);
	bank_ctrl_popover.set_child(bank_ctrl);
	bank_ctrl_popover.set_autohide(false);
// Setup instrument edit popover (instrument_edit_menu).
	shared_ptr<Gio::Menu> edit_menu_model = Gio::Menu::create();
	instrument_edit_menu.set_menu_model(edit_menu_model);
	instrument_edit_menu.set_parent(instrument_entry);
	shared_ptr<Gio::Menu> bank_edit_menu_model = Gio::Menu::create();
	shared_ptr<Gio::Menu> instrument_edit_menu_model = Gio::Menu::create();
	bank_edit_menu_model->append("Create Instrument", "insmaker.create_instrument");
	bank_edit_menu_model->append("Delete Instrument", "insmaker.delete_instrument");
	bank_edit_menu_model->append("Rename Instrument", "insmaker.rename_instrument");
	bank_edit_menu_model->append("Duplicate Instrument", "insmaker.duplicate_instrument");
	instrument_edit_menu_model->append("Additive Synthesis", "insmaker.toggle_additive_synth");
	menu_append_radio(instrument_edit_menu_model, "Melodic Mode", "insmaker.set_rhythm_mode", 0);
	menu_append_radio(instrument_edit_menu_model, "Bass Drum Mode", "insmaker.set_rhythm_mode", 6);
	menu_append_radio(instrument_edit_menu_model, "Snare Drum Mode", "insmaker.set_rhythm_mode", 7);
	menu_append_radio(instrument_edit_menu_model, "Tom Drum Mode", "insmaker.set_rhythm_mode", 8);
	menu_append_radio(instrument_edit_menu_model, "Cymbal Mode", "insmaker.set_rhythm_mode", 9);
	menu_append_radio(instrument_edit_menu_model, "Hi-Hat Mode", "insmaker.set_rhythm_mode", 10);
	edit_menu_model->append_section(bank_edit_menu_model);
	edit_menu_model->append_section(instrument_edit_menu_model);
}

MainWindow::MainWindow() {
using namespace Gtk;
	// Initialize.
	css_provider->load_from_path(get_advisual_dir() + (string)"share/advisual/themes/" + (string)"defaultstyle.css");
	StyleProvider::add_provider_for_display(get_display(), css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
	set_name("mainframe");
	set_title("AdVisual");
	set_default_size(1024, 720);
	Box vertical_box = Box(Orientation::VERTICAL, 0);
	set_child(vertical_box);

	global_lmb_gesture = GestureClick::create();
	global_lmb_gesture->set_button(GDK_BUTTON_PRIMARY);
	add_controller(global_lmb_gesture);

	toolbar = make_shared<Toolbar>();

	status = make_shared<Label>("Welcome to AdVisual!");
	status->set_name("statusbar");
	status->set_valign(Align::END);
	status->set_halign(Align::FILL);
	status->set_hexpand(true);
	status->set_single_line_mode(true);
	status->set_xalign(0);

	composer_panel = make_shared<ComposerPanel>();
	composer_panel->set_vexpand(true);
	insmaker_panel = make_shared<InsmakerPanel>();
	toolbar->create_composer_tools(composer_panel);
	toolbar->create_insmaker_tools(insmaker_panel);
	// Append items in proper order.
	vertical_box.append(*toolbar);
	vertical_box.append(*composer_panel);
	vertical_box.append(*insmaker_panel);
	vertical_box.append(*status);
	insmaker_panel->hide();
	// Create adplayer after current_track and current_bank have been initialized.
	adplayer = make_unique<AdPlayer>();
// Create common actions.
// File actions.
	common_action_group = Gio::SimpleActionGroup::create();
	common_action_group->add_action_bool("toggle_play_song", mem_fun(*adplayer, &AdPlayer::toggle_play_song), false);
	common_action_group->add_action("load", mem_fun(*this, &MainWindow::load));
	common_action_group->add_action("save", mem_fun(*this, &MainWindow::save));
	common_action_group->add_action("save_as", mem_fun(*this, &MainWindow::save_as));
	common_action_group->add_action("load_panel", mem_fun(*this, &MainWindow::load_panel));
	common_action_group->add_action("save_panel", mem_fun(*this, &MainWindow::save_panel));
	common_action_group->add_action("show_help", mem_fun(*this, &MainWindow::show_help));
// Toolbar actions.
	common_action_group->add_action_radio_integer("open_panel", mem_fun(*this, &MainWindow::open_panel), 0);
	insert_action_group("actions", common_action_group);
// Create a few insmaker actions.
	insmaker_panel->action_group->add_action("create_instrument",
		sigc::bind(mem_fun(*toolbar, &Toolbar::set_instrument_entry_mode), Toolbar::InsEntryMode::CREATE_INS));
	insmaker_panel->action_group->add_action("delete_instrument",
		sigc::bind(mem_fun(*toolbar, &Toolbar::set_instrument_entry_mode), Toolbar::InsEntryMode::DELETE_INS));
	insmaker_panel->action_group->add_action("rename_instrument",
		sigc::bind(mem_fun(*toolbar, &Toolbar::set_instrument_entry_mode), Toolbar::InsEntryMode::RENAME_INS));
	insmaker_panel->action_group->add_action("duplicate_instrument",
		sigc::bind(mem_fun(*toolbar, &Toolbar::set_instrument_entry_mode), Toolbar::InsEntryMode::DUPLICATE_INS));

	insert_action_group("insmaker", insmaker_panel->action_group);
	insert_action_group("composer", composer_panel->action_group);
// Create common shortcuts.
	app->set_accel_for_action("actions.toggle_play_song", "<Ctrl>space");
	app->set_accel_for_action("actions.load", "<Ctrl>l");
	app->set_accel_for_action("actions.save", "<Ctrl>s");
	app->set_accel_for_action("actions.quit", "<Ctrl>q");
// Signal handlers.
	signal_close_request().connect(mem_fun(*this, &MainWindow::on_close_request), false);

	show();
}

int main(int argc, char* argv[]) {
using namespace Gtk;
	app = Application::create("com.github.advisual", Application::Flags::NON_UNIQUE);
	return app->make_window_and_run<MainWindow>(argc, argv);
}

void MainWindow::show_help() {
	help_window.show();
}

void MainWindow::open_panel(int p_panel) {
	if (!insmaker_panel || !composer_panel) return;
	if (p_panel == 0) {
		composer_panel->show();
		insmaker_panel->hide();
		toolbar->composer_toolbar.show();
		toolbar->insmaker_toolbar.hide();
		insert_action_group("composer", composer_panel->action_group);
		remove_action_group("insmaker");
	}
	if (p_panel == 1) {
		insmaker_panel->show();
		composer_panel->hide();
		toolbar->insmaker_toolbar.show();
		toolbar->composer_toolbar.hide();
		insert_action_group("insmaker", insmaker_panel->action_group);
		remove_action_group("composer");
	}
	common_action_group->change_action_state("open_panel", Glib::Variant<int>::create(p_panel));
}

void MainWindow::load() {
	on_select_file(false, false);
	on_select_file(true, false);
}
void MainWindow::save_as() {
	on_select_file(false, true);
	on_select_file(true, true);
}
void MainWindow::save() {
	if (!current_track->file_path.empty()) { current_track->save_file(current_track->file_path); }
	else { on_select_file(false, true); }

	if (!current_bank->file_path.empty()) {
		insmaker_panel->save_instruments();
		current_bank->save_file(current_bank->file_path);
	}
	else { on_select_file(true, true); }
}
void MainWindow::load_panel() {
	on_select_file(insmaker_panel->get_visible(), false);
}
void MainWindow::save_panel() {
	on_select_file(insmaker_panel->get_visible(), true);
}

void MainWindow::on_select_file(bool bank, bool saving) {
using namespace Gtk;
	shared_ptr<FileDialog> dialog = FileDialog::create();
	shared_ptr<Gio::ListStore<FileFilter>> filters = Gio::ListStore<FileFilter>::create();

	shared_ptr<Gtk::FileFilter> track_filter = FileFilter::create();
	track_filter->set_name("Track files");
	track_filter->add_pattern("*.ROL");

	shared_ptr<Gtk::FileFilter> bank_filter = FileFilter::create();
	bank_filter->set_name("Bank files");
	bank_filter->add_pattern("*.BNK");

	filters->append(track_filter);
	filters->append(bank_filter);

	dialog->set_filters(filters);
	if (bank) {
		dialog->set_default_filter(bank_filter);
		dialog->set_accept_label(saving ? "Save Bank" : "Load Bank");
	}
	else      {
		dialog->set_default_filter(track_filter);
		dialog->set_accept_label(saving ? "Save Track" : "Load Track");
	}
	if (saving) { dialog->save(sigc::bind(mem_fun(*this, &MainWindow::on_file_selected), dialog, saving)); }
	else { dialog->open(sigc::bind(mem_fun(*this, &MainWindow::on_file_selected), dialog, saving)); }
}

void MainWindow::on_file_selected(const shared_ptr<Gio::AsyncResult>& result, const Glib::RefPtr<Gtk::FileDialog>& dialog, bool saving) {
using namespace Gtk;
	try {
		shared_ptr<Gio::File> file = (saving ? dialog->save_finish(result) : dialog->open_finish(result));
		string filename = file->get_path();

		// Get file extension.
		string file_ext;
		int extension_pos = filename.rfind(".");
		if (extension_pos == -1 && extension_pos != filename.length() - 1) {
			cerr << "Filename doesn't have a file extension\n";
			return;
		}
		file_ext = filename.substr(extension_pos + 1);
		string_to_upper(file_ext);

		// Compare file extension.
		if (file_ext == "ROL") {
			if (saving) {
				current_track->save_file(filename);
				composer_panel->on_track_saved();
			}
			else  { current_track->load_file(filename); }
		}
		if (file_ext == "BNK") {
			if (saving) {
				insmaker_panel->save_instruments();
				current_bank->save_file(filename);
			}
			else { current_bank->load_file(filename); }
		}
	}
	catch (const Gtk::DialogError& err) {
		cout << "No file selected\n";
	}
}

bool MainWindow::on_close_request() {
	if (composer_panel->is_unsaved() && insmaker_panel->get_unsaved_instrument_count() == 0) {
		return false; // Both composer and insmaker are saved already, so allow quit.
	}
	cout << "You have unsaved changes!" << "\n";
	shared_ptr<Gtk::AlertDialog> dialog = Gtk::AlertDialog::create("You have unsaved Track or Bank changes!");
	dialog->set_buttons({"Quit", "Save", "Cancel"});
	dialog->set_cancel_button(2);
	dialog->set_default_button(2);
	dialog->choose(*this, sigc::bind(mem_fun(*this, &MainWindow::on_close_dialog_choose), dialog));
	
	return true; // Don't quit.
}

void MainWindow::on_close_dialog_choose(const shared_ptr<Gio::AsyncResult>& result, const shared_ptr<Gtk::AlertDialog>& dialog) {
	int pressed = dialog->choose_finish(result);
	if (pressed == 2) { return; } // Cancel.
	if (pressed == 1) {
		save();
		return;
	}
	if (pressed == 0) {
		app->quit();
		return;
	}
}
