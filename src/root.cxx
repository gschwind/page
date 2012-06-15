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
#include "page.hxx"
#include "notebook.hxx"

namespace page_next {

root_t::root_t(main_t & page, box_t<int> &allocation) :
		root_t::tree_t(0, allocation) {
	_pack0 = new notebook_t(page);
	_pack0->reparent(this);
	_pack0->update_allocation(allocation);
}

root_t::~root_t() {

}

void root_t::update_allocation(box_t<int> & allocation) {
	printf("update_allocation %dx%d+%d+%d\n", allocation.x, allocation.y, allocation.w, allocation.h);
	_allocation = allocation;
	_pack0->update_allocation(allocation);
}

void root_t::render() {
	_pack0->render();

	//cairo_save(cr);
	//cairo_set_line_width(cr, 1.0);
	//cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
	//cairo_rectangle(cr, _allocation.x + 0.5, _allocation.y + 0.5, _allocation.w - 1.0, _allocation.h - 1.0);
	//cairo_stroke(cr);
	//cairo_restore(cr);
}

bool root_t::process_button_press_event(XEvent const * e) {
	return _pack0->process_button_press_event(e);
}

bool root_t::add_notebook(client_t *c) {
	if (_pack0)
		return _pack0->add_notebook(c);
	else
		return false;
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

void root_t::activate_client(client_t * c) {
	if (_pack0)
		_pack0->activate_client(c);
}

client_list_t * root_t::get_clients() {
	return 0;
}

void root_t::remove_client(client_t * c) {
	_pack0->remove_client(c);
}

}
