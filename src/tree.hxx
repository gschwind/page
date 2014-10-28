/*
 * tree.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef TREE_HXX_
#define TREE_HXX_

#include <stdint.h>

#include <X11/Xlib.h>
#include <cairo.h>
#include <list>
#include <vector>
#include "region.hxx"
#include "time.hxx"
#include "renderable.hxx"

namespace page {

using namespace std;

class tree_t {
public:
	tree_t() { }

	virtual ~tree_t();

	virtual auto parent() const -> tree_t * = 0;
	virtual auto get_node_name() const -> string = 0;
	virtual auto raise_child(tree_t * t = nullptr) -> void = 0;
	virtual auto remove(tree_t * t) -> void = 0;
	virtual auto set_parent(tree_t * parent) -> void = 0;

	virtual auto children(vector<tree_t *> & out) const -> void = 0;
	virtual auto get_all_children(vector<tree_t *> & out) const -> void = 0;
	virtual auto get_visible_children(vector<tree_t *> & out) -> void = 0;

	virtual auto hide() -> void = 0;
	virtual auto show() -> void = 0;
	virtual auto prepare_render(vector<ptr<renderable_t>> & out, page::time_t const & time) -> void = 0;


	template<char const c>
	string _get_node_name() const {
		char buffer[64];
		snprintf(buffer, 64, "%c #%016lx #%016lx", c, (uintptr_t) parent(),
				(uintptr_t) this);
		return string(buffer);
	}


	void print_tree(int level = 0) const {
		char space[] = "                               ";
		space[level] = 0;
		cout << space << get_node_name() << endl;
		for(auto i: children()) {
			i->print_tree(level+1);
		}

	}

	vector<tree_t *> children() const {
		vector<tree_t *> ret;
		children(ret);
		return ret;
	}

	vector<tree_t *> get_all_children() const {
		vector<tree_t *> ret;
		get_all_children(ret);
		return ret;
	}

	vector<tree_t *> get_visible_children() {
		vector<tree_t *> ret;
		get_visible_children(ret);
		return ret;
	}

};


}

#endif /* TREE_HXX_ */
