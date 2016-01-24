/*
 * renderable_floating_outer_gradien.hxx
 *
 *  Created on: 25 sept. 2014
 *      Author: gschwind
 */

#ifndef SRC_RENDERABLE_UNMANAGED_GAUSSIAN_SHADOW_HXX_
#define SRC_RENDERABLE_UNMANAGED_GAUSSIAN_SHADOW_HXX_

#include "tree.hxx"

#include <cmath>

namespace page {


inline double _gaussian(double const sigma, double d) {
	return exp(-(d*d)/(2.0*sigma*sigma))/(sigma*sqrt(2.0*M_PI));
}


template<unsigned const SIZE>
class renderable_unmanaged_gaussian_shadow_t : public tree_t {
	enum {
		TOP_LEFT = 0,
		TOP = 1,
		TOP_RIGHT = 2,
		LEFT = 3,
		RIGHT = 4,
		BOT_LEFT = 5,
		BOT = 6,
		BOT_RIGHT = 7,
		MAX = 8
	};

	static unsigned char xmask[MAX][SIZE*SIZE*4];
	static cairo_surface_t * surf[MAX];
	static cairo_pattern_t * pattern[MAX];

	rect _r;
	color_t _color;

public:

	renderable_unmanaged_gaussian_shadow_t(rect r, color_t c) :
			_r(r), _color{c} {

		if(surf[0] == nullptr) {

			for(int y = 0; y < SIZE; ++y) {
				for(int x = 0; x < SIZE; ++x) {
					unsigned char v = 255*_gaussian(1.0, sqrt((SIZE-x)*(SIZE-x)+(SIZE-y)*(SIZE-y))*3.0/SIZE);

					xmask[TOP_LEFT][4*(x+SIZE*y) + 3] = v;
					xmask[BOT_RIGHT][4*((SIZE-1-x) + SIZE*(SIZE-1-y)) + 3] = v;
					xmask[TOP_RIGHT][4*((SIZE-1-x) + SIZE*(y)) + 3] = v;
					xmask[BOT_LEFT][4*(x + SIZE*(SIZE-1-y)) + 3] = v;
				}
			}

			for(int i = 0; i < SIZE; ++i) {
				for(int j = 0; j < SIZE; ++j) {
					unsigned char v = 255*_gaussian(1.0, (SIZE-i)*3.0/SIZE);
					xmask[LEFT][4*(i + SIZE*j) + 3] = v;
					xmask[TOP][4*(j + SIZE*i) + 3] = v;
					xmask[RIGHT][4*(SIZE-1-i + SIZE*j) + 3] = v;
					xmask[BOT][4*(SIZE-1-j + SIZE*(SIZE-1-i)) + 3] = v;
				}
			}

			for(int i = 0; i < MAX; ++i) {
				surf[i] = cairo_image_surface_create_for_data(xmask[i], CAIRO_FORMAT_ARGB32, SIZE, SIZE, SIZE*4);
				pattern[i] = cairo_pattern_create_for_surface(surf[i]);
				cairo_pattern_set_extend(pattern[i], CAIRO_EXTEND_REPEAT);
			}

		}


	}

	virtual ~renderable_unmanaged_gaussian_shadow_t() { }

	/**
	 * draw the area of a renderable to the destination surface
	 * @param cr the destination surface context
	 * @param area the area to redraw
	 **/
	virtual void render(cairo_t * cr, region const & area) {

		for (auto & cl : area.rects()) {

			cairo_save(cr);

			/** draw left shawdow **/
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_clip(cr, cl & rect(_r.x - SIZE, _r.y, SIZE, _r.h));
			cairo_translate(cr, _r.x - SIZE, _r.y);
			::cairo_set_source_rgb(cr, _color.r, _color.g, _color.b);
			cairo_mask(cr, pattern[LEFT]);
			cairo_restore(cr);

			/** draw right shadow **/
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_clip(cr, cl & rect(_r.x + _r.w, _r.y, SIZE, _r.h));
			cairo_translate(cr, _r.x + _r.w, _r.y);
			::cairo_set_source_rgb(cr, _color.r, _color.g, _color.b);
			cairo_mask(cr, pattern[RIGHT]);
			cairo_restore(cr);

			/** draw top shadow **/
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_clip(cr, cl & rect(_r.x, _r.y - SIZE, _r.w, SIZE));
			cairo_translate(cr, _r.x, _r.y - SIZE);
			::cairo_set_source_rgb(cr, _color.r, _color.g, _color.b);
			cairo_mask(cr, pattern[TOP]);
			cairo_restore(cr);

			/** draw bottom shadow **/
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_clip(cr, cl & rect(_r.x, _r.y + _r.h, _r.w, SIZE));
			cairo_translate(cr, _r.x, _r.y + _r.h);
			::cairo_set_source_rgb(cr, _color.r, _color.g, _color.b);
			cairo_mask(cr, pattern[BOT]);
			cairo_restore(cr);

			/** draw top-left corner **/
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_clip(cr, cl & rect(_r.x - SIZE, _r.y - SIZE, SIZE, SIZE));
			cairo_translate(cr, _r.x - SIZE, _r.y - SIZE);
			::cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
			cairo_mask(cr, pattern[TOP_LEFT]);
			cairo_restore(cr);

			/** draw top-right corner **/
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_clip(cr, cl & rect(_r.x + _r.w, _r.y - SIZE, SIZE, SIZE));
			cairo_translate(cr, _r.x + _r.w, _r.y - SIZE);
			::cairo_set_source_rgb(cr, _color.r, _color.g, _color.b);
			cairo_mask(cr, pattern[TOP_RIGHT]);
			cairo_restore(cr);

			/** bottom-left corner **/
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_clip(cr, cl & rect(_r.x - SIZE, _r.y + _r.h, SIZE, SIZE));
			cairo_translate(cr, _r.x - SIZE, _r.y + _r.h);
			::cairo_set_source_rgb(cr, _color.r, _color.g, _color.b);
			cairo_mask(cr, pattern[BOT_LEFT]);
			cairo_restore(cr);

			/** draw bottom-right corner **/
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_clip(cr, cl & rect(_r.x + _r.w, _r.y + _r.h, SIZE, SIZE));
			cairo_translate(cr, _r.x + _r.w, _r.y + _r.h);
			::cairo_set_source_rgb(cr, _color.r, _color.g, _color.b);
			cairo_mask(cr, pattern[BOT_RIGHT]);
			cairo_restore(cr);

			cairo_restore(cr);
		}

	}

	/**
	 * Derived class must return opaque region for this object,
	 * If unknown it's safe to leave this empty.
	 **/
	virtual region get_opaque_region() {
		return region{};
	}

	/**
	 * Derived class must return visible region,
	 * If unknow the whole screen can be returned, but draw will be called each time.
	 **/
	virtual region get_visible_region() {
		region ret;
		ret += rect(_r.x - SIZE, _r.y, SIZE, _r.h);
		ret += rect(_r.x + _r.w, _r.y, SIZE, _r.h);
		ret += rect(_r.x, _r.y - SIZE, _r.w, SIZE);
		ret += rect(_r.x, _r.y + _r.h, _r.w, SIZE);
		ret += rect(_r.x - SIZE, _r.y - SIZE, SIZE, SIZE);
		ret += rect(_r.x + _r.w, _r.y - SIZE, SIZE, SIZE);
		ret += rect(_r.x - SIZE, _r.y + _r.h, SIZE, SIZE);
		ret += rect(_r.x + _r.w, _r.y + _r.h, SIZE, SIZE);
		return ret;
	}

	virtual region get_damaged() {
		return region{};
	}

};

/** C++ allow multiple definition of this, if they are identical, thus don't worry **/

template<unsigned const SIZE>
unsigned char renderable_unmanaged_gaussian_shadow_t<SIZE>::xmask[MAX][SIZE*SIZE*4] = { 0 };

template<unsigned const SIZE>
cairo_surface_t * renderable_unmanaged_gaussian_shadow_t<SIZE>::surf[MAX] = { nullptr };

template<unsigned const SIZE>
cairo_pattern_t * renderable_unmanaged_gaussian_shadow_t<SIZE>::pattern[MAX] = { nullptr };


}



#endif /* RENDERABLE_FLOATING_OUTER_GRADIEN_HXX_ */
