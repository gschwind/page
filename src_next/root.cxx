/*
 * root.cxx
 *
 *  Created on: Feb 27, 2011
 *      Author: gschwind
 */

#include <X11/Xlib.h>
#include <stdio.h>
#include "box.hxx"
#include "root.hxx"
#include "notebook.hxx"

namespace page_next {

root_t::root_t(Display * dpy, Window w, box_t<int> &allocation) :
	root_t::tree_t(dpy, w, 0, allocation) {
	_pack0 = new notebook_t();
	_pack0->reparent(this);
	_pack0->update_allocation(allocation);
}

void root_t::update_allocation(box_t<int> & allocation) {
	_allocation = allocation;
	_pack0->update_allocation(allocation);
}

void root_t::render(cairo_t * cr) {
	_pack0->render(cr);
}

bool root_t::process_button_press_event(XEvent const * e) {
	_pack0->process_button_press_event(e);
}

void root_t::add_notebook(client_t *c) {
	_pack0->add_notebook(c);
}

void root_t::replace(tree_t * src, tree_t * by) {
	printf("replace %p by %p\n", src, by);
	if (_pack0 == src) {
		_pack0 = by;
		_pack0->reparent(this);
		_pack0->update_allocation(_allocation);
	}
}

void root_t::close(tree_t * src) {

}


void root_t::remove(tree_t * src) {

}

std::list<client_t *> * root_t::get_clients() {
	return 0;
}

}
