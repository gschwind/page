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

	static uint64_t const animation_duration = 400000000;

	theme_t const * _theme;

	bool _is_default;

	/* child stack order */
	list<tree_t *> _children;

	page::time_t swap_start;

	shared_ptr<pixmap_t> prev_surf;
	rectangle prev_loc;

	/* always handle current surface in case of unmap */
	composite_surface_handler_t cur_surf;


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
	void set_theme(theme_t const * theme);
	list<managed_window_base_t const *> clients() const;
	managed_window_base_t const * selected() const;
	bool is_default() const;
	void set_default(bool x);
	list<tree_t *> childs() const;
	void raise_child(tree_t * t);
	string get_node_name() const;
	void render_legacy(cairo_t * cr, rectangle const & area) const;
	void render(cairo_t * cr, time_t time);
	bool need_render(time_t time);

	managed_window_t * get_selected();

};

}

#endif /* NOTEBOOK_HXX_ */
