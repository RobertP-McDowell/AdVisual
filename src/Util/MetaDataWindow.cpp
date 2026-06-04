#include <Util/MetaDataWindow.h>
#include <FileAccess.h>
#include <common.h>

using namespace Gtk;

MetaDataWindow::MetaDataWindow() {
	set_size_request(400, 400);
	set_modal(true);
	set_title("Meta Data");
	text_view.set_editable(true);
	set_child(scrolled_window);
	scrolled_window.set_child(text_view);
	signal_close_request().connect(mem_fun(*this, &MetaDataWindow::on_close_request), false);
}

void MetaDataWindow::set_base_string(string* p_string_ptr) {
	string_ptr = p_string_ptr;
	if (string_ptr == nullptr || string_ptr->empty()) { return; }
	shared_ptr<TextBuffer> text_buffer = TextBuffer::create();
	Glib::ustring ustring_buffer = Glib::ustring(string_ptr->data(), string_ptr->data() + string_ptr->find('\0')).make_valid();
	text_buffer->set_text(ustring_buffer);
	text_buffer->signal_changed().connect(mem_fun(*this, &MetaDataWindow::on_change_text), true);
	text_view.set_buffer(text_buffer);
}

void MetaDataWindow::set_max_length(size_t p_max_len) {
	max_length = p_max_len;
}

void MetaDataWindow::on_change_text() {
		shared_ptr<TextBuffer> text_buffer = text_view.get_buffer();
	if (text_buffer->get_text().length() > max_length) {
		// Pretty sure we're good to ignore the iterator warning caused by this.
		// It warns against using the iterator twice, but it's only used once.
		int difference = text_buffer->get_text().length() - max_length;
		text_buffer->erase_interactive(text_buffer->get_iter_at_offset(max_length), text_buffer->end());
	}
}

bool MetaDataWindow::on_close_request() {
	if (string_ptr != nullptr) {
		*string_ptr = text_view.get_buffer()->get_text();
	}
	return false;
}
