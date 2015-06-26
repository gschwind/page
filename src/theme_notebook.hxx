/*
 * notebook_base.hxx
 *
 *  Created on: 10 mai 2014
 *      Author: gschwind
 */

#ifndef THEME_NOTEBOOK_HXX_
#define THEME_NOTEBOOK_HXX_

#include <memory>

namespace page {

struct theme_notebook_t {
	int root_x, root_y;
	i_rect allocation;
	i_rect client_position;
	theme_tab_t selected_client;
	std::vector<theme_tab_t> clients_tab;
	bool is_default;
	bool has_selected_client;


	theme_notebook_t() :
		root_x{}, root_y{},
		allocation{},
		client_position{},
		selected_client{},
		clients_tab{},
		is_default{},
		has_selected_client{}
	{

	}

	theme_notebook_t(theme_notebook_t const & x) :
		root_x{x.root_x}, root_y{x.root_y},
		allocation{x.allocation},
		client_position{x.client_position},
		selected_client{x.selected_client},
		clients_tab{x.clients_tab},
		is_default{x.is_default},
		has_selected_client{x.has_selected_client}
	{

	}

};

}


#endif /* NOTEBOOK_BASE_HXX_ */
