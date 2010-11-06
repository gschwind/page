/*
 * Copyright (C) 2010 Benoit Gschwind
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "client.h"

void client_update_size_hints(client * ths) {
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(ths->ctx->xdpy, ths->xwin, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;

	if (size.flags & PBaseSize) {
		ths->basew = size.base_width;
		ths->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		ths->basew = size.min_width;
		ths->baseh = size.min_height;
	} else {
		ths->basew = 0;
		ths->baseh = 0;
	}

	if (size.flags & PResizeInc) {
		ths->incw = size.width_inc;
		ths->inch = size.height_inc;
	} else {
		ths->incw = 0;
		ths->inch = 0;
	}

	if (size.flags & PMaxSize) {
		ths->maxw = size.max_width;
		ths->maxh = size.max_height;
	} else {
		ths->maxw = 0;
		ths->maxh = 0;
	}

	if (size.flags & PMinSize) {
		ths->minw = size.min_width;
		ths->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		ths->minw = size.base_width;
		ths->minh = size.base_height;
	} else {
		ths->minw = 0;
		ths->minh = 0;
	}

	if (size.flags & PAspect) {
		if (size.min_aspect.x != 0 && size.max_aspect.y != 0) {
			ths->mina = (gdouble) size.min_aspect.y
					/ (gdouble) size.min_aspect.x;
			ths->maxa = (gdouble) size.max_aspect.x
					/ (gdouble) size.max_aspect.y;
		}
	} else {
		ths->maxa = 0.0;
		ths->mina = 0.0;
	}
	ths->is_fixed_size = (ths->maxw && ths->minw && ths->maxh && ths->minh
			&& ths->maxw == ths->minw && ths->maxh == ths->minh);
}
