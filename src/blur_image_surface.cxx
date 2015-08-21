/*
 * Copyright © 2008 Kristian Høgsberg
 * Copyright © 2009 Chris Wilson
 * Copyright © 2015 Benoit Gschwind
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "blur_image_surface.hxx"

#include <cmath>
#include <stdint.h>
#include <iostream>

namespace page {

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

/* Performs a simple 2D Gaussian blur of radius @radius on surface @surface. */
/*
 * Image borders are blurred with left-right and top-bottom mirror
 *
 */
void
blur_image_surface (cairo_surface_t *surface, double sigma)
{
    cairo_surface_t *tmp;
    int width, height;
    int src_stride, dst_stride;
    uint32_t x, y, z, w;
    uint8_t *src, *dst;
    uint32_t *s, *d, p;
    uint32_t a;
    int i, j, k;
    int kernel[101];
    const int size = ARRAY_LENGTH (kernel);
    const int half = size / 2;

    if (cairo_surface_status (surface))
	return;

    width = cairo_image_surface_get_width (surface);
    height = cairo_image_surface_get_height (surface);

    switch (cairo_image_surface_get_format (surface)) {
    case CAIRO_FORMAT_A1:
    default:
	/* Don't even think about it! */
	return;

    case CAIRO_FORMAT_A8:
	/* Handle a8 surfaces by effectively unrolling the loops by a
	 * factor of 4 - this is safe since we know that stride has to be a
	 * multiple of uint32_t. */
	width /= 4;
	break;

    case CAIRO_FORMAT_RGB24:
    case CAIRO_FORMAT_ARGB32:
	break;
    }

    tmp = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status (tmp))
    	return;

    src = cairo_image_surface_get_data (surface);
    src_stride = cairo_image_surface_get_stride (surface);

    dst = cairo_image_surface_get_data (tmp);
    dst_stride = cairo_image_surface_get_stride (tmp);

    a = 0;
    for (i = 0; i < size; i++) {
	double f = i - half;
	/**
	 * 1000 is for fixed floating point. its must be big enough in regard of kernel size
	 */
	a += kernel[i] = exp (- f * f / (2.0 * sigma * sigma)) / (sigma*sqrt(2.0*M_PI)) * 1000;
    }

    /* Horizontally blur from surface -> tmp */
    for (i = 0; i < height; i++) {
	s = (uint32_t *) (src + i * src_stride);
	d = (uint32_t *) (dst + i * dst_stride);
	for (j = 0; j < width; j++) {
	    x = y = z = w = 0;
	    for (k = 0; k < size; k++) {
	    int jj = j - half + k;

	    /* fix boundaries */
		for(;;) {
			if(jj < 0)
				jj += width;
			if(jj >= width)
				jj -= width;
			break;
		}

		p = s[jj];

		x += ((p >> 24) & 0xff) * kernel[k];
		y += ((p >> 16) & 0xff) * kernel[k];
		z += ((p >>  8) & 0xff) * kernel[k];
		w += ((p >>  0) & 0xff) * kernel[k];
	    }
	    d[j] = (x / a << 24) | (y / a << 16) | (z / a << 8) | w / a;
	}
    }

    /* Then vertically blur from tmp -> surface */
    for (i = 0; i < height; i++) {
	s = (uint32_t *) (dst + i * dst_stride);
	d = (uint32_t *) (src + i * src_stride);
	for (j = 0; j < width; j++) {
	    x = y = z = w = 0;
	    for (k = 0; k < size; k++) {
	    int ii = i - half + k;

	    /* fix boundaries */
		for(;;) {
			if(ii < 0)
				ii += height;
			if(ii >= height)
				ii -= height;
			break;
		}

		s = (uint32_t *) (dst + (ii) * dst_stride);
		p = s[j];

		x += ((p >> 24) & 0xff) * kernel[k];
		y += ((p >> 16) & 0xff) * kernel[k];
		z += ((p >>  8) & 0xff) * kernel[k];
		w += ((p >>  0) & 0xff) * kernel[k];
	    }
	    d[j] = (x / a << 24) | (y / a << 16) | (z / a << 8) | w / a;
	}
    }

    cairo_surface_destroy (tmp);
    cairo_surface_mark_dirty (surface);
}

}
