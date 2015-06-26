/*
 * renderable_thumbnail.hxx
 *
 *  Created on: 26 juin 2015
 *      Author: gschwind
 */

#ifndef SRC_RENDERABLE_THUMBNAIL_HXX_
#define SRC_RENDERABLE_THUMBNAIL_HXX_


#include "pixmap.hxx"

namespace page {

class renderable_thumbnail_t : public renderable_t {
	page_context_t * _ctx;
	i_rect _position;
	client_managed_t * _c;

	region _visible_region;
	theme_thumbnail_t _tt;

public:

	renderable_thumbnail_t(page_context_t * ctx, i_rect const & position, client_managed_t * c) :
		_ctx{ctx},
		_c{c},
		_position{position},
		_visible_region{_position}
	{
		_tt.pix = _c->get_last_pixmap();
		_tt.title = _ctx->cmp()->create_composite_pixmap(position.w, 20);

		cairo_t * cr = cairo_create(_tt.title->get_cairo_surface());
		_ctx->theme()->render_thumbnail_title(cr, i_rect{0, 0, position.w, 20}, c->title());
		cairo_destroy(cr);

	}

	virtual ~renderable_thumbnail_t() { }


	virtual void render(cairo_t * cr, region const & area) {
		_tt.pix = _c->get_last_pixmap();
		if (_tt.pix != nullptr) {
			cairo_save(cr);
			region r = _visible_region & area;
			for (auto &i : r) {
				cairo_reset_clip(cr);
				cairo_clip(cr, i);
				_ctx->theme()->render_thumbnail(cr, _position, _tt);
			}
			cairo_restore(cr);
		}

		_ctx->csm()->clear_damaged(_c->base());

	}

	/**
	 * Derived class must return opaque region for this object,
	 * If unknown it's safe to leave this empty.
	 **/
	virtual region get_opaque_region() {
		return region{};
	}

	/**
	 * Derived class must return visible region,
	 * If unknow the whole screen can be returned, but draw will be called each time.
	 **/
	virtual region get_visible_region() {
		return _visible_region;
	}

	virtual region get_damaged() {
		if(_ctx->csm()->has_damage(_c->base())) {
			return region{_position};
		}
		return region{};
	}

};



}




#endif /* SRC_RENDERABLE_THUMBNAIL_HXX_ */
