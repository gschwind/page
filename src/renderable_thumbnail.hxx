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
	int _title_width;

	i_rect _thumbnail_position;
	client_managed_t * _c;

	region _visible_region;
	theme_thumbnail_t _tt;

public:

	renderable_thumbnail_t(page_context_t * ctx, i_rect const & position, client_managed_t * c) :
		_ctx{ctx},
		_c{c},
		_position{position},
		_visible_region{_position},
		_title_width{0}
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
			i_rect tmp = _position;
			tmp.h -= 20;

			double src_width = _tt.pix->witdh();
			double src_height = _tt.pix->height();

			double x_ratio = tmp.w / src_width;
			double y_ratio = tmp.h / src_height;

			double x_offset, y_offset;

			if (x_ratio < y_ratio) {

				_thumbnail_position = i_rect(tmp.x,
						tmp.y + (tmp.h - src_height * x_ratio) / 2.0, tmp.w,
						src_height * x_ratio

						);

				region r = area & _thumbnail_position;
				for (auto &i : r) {
					cairo_save(cr);
					cairo_reset_clip(cr);
					cairo_clip(cr, i);
					cairo_translate(cr, _thumbnail_position.x,
							_thumbnail_position.y);
					cairo_scale(cr, x_ratio, x_ratio);
					cairo_set_source_surface(cr, _tt.pix->get_cairo_surface(),
							0.0, 0.0);
					cairo_pattern_set_filter(cairo_get_source(cr),
							CAIRO_FILTER_NEAREST);
					cairo_paint(cr);
					cairo_restore(cr);
				}

			} else {

				_thumbnail_position = i_rect(
						tmp.x + (tmp.w - src_width * y_ratio) / 2.0, tmp.y,
						src_width * y_ratio, tmp.h);

				region r = area & _thumbnail_position;
				for (auto &i : r) {
					cairo_save(cr);
					cairo_reset_clip(cr);
					cairo_clip(cr, i);
					cairo_translate(cr, _thumbnail_position.x,
							_thumbnail_position.y);
					cairo_scale(cr, y_ratio, y_ratio);
					cairo_set_source_surface(cr, _tt.pix->get_cairo_surface(),
							x_offset, y_offset);
					cairo_pattern_set_filter(cairo_get_source(cr),
							CAIRO_FILTER_NEAREST);
					cairo_paint(cr);
					cairo_restore(cr);
				}

			}

			if (_title_width != _thumbnail_position.w) {
				_title_width = _thumbnail_position.w;
				_tt.title = _ctx->cmp()->create_composite_pixmap(
						_thumbnail_position.w, 20);
				cairo_t * cr = cairo_create(_tt.title->get_cairo_surface());
				_ctx->theme()->render_thumbnail_title(cr, i_rect { 0, 0,
						_thumbnail_position.w, 20 }, _c->title());
				cairo_destroy(cr);
			}

			cairo_save(cr);
			region r = _visible_region & area;
			for (auto &i : r) {
				cairo_reset_clip(cr);
				cairo_clip(cr, i);
				cairo_set_source_surface(cr, _tt.title->get_cairo_surface(), _thumbnail_position.x, _thumbnail_position.y+_thumbnail_position.h);
				cairo_paint(cr);

//				cairo_identity_matrix(cr);
//				cairo_rectangle(cr, _thumbnail_position.x,
//						_thumbnail_position.y, _thumbnail_position.w,
//						_thumbnail_position.h);
//				cairo_set_source_rgb(cr, 1.0, 0.0, 1.0);
//				cairo_stroke(cr);
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

	i_rect get_real_position() {
		return i_rect{_thumbnail_position.x, _thumbnail_position.y, _thumbnail_position.w, _thumbnail_position.h + 20};
	}


};



}




#endif /* SRC_RENDERABLE_THUMBNAIL_HXX_ */
