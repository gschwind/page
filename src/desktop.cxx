/*
 * desktop.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <algorithm>
#include <typeinfo>
#include "viewport.hxx"
#include "desktop.hxx"

namespace page {

void desktop_t::replace(page_component_t * src, page_component_t * by) {
	throw std::runtime_error("desktop_t::replace implemented yet!");
}

void desktop_t::remove(tree_t * src) {
	for(auto i : _viewport_outputs) {
		if(i.second == src) {
			_viewport_outputs.erase(i.first);
			return;
		}
	}
}

void desktop_t::set_allocation(i_rect const & area) {
	_allocation = area;
}

void desktop_t::get_all_children(vector<tree_t *> & out) const {
	for(auto i: _viewport_outputs) {
		out.push_back(i.second);
		i.second->get_all_children(out);
	}
}

}
