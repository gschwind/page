/*
 * notebook.hxx
 *
 *  Created on: 27 févr. 2011
 *      Author: gschwind
 */

#ifndef NOTEBOOK_HXX_
#define NOTEBOOK_HXX_

#include <list>
#include <cairo.h>

#include "xconnection.hxx"
#include "box.hxx"
#include "client.hxx"
#include "tree.hxx"
#include "split.hxx"
#include "page.hxx"

namespace page_next {

typedef std::list<client_t *> client_list_t;

struct img_t {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 3:RGB, 4:RGBA */
  unsigned char  pixel_data[16 * 16 * 4 + 1];
};

class notebook_t: public tree_t {
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

	static std::list<notebook_t *> notebooks;
	main_t & page;

	Cursor cursor;

	bool back_buffer_is_valid;
	cairo_surface_t * back_buffer;
	cairo_t * back_buffer_cr;

	box_t<int> button_close;
	box_t<int> button_vsplit;
	box_t<int> button_hsplit;

	box_t<int> tab_area;
	box_t<int> top_area;
	box_t<int> bottom_area;
	box_t<int> left_area;
	box_t<int> right_area;

	client_list_t _clients;
	client_list_t _selected;

	void set_selected(client_t * c);

	static Bool drag_and_drop_filter(Display * dpy, XEvent * ev, char * arg);
	void process_drag_and_drop(client_t * c);

	void update_client_position(client_t * c);

public:
	notebook_t(main_t & cnx);
	~notebook_t();
	void update_allocation(box_t<int> & allocation);
	void render();
	bool process_button_press_event(XEvent const * e);
	bool add_notebook(client_t *c);
	void split(split_type_e type);

	void split_left(client_t * c);
	void split_right(client_t * c);
	void split_top(client_t * c);
	void split_bottom(client_t * c);

	cairo_t * get_cairo();
	void replace(tree_t * src, tree_t * by);
	void close(tree_t * src);
	void remove(tree_t * src);
	client_list_t * get_clients();
	void remove_client(client_t * c);
	void activate_client(client_t * c);

	void select_next();
	void rounded_rectangle(cairo_t * cr, double x, double y, double w, double h, double r);

};

}

#endif /* NOTEBOOK_HXX_ */
