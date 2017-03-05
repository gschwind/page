/*
 * renderable_thumbnail.hxx
 *
 *  Created on: 26 juin 2015
 *      Author: gschwind
 */

#ifndef SRC_RENDERABLE_THUMBNAIL_HXX_
#define SRC_RENDERABLE_THUMBNAIL_HXX_

#include "tree.hxx"
#include "pixmap.hxx"
#include "client_managed.hxx"
#include "client_proxy.hxx"
#include "page-types.hxx"

namespace page {

enum thumnail_anchor_e {
	ANCHOR_TOP,
	ANCHOR_LEFT,
	ANCHOR_BOTTOM,
	ANCHOR_RIGHT,
	ANCHOR_TOP_LEFT,
	ANCHOR_TOP_RIGHT,
	ANCHOR_BOTTOM_LEFT,
	ANCHOR_BOTTOM_RIGHT,
	ANCHOR_CENTER
};


class renderable_thumbnail_t : public tree_t {
	page_t * _ctx;
	int _title_width;

	thumnail_anchor_e _target_anchor;
	rect _target_position;
	rect _thumbnail_position;
	double _ratio;

	view_w _c;
	client_view_p _client_view;
	theme_thumbnail_t _tt;
	bool _is_mouse_over;

	region _damaged_cache;

	renderable_thumbnail_t(renderable_thumbnail_t const &) = delete;
	renderable_thumbnail_t & operator=(renderable_thumbnail_t const &) = delete;
public:

	renderable_thumbnail_t(tree_t * ref, view_p c, rect const & target_position, thumnail_anchor_e target_anchor);
	virtual ~renderable_thumbnail_t();

	/** @return scale factor */
	static double fit_to(double target_width, double target_height, double src_width, double src_height);
	virtual void render(cairo_t * cr, region const & area) override;

	/**
	 * Derived class must return opaque region for this object,
	 * If unknown it's safe to leave this empty.
	 **/
	virtual region get_opaque_region() override;

	/**
	 * Derived class must return visible region,
	 * If unknow the whole screen can be returned, but draw will be called each time.
	 **/
	virtual region get_visible_region() override;
	virtual region get_damaged() override;

	rect get_real_position();
	void set_mouse_over(bool x);
	void update_title();
	void move_to(rect const & target_position);
	void render_finished();

	virtual void update_layout(time64_t const time) override;
	virtual void show() override;
	virtual void hide() override;

};

using renderable_thumbnail_p = shared_ptr<renderable_thumbnail_t>;
using renderable_thumbnail_w = weak_ptr<renderable_thumbnail_t>;

}




#endif /* SRC_RENDERABLE_THUMBNAIL_HXX_ */
