#pragma once
#include <iostream>
#include <iterator>
#include <stdexcept>

using namespace std;

struct UndoCommand {
	enum Reason {
		TYPE_INVALID,
		REASON_INVALID,
		TYPE_UNDO_NOTES,
		CREATE_NOTE,
		ERASE_NOTE,
		ERASE_SELECTION,
		PASTE_SELECTION,
		TYPE_UNDO_SELECTION,
		MOVE_PITCH,
		MOVE_TICKS
	};
	virtual void undo() = 0;
	virtual void redo() = 0;
	Reason get_reason() const {
		return reason;
	}
	Reason get_type() const {
		if (reason >= Reason::TYPE_UNDO_SELECTION) { return TYPE_UNDO_SELECTION; }
		if (reason >= Reason::TYPE_UNDO_NOTES) { return TYPE_UNDO_NOTES; }
		if (reason >= Reason::TYPE_INVALID) { return TYPE_INVALID; }
	}
	bool match_reason(Reason compare_reason) {
		return reason == compare_reason;
	}
protected:
	Reason reason;
	UndoCommand() {}
	UndoCommand(Reason p_reason) : reason(p_reason) {}
};

template <class T, class Allocator = allocator<T>> class UndoBuffer {
public:
	using value_type = T;
	using allocator_type = Allocator;
	using reference = value_type&;
	using const_reference = const value_type&;
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	class iterator {
	public:
		using iterator_category = random_access_iterator_tag;
		using value_type = T;
		using difference_type = ptrdiff_t;
		using pointer = T*;
		using reference = T&;
		iterator(pointer p_ptr) : ptr(p_ptr) {}
		reference operator*() const { return *ptr; }
		pointer operator->() const { return ptr; }
		iterator& operator++() {
			ptr++;
			return *this;
		}
		iterator operator++(int) {
			iterator temp = *this;
			ptr++;
			return temp;
		}
		bool operator==(const iterator& compare) const { return ptr == compare.ptr; }
		bool operator!=(const iterator& compare) const { return ptr != compare.ptr; }
	private:
		pointer ptr;
	};
	UndoBuffer() : data(nullptr), msize(0), mcapacity(0) {
		static_cast<UndoCommand*>((T)0); // Test type is derived from UndoCommand.
	}
	UndoBuffer(const UndoBuffer& other) : msize(other.msize), mcapacity(other.mcapacity) {
		data = allocator_type().allocate(mcapacity);
		copy(other.data, other.data + size, data);
	}
	~UndoBuffer() {
		allocator_type().deallocate(data, mcapacity);
	}
	size_type size() const { return msize; }
	bool empty() const { return msize == 0; }
	const_reference at(size_type index) const {
		if (index >= msize) {
			throw out_of_range("Index out of range");
		}
		return data[index];
	}
	reference at(size_type index) {
		if (index >= msize) {
			throw out_of_range("Index out of range");
		}
		return data[index];
	}
	void push_back(const reference value) {
		if (msize == mcapacity) {
			size_type new_capacity = (mcapacity != 0 ? mcapacity * 2 : 1);
			T* old_data = data;
			data = allocator_type().allocate(new_capacity);
			copy(old_data, old_data + msize, data);
			allocator_type().deallocate(old_data, mcapacity);
			mcapacity = new_capacity;
		}
		data[msize++] = value;
	}
	void shrink(size_type new_size) {
		if (new_size >= msize || empty()) { return; }
		for (size_type i = new_size; i < msize; i++) {
			delete data[i];
		}
		msize = new_size;
		undo_index = msize;
	}
	void clear() {
		if (!empty()) {
			shrink(0);
		}
		msize = 0;
		undo_index = 0;
		continuous = false;
		last_saved_index = 0;
	}
	reference get_current_undo() { return at(undo_index - 1); }

	void add_undo(value_type new_undo, bool is_continuous = false) {
		if (undo_index != msize) { // We split history.
			shrink(undo_index);
			if (undo_index < last_saved_index) { last_saved_index = -1; }
		}
		undo_index += 1;
		push_back(new_undo);
		continuous = is_continuous;
	}

	void undo() {
		if (undo_index <= 0) { return; }
		undo_index -= 1;
		at(undo_index)->undo();
		continuous = false;
	}

	void redo() {
		if (undo_index >= msize) { return; }
		at(undo_index)->redo();
		undo_index += 1;
	}

	UndoCommand::Reason get_last_undo_reason() {
		if (undo_index <= 0 || empty()) { return UndoCommand::Reason::REASON_INVALID; }
		return at(undo_index - 1)->get_reason();
	}

	void set_continuous(bool value) { continuous = value; }
	bool get_continuous() { return continuous && !empty(); }
	size_type get_undo_index() const { return undo_index; }
	void set_saved() { last_saved_index = undo_index; }
	bool is_saved() const { return last_saved_index == undo_index; }
private:
	T* data;
	size_type msize;
	size_type mcapacity;
	size_type last_saved_index = 0;
	size_type undo_index = 0;
	bool continuous = false;
};

