/*
 * window_overlay.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef WINDOW_OVERLAY_HXX_
#define WINDOW_OVERLAY_HXX_
//
//
//namespace page {
//
//
//class window_overlay_t : public renderable_t {
//
//	static long const OVERLAY_EVENT_MASK = ExposureMask | StructureNotifyMask;
//
//protected:
//	i_rect _position;
//
//	bool _has_alpha;
//	bool _is_durty;
//	bool _is_visible;
//
//public:
//	window_overlay_t(i_rect position = i_rect(-10,-10, 1, 1)) {
//		_position = position;
//		_has_alpha = true;
//		_is_durty = true;
//		_is_visible = false;
//	}
//
//	void mark_durty() {
//		_is_durty = true;
//	}
//
//	virtual ~window_overlay_t() {
//
//	}
//
//	void move_resize(i_rect const & area) {
//		_position = area;
//	}
//
//	void move(int x, int y) {
//		_position.x = x;
//		_position.y = y;
//	}
//
//	void show() {
//		_is_visible = true;
//	}
//
//	void hide() {
//		_is_visible = false;
//	}
//
//	bool is_visible() {
//		return _is_visible;
//	}
//
//	i_rect const & position() {
//		return _position;
//	}
//
//	/**
//	 * Derived class must return opaque region for this object,
//	 * If unknown it's safe to leave this empty.
//	 **/
//	virtual region get_opaque_region() {
//		return region{_position};
//	}
//
//	/**
//	 * Derived class must return visible region,
//	 * If unknow the whole screen can be returned, but draw will be called each time.
//	 **/
//	virtual region get_visible_region() {
//		return region{_position};
//	}
//
//	/**
//	 * return currently damaged area (absolute)
//	 **/
//	virtual region get_damaged()  {
//		if(_is_durty) {
//			return region{_position};
//		} else {
//			return region{};
//		}
//	}
//
//};
//
//}
//


#endif /* WINDOW_OVERLAY_HXX_ */
