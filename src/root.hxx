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
#include "page.hxx"

namespace page_next {

class root_t: public tree_t {
	tree_t * _pack0;
public:
	root_t(main_t & page, box_t<int> &allocation);
	~root_t();
	void update_allocation(box_t<int> & allocation);

	void render();
	bool process_button_press_event(XEvent const * e);
	bool add_notebook(client_t *c);
	void replace(tree_t * src, tree_t * by);
	void close(tree_t * src);
	void remove(tree_t * src);
	std::list<client_t *> *  get_clients();
	void remove_client(client_t * c);
	void activate_client(client_t * c);
};

}

#endif /* ROOT_HXX_ */
