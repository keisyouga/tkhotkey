/* tk hotkey for x11 */

#include <tcl.h>
#include <tk.h>
#include <stdio.h>

#define NS "hotkey"

/* dict of hotkey/script pairs */
static Tcl_Obj *hotkeyInfo;

/* pass this to XGrabKey(), XUngrabKey() */
struct XGrabkeyInfo {
	Display *display;
	int keycode;
	unsigned int modifiers;
	Window grab_window;
};

/* eval script if pressed key is hotkey, otherwise through the event */
static int genericProc(ClientData clientData, XEvent *eventPtr)
{
	Tcl_Interp *interp = clientData;
	if ((eventPtr->type == KeyPress)) {
		Tcl_Obj *key = Tcl_ObjPrintf("%d+%d",
		                             eventPtr->xkey.keycode,
		                             eventPtr->xkey.state);
		Tcl_Obj *value;
		Tcl_DictObjGet(interp, hotkeyInfo, key, &value);
		if (value) {
			Tcl_EvalObjEx(interp, value, 0);
			return 1;
		}
	}

	return 0;
}

/* set XGrabkeyInfo
   return TCL_ERROR if fails
 */
static int SetXGrabKeyInfo(Tcl_Interp *interp, Tcl_Obj *key, Tcl_Obj *mod,
                           struct XGrabkeyInfo *info)
{
	Tk_Window tkwin = Tk_MainWindow(interp);
	if (!tkwin) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj("Tk_MainWindow() return null\n", -1));
		return TCL_ERROR;
	}
	Display *dpy = Tk_Display(tkwin);
	if (!dpy) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj("Tk_Display() return null\n", -1));
		return TCL_ERROR;
	}
	Window root = DefaultRootWindow(dpy);
	Window winid = Tk_WindowId(tkwin);
	if (!winid) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj("Tk_WindowId() return null\n", -1));
		return TCL_ERROR;
	}

	int keycode;
	Tcl_GetIntFromObj(interp, key, (int *)&keycode);
	int min_keycode;
	int max_keycode;
	XDisplayKeycodes(dpy, &min_keycode, &max_keycode);
	if ((keycode < min_keycode) || (keycode > max_keycode)) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj("keycode out of range", -1));
		return TCL_ERROR;
	}

	unsigned int modifiers;
	Tcl_GetIntFromObj(interp, mod, (int *)&modifiers);
	if (modifiers > AnyModifier) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj("modifiers out of range", -1));
		return TCL_ERROR;
	}

	info->display = dpy;
	info->keycode = keycode;
	info->modifiers = modifiers;
	info->grab_window = root;

	return TCL_OK;
}

/* register hotkey */
static int Hotkey_RegisterCmd(ClientData clientData,
                              Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
	/* fprintf(stderr, "hotkey_Cmd: called with %d arguments\n", objc); */

	if (objc != 4) {
		Tcl_WrongNumArgs(interp, 1, objv, "keycode modifiers script");
		return TCL_ERROR;
	}

	struct XGrabkeyInfo info;
	if (SetXGrabKeyInfo(interp, objv[1], objv[2], &info) == TCL_ERROR) {
		return TCL_ERROR;
	}

	XGrabKey(info.display, info.keycode, info.modifiers, info.grab_window,
	         True, GrabModeAsync, GrabModeAsync);

	Tcl_Obj *key = Tcl_ObjPrintf("%d+%d", info.keycode, info.modifiers);
	int ret = Tcl_DictObjPut(interp, hotkeyInfo, key, objv[3]);
	if (ret == TCL_ERROR) {
		return TCL_ERROR;
	}

	/* create the handler only first time */
	int size;
	Tcl_DictObjSize(interp, hotkeyInfo, &size);
	if (size == 1) {
		Tk_CreateGenericHandler(genericProc, interp);
	}
	return TCL_OK;
}

/* unregister hotkey */
static int Hotkey_UnregisterCmd(ClientData clientData,
                                Tcl_Interp *interp, int objc,
                                Tcl_Obj *const objv[])
{
	/* fprintf(stderr, "unhotkey_Cmd: called with %d arguments\n", objc); */

	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "keycode modifiers");
		return TCL_ERROR;
	}

	struct XGrabkeyInfo info;
	if (SetXGrabKeyInfo(interp, objv[1], objv[2], &info) == TCL_ERROR) {
		return TCL_ERROR;
	}

	XUngrabKey(info.display, info.keycode, info.modifiers, info.grab_window);

	/* no error even if key did not exist */
	Tcl_Obj *key = Tcl_ObjPrintf("%d+%d", info.keycode, info.modifiers);
	int ret = Tcl_DictObjRemove(interp, hotkeyInfo, key);
	if (ret == TCL_ERROR) {
		return TCL_ERROR;
	}

	/* delete the handler when none of the hotkey is present */
	int size;
	Tcl_DictObjSize(interp, hotkeyInfo, &size);
	if (size == 0) {
		Tk_DeleteGenericHandler(genericProc, interp);
	}

	return TCL_OK;
}

/* initialize extension */
int Hotkey_Init(Tcl_Interp *interp)
{
	/* fprintf(stderr, "Hotkey_Init\n"); */
	if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
		return TCL_ERROR;
	}
	if (Tk_InitStubs(interp, TK_VERSION, 0) == NULL) {
		return TCL_ERROR;
	}

	if (Tcl_PkgProvide(interp, "hotkey", "0.1") == TCL_ERROR) {
		return TCL_ERROR;
	}

	Tcl_CreateObjCommand(interp, NS "::register", Hotkey_RegisterCmd, NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::unregister", Hotkey_UnregisterCmd, NULL, NULL);

	/* store hotkey info as dict */
	hotkeyInfo = Tcl_NewDictObj();
	return TCL_OK;
}
