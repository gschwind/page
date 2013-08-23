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
#include <typeinfo>

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
	box_t<int> position;
	virtual ~box_any_t() { }
};

//THEME_NOTEBOOK_CLIENT,
struct box_notebook_client_t : public box_any_t {
	managed_window_base_t const * client;
	notebook_base_t const * notebook;
};

//THEME_NOTEBOOK_CLIENT_CLOSE,
struct box_notebook_client_close_t : public box_any_t {
	managed_window_base_t const * client;
	notebook_base_t const * notebook;
};

//THEME_NOTEBOOK_CLIENT_UNBIND,
struct box_notebook_client_unbind_t : public box_any_t {
	managed_window_base_t const * client;
	notebook_base_t const * notebook;
};

//THEME_NOTEBOOK_CLOSE,
struct box_notebook_close_t : public box_any_t {
	notebook_base_t const * notebook;
};

//THEME_NOTEBOOK_VSPLIT,
struct box_notebook_vsplit_t : public box_any_t {
	notebook_base_t const * notebook;
};

//THEME_NOTEBOOK_HSPLIT,
struct box_notebook_hsplit_t : public box_any_t {
	notebook_base_t const * notebook;
};


//THEME_NOTEBOOK_MARK,
struct box_notebook_mark_t : public box_any_t {
	notebook_base_t const * notebook;
};

//THEME_SPLIT
struct box_split_t : public box_any_t {
	split_base_t const * split;
};

/**
 * Floating windows areas
 **/

struct box_floating_close_t : public box_any_t {
};

struct box_floating_bind_t : public box_any_t {
};

struct box_floating_title_t : public box_any_t {
};

struct box_floating_grip_top_t : public box_any_t {
};

struct box_floating_grip_left_t : public box_any_t {
};

struct box_floating_grip_right_t : public box_any_t {
};

struct box_floating_grip_bottom_t : public box_any_t {
};

struct box_floating_grip_top_left_t : public box_any_t {
};

struct box_floating_grip_top_right_t : public box_any_t {
};

struct box_floating_grip_bottom_left_t : public box_any_t {
};

struct box_floating_grip_bottom_right_t : public box_any_t {
};


}


#endif /* TREE_BASE_HXX_ */
