#include <gtkmm.h>
#include <common.h>
#include <ComposerPanel.h>
#include <ChannelButton.h>

Gtk::ToggleButton create_image_button(string image_name) {
using namespace Gtk;
	ToggleButton bttn;
	Image bttn_img(ICON_PATH(image_name));
	bttn.set_child(bttn_img);
	bttn_img.set_icon_size(IconSize::LARGE);
	return bttn;
}

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
	void create_insmaker_tools(shared_ptr<ComposerPanel> p_insmaker_panel);
private:
	void on_file_pressed();
	void on_help_pressed();
	void on_show_composer_panel();
	void on_show_insmaker_panel();
	void on_play_track();
	Gtk::ToggleButton composer_button;
	Gtk::ToggleButton play_button;
	vector<unique_ptr<ChannelButton>> channel_buttons;
	shared_ptr<ComposerPanel> composer_panel;
	shared_ptr<ComposerPanel> insmaker_panel;
};

Toolbar::Toolbar() : Gtk::Box(Gtk::Orientation::HORIZONTAL, 0) {
using namespace Gtk;
	set_css_classes({"toolbar"});
	MenuButton file_button = create_image_menu_button("FloppyDisk.svg");
	shared_ptr<Gio::Menu> file_menu = static_pointer_cast<Gio::Menu>(file_button.get_menu_model());
	file_menu->append("Load Track", "actions.load_track");
	file_menu->append("Save Track", "actions.save_track");
	file_menu->append("Load Bank", "actions.load_bank");
	file_menu->append("Save Bank", "actions.save_bank");
	append(file_button);
	MenuButton help_button = create_image_menu_button("HelpBook.svg");
	shared_ptr<Gio::Menu> help_menu = static_pointer_cast<Gio::Menu>(help_button.get_menu_model());
	help_menu->append("Docs", "actions.docs");
	help_menu->append("About", "actions.about");
	append(help_button);
	composer_button = create_image_button("ComposerIcon.svg");
	composer_button.signal_toggled().connect(mem_fun(*this, &Toolbar::on_play_track));
	append(composer_button);
	ToggleButton insmaker_button = create_image_button("InsmakerIcon.svg");
	insmaker_button.set_group(composer_button);
	append(insmaker_button);
	play_button = create_image_button("PlayButton.svg");
	append(play_button);
	play_button.signal_clicked().connect(mem_fun(*this, &Toolbar::on_play_track));
}

void Toolbar::create_composer_tools(shared_ptr<ComposerPanel> p_composer_panel) {
using namespace Gtk;
	composer_panel = p_composer_panel;
	for (int i = 0; i < 11; i++) {
		string channel_name = "CH" + to_string(i + 1);
		ChannelButton* channel_button = new ChannelButton(i);
		channel_button->set_name(channel_name);
		//channel_button.set_group(channel_button0);
		append(*channel_button);
		channel_button->show();
	}
}

void Toolbar::on_play_track() {
using namespace Gtk;
	cout << play_button.get_active() << " Play\n";
}

class MainWindow : public Gtk::Window {
public:
	MainWindow();
	double opacity = 0.5;
	shared_ptr<Toolbar> toolbar;
	shared_ptr<Gtk::Statusbar> statusbar;
	shared_ptr<Gtk::CssProvider> css_provider = Gtk::CssProvider::create();
	shared_ptr<ComposerPanel> composer_panel;
	shared_ptr<ComposerPanel> insmaker_panel;
protected:
	void on_load_track();
	
};

MainWindow::MainWindow() {
using namespace Gtk;
	// Initialize.
	css_provider->load_from_path((string)THEMES_PATH + (string)"defaultstyle.css");
	StyleProvider::add_provider_for_display(get_display(), css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
	set_name("mainframe");
	set_title("AdVisual");
	set_default_size(1024, 720);
	Box vertical_box = Box(Orientation::VERTICAL, 0);
	set_child(vertical_box);

	toolbar = make_shared<Toolbar>();

	statusbar = make_shared<Statusbar>();
	statusbar->set_valign(Align::END);

	composer_panel = make_shared<ComposerPanel>();
	composer_panel->set_vexpand(true);
	insmaker_panel = make_shared<ComposerPanel>();
	toolbar->create_composer_tools(composer_panel);
	// Append items in proper order.
	vertical_box.append(*toolbar);
	vertical_box.append(*composer_panel);
	vertical_box.append(*insmaker_panel);
	vertical_box.append(*statusbar);
	insmaker_panel->hide();

	shared_ptr<Gio::SimpleActionGroup> action_group = Gio::SimpleActionGroup::create();
	action_group->add_action("load_track", mem_fun(*this, &MainWindow::on_load_track));
	insert_action_group("actions", action_group);
	show();
}

int main(int argc, char* argv[]) {
using namespace Gtk;
	auto app = Application::create("com.github.advisual");
	return app->make_window_and_run<MainWindow>(argc, argv);
}

void MainWindow::on_load_track() {
	cout << "Load track\n";
}