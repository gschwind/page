/*
 * split.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef SPLIT_HXX_
#define SPLIT_HXX_

#include "theme.hxx"
#include "page_context.hxx"
#include "page_component.hxx"

namespace page {

using time_t = page::time_t;

class split_t : public page_component_t {
	page_context_t * _ctx;
	page_component_t * _parent;

	rect _allocation;
	rect _split_bar_area;
	split_type_e _type;
	double _ratio;
	page_component_t * _pack0;
	page_component_t * _pack1;

	rect bpack0;
	rect bpack1;

	std::list<tree_t *> _children;

	bool _is_hidden;

	split_t(split_t const &);
	split_t & operator=(split_t const &);

	void update_allocation_pack0();
	void update_allocation_pack1();
	void update_allocation();

public:
	split_t(page_context_t * ctx, split_type_e type);
	~split_t();

	/* access to stuff */
	auto get_split_bar_area() const -> rect const & { return _split_bar_area; }
	auto get_pack0() const -> page_component_t * { return _pack0; }
	auto get_pack1() const -> page_component_t * { return _pack1; }
	auto allocation() const -> rect { return _allocation; }
	auto ratio() const -> double { return _ratio; }
	auto type() const -> split_type_e { return _type; }
	auto parent() const -> page_component_t * { return _parent; }
	auto get_node_name() const -> std::string { return _get_node_name<'S'>(); }


	void replace(page_component_t * src, page_component_t * by);
	void compute_split_bar_area(rect & area, double split) const;
	void set_allocation(rect const & area);
	void set_split(double split);
	void set_theme(theme_t const * theme);
	void set_pack0(page_component_t * x);
	void set_pack1(page_component_t * x);
	void compute_split_location(double split, int & x, int & y) const;
	void compute_split_size(double split, int & w, int & h) const;
	void render_legacy(cairo_t * cr) const;
	void activate(tree_t * t = nullptr);
	void remove(tree_t * t);
	virtual void prepare_render(std::vector<std::shared_ptr<renderable_t>> & out, time_t const & time);
	rect compute_split_bar_location() const;
	void set_parent(tree_t * t);
	void set_parent(page_component_t * t);
//	void get_all_children(std::vector<tree_t *> & out) const;
	void children(std::vector<tree_t *> & out) const;
	void hide();
	void show();
	void get_visible_children(std::vector<tree_t *> & out);

	bool button_press(xcb_button_press_event_t const * ev);

};

}

#endif /* SPLIT_HXX_ */
