/*
 * events_handler_interface.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef EVENTS_HANDLER_INTERFACE_HXX_
#define EVENTS_HANDLER_INTERFACE_HXX_

namespace page {

class events_handler_interface {

public:

	virtual ~events_handler_interface() { }

	virtual void process_event(XKeyEvent const & e) = 0;
	virtual void process_event(XButtonEvent const & e) = 0;
	virtual void process_event(XMotionEvent const & e) = 0;
	virtual void process_event(XCrossingEvent const & e) = 0;
	virtual void process_event(XFocusChangeEvent const & e) = 0;
	virtual void process_event(XExposeEvent const & e) = 0;
	virtual void process_event(XGraphicsExposeEvent const & e) = 0;
	virtual void process_event(XNoExposeEvent const & e) = 0;
	virtual void process_event(XVisibilityEvent const & e) = 0;
	virtual void process_event(XCreateWindowEvent const & e) = 0;
	virtual void process_event(XDestroyWindowEvent const & e) = 0;
	virtual void process_event(XUnmapEvent const & e) = 0;
	virtual void process_event(XMapEvent const & e) = 0;
	virtual void process_event(XMapRequestEvent const & e) = 0;
	virtual void process_event(XReparentEvent const & e) = 0;
	virtual void process_event(XConfigureEvent const & e) = 0;
	virtual void process_event(XGravityEvent const & e) = 0;
	virtual void process_event(XResizeRequestEvent const & e) = 0;
	virtual void process_event(XConfigureRequestEvent const & e) = 0;
	virtual void process_event(XCirculateEvent const & e) = 0;
	virtual void process_event(XCirculateRequestEvent const & e) = 0;
	virtual void process_event(XPropertyEvent const & e) = 0;
	virtual void process_event(XSelectionClearEvent const & e) = 0;
	virtual void process_event(XSelectionRequestEvent const & e) = 0;
	virtual void process_event(XSelectionEvent const & e) = 0;
	virtual void process_event(XColormapEvent const & e) = 0;
	virtual void process_event(XClientMessageEvent const & e) = 0;
	virtual void process_event(XMappingEvent const & e) = 0;
	virtual void process_event(XErrorEvent const & e) = 0;
	virtual void process_event(XKeymapEvent const & e) = 0;
	virtual void process_event(XGenericEvent const & e) = 0;
	virtual void process_event(XGenericEventCookie const & e) = 0;

	/* extension events */
	virtual void process_event(XDamageNotifyEvent const & e) = 0;

	virtual void process_event(XRRNotifyEvent const & e) = 0;

	virtual void process_event(XShapeEvent const & e) = 0;




};


}



#endif /* EVENTS_HANDLER_INTERFACE_HXX_ */
