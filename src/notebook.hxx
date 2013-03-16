/*
 * notebook.hxx
 *
 *  Created on: 27 févr. 2011
 *      Author: gschwind
 */

#ifndef NOTEBOOK_HXX_
#define NOTEBOOK_HXX_

#include "box.hxx"
#include "client.hxx"
#include "tree.hxx"
#include "window.hxx"
#include "window_icon_handler.hxx"
#include "tab_window.hxx"
#include "managed_window.hxx"


namespace page {

struct img_t {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 3:RGB, 4:RGBA */
  unsigned char  pixel_data[16 * 16 * 4 + 1];
};

class notebook_t : public tree_t {

public:

	static int const BORDER_SIZE = 4;
	static int const HEIGHT = 24;

	enum select_e {
		SELECT_NONE,
		SELECT_TAB,
		SELECT_TOP,
		SELECT_BOTTOM,
		SELECT_LEFT,
		SELECT_RIGHT
	};

	// list of client to maintain tab order
	tab_window_list_t _clients;
	// list of selected to have smart unselect (when window is closed we
	// select the previous window selected
	tab_window_list_t _selected;
	// set of map for fast check is window is in this notebook
	tab_window_set_t _client_map;

	std::map<window_t *, window_icon_handler_t *> icons;

	box_int_t client_area;

	box_t<int> button_close;
	box_t<int> button_vsplit;
	box_t<int> button_hsplit;
	box_t<int> button_pop;

	box_t<int> tab_area;
	box_t<int> top_area;
	box_t<int> bottom_area;
	box_t<int> left_area;
	box_t<int> right_area;

	box_t<int> popup_top_area;
	box_t<int> popup_bottom_area;
	box_t<int> popup_left_area;
	box_t<int> popup_right_area;

	box_t<int> close_client_area;

	void set_selected(managed_window_t * c);

	void update_client_position(managed_window_t * c);

public:
	notebook_t();
	~notebook_t();
	void update_allocation(box_t<int> & allocation);
	void render(cairo_t * cr);


	bool process_button_press_event(XEvent const * e);

	cairo_t * get_cairo();
	void replace(tree_t * src, tree_t * by);
	void close(tree_t * src);
	void remove(tree_t * src);
	tab_window_set_t get_windows();

	bool add_client(managed_window_t * c, bool prefer_activate);
	void remove_client(managed_window_t * c);

	void activate_client(managed_window_t * x);
	void iconify_client(managed_window_t * x);

	box_int_t get_new_client_size();

	void select_next();
	void delete_all();

	virtual void unmap_all();
	virtual void map_all();

	notebook_t * get_nearest_notebook();

	virtual box_int_t get_absolute_extend();
	virtual region_t<int> get_area();
	virtual void set_allocation(box_int_t const & area);

	managed_window_t * find_client_tab(int x, int y);

	void update_close_area();

	/*
	 * Compute client size taking in account possible max_width and max_heigth
	 * and the window Hints.
	 * @output width: width result
	 * @output height: height result
	 */

	static void compute_client_size_with_constraint(window_t * c,
			unsigned int max_width, unsigned int max_height,
			unsigned int & width, unsigned int & height);

	cairo_surface_t * get_icon_surface(window_t * w);

	box_int_t compute_client_size(window_t * c);

};

}

#endif /* NOTEBOOK_HXX_ */
