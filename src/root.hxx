/*
 * root.hxx
 *
 *  Created on: Feb 27, 2011
 *      Author: gschwind
 */

#ifndef ROOT_HXX_
#define ROOT_HXX_

#include <cairo.h>
#include "tree.hxx"
#include "box.hxx"

namespace page {

//class page_t;
//
//class root_t: public tree_t {
//	page_t & page;
//	std::list<screen_t *> subarea;
//public:
//	root_t(page_t & page);
//	~root_t();
//	void update_allocation(box_t<int> & allocation);
//	void render();
//	bool process_button_press_event(XEvent const * e);
//	void replace(tree_t * src, tree_t * by);
//	void close(tree_t * src);
//	void remove(tree_t * src);
//	window_list_t get_windows();
//
//	virtual bool add_client(window_t * x);
//	virtual void remove_client(window_t * x);
//	virtual void activate_client(window_t * x);
//	virtual void iconify_client(window_t * x);
//
//	void add_aera(box_t<int> & area);
//	void delete_all();
//};

}

#endif /* ROOT_HXX_ */
