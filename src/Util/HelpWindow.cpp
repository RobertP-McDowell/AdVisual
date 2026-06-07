#include <Util/HelpWindow.h>
#include <FileAccess.h>
#include <common.h>

using namespace Gtk;

HelpWindow::HelpWindow() {
	set_hide_on_close(false);
	set_modal(false);
	set_size_request(800, 600);
	set_title("AdVisual Docs");
	shared_ptr<TextBuffer> text_buffer = TextBuffer::create();

	if (!FileAccess::access_file(get_advisual_dir() + string("share/advisual/docs.txt"), false)) { return; }
	char* c_str_buffer = (char*)malloc(FileAccess::file_length);
	FileAccess::fieldcpy_char(&c_str_buffer[0], FileAccess::file_length);
	Glib::ustring ustring_buffer(&c_str_buffer[0], &c_str_buffer[0] + FileAccess::file_length);
	free(c_str_buffer); // Free malloc ptr!
	FileAccess::close_file();

	text_buffer->set_text(ustring_buffer);
	text_view = TextView(text_buffer);
	text_view.set_editable(false);
	set_child(scrolled_window);
	scrolled_window.set_child(text_view);
}
