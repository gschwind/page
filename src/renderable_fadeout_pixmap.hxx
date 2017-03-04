/*
 * renderable_surface.hxx
 *
 *  Created on: 23 sept. 2014
 *      Author: gschwind
 */

#ifndef RENDERABLE_FADEOUT_PIXMAP_HXX_
#define RENDERABLE_FADEOUT_PIXMAP_HXX_

#include "tree.hxx"
#include "pixmap.hxx"
#include "renderable_pixmap.hxx"

#include <cmath>

#include "page-types.hxx"

namespace page {

using namespace std;

class renderable_fadeout_pixmap_t : public renderable_pixmap_t {
	time64_t _start_time;
	double _alpha;

public:
	renderable_fadeout_pixmap_t(tree_t * ref, shared_ptr<pixmap_t> s,
			int x, int y);
	virtual ~renderable_fadeout_pixmap_t();
	virtual void render(cairo_t * cr, region const & area);
	virtual void update_layout(time64_t const time);

};



}

#endif /* RENDERABLE_FADEOUT_PIXMAP_HXX_ */
