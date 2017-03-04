/*
 * renderable_surface.hxx
 *
 *  Created on: 23 sept. 2014
 *      Author: gschwind
 */

#ifndef RENDERABLE_PIXMAP_HXX_
#define RENDERABLE_PIXMAP_HXX_

#include "page-types.hxx"
#include "tree.hxx"
#include "pixmap.hxx"

namespace page {

using namespace std;

class renderable_pixmap_t : public tree_t {

protected:

	page_t * _ctx;

	rect _location;
	shared_ptr<pixmap_t> _surf;
	region _damaged;
	region _opaque_region;

public:

	renderable_pixmap_t(tree_t * ctx, shared_ptr<pixmap_t> s, int x, int y);
	virtual ~renderable_pixmap_t();
	virtual void render(cairo_t * cr, region const & area);
	virtual region get_opaque_region();
	virtual region get_visible_region();
	virtual region get_damaged();
	virtual void render_finished();
	void set_opaque_region(region const & r);
	void move(int x, int y);

};



}



#endif /* RENDERABLE_SURFACE_HXX_ */
