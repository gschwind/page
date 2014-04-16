/*
 * events_handler_default.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef EVENTS_HANDLER_DEFAULT_HXX_
#define EVENTS_HANDLER_DEFAULT_HXX_

#include "page_interface.hxx"
#include "events_handler_interface.hxx"

namespace page {

class events_handler_default : public events_handler_interface {

	page_interface * _page;

	virtual ~events_handler_default() { }

	virtual void process_event(XKeyEvent const & e);
	virtual void process_event(XButtonEvent const & e);
	virtual void process_event(XMotionEvent const & e);
	virtual void process_event(XCrossingEvent const & e);
	virtual void process_event(XFocusChangeEvent const & e);
	virtual void process_event(XExposeEvent const & e);
	virtual void process_event(XGraphicsExposeEvent const & e);
	virtual void process_event(XNoExposeEvent const & e);
	virtual void process_event(XVisibilityEvent const & e);
	virtual void process_event(XCreateWindowEvent const & e);
	virtual void process_event(XDestroyWindowEvent const & e);
	virtual void process_event(XUnmapEvent const & e);
	virtual void process_event(XMapEvent const & e);
	virtual void process_event(XMapRequestEvent const & e);
	virtual void process_event(XReparentEvent const & e);
	virtual void process_event(XConfigureEvent const & e);
	virtual void process_event(XGravityEvent const & e);
	virtual void process_event(XResizeRequestEvent const & e);
	virtual void process_event(XConfigureRequestEvent const & e);
	virtual void process_event(XCirculateEvent const & e);
	virtual void process_event(XCirculateRequestEvent const & e);
	virtual void process_event(XPropertyEvent const & e);
	virtual void process_event(XSelectionClearEvent const & e);
	virtual void process_event(XSelectionRequestEvent const & e);
	virtual void process_event(XSelectionEvent const & e);
	virtual void process_event(XColormapEvent const & e);
	virtual void process_event(XClientMessageEvent const & e);
	virtual void process_event(XMappingEvent const & e);
	virtual void process_event(XErrorEvent const & e);
	virtual void process_event(XKeymapEvent const & e);
	virtual void process_event(XGenericEvent const & e);
	virtual void process_event(XGenericEventCookie const & e);

	/* extension events */
	virtual void process_event(XDamageNotifyEvent const & e);

	virtual void process_event(XRRNotifyEvent const & e);

	virtual void process_event(XShapeEvent const & e);

};

}

#endif /* EVENTS_HANDLER_DEFAULT_HXX_ */
