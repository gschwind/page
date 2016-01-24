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
	page_context_t * _ctx;
	int _title_width;

	thumnail_anchor_e _target_anchor;
	rect _target_position;
	rect _thumbnail_position;
	double _ratio;

	client_managed_w _c;
	shared_ptr<client_view_t> _client_view;
	theme_thumbnail_t _tt;
	bool _is_mouse_over;

	region _damaged_cache;

	renderable_thumbnail_t(renderable_thumbnail_t const &);
	renderable_thumbnail_t & operator=(renderable_thumbnail_t const &);
public:

	renderable_thumbnail_t(page_context_t * ctx, shared_ptr<client_managed_t> c, rect const & target_position, thumnail_anchor_e target_anchor) :
		_ctx{ctx},
		_c{c},
		_title_width{0},
		_is_mouse_over{false},
		_ratio{1.0},
		_target_position{target_position},
		_target_anchor{target_anchor},
		_client_view{nullptr}
	{

	}

	virtual ~renderable_thumbnail_t() {
		_ctx->add_global_damage(get_real_position());
	}

	/** @return scale factor */
	static double fit_to(double target_width, double target_height, double src_width, double src_height) {
		double x_ratio = target_width / src_width;
		double y_ratio = target_height / src_height;
		if (x_ratio < y_ratio) {
			return x_ratio;
		} else {
			return y_ratio;
		}
	}


	virtual void render(cairo_t * cr, region const & area) {
		if(_c.expired() or _tt.pix == nullptr)
			return;

		{
			region r = area & _thumbnail_position;
			for (auto &i : r.rects()) {
				cairo_save(cr);
				cairo_reset_clip(cr);
				cairo_clip(cr, i);
				cairo_translate(cr, _thumbnail_position.x,
						_thumbnail_position.y);
				cairo_scale(cr, _ratio, _ratio);
				cairo_set_source_surface(cr, _tt.pix->get_cairo_surface(),
						0.0, 0.0);
				cairo_pattern_set_filter(cairo_get_source(cr),
						CAIRO_FILTER_NEAREST);
				cairo_paint(cr);;
				cairo_restore(cr);
			}
		}

		if (_title_width != _thumbnail_position.w or _tt.title == nullptr) {
			_title_width = _thumbnail_position.w;
			update_title();
		}

		cairo_save(cr);
		region r =  area & get_real_position();
		for (auto &i : r.rects()) {
			cairo_reset_clip(cr);
			cairo_clip(cr, i);
			cairo_set_source_surface(cr, _tt.title->get_cairo_surface(), _thumbnail_position.x, _thumbnail_position.y+_thumbnail_position.h);
			cairo_paint(cr);

			if (_is_mouse_over) {
				cairo_identity_matrix(cr);
				cairo_set_line_width(cr, 2.0);
				cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
				cairo_rectangle(cr, _thumbnail_position.x+1,
						_thumbnail_position.y+1, _thumbnail_position.w-2.0,
						_thumbnail_position.h+20-2.0);
				cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
				cairo_stroke(cr);
			}
		}
		cairo_restore(cr);

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
		return get_real_position();
	}

	virtual region get_damaged() {
		return _damaged_cache;
	}

	rect get_real_position() {
		return rect{_thumbnail_position.x, _thumbnail_position.y, _thumbnail_position.w, _thumbnail_position.h + 20};
	}

	void set_mouse_over(bool x) {
		_damaged_cache += region{get_real_position()};
		_is_mouse_over = x;
	}

	void update_title() {
		_tt.title = make_shared<pixmap_t>(_ctx->dpy(), PIXMAP_RGB, _thumbnail_position.w, 20);
		cairo_t * cr = cairo_create(_tt.title->get_cairo_surface());
		_ctx->theme()->render_thumbnail_title(cr, rect{0 + 3, 0, _thumbnail_position.w - 6, 20}, _c.lock()->title());
		cairo_destroy(cr);
	}


	void move_to(rect const & target_position) {
		_damaged_cache += get_real_position();
		_target_position = target_position;
		update_title();
	}

	void render_finished() {
		_damaged_cache.clear();
	}

	void update_layout(time64_t const time) {
		if(_c.expired() or not _is_visible)
			return;

		_tt.pix = _client_view->get_pixmap();

		if (_tt.pix != nullptr) {

			int src_width = _tt.pix->witdh();
			int src_height = _tt.pix->height();

			_ratio = fit_to(_target_position.w, _target_position.h, src_width, src_height);

			_thumbnail_position = rect(0, 0, src_width  * _ratio, src_height * _ratio);


			switch(_target_anchor) {
			case ANCHOR_TOP:
			case ANCHOR_TOP_LEFT:
			case ANCHOR_TOP_RIGHT:
				_thumbnail_position.y = _target_position.y;
				break;
			case ANCHOR_LEFT:
			case ANCHOR_CENTER:
			case ANCHOR_RIGHT:
				_thumbnail_position.y = _target_position.y + ((_target_position.h - 20) - src_height * _ratio) / 2.0;
				break;
			case ANCHOR_BOTTOM:
			case ANCHOR_BOTTOM_LEFT:
			case ANCHOR_BOTTOM_RIGHT:
				_thumbnail_position.y = _target_position.y + (_target_position.h - 20) - src_height * _ratio;
				break;
			}

			switch(_target_anchor) {
			case ANCHOR_LEFT:
			case ANCHOR_TOP_LEFT:
			case ANCHOR_BOTTOM_LEFT:
				_thumbnail_position.x = _target_position.x;
				break;
			case ANCHOR_TOP:
			case ANCHOR_BOTTOM:
			case ANCHOR_CENTER:
				_thumbnail_position.x = _target_position.x + (_target_position.w - src_width * _ratio) / 2.0;
				break;
			case ANCHOR_RIGHT:
			case ANCHOR_TOP_RIGHT:
			case ANCHOR_BOTTOM_RIGHT:
				_thumbnail_position.x = _target_position.x + _target_position.w - src_width * _ratio;
				break;
			}
		}

		if(_client_view->has_damage()) {
			_damaged_cache += region{get_real_position()};
			_client_view->clear_damaged();
		}
	}

	void show() {
		if(_is_visible)
			return;

		_is_visible = true;
		if (not _c.expired() and _client_view == nullptr) {
			_client_view = _c.lock()->create_view();
		}
	}

	void hide() {
		if(not _is_visible)
			return;

		_is_visible = false;
		_ctx->add_global_damage(get_real_position());

		_client_view = nullptr;
		_tt.pix = nullptr;
		_tt.title = nullptr;

	}


};

using renderable_thumbnail_p = shared_ptr<renderable_thumbnail_t>;
using renderable_thumbnail_w = weak_ptr<renderable_thumbnail_t>;

}




#endif /* SRC_RENDERABLE_THUMBNAIL_HXX_ */
