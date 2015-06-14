/*
 * notebook.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef NOTEBOOK_HXX_
#define NOTEBOOK_HXX_

#include <algorithm>
#include <cmath>
#include <cassert>
#include <memory>

#include "theme.hxx"
#include "pixmap.hxx"
#include "composite_surface.hxx"
#include "renderable_notebook_fading.hxx"
#include "page_event.hxx"
#include "renderable_notebook_fading.hxx"
#include "renderable_pixmap.hxx"
#include "renderable_empty.hxx"

#include "page_context.hxx"
#include "page_component.hxx"
#include "client_managed.hxx"

namespace page {

class client_managed_t;

struct img_t {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 3:RGB, 4:RGBA */
  unsigned char  pixel_data[16 * 16 * 4 + 1];
};

class notebook_t : public page_component_t {
	double const XN = 0.0;
	page::time_t const animation_duration{0, 500000000};

	page_context_t * _ctx;
	page_component_t * _parent;

	i_rect _allocation;

	/* child stack order the first one is the lowest one */
	std::list<tree_t *> _children;

	page::time_t swap_start;

	std::shared_ptr<renderable_notebook_fading_t> fading_notebook;

	mutable theme_notebook_t theme_notebook;

	bool _is_default;
	bool _is_hidden;
	bool _keep_selected;

public:

	enum select_e {
		SELECT_NONE,
		SELECT_TAB,
		SELECT_TOP,
		SELECT_BOTTOM,
		SELECT_LEFT,
		SELECT_RIGHT
	};
	// list of client to maintain tab order
	std::list<client_managed_t *> _clients;
	client_managed_t * _selected;

	// link the client to the corresponding tab theme
	mutable std::map<client_managed_t *, std::shared_ptr<theme_tab_t>> _client_to_tab;

	i_rect client_area;

	i_rect button_close;
	i_rect button_vsplit;
	i_rect button_hsplit;
	i_rect button_pop;

	i_rect tab_area;
	i_rect top_area;
	i_rect bottom_area;
	i_rect left_area;
	i_rect right_area;

	i_rect popup_top_area;
	i_rect popup_bottom_area;
	i_rect popup_left_area;
	i_rect popup_right_area;
	i_rect popup_center_area;

	i_rect close_client_area;
	i_rect undck_client_area;

	void set_selected(client_managed_t * c);

	void update_client_position(client_managed_t * c);

	void start_fading();

public:
	notebook_t(page_context_t * ctx, bool keep_selected);
	~notebook_t();
	void update_allocation(i_rect & allocation);

	bool process_button_press_event(XEvent const * e);

	void replace(page_component_t * src, page_component_t * by);
	void close(tree_t * src);
	void remove(tree_t * src);

	std::list<client_managed_t *> const & get_clients();

	bool add_client(client_managed_t * c, bool prefer_activate);
	void remove_client(client_managed_t * c);

	void activate_client(client_managed_t * x);
	void iconify_client(client_managed_t * x);

	i_rect get_new_client_size();

	void select_next();
	void delete_all();

	virtual void unmap_all();
	virtual void map_all();

	notebook_t * get_nearest_notebook();

	virtual i_rect get_absolute_extend();
	virtual region get_area();

	void set_allocation(i_rect const & area);

	void set_parent(tree_t * t) {
		if(t != nullptr) {
			throw exception_t("notebook_t cannot have tree_t has parent");
		} else {
			_parent = nullptr;
		}
	}

	void set_parent(page_component_t * t) {
		_parent = t;
	}

	client_managed_t * find_client_tab(int x, int y);

	void update_close_area();

	i_rect compute_client_size(client_managed_t * c);
	i_rect const & get_allocation();
	void set_theme(theme_t const * theme);
	std::list<client_managed_t const *> clients() const;
	client_managed_t const * selected() const;
	bool is_default() const;
	void set_default(bool x);
	std::list<tree_t *> childs() const;
	void raise_child(tree_t * t = nullptr);
	std::string get_node_name() const;
	void render_legacy(cairo_t * cr, int x_offset, int y_offset) const;
	void render(cairo_t * cr, time_t time);
	bool need_render(time_t time);

	client_managed_t * get_selected();

	virtual void prepare_render(std::vector<std::shared_ptr<renderable_t>> & out, page::time_t const & time);

	i_rect compute_notebook_close_window_position(int number_of_client, int selected_client_index) const;
	i_rect compute_notebook_unbind_window_position(int number_of_client, int selected_client_index) const;
	i_rect compute_notebook_bookmark_position() const;
	i_rect compute_notebook_vsplit_position() const;
	i_rect compute_notebook_hsplit_position() const;
	i_rect compute_notebook_close_position() const;
	i_rect compute_notebook_menu_position() const;

	void compute_areas_for_notebook(std::vector<page_event_t> * l, int x_offset, int y_offset) const;

	i_rect allocation() const {
		return _allocation;
	}

	page_component_t * parent() const {
		return _parent;
	}

	void get_all_children(std::vector<tree_t *> & out) const;

	void update_theme_notebook(int x_offset, int y_offset) const;

	void children(std::vector<tree_t *> & out) const {
		out.insert(out.end(), _children.begin(), _children.end());
	}

	void hide() {
		_is_hidden = true;
		for(auto i: tree_t::children()) {
			i->hide();
		}
	}

	void show() {
		_is_hidden = false;
		for(auto i: tree_t::children()) {
			i->show();
		}
	}

	void get_visible_children(std::vector<tree_t *> & out) {
		if (not _is_hidden) {
			out.push_back(this);
			for (auto i : tree_t::children()) {
				i->get_visible_children(out);
			}
		}
	}

	bool has_client(client_managed_t * c) {
		return has_key(_clients, c);
	}

	void set_keep_selected(bool x) {
		_keep_selected = x;
	}

};

}

#endif /* NOTEBOOK_HXX_ */
