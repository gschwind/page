/*
 * tree_base.hxx
 *
 *  Created on: 18 août 2013
 *      Author: bg
 */

#ifndef TREE_BASE_HXX_
#define TREE_BASE_HXX_

#include <list>
#include <string>
#include <typeinfo>

#include "managed_window_base.hxx"
#include "tree.hxx"

using namespace std;

namespace page {

enum box_page_type_e {
	THEME_NONE,
	THEME_NOTEBOOK_CLIENT,
	THEME_NOTEBOOK_CLIENT_CLOSE,
	THEME_NOTEBOOK_CLIENT_UNBIND,
	THEME_NOTEBOOK_CLOSE,
	THEME_NOTEBOOK_VSPLIT,
	THEME_NOTEBOOK_HSPLIT,
	THEME_NOTEBOOK_MARK,
	THEME_SPLIT
};

struct box_page_event_t {
	box_page_type_e type;
	box_t<int> position;
	union {
		struct {
			void * _p0;
			void * _p1;
		};
		struct {
			notebook_base_t const * nbk;
			managed_window_base_t const * clt;
		};
		split_base_t const * spt;
	};

	box_page_event_t(box_page_type_e type = THEME_NONE) :
			type(type), position(), nbk(0), clt(0) {
	}

	box_page_event_t(box_page_event_t const & x) :
			type(x.type), position(x.position), _p0(x._p0), _p1(x._p1) {
	}

	box_page_event_t & operator=(box_page_event_t const & x) {
		if(this != &x) {
			type = x.type;
			position = x.position;
			_p0 = x._p0;
			_p1 = x._p1;
		}
		return *this;
	}

};

enum box_floating_type_e {
	THEME_FLOATING_NONE,
	THEME_FLOATING_CLOSE,
	THEME_FLOATING_BIND,
	THEME_FLOATING_TITLE,
	THEME_FLOATING_GRIP_TOP,
	THEME_FLOATING_GRIP_BOTTOM,
	THEME_FLOATING_GRIP_LEFT,
	THEME_FLOATING_GRIP_RIGHT,
	THEME_FLOATING_GRIP_TOP_LEFT,
	THEME_FLOATING_GRIP_TOP_RIGHT,
	THEME_FLOATING_GRIP_BOTTOM_LEFT,
	THEME_FLOATING_GRIP_BOTTOM_RIGHT
};

struct box_floating_event_t {
	box_floating_type_e type;
	box_t<int> position;

	box_floating_event_t(box_floating_type_e type = THEME_FLOATING_NONE) :
			type(type), position() {
	}

	box_floating_event_t(box_floating_event_t const & x) :
			type(x.type), position(x.position) {
	}

	box_floating_event_t & operator=(box_floating_event_t const & x) {
		if (this != &x) {
			type = x.type;
			position = x.position;
		}
		return *this;
	}

};

}


#endif /* TREE_BASE_HXX_ */
