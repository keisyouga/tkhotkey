/* tk hotkey for x11 */

#include <tcl.h>
#include <tk.h>
#include <stdio.h>

#define NS "hotkey"

/* dict of hotkey/script pairs */
static Tcl_Obj *hotkeyInfo;

/* eval script if hotkey, otherwise through the event */
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

/* register hotkey */
static int Hotkey_RegisterCmd(ClientData clientData,
                      Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
	/* fprintf(stderr, "hotkey_Cmd: called with %d arguments\n", objc); */

	Tk_Window tkwin = Tk_MainWindow(interp);
	if (!tkwin) {
		fprintf(stderr, "Tk_MainWindow() return null\n");
		return TCL_ERROR;
	}
	Display *dpy = Tk_Display(tkwin);
	if (!dpy) {
		fprintf(stderr, "Tk_Display() return null\n");
		return TCL_ERROR;
	}
	Window root = DefaultRootWindow(dpy);
	Window winid = Tk_WindowId(tkwin);
	if (!winid) {
		fprintf(stderr, "Tk_WindowId() return null\n");
		return TCL_ERROR;
	}

	int ret;
	unsigned int keycode;
	unsigned int modifiers;
	Tcl_GetIntFromObj(interp, objv[1], (int *)&keycode);
	Tcl_GetIntFromObj(interp, objv[2], (int *)&modifiers);
	XGrabKey(dpy, keycode, modifiers, root, True, GrabModeAsync, GrabModeAsync);

	Tcl_Obj *key = Tcl_ObjPrintf("%d+%d", keycode, modifiers);
	ret = Tcl_DictObjPut(interp, hotkeyInfo, key, objv[3]);
	if (ret == TCL_ERROR) {
		return TCL_ERROR;
	}
	int size;
	Tcl_DictObjSize(interp, hotkeyInfo, &size);

	/* create the handler only first time */
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

	Tk_Window tkwin = Tk_MainWindow(interp);
	if (!tkwin) {
		fprintf(stderr, "Tk_MainWindow() return null\n");
		return TCL_ERROR;
	}
	Display *dpy = Tk_Display(tkwin);
	if (!dpy) {
		fprintf(stderr, "Tk_Display() return null\n");
		return TCL_ERROR;
	}
	Window root = DefaultRootWindow(dpy);
	Window winid = Tk_WindowId(tkwin);
	if (!winid) {
		fprintf(stderr, "Tk_WindowId() return null\n");
		return TCL_ERROR;
	}

	int ret;
	unsigned int keycode;
	unsigned int modifiers;
	Tcl_GetIntFromObj(interp, objv[1], (int *)&keycode);
	Tcl_GetIntFromObj(interp, objv[2], (int *)&modifiers);
	XUngrabKey(dpy, keycode, modifiers, root);

	/* no error even if key did not exist */
	Tcl_Obj *key = Tcl_ObjPrintf("%d+%d", keycode, modifiers);
	ret = Tcl_DictObjRemove(interp, hotkeyInfo, key);
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
