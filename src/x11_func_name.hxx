/*
 * x11_fun_name.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef X11_FUNC_NAME_HXX_
#define X11_FUNC_NAME_HXX_

namespace page {

/* Datas to print X errors */
static char const * const xcore_request_name[] = { 
/*  0 */"(null)",
	"X_CreateWindow", 
	"X_ChangeWindowAttributes", 
	"X_GetWindowAttributes", 
	"X_DestroyWindow", 
	"X_DestroySubwindows", 
	"X_ChangeSaveSet", 
	"X_ReparentWindow", 
	"X_MapWindow", 
	"X_MapSubwindows", 
/*  10 */"X_UnmapWindow",
	"X_UnmapSubwindows", 
	"X_ConfigureWindow", 
	"X_CirculateWindow", 
	"X_GetGeometry", 
	"X_QueryTree", 
	"X_InternAtom", 
	"X_GetAtomName", 
	"X_ChangeProperty", 
	"X_DeleteProperty", 
/*  20 */"X_GetProperty",
	"X_ListProperties", 
	"X_SetSelectionOwner", 
	"X_GetSelectionOwner", 
	"X_ConvertSelection", 
	"X_SendEvent", 
	"X_GrabPointer", 
	"X_UngrabPointer", 
	"X_GrabButton", 
	"X_UngrabButton", 
/*  30 */"X_ChangeActivePointerGrab",
	"X_GrabKeyboard", 
	"X_UngrabKeyboard", 
	"X_GrabKey", 
	"X_UngrabKey", 
	"X_AllowEvents", 
	"X_GrabServer", 
	"X_UngrabServer", 
	"X_QueryPointer", 
	"X_GetMotionEvents", 
	/*  40 */"X_TranslateCoords",
	"X_WarpPointer", 
	"X_SetInputFocus", 
	"X_GetInputFocus", 
	"X_QueryKeymap", 
	"X_OpenFont", 
	"X_CloseFont", 
	"X_QueryFont", 
	"X_QueryTextExtents", 
	"X_ListFonts", 
	/*  50 */"X_ListFontsWithInfo",
	"X_SetFontPath", 
	"X_GetFontPath", 
	"X_CreatePixmap", 
	"X_FreePixmap", 
	"X_CreateGC", 
	"X_ChangeGC", 
	"X_CopyGC", 
	"X_SetDashes", 
	"X_SetClipRectangles", 
	/*  60 */"X_FreeGC",
	"X_ClearArea", 
	"X_CopyArea", 
	"X_CopyPlane", 
	"X_PolyPoint", 
	"X_PolyLine", 
	"X_PolySegment", 
	"X_PolyRectangle", 
	"X_PolyArc", 
	"X_FillPoly", 
	/*  70 */"X_PolyFillRectangle",
	"X_PolyFillArc", 
	"X_PutImage", 
	"X_GetImage", 
	"X_PolyText8", 
	"X_PolyText16", 
	"X_ImageText8", 
	"X_ImageText16", 
	"X_CreateColormap", 
	"X_FreeColormap", 
	/*  80 */"X_CopyColormapAndFree",
	"X_InstallColormap", 
	"X_UninstallColormap", 
	"X_ListInstalledColormaps", 
	"X_AllocColor", 
	"X_AllocNamedColor", 
	"X_AllocColorCells", 
	"X_AllocColorPlanes", 
	"X_FreeColors", 
	"X_StoreColors", 
	/*  90 */"X_StoreNamedColor",
	"X_QueryColors", 
	"X_LookupColor", 
	"X_CreateCursor", 
	"X_CreateGlyphCursor", 
	"X_FreeCursor", 
	"X_RecolorCursor", 
	"X_QueryBestSize", 
	"X_QueryExtension", 
	"X_ListExtensions", 
	/* 100 */"X_ChangeKeyboardMapping",
	"X_GetKeyboardMapping", 
	"X_ChangeKeyboardControl", 
	"X_GetKeyboardControl", 
	"X_Bell", 
	"X_ChangePointerControl", 
	"X_GetPointerControl", 
	"X_SetScreenSaver", 
	"X_GetScreenSaver", 
	"X_ChangeHosts", 
	/*  110 */"X_ListHosts",
	"X_SetAccessControl", 
	"X_SetCloseDownMode", 
	"X_KillClient", 
	"X_RotateProperties", 
	"X_ForceScreenSaver", 
	"X_SetPointerMapping", 
	"X_GetPointerMapping", 
	"X_SetModifierMapping", 
	"X_GetModifierMapping",
	/*  20 */"??",
	"??",
	"??",
	"??",
	"??",
	"??",
	"??",
	"X_NoOperation" 
};

static char const * const xcore_event_name[] = {
	"CoreError",
	"(null)",
	"KeyPress",
	"KeyRelease",
	"ButtonPress",
	"ButtonRelease",
	"MotionNotify",
	"EnterNotify",
	"LeaveNotify",
	"FocusIn",
	"FocusOut",
	"KeymapNotify",
	"Expose",
	"GraphicsExpose",
	"NoExpose",
	"VisibilityNotify",
	"CreateNotify",
	"DestroyNotify",
	"UnmapNotify",
	"MapNotify",
	"MapRequest",
	"ReparentNotify",
	"ConfigureNotify",
	"ConfigureRequest",
	"GravityNotify",
	"ResizeRequest",
	"CirculateNotify",
	"CirculateRequest",
	"PropertyNotify",
	"SelectionClear",
	"SelectionRequest",
	"SelectionNotify",
	"ColormapNotify",
	"ClientMessage",
	"MappingNotify",
	"GenericEvent"
};

static char const * const xcore_minor_error_name[] = {
/*  0 */		"NULL_ERROR",
/*  1 */		"XCB_REQUEST",
/*  2 */		"XCB_VALUE",
/*  3 */  		"XCB_WINDOW",
/*  4 */		"XCB_PIXMAP",
/*  5 */		"XCB_ATOM",
/*  6 */		"XCB_CURSOR",
/*  7 */		"XCB_FONT",
/*  8 */		"XCB_MATCH",
/*  9 */		"XCB_DRAWABLE",
/* 10 */		"XCB_ACCESS",
/* 11 */		"XCB_ALLOC",
/* 12 */		"XCB_COLORMAP",
/* 13 */		"XCB_G_CONTEXT",
/* 14 */		"XCB_ID_CHOICE",
/* 15 */		"XCB_NAME",
/* 16 */		"XCB_LENGTH",
/* 17 */		"XCB_IMPLEMENTATION"
};

static char const * const xfixes_request_name[] = {
/*  0 */		"XCB_XFIXES_QUERY_VERSION",
/*  1 */		"XCB_XFIXES_CHANGE_SAVE_SET",
/*  2 */		"XCB_XFIXES_SELECT_SELECTION_INPUT",
/*  3 */		"XCB_XFIXES_SELECT_CURSOR_INPUT",
/*  4 */		"XCB_XFIXES_GET_CURSOR_IMAGE",
/*  5 */		"XCB_XFIXES_CREATE_REGION",
/*  6 */		"XCB_XFIXES_CREATE_REGION_FROM_BITMAP",
/*  7 */		"XCB_XFIXES_CREATE_REGION_FROM_WINDOW",
/*  8 */		"XCB_XFIXES_CREATE_REGION_FROM_GC",
/*  9 */		"XCB_XFIXES_CREATE_REGION_FROM_PICTURE",
/* 10 */		"XCB_XFIXES_DESTROY_REGION",
/* 11 */		"XCB_XFIXES_SET_REGION",
/* 12 */		"XCB_XFIXES_COPY_REGION",
/* 13 */		"XCB_XFIXES_UNION_REGION",
/* 14 */		"XCB_XFIXES_INTERSECT_REGION",
/* 15 */		"XCB_XFIXES_SUBTRACT_REGION",
/* 16 */		"XCB_XFIXES_INVERT_REGION",
/* 17 */		"XCB_XFIXES_TRANSLATE_REGION",
/* 18 */		"XCB_XFIXES_REGION_EXTENTS",
/* 19 */		"XCB_XFIXES_FETCH_REGION",
/* 20 */		"XCB_XFIXES_SET_GC_CLIP_REGION",
/* 21 */		"XCB_XFIXES_SET_WINDOW_SHAPE_REGION",
/* 22 */		"XCB_XFIXES_SET_PICTURE_CLIP_REGION",
/* 23 */		"XCB_XFIXES_SET_CURSOR_NAME",
/* 24 */		"XCB_XFIXES_GET_CURSOR_NAME",
/* 25 */		"XCB_XFIXES_GET_CURSOR_IMAGE_AND_NAME",
/* 26 */		"XCB_XFIXES_CHANGE_CURSOR",
/* 27 */		"XCB_XFIXES_CHANGE_CURSOR_BY_NAME",
/* 28 */		"XCB_XFIXES_EXPAND_REGION",
/* 29 */		"XCB_XFIXES_HIDE_CURSOR",
/* 30 */		"XCB_XFIXES_SHOW_CURSOR",
/* 31 */		"XCB_XFIXES_CREATE_POINTER_BARRIER",
/* 32 */		"XCB_XFIXES_DELETE_POINTER_BARRIER"

};

static char const * const xfixes_event_name[] = {
		"XCB_XFIXES_SELECTION_NOTIFY",
		"XCB_XFIXES_CURSOR_NOTIFY"
};

static char const * const xfixes_error_name[] = {
		"XCB_XFIXES_BAD_REGION"
};

static char const * const xshape_request_name[] = {
		"XCB_SHAPE_QUERY_VERSION",
		"XCB_SHAPE_RECTANGLES",
		"XCB_SHAPE_MASK",
		"XCB_SHAPE_COMBINE",
		"XCB_SHAPE_OFFSET",
		"XCB_SHAPE_QUERY_EXTENTS",
		"XCB_SHAPE_SELECT_INPUT",
		"XCB_SHAPE_INPUT_SELECTED",
		"XCB_SHAPE_GET_RECTANGLES",
};

static char const * const xshape_event_name[] = {
		"XCB_SHAPE_NOTIFY"
};

static char const * const xshape_error_name[] = {

};

static char const * const xrandr_request_name[] = {
		"XCB_RANDR_QUERY_VERSION",
		"XRANDR ??"
		"XCB_RANDR_SET_SCREEN_CONFIG",
		"XRANDR ??",
		"XCB_RANDR_SELECT_INPUT",
		"XCB_RANDR_GET_SCREEN_INFO",
		"XCB_RANDR_GET_SCREEN_SIZE_RANGE",
		"XCB_RANDR_SET_SCREEN_SIZE",
		"XCB_RANDR_GET_SCREEN_RESOURCES",
		"XCB_RANDR_GET_OUTPUT_INFO",
		"XCB_RANDR_LIST_OUTPUT_PROPERTIES",
		"XCB_RANDR_QUERY_OUTPUT_PROPERTY",
		"XCB_RANDR_CONFIGURE_OUTPUT_PROPERTY",
		"XCB_RANDR_CHANGE_OUTPUT_PROPERTY",
		"XCB_RANDR_DELETE_OUTPUT_PROPERTY",
		"XCB_RANDR_GET_OUTPUT_PROPERTY",
		"XCB_RANDR_CREATE_MODE",
		"XCB_RANDR_DESTROY_MODE",
		"XCB_RANDR_ADD_OUTPUT_MODE",
		"XCB_RANDR_DELETE_OUTPUT_MODE",
		"XCB_RANDR_GET_CRTC_INFO",
		"XCB_RANDR_SET_CRTC_CONFIG",
		"XCB_RANDR_GET_CRTC_GAMMA_SIZE",
		"XCB_RANDR_GET_CRTC_GAMMA",
		"XCB_RANDR_SET_CRTC_GAMMA",
		"XCB_RANDR_GET_SCREEN_RESOURCES_CURRENT",
		"XCB_RANDR_SET_CRTC_TRANSFORM",
		"XCB_RANDR_GET_CRTC_TRANSFORM",
		"XCB_RANDR_GET_PANNING",
		"XCB_RANDR_SET_PANNING",
		"XCB_RANDR_SET_OUTPUT_PRIMARY",
		"XCB_RANDR_GET_OUTPUT_PRIMARY",
		"XCB_RANDR_GET_PROVIDERS",
		"XCB_RANDR_GET_PROVIDER_INFO",
		"XCB_RANDR_SET_PROVIDER_OFFLOAD_SINK",
		"XCB_RANDR_SET_PROVIDER_OUTPUT_SOURCE",
		"XCB_RANDR_LIST_PROVIDER_PROPERTIES",
		"XCB_RANDR_QUERY_PROVIDER_PROPERTY",
		"XCB_RANDR_CONFIGURE_PROVIDER_PROPERTY",
		"XCB_RANDR_CHANGE_PROVIDER_PROPERTY",
		"XCB_RANDR_DELETE_PROVIDER_PROPERTY",
		"XCB_RANDR_GET_PROVIDER_PROPERTY"

};

static char const * const xrandr_event_name[] = {
		"XCB_RANDR_SCREEN_CHANGE_NOTIFY"
};

static char const * const xrandr_error_name[] = {
"XCB_RANDR_BAD_OUTPUT",
"XCB_RANDR_BAD_CRTC",
"XCB_RANDR_BAD_MODE",
"XCB_RANDR_BAD_PROVIDER"
};

static char const * const xdamage_request_name[] = {
	"X_DamageQueryVersion",
	"X_DamageCreate",
	"X_DamageDestroy",
	"X_DamageSubtract",
	"X_DamageAdd"
};

static char const * const xdamage_event_name[] = {
	"DAMAGE_NOTIFY"
};

static char const * const xdamage_error_name[] = {
	"XCB_DAMAGE_BAD_DAMAGE"
};

static char const * const xcomposite_request_name[] = {
	"X_CompositeQueryVersion",
	"X_CompositeRedirectWindow",
	"X_CompositeRedirectSubwindows",
	"X_CompositeUnredirectWindow",
	"X_CompositeUnredirectSubwindows",
	"X_CompositeCreateRegionFromBorderClip",
	"X_CompositeNameWindowPixmap",
	"X_CompositeGetOverlayWindow",
	"X_CompositeReleaseOverlayWindow"
};

static char const * const xcomposite_event_name[] = {

};

static char const * const xcomposite_error_name[] = {

};

}


#endif /* X11_FUNC_NAME_HXX_ */
