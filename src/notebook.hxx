/*
 * notebook.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef NOTEBOOK_HXX_
#define NOTEBOOK_HXX_

#include "tree.hxx"
#include "managed_window.hxx"
#include "region.hxx"

namespace page {

struct img_t {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 3:RGB, 4:RGBA */
  unsigned char  pixel_data[16 * 16 * 4 + 1];
};

class notebook_t : public notebook_base_t  {

	static uint64_t const animation_duration = 300000000;

	theme_t const * _theme;

	bool _is_default;

	/* child stack order */
	list<tree_t *> _children;

	page::time_t swap_start;
	composite_surface_handler_t prev_surf;
	rectangle prev_loc;

public:

	enum select_e {
		SELECT_NONE,
		SELECT_TAB,
		SELECT_TOP,
		SELECT_BOTTOM,
		SELECT_LEFT,
		SELECT_RIGHT
	};
	// list of client to maintain tab order
	list<managed_window_t *> _clients;
	// list of selected to have smart unselect (when window is closed we
	// select the previous window selected
	list<managed_window_t *> _selected;
	// set of map for fast check is window is in this notebook
	set<managed_window_t *> _client_map;

	rectangle client_area;

	rectangle button_close;
	rectangle button_vsplit;
	rectangle button_hsplit;
	rectangle button_pop;

	rectangle tab_area;
	rectangle top_area;
	rectangle bottom_area;
	rectangle left_area;
	rectangle right_area;

	rectangle popup_top_area;
	rectangle popup_bottom_area;
	rectangle popup_left_area;
	rectangle popup_right_area;
	rectangle popup_center_area;

	rectangle close_client_area;
	rectangle undck_client_area;

	void set_selected(managed_window_t * c);

	void update_client_position(managed_window_t * c);

public:
	notebook_t(theme_t const * theme);
	~notebook_t();
	void update_allocation(rectangle & allocation);

	bool process_button_press_event(XEvent const * e);

	void replace(tree_t * src, tree_t * by);
	void close(tree_t * src);
	void remove(tree_t * src);

	list<managed_window_t *> const & get_clients();

	bool add_client(managed_window_t * c, bool prefer_activate);
	void remove_client(managed_window_t * c);

	void activate_client(managed_window_t * x);
	void iconify_client(managed_window_t * x);

	rectangle get_new_client_size();

	void select_next();
	void delete_all();

	virtual void unmap_all();
	virtual void map_all();

	notebook_t * get_nearest_notebook();

	virtual rectangle get_absolute_extend();
	virtual region get_area();
	virtual void set_allocation(rectangle const & area);

	managed_window_t * find_client_tab(int x, int y);

	void update_close_area();

	/*
	 * Compute client size taking in account possible max_width and max_heigth
	 * and the window Hints.
	 * @output width: width result
	 * @output height: height result
	 */

	static void compute_client_size_with_constraint(managed_window_t * c,
			unsigned int max_width, unsigned int max_height,
			unsigned int & width, unsigned int & height);

	rectangle compute_client_size(managed_window_t * c);
	rectangle const & get_allocation();
	managed_window_t const * get_selected();

	void set_theme(theme_t const * theme);

	virtual list<managed_window_base_t const *> clients() const {
		return list<managed_window_base_t const *>(_clients.begin(), _clients.end());
	}

	virtual managed_window_base_t const * selected() const {
		if(_selected.empty()) {
			return nullptr;
		} else {
			return _selected.front();
		}
	}

	virtual bool is_default() const {
		return _is_default;
	}

	void set_default(bool x) {
		_is_default = x;
	}

	virtual list<tree_t *> childs() const {
		list<tree_t *> ret(_children.begin(), _children.end());
		return ret;
	}

	virtual void raise_child(tree_t * t) {

		if (has_key(_children, t)) {
			_children.remove(t);
			_children.push_back(t);
		}

		if(_parent != nullptr) {
			_parent->raise_child(this);
		}

	}

	virtual string get_node_name() const {
		return _get_node_name<'N'>();
	}

	void render_legacy(cairo_t * cr, rectangle const & area) const {
		_theme->render_notebook(cr, this, area);
	}

	virtual void render(cairo_t * cr, time_t time) {

		//_theme->render_notebook(cr, this, _allocation);

		if (not _selected.empty()) {

			page::time_t d(0, animation_duration);
			if (time < (swap_start + d)) {
				double ratio = static_cast<double>(time - swap_start)/animation_duration;

				double y = floor(ratio * _allocation.h);
				if(y < 0.0)
					y = 0.0;
				if(y > _allocation.h)
					y = _allocation.h;

				managed_window_t * mw = _selected.front();
				composite_surface_handler_t psurf = mw->surf();

				if (prev_surf != nullptr) {
					cairo_surface_t * s = prev_surf->get_surf();
					rectangle location = prev_loc;
					display_t::create_context(__FILE__, __LINE__);
					cairo_save(cr);
					cairo_rectangle(cr, location.x, location.y, location.w,
							location.h);
					cairo_clip(cr);
					cairo_set_source_surface(cr, s, location.x, location.y);
					cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
					cairo_pattern_t * p = cairo_pattern_create_linear(0.0,
							y - 150.0, 0.0, y);
					cairo_pattern_add_color_stop_rgba(p, 0.0, 1.0, 1.0, 1.0,
							0.0);
					cairo_pattern_add_color_stop_rgba(p, 1.0, 1.0, 1.0, 1.0,
							1.0);
					cairo_mask(cr, p);
					cairo_pattern_destroy(p);
					display_t::destroy_context(__FILE__, __LINE__);
					cairo_restore(cr);
				}

				if (psurf != nullptr) {
					cairo_surface_t * s = psurf->get_surf();
					rectangle old = mw->base_position();
					rectangle location = mw->base_position();
					display_t::create_context(__FILE__, __LINE__);
					cairo_save(cr);
					cairo_rectangle(cr, location.x, location.y, location.w,
							location.h);
					cairo_clip(cr);
					cairo_set_source_surface(cr, s, location.x, location.y);
					cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
					cairo_pattern_t * p = cairo_pattern_create_linear(0.0,
							y, 0.0, y + 150.0);
					cairo_pattern_add_color_stop_rgba(p, 1.0, 1.0, 1.0, 1.0,
							0.0);
					cairo_pattern_add_color_stop_rgba(p, 0.0, 1.0, 1.0, 1.0,
							1.0);
					cairo_mask(cr, p);
					cairo_pattern_destroy(p);
					display_t::destroy_context(__FILE__, __LINE__);
					cairo_restore(cr);

				}



				for (auto i : mw->childs()) {
					i->render(cr, time);
				}


			} else {

				prev_surf.reset();

				managed_window_t * mw = _selected.front();
				composite_surface_handler_t psurf = mw->surf();
				if (psurf != nullptr) {
					cairo_surface_t * s = psurf->get_surf();
					rectangle location = mw->get_base_position();
					display_t::create_context(__FILE__, __LINE__);
					cairo_save(cr);
					cairo_set_source_surface(cr, s, location.x, location.y);
					cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
					cairo_rectangle(cr, location.x, location.y, location.w,
							location.h);
					cairo_fill(cr);
					display_t::destroy_context(__FILE__, __LINE__);
					cairo_restore(cr);

				}

				for (auto i : mw->childs()) {
					i->render(cr, time);
				}

			}



		}

	}

	bool need_render(time_t time) {

		page::time_t d(0, animation_duration);
		if (time < (swap_start + d)) {
			return true;
		}

		for(auto i: childs()) {
			if(i->need_render(time)) {
				return true;
			}
		}
		return false;
	}


};

}

#endif /* NOTEBOOK_HXX_ */
