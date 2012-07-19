/*
 * viewport.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include <cassert>
#include <algorithm>
#include "notebook.hxx"
#include "viewport.hxx"

namespace page {

viewport_t::viewport_t(page_base_t & page, box_t<int> const & area) :
		page(page), raw_aera(area), effective_aera(area), fullscreen_client(0) {

	printf("create viewport %dx%d+%d+%d\n", raw_aera.w, raw_aera.h, raw_aera.x,
			raw_aera.y);

	fix_allocation();
	_subtree = new notebook_t(page);
	_subtree->reparent(this);
	_subtree->reconfigure(effective_aera);

}

bool viewport_t::add_client(window_t * x) {
	assert(_subtree != 0);
	return _subtree->add_client(x);
}

void viewport_t::replace(tree_t * src, tree_t * by) {
	printf("replace %p by %p\n", src, by);

	if (_subtree == src) {
		_subtree = by;
		_subtree->reparent(this);
		_subtree->reconfigure(effective_aera);
	}
}

void viewport_t::close(tree_t * src) {

}

void viewport_t::remove(tree_t * src) {

}

void viewport_t::reconfigure(box_int_t const & area) {
	raw_aera = area;
	fix_allocation();
	_subtree->reconfigure(effective_aera);
}

void viewport_t::repair1(cairo_t * cr, box_int_t const & area) {
	assert(_subtree != 0);
	_subtree->repair1(cr, area);
}

box_int_t viewport_t::get_absolute_extend() {
	return raw_aera;
}

void viewport_t::activate_client(window_t * c) {
	assert(_subtree != 0);
	_subtree->activate_client(c);
}

window_list_t viewport_t::get_windows() {
	assert(_subtree != 0);
	return _subtree->get_windows();
}

void viewport_t::remove_client(window_t * c) {
	assert(_subtree != 0);
	_subtree->remove_client(c);
}

void viewport_t::iconify_client(window_t * c) {
	assert(_subtree != 0);
	_subtree->iconify_client(c);
}

void viewport_t::delete_all() {
	assert(_subtree != 0);
	_subtree->delete_all();
	delete _subtree;
}

void viewport_t::fix_allocation() {
	printf("update_allocation %dx%d+%d+%d\n", raw_aera.x, raw_aera.y,
			raw_aera.w, raw_aera.h);

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

	effective_aera = raw_aera;
	int xtop = 0, xleft = 0, xright = 0, xbottom = 0;

	window_list_t::iterator j = page.windows_stack.begin();
	while (j != page.windows_stack.end()) {
		long const * ps = (*j)->read_partial_struct();
		if (ps) {
			window_t * c = (*j);
			/* this is very crappy, but there is a way to do it better ? */
			if (ps[X_LEFT] >= raw_aera.x
					&& ps[X_LEFT] <= raw_aera.x + raw_aera.w) {
				int top = std::max<int const>(ps[X_LEFT_START_Y], raw_aera.y);
				int bottom = std::min<int const>(ps[X_LEFT_END_Y], raw_aera.y + raw_aera.h);
				if (bottom - top > 0) {
					xleft = std::max<int const>(xleft, ps[X_LEFT] - effective_aera.x);
				}
			}

			if (page.get_xconnection().root_size.w - ps[X_RIGHT] >= raw_aera.x
					&& page.get_xconnection().root_size.w - ps[X_RIGHT]
							<= raw_aera.x + raw_aera.w) {
				int top = std::max<int const>(ps[X_RIGHT_START_Y], raw_aera.y);
				int bottom = std::min<int const>(ps[X_RIGHT_END_Y],
						raw_aera.y + raw_aera.h);
				if (bottom - top > 0) {
					xright = std::max<int const>(xright,
							(raw_aera.x + raw_aera.w)
									- (page.get_xconnection().root_size.w - ps[X_RIGHT]));
				}
			}

			if (ps[X_TOP] >= raw_aera.y
					&& ps[X_TOP] <= raw_aera.y + raw_aera.h) {
				int left = std::max<int const>(ps[X_TOP_START_X], raw_aera.x);
				int right = std::min<int const>(ps[X_TOP_END_X], raw_aera.x + raw_aera.w);
				if (right - left > 0) {
					xtop = std::max<int const>(xtop, ps[X_TOP] - raw_aera.y);
				}
			}

			if (page.get_xconnection().root_size.h - ps[X_BOTTOM] >= raw_aera.y
					&& page.get_xconnection().root_size.h - ps[X_BOTTOM]
							<= raw_aera.y + raw_aera.h) {
				int left = std::max<int const>(ps[X_BOTTOM_START_X], raw_aera.x);
				int right = std::min<int const>(ps[X_BOTTOM_END_X],
						raw_aera.x + raw_aera.w);
				if (right - left > 0) {
					xbottom = std::max<int const>(xbottom,
							(effective_aera.h + effective_aera.y)
									- (page.get_xconnection().root_size.h - ps[X_BOTTOM]));
				}
			}

		}
		++j;
	}

	effective_aera.x += xleft;
	effective_aera.w -= xleft + xright;
	effective_aera.y += xtop;
	effective_aera.h -= xtop + xbottom;

	printf("subarea %dx%d+%d+%d\n", effective_aera.w, effective_aera.h,
			effective_aera.x, effective_aera.y);
}

}
