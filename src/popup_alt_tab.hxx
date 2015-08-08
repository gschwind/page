/*
 * popup_alt_tab.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef POPUP_ALT_TAB_HXX_
#define POPUP_ALT_TAB_HXX_

#include <cairo/cairo.h>

#include <memory>

#include "renderable.hxx"
#include "icon_handler.hxx"
#include "renderable_thumbnail.hxx"

namespace page {

using namespace std;

class cycle_window_entry_t {
	shared_ptr<icon64> icon;
	string title;
	weak_ptr<client_managed_t> id;

	decltype(client_managed_t::on_destroy)::signal_func_t destroy_func;

	cycle_window_entry_t(cycle_window_entry_t const &);
	cycle_window_entry_t & operator=(cycle_window_entry_t const &);

public:
	cycle_window_entry_t(weak_ptr<client_managed_t> mw, string title, shared_ptr<icon64> icon) :
			icon(icon), title(title), id(mw) {
	}

	~cycle_window_entry_t() { }

	friend class popup_alt_tab_t;

};

class popup_alt_tab_t : public tree_t {
	page_context_t * _ctx;
	xcb_window_t _wid;
	shared_ptr<pixmap_t> _surf;
	rect _position;
	list<shared_ptr<cycle_window_entry_t>> _client_list;
	list<shared_ptr<cycle_window_entry_t>>::iterator _selected;
	bool _is_durty;
	bool _exposed;
	bool _damaged;

public:

	popup_alt_tab_t(page_context_t * ctx, list<shared_ptr<cycle_window_entry_t>> client_list, int selected) :
		_ctx{ctx},
		_selected{},
		_client_list{client_list},
		_is_durty{true},
		_exposed{true},
		_damaged{true}
	{

		_selected = _client_list.begin();

		_position.x = 0;
		_position.y = 0;
		_position.w = 80 * 4;
		_position.h = 80 * (_client_list.size()/4 + 1);

		_create_composite_window();

		_surf = _ctx->cmp()->create_composite_pixmap(_position.w, _position.h);
		_ctx->dpy()->map(_wid);

		for(auto const & x: _client_list) {
			if(not x->id.expired())
				x->destroy_func = x->id.lock()->on_destroy.connect(this, &popup_alt_tab_t::destroy_client);
		}

	}


	void _create_composite_window() {
		xcb_colormap_t cmap = xcb_generate_id(_ctx->dpy()->xcb());
		xcb_create_colormap(_ctx->dpy()->xcb(), XCB_COLORMAP_ALLOC_NONE, cmap, _ctx->dpy()->root(), _ctx->dpy()->root_visual()->visual_id);

		uint32_t value_mask = 0;
		uint32_t value[5];

		value_mask |= XCB_CW_BACK_PIXEL;
		value[0] = _ctx->dpy()->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_BORDER_PIXEL;
		value[1] = _ctx->dpy()->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_OVERRIDE_REDIRECT;
		value[2] = True;

		value_mask |= XCB_CW_EVENT_MASK;
		value[3] = XCB_EVENT_MASK_EXPOSURE;

		value_mask |= XCB_CW_COLORMAP;
		value[4] = cmap;

		_wid = xcb_generate_id(_ctx->dpy()->xcb());
		xcb_create_window(_ctx->dpy()->xcb(), _ctx->dpy()->root_depth(), _wid, _ctx->dpy()->root(),
				_position.x, _position.y, _position.w, _position.h, 0,
				XCB_WINDOW_CLASS_INPUT_OUTPUT, _ctx->dpy()->root_visual()->visual_id,
				value_mask, value);
	}

	void move(int x, int y) {
		_ctx->add_global_damage(_position);
		_position.x = x;
		_position.y = y;
		_ctx->dpy()->move_resize(_wid, _position);
		_ctx->add_global_damage(_position);
	}

	void show() {
		_is_visible = true;
		_ctx->dpy()->map(_wid);
	}

	void hide() {
		_is_visible = false;
		_ctx->dpy()->unmap(_wid);
	}

	rect const & position() {
		return _position;
	}

	~popup_alt_tab_t() {
		xcb_destroy_window(_ctx->dpy()->xcb(), _wid);
		_client_list.clear();
	}

	void select_next() {
		++_selected;
		if(_selected == _client_list.end())
			_selected = _client_list.begin();
		_is_durty = true;
		_exposed = true;
	}

	weak_ptr<client_managed_t> get_selected() {
		return (*_selected)->id;
	}

	void update_backbuffer() {
		if(not _is_durty)
			return;

		_is_durty = false;
		_damaged = true;

		rect a{0,0,_position.w,_position.h};
		cairo_t * cr = cairo_create(_surf->get_cairo_surface());
		//cairo_clip(cr, a);
		//cairo_translate(cr, _position.x, _position.y);
		cairo_rectangle(cr, 0, 0, _position.w, _position.h);
		cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_fill(cr);

		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

		int n = 0;
		for (auto i : _client_list) {
			int x = n % 4;
			int y = n / 4;

			if (i->icon != nullptr) {
				if (i->icon->get_cairo_surface() != nullptr) {
					cairo_set_source_surface(cr,
							i->icon->get_cairo_surface(), x * 80 + 8,
							y * 80 + 8);
					cairo_mask_surface(cr, i->icon->get_cairo_surface(),
							x * 80 + 8, y * 80 + 8);

				}
			}

			if (i == *_selected) {
				/** draw a beautiful yellow box **/
				cairo_set_line_width(cr, 2.0);
				::cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 1.0);
				cairo_rectangle(cr, x * 80 + 8, y * 80 + 8, 64, 64);
				cairo_stroke(cr);
			}

			++n;

		}
		cairo_destroy(cr);
	}

	void paint_exposed() {
		cairo_surface_t * surf = cairo_xcb_surface_create(_ctx->dpy()->xcb(), _wid,
				_ctx->dpy()->root_visual(), _position.w, _position.h);
		cairo_t * cr = cairo_create(surf);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_surface(cr, _surf->get_cairo_surface(), 0.0, 0.0);
		cairo_rectangle(cr, 0, 0, _position.w, _position.h);
		cairo_fill(cr);
		cairo_destroy(cr);
		cairo_surface_destroy(surf);
	}

	region get_damaged() {
		if(_damaged) {
			return region{_position};
		} else {
			return region{};
		}
	}

	region get_opaque_region() {
		return region{_position};
	}

	region get_visible_region() {
		return region{_position};
	}

	void render(cairo_t * cr, region const & area) {
		cairo_save(cr);
		for (auto & a : area) {
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_clip(cr, a);
			cairo_set_source_surface(cr, _surf->get_cairo_surface(), _position.x, _position.y);
			cairo_paint(cr);
		}
		cairo_restore(cr);
	}

	void destroy_client(client_managed_t * c) {
		_client_list.remove_if([](shared_ptr<cycle_window_entry_t> const & x) -> bool { return x->id.expired(); });
	}

	xcb_window_t get_xid() const {
		return _wid;
	}

	xcb_window_t get_parent_xid () const {
		return _wid;
	}

	string get_node_name() const {
		return string{"popup_alt_tab_t"};
	}

	virtual void trigger_redraw() {
		if(_exposed) {
			_exposed = false;
			update_backbuffer();
			paint_exposed();
		}
	}

	virtual void render_finished() {
		_damaged = false;
	}

	virtual void update_layout(time64_t const time) {
		update_backbuffer();
	}

	virtual void expose(xcb_expose_event_t const * ev) {
		if(ev->window == _wid)
			_exposed = true;
	}

	using tree_t::activate;

};

}



#endif /* POPUP_ALT_TAB_HXX_ */
