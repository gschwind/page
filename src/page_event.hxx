/*
 * page_event.hxx
 *
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef PAGE_EVENT_HXX_
#define PAGE_EVENT_HXX_

#include "box.hxx"
#include "tree.hxx"

using namespace std;

namespace page {

enum page_event_type_e {
	PAGE_EVENT_NONE,
	PAGE_EVENT_NOTEBOOK_CLIENT,
	PAGE_EVENT_NOTEBOOK_CLIENT_CLOSE,
	PAGE_EVENT_NOTEBOOK_CLIENT_UNBIND,
	PAGE_EVENT_NOTEBOOK_CLOSE,
	PAGE_EVENT_NOTEBOOK_VSPLIT,
	PAGE_EVENT_NOTEBOOK_HSPLIT,
	PAGE_EVENT_NOTEBOOK_MARK,
	PAGE_EVENT_SPLIT
};

struct page_event_t {
	page_event_type_e type;
	rectangle position;
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

	page_event_t(page_event_type_e type = PAGE_EVENT_NONE) :
			type(type), position(), nbk(0), clt(0) {
	}

	page_event_t(page_event_t const & x) :
			type(x.type), position(x.position), _p0(x._p0), _p1(x._p1) {
	}

	page_event_t & operator=(page_event_t const & x) {
		if(this != &x) {
			type = x.type;
			position = x.position;
			_p0 = x._p0;
			_p1 = x._p1;
		}
		return *this;
	}

};

}


#endif /* PAGE_EVENT_HXX_ */
