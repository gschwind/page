/*
 * theme_layout.hxx
 *
 *  Created on: 23 mars 2013
 *      Author: gschwind
 */

#ifndef THEME_LAYOUT_HXX_
#define THEME_LAYOUT_HXX_

#include "box.hxx"

using namespace std;

namespace page {

struct margin_t {
	unsigned char top;
	unsigned char left;
	unsigned char right;
	unsigned char bottom;
};

union theme_box_u {

};

struct theme_layout_t {
	margin_t notebook_margin;
	margin_t split_margin;
	margin_t floating_margin;

	unsigned char split_width;

	virtual ~theme_layout_t() { }

	virtual list<box_int_t> compute_client_tab(box_int_t const & allocation, int number_of_client, int selected_client_index) const = 0;
	virtual box_int_t compute_notebook_close_window_position(box_int_t const & allocation, int number_of_client, int selected_client_index) const = 0;
	virtual box_int_t compute_notebook_unbind_window_position(box_int_t const & allocation, int number_of_client, int selected_client_index) const = 0;

	virtual box_int_t compute_notebook_bookmark_position(box_int_t const & allocation, int number_of_client, int selected_client_index) const = 0;
	virtual box_int_t compute_notebook_vsplit_position(box_int_t const & allocation, int number_of_client, int selected_client_index) const = 0;
	virtual box_int_t compute_notebook_hsplit_position(box_int_t const & allocation, int number_of_client, int selected_client_index) const = 0;
	virtual box_int_t compute_notebook_close_position(box_int_t const & allocation, int number_of_client, int selected_client_index) const = 0;


	virtual box_int_t compute_floating_close_position(box_int_t const & _allocation) const = 0;
	virtual box_int_t compute_floating_bind_position(box_int_t const & _allocation) const = 0;

};

}


#endif /* THEME_LAYOUT_HXX_ */
