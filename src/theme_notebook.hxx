/*
 * notebook_base.hxx
 *
 *  Created on: 10 mai 2014
 *      Author: gschwind
 */

#ifndef THEME_NOTEBOOK_HXX_
#define THEME_NOTEBOOK_HXX_

namespace page {

struct theme_notebook_t {
	int root_x, root_y;
	i_rect allocation;
	i_rect client_position;
	theme_tab_t selected_client;
	std::vector<theme_tab_t> clients_tab;
	bool is_default;
	bool has_selected_client;
};

}


#endif /* NOTEBOOK_BASE_HXX_ */
