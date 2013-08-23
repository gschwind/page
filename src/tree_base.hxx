/*
 * tree_base.hxx
 *
 *  Created on: 18 août 2013
 *      Author: bg
 */

#ifndef TREE_BASE_HXX_
#define TREE_BASE_HXX_

#include <list>
#include <string>

#include "managed_window_base.hxx"
#include "tree.hxx"

using namespace std;

namespace page {


enum box_type_e {
	THEME_NOTEBOOK_CLIENT,
	THEME_NOTEBOOK_CLIENT_CLOSE,
	THEME_NOTEBOOK_CLIENT_UNBIND,
	THEME_MOTEBOOK_CLOSE,
	THEME_NOTEBOOK_VSPLIT,
	THEME_NOTEBOOK_HSPLIT,
	THEME_NOTEBOOK_MARK,
	THEME_SPLIT
};




struct box_any_t {
	box_type_e type;
	box_t<int> position;
};

//THEME_NOTEBOOK_CLIENT,
struct box_notebook_client_t : public box_any_t {
	managed_window_base_t * client;
	notebook_base_t * notebook;
};

//THEME_NOTEBOOK_CLIENT_CLOSE,
struct box_notebook_client_close_t : public box_any_t {
	managed_window_base_t * client;
	notebook_base_t * notebook;
};

//THEME_NOTEBOOK_CLIENT_UNBIND,
struct box_notebook_client_unbind_t : public box_any_t {
	managed_window_base_t * client;
	notebook_base_t * notebook;
};

//THEME_NOTEBOOK_CLOSE,
struct box_notebook_close_t : public box_any_t {
	notebook_base_t * notebook;
};

//THEME_NOTEBOOK_VSPLIT,
struct box_notebook_vsplit_t : public box_any_t {
	notebook_base_t * notebook;
};

//THEME_NOTEBOOK_HSPLIT,
struct box_notebook_hsplit_t : public box_any_t {
	notebook_base_t * notebook;
};


//THEME_NOTEBOOK_MARK,
struct box_notebook_mark_t : public box_any_t {
	notebook_base_t * notebook;
};

//THEME_SPLIT
struct box_split_t : public box_any_t {
	split_base_t * split;
};



}


#endif /* TREE_BASE_HXX_ */
