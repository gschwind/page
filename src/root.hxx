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

namespace page_next {

class main_t;

struct screen_t {
	box_t<int> aera;
	box_t<int> sub_aera;
	tree_t * _subtree;
};

class root_t: public tree_t {
	main_t & page;
	std::list<screen_t *> subarea;
public:
	root_t(main_t & page);
	~root_t();
	void update_allocation(box_t<int> & allocation);
	void render();
	bool process_button_press_event(XEvent const * e);
	bool add_client(client_t *c);
	void replace(tree_t * src, tree_t * by);
	void close(tree_t * src);
	void remove(tree_t * src);
	std::list<client_t *> *  get_clients();
	void remove_client(client_t * c);
	void activate_client(client_t * c);
	void add_aera(box_t<int> & area);
	void iconify_client(client_t * c);

};

}

#endif /* ROOT_HXX_ */
