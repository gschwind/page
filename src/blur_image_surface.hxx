/*
 * blur_image_surface.hxx
 *
 *  Created on: 21 août 2015
 *      Author: gschwind
 */

#ifndef SRC_BLUR_IMAGE_SURFACE_HXX_
#define SRC_BLUR_IMAGE_SURFACE_HXX_

#include <cairo.h>

namespace page {

void blur_image_surface (cairo_surface_t *surface, double sigma);

}

#endif /* SRC_BLUR_IMAGE_SURFACE_HXX_ */
