#include <gtkmm.h>
#include <gtkmm/cssprovider.h>
#include <common.h>
#include <ComposerPanel.h>

using namespace std;
using namespace Gtk;
using namespace Gdk;

ToggleButton create_image_button(string image_name) {
	ToggleButton bttn;
	Image bttn_img(ICON_PATH(image_name));
	bttn.set_child(bttn_img);
	return bttn;
}

class Toolbar : public Box {
public:
	Toolbar();
};

Toolbar::Toolbar() : Box(Orientation::HORIZONTAL, 0) {
	set_css_classes({"toolbar"});
	ToggleButton file_button = create_image_button("FloppyDisk.svg");
	append(file_button);
	ToggleButton help_button = create_image_button("HelpBook.svg");
	append(help_button);
	ToggleButton composer_button = create_image_button("ComposerIcon.svg");
	append(composer_button);
	ToggleButton insmaker_button = create_image_button("InsmakerIcon.svg");
	append(insmaker_button);
	ToggleButton play_button = create_image_button("PlayButton.svg");
	append(play_button);
}

class MainWindow : public Window {
public:
	MainWindow();
	double opacity = 0.5;
	shared_ptr<Box> toolbar;
	shared_ptr<Statusbar> statusbar;
	shared_ptr<CssProvider> css_provider = CssProvider::create();
	shared_ptr<ComposerPanel> composer_panel;
	shared_ptr<ComposerPanel> insmaker_panel;
};

MainWindow::MainWindow() {
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
	// Append items in proper order.
	vertical_box.append(*toolbar);
	vertical_box.append(*composer_panel);
	vertical_box.append(*insmaker_panel);
	vertical_box.append(*statusbar);
	show();
}

int main(int argc, char* argv[]) {
	auto app = Application::create("com.github.advisual");
	return app->make_window_and_run<MainWindow>(argc, argv);
}