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

#include "renderable_notebook_fading.hxx"
#include "tree.hxx"
#include "managed_window.hxx"
#include "region.hxx"
#include "theme_tab.hxx"

class managed_window_t;

namespace page {

struct img_t {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 3:RGB, 4:RGBA */
  unsigned char  pixel_data[16 * 16 * 4 + 1];
};

class notebook_t : public tree_t {
	double const XN = 0.0;
	page::time_t const animation_duration{0, 500000000};

	theme_t const * _theme;

	bool _is_default;

	/* child stack order */
	list<tree_t *> _children;

	page::time_t swap_start;

	shared_ptr<pixmap_t> prev_surf;
	i_rect prev_loc;

	/* always handle current surface in case of unmap */
	shared_ptr<composite_surface_t> cur_surf;

	ptr<renderable_notebook_fading_t> fading_notebook;

	mutable theme_notebook_t theme_notebook;

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
	managed_window_t * _selected;
	// set of map for fast check is window is in this notebook
	set<managed_window_t *> _client_map;

	i_rect client_area;

	i_rect button_close;
	i_rect button_vsplit;
	i_rect button_hsplit;
	i_rect button_pop;

	i_rect tab_area;
	i_rect top_area;
	i_rect bottom_area;
	i_rect left_area;
	i_rect right_area;

	i_rect popup_top_area;
	i_rect popup_bottom_area;
	i_rect popup_left_area;
	i_rect popup_right_area;
	i_rect popup_center_area;

	i_rect close_client_area;
	i_rect undck_client_area;

	void set_selected(managed_window_t * c);

	void update_client_position(managed_window_t * c);

public:
	notebook_t(theme_t const * theme);
	~notebook_t();
	void update_allocation(i_rect & allocation);

	bool process_button_press_event(XEvent const * e);

	void replace(tree_t * src, tree_t * by);
	void close(tree_t * src);
	void remove(tree_t * src);

	list<managed_window_t *> const & get_clients();

	bool add_client(managed_window_t * c, bool prefer_activate);
	void remove_client(managed_window_t * c);

	void activate_client(managed_window_t * x);
	void iconify_client(managed_window_t * x);

	i_rect get_new_client_size();

	void select_next();
	void delete_all();

	virtual void unmap_all();
	virtual void map_all();

	notebook_t * get_nearest_notebook();

	virtual i_rect get_absolute_extend();
	virtual region get_area();
	virtual void set_allocation(i_rect const & area);

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

	i_rect compute_client_size(managed_window_t * c);
	i_rect const & get_allocation();
	void set_theme(theme_t const * theme);
	list<managed_window_t const *> clients() const;
	managed_window_t const * selected() const;
	bool is_default() const;
	void set_default(bool x);
	list<tree_t *> childs() const;
	void raise_child(tree_t * t);
	string get_node_name() const;
	void render_legacy(cairo_t * cr, i_rect const & area) const;
	void render(cairo_t * cr, time_t time);
	bool need_render(time_t time);

	managed_window_t * get_selected();

	virtual void prepare_render(vector<ptr<renderable_t>> & out, page::time_t const & time);

	i_rect compute_notebook_close_window_position(i_rect const & allocation,
			int number_of_client, int selected_client_index) const;
	i_rect compute_notebook_unbind_window_position(i_rect const & allocation,
			int number_of_client, int selected_client_index) const;
	i_rect compute_notebook_bookmark_position(i_rect const & allocation) const;
	i_rect compute_notebook_vsplit_position(i_rect const & allocation) const;
	i_rect compute_notebook_hsplit_position(i_rect const & allocation) const;
	i_rect compute_notebook_close_position(i_rect const & allocation) const;
	i_rect compute_notebook_menu_position(i_rect const & allocation) const;

	void compute_areas_for_notebook(vector<page_event_t> * l) const;

};

}

#endif /* NOTEBOOK_HXX_ */
