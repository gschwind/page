/*
 * xconnection.cxx
 *
 *  Created on: Oct 30, 2011
 *      Author: gschwind
 */

#include "xconnection.hxx"

namespace page_next {

long int xconnection_t::last_serial = 0;


bool xconnection_t::filter(event_t e) {
	return (e.serial < xconnection_t::last_serial);
}

}



