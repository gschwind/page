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

root_t::root_t(main_t & page) :
		root_t::tree_t(0, box_t<int>()), page(page) {
}

void root_t::add_aera(box_t<int> & area) {
	screen_t * s = new screen_t();
	s->aera = area;
	s->_subtree = new notebook_t(page);
	s->_subtree->reparent(this);
	subarea.push_back(s);
	box_t<int> x;
	update_allocation(x);
}

root_t::~root_t() {

}

#define min(x,y) (((x)>(y))?(y):(x))
#define max(x,y) (((x)<(y))?(y):(x))

void root_t::update_allocation(box_t<int> & allocation) {
	printf("update_allocation %dx%d+%d+%d\n", allocation.x, allocation.y,
			allocation.w, allocation.h);

	enum {
		X_LEFT = 0,
		X_RIGHT = 1,
		X_TOP = 2,
		X_BOTTOM = 3,
		X_LEFT_START_Y = 4,
		X_LEFT_END_Y = 5,
		X_RIGHT_START_Y = 6,
		X_RIGHT_END_Y = 7,
		X_TOP_START_X = 8,
		X_TOP_END_X = 9,
		X_BOTTOM_START_X = 10,
		X_BOTTOM_END_X = 11,
	};

	_allocation = allocation;

	std::list<screen_t *>::iterator i = subarea.begin();
	while (i != subarea.end()) {

		(*i)->sub_aera = (*i)->aera;
		int xtop = 0, xleft = 0, xright = 0, xbottom = 0;

		client_set_t::iterator j = page.clients.begin();
		while (j != page.clients.end()) {
			if ((*j)->has_partial_struct) {
				client_t * c = (*j);
				/* this is very crappy, but there is a way to do it better ? */
				if (c->partial_struct[X_LEFT] >= (*i)->aera.x
						&& c->partial_struct[X_LEFT]
								<= (*i)->aera.x + (*i)->aera.w) {
					int top =
							max(c->partial_struct[X_LEFT_START_Y], (*i)->aera.y);
					int bottom =
							min(c->partial_struct[X_LEFT_END_Y], (*i)->aera.y + (*i)->aera.h);
					if (bottom - top > 0) {
						xleft =
								max(xleft, c->partial_struct[X_LEFT] - (*i)->sub_aera.x);
					}
				}

				if (page.cnx.root_size.w - c->partial_struct[X_RIGHT]
						>= (*i)->aera.x
						&& page.cnx.root_size.w - c->partial_struct[X_RIGHT]
								<= (*i)->aera.x + (*i)->aera.w) {
					int top =
							max(c->partial_struct[X_RIGHT_START_Y], (*i)->aera.y);
					int bottom =
							min(c->partial_struct[X_RIGHT_END_Y], (*i)->aera.y + (*i)->aera.h);
					if (bottom - top > 0) {
						xright =
								max(xright, ((*i)->aera.x + (*i)->aera.w) - (page.cnx.root_size.w - c->partial_struct[X_RIGHT]));
					}
				}

				if (c->partial_struct[X_TOP] >= (*i)->aera.y
						&& c->partial_struct[X_TOP]
								<= (*i)->aera.y + (*i)->aera.h) {
					int left =
							max(c->partial_struct[X_TOP_START_X], (*i)->aera.x);
					int right =
							min(c->partial_struct[X_TOP_END_X], (*i)->aera.x + (*i)->aera.w);
					if (right - left > 0) {
						xtop =
								max(xtop, c->partial_struct[X_TOP] - (*i)->aera.y);
					}
				}

				if (page.cnx.root_size.h - c->partial_struct[X_BOTTOM]
						>= (*i)->aera.y
						&& page.cnx.root_size.h - c->partial_struct[X_BOTTOM]
								<= (*i)->aera.y + (*i)->aera.h) {
					int left =
							max(c->partial_struct[X_BOTTOM_START_X], (*i)->aera.x);
					int right =
							min(c->partial_struct[X_BOTTOM_END_X], (*i)->aera.x + (*i)->aera.w);
					if (right - left > 0) {
						xbottom =
								max(xbottom, ((*i)->sub_aera.h + (*i)->sub_aera.y) - (page.cnx.root_size.h - c->partial_struct[X_BOTTOM]));
					}
				}

			}
			++j;
		}

		(*i)->sub_aera.x += xleft;
		(*i)->sub_aera.w -= xleft + xright;
		(*i)->sub_aera.y += xtop;
		(*i)->sub_aera.h -= xtop + xbottom;

		printf("subarea %dx%d+%d+%d\n", (*i)->sub_aera.w, (*i)->sub_aera.h,
				(*i)->sub_aera.x, (*i)->sub_aera.y);
		(*i)->_subtree->update_allocation((*i)->sub_aera);
		++i;
	}
}

void root_t::render() {
	//_pack0->render();

	std::list<screen_t *>::iterator i = subarea.begin();
	while (i != subarea.end()) {
		(*i)->_subtree->render();
		++i;
	}

	//cairo_save(cr);
	//cairo_set_line_width(cr, 1.0);
	//cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
	//cairo_rectangle(cr, _allocation.x + 0.5, _allocation.y + 0.5, _allocation.w - 1.0, _allocation.h - 1.0);
	//cairo_stroke(cr);
	//cairo_restore(cr);
}

bool root_t::process_button_press_event(XEvent const * e) {
	//return _pack0->process_button_press_event(e);
	std::list<screen_t *>::iterator i = subarea.begin();
	while (i != subarea.end()) {
		if ((*i)->_subtree->process_button_press_event(e))
			return true;
		++i;
	}
	return false;
}

bool root_t::add_client(client_t *c) {
	if (!subarea.empty())
		return subarea.front()->_subtree->add_client(c);
	else
		return false;
}

void root_t::replace(tree_t * src, tree_t * by) {
	printf("replace %p by %p\n", src, by);

	std::list<screen_t *>::iterator i = subarea.begin();
	while (i != subarea.end()) {
		if ((*i)->_subtree == src) {
			(*i)->_subtree = by;
			(*i)->_subtree->reparent(this);
			(*i)->_subtree->update_allocation((*i)->sub_aera);
			break;
		}
		++i;
	}
}

void root_t::close(tree_t * src) {

}

void root_t::remove(tree_t * src) {

}

void root_t::activate_client(client_t * c) {
	std::list<screen_t *>::iterator i = subarea.begin();
	while (i != subarea.end()) {
		(*i)->_subtree->activate_client(c);
		++i;
	}
}

client_list_t * root_t::get_clients() {
	return 0;
}

void root_t::remove_client(client_t * c) {
	std::list<screen_t *>::iterator i = subarea.begin();
	while (i != subarea.end()) {
		(*i)->_subtree->remove_client(c);
		++i;
	}
}

void root_t::iconify_client(client_t * c) {
	std::list<screen_t *>::iterator i = subarea.begin();
	while (i != subarea.end()) {
		(*i)->_subtree->iconify_client(c);
		++i;
	}
}

}
