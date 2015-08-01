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

class notebook_t;
class split_t;
class client_managed_t;

/* page event area */
enum page_event_type_e {
	PAGE_EVENT_NONE = 0,
	PAGE_EVENT_NOTEBOOK_CLIENT,
	PAGE_EVENT_NOTEBOOK_CLIENT_CLOSE,
	PAGE_EVENT_NOTEBOOK_CLIENT_UNBIND,
	PAGE_EVENT_NOTEBOOK_CLOSE,
	PAGE_EVENT_NOTEBOOK_VSPLIT,
	PAGE_EVENT_NOTEBOOK_HSPLIT,
	PAGE_EVENT_NOTEBOOK_MARK,
	PAGE_EVENT_NOTEBOOK_MENU,
	PAGE_EVENT_SPLIT,
	PAGE_EVENT_COUNT // must be the last one
};

struct page_event_t {
	page_event_type_e type;
	rect position;
	union {
		struct {
			void * _p0;
			void * _p1;
		};
		struct {
			notebook_t const * nbk;
			client_managed_t const * clt;
		};
		split_t const * spt;
	};

	page_event_t(page_event_type_e type = PAGE_EVENT_NONE) :
			type(type), position(), nbk(nullptr), clt(nullptr) {
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
