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

#include "window_icon_handler.hxx"
#include "icon.hxx"
#include "box.hxx"

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

enum split_type_e {
	HORIZONTAL_SPLIT, VERTICAL_SPLIT,
};

struct managed_window_base_t {

	virtual ~managed_window_base_t() { }

	virtual box_t<int> const & base_position() = 0;
	virtual window_icon_handler_t * icon() = 0;
	virtual char const * title() = 0;

	/** create a cairo context for top border, must be destroyed with cairo_destroy() **/
	virtual cairo_t * cairo_top() = 0;
	/** create a cairo context for top border, must be destroyed with cairo_destroy() **/
	virtual cairo_t * cairo_bottom() = 0;
	/** create a cairo context for top border, must be destroyed with cairo_destroy() **/
	virtual cairo_t * cairo_left() = 0;
	/** create a cairo context for top border, must be destroyed with cairo_destroy() **/
	virtual cairo_t * cairo_right() = 0;

	virtual bool is_focused() = 0;
};

struct notebook_base_t {
	box_t<int> allocation;
	list<managed_window_base_t const *> clients;
	managed_window_base_t const * selected;
};

struct split_base_t {
	box_t<int> allocation;
	split_type_e type;
	double split;
};

struct viewport_base_t {
	box_t<int> allocation;
};

struct page_base_t {
	list<viewport_base_t const *> viewports;
	list<split_base_t const *> splits;
	list<notebook_base_t const *> notebooks;
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
