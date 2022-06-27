/* tk hotkey for x11 */

#include <tcl.h>
#include <tk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
static int SetXGrabKeyInfo(Tcl_Interp *interp, int keycode, int modifiers,
                           struct XGrabkeyInfo *info)
{
	Tk_Window tkwin = Tk_MainWindow(interp);
	if (!tkwin) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj("Tk_MainWindow() return null", -1));
		return TCL_ERROR;
	}
	Display *dpy = Tk_Display(tkwin);
	if (!dpy) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj("Tk_Display() return null", -1));
		return TCL_ERROR;
	}
	Window root = DefaultRootWindow(dpy);

	int min_keycode;
	int max_keycode;
	XDisplayKeycodes(dpy, &min_keycode, &max_keycode);
	if ((keycode < min_keycode) || (keycode > max_keycode)) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj("keycode out of range", -1));
		return TCL_ERROR;
	}
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

/* register hotkey
   append to hotkeyInfo
 */
static int appendHotkey(Tcl_Interp *interp, const struct XGrabkeyInfo *info,
                        Tcl_Obj *script)
{
	XGrabKey(info->display, info->keycode, info->modifiers, info->grab_window,
	         True, GrabModeAsync, GrabModeAsync);

	Tcl_Obj *key = Tcl_ObjPrintf("%d+%d", info->keycode, info->modifiers);
	int ret = Tcl_DictObjPut(interp, hotkeyInfo, key, script);
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

/* unregister hotkey
   remove from hotkeyInfo
 */
static int removeHotkey(Tcl_Interp *interp, const struct XGrabkeyInfo *info)
{
	XUngrabKey(info->display, info->keycode, info->modifiers, info->grab_window);

	/* no error even if key did not exist */
	Tcl_Obj *key = Tcl_ObjPrintf("%d+%d", info->keycode, info->modifiers);
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

/* register hotkey with keycode, modifiers and script */
static int Hotkey_RegisterCmd(ClientData clientData,
                              Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
	/* fprintf(stderr, "hotkey_Cmd: called with %d arguments\n", objc); */

	if (objc != 4) {
		Tcl_WrongNumArgs(interp, 1, objv, "keycode modifiers script");
		return TCL_ERROR;
	}

	int keycode;
	unsigned int modifiers;
	struct XGrabkeyInfo info;
	Tcl_GetIntFromObj(interp, objv[1], (int *)&keycode);
	Tcl_GetIntFromObj(interp, objv[2], (int *)&modifiers);
	if (SetXGrabKeyInfo(interp, keycode, modifiers, &info) == TCL_ERROR) {
		return TCL_ERROR;
	}

	if (appendHotkey(interp, &info, objv[3]) == TCL_ERROR) {
		return TCL_ERROR;
	}

	return TCL_OK;
}

/* unregister hotkey with keycode and modifiers */
static int Hotkey_UnregisterCmd(ClientData clientData,
                                Tcl_Interp *interp, int objc,
                                Tcl_Obj *const objv[])
{
	/* fprintf(stderr, "unhotkey_Cmd: called with %d arguments\n", objc); */

	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "keycode modifiers");
		return TCL_ERROR;
	}

	int keycode;
	unsigned int modifiers;
	struct XGrabkeyInfo info;
	Tcl_GetIntFromObj(interp, objv[1], (int *)&keycode);
	Tcl_GetIntFromObj(interp, objv[2], (int *)&modifiers);
	if (SetXGrabKeyInfo(interp, keycode, modifiers, &info) == TCL_ERROR) {
		return TCL_ERROR;
	}

	if (removeHotkey(interp, &info) == TCL_ERROR) {
		return TCL_ERROR;
	}

	return TCL_OK;
}

/* return value of modifier for XGrabkey()
   Shift (1<<0)
   Control (1<<2)
   Alt (1<<3)
   win (1<<6)
*/
static int getModValue(const char *modstr)
{
	if ((strcasecmp(modstr, "shift") == 0) || (strcmp(modstr, "S") == 0)) {
		return 1;
	} else if ((strcasecmp(modstr, "control") == 0) || (strcasecmp(modstr, "ctrl") == 0) || (strcmp(modstr, "C") == 0)) {
		return 1 << 2;
	} else if ((strcasecmp(modstr, "alt") == 0) || (strcmp(modstr, "A") == 0) ||
	           (strcasecmp(modstr, "meta") == 0) || (strcmp(modstr, "M") == 0) ||
	           (strcasecmp(modstr, "mod1") == 0)) {
		return 1 << 3;
	} else if ((strcasecmp(modstr, "win") == 0) || (strcmp(modstr, "W") == 0) ||
	           (strcasecmp(modstr, "super") == 0) || (strcmp(modstr, "s") == 0) ||
	           (strcasecmp(modstr, "mod4") == 0)) {
		return 1 << 6;
	}
	return 0;
}

/* get keycode and modifier value from string */
static int keycodeFromKeystr(Tcl_Interp *interp, const char *keystr, int *keycode, unsigned int *modifiers)
{
	char *s = strdup(keystr);
	char *p;
	char *p2;

	/* get modifiers */
	p = s;
	*modifiers = 0;
	while ((p2 = (strchr(p, '-'))) != NULL) {
		*p2 = '\0';
		*modifiers |= getModValue(p);
		p = p2 + 1;
	}

	/* get keycode */
	if (!*p) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj("can not get keysym", -1));
		free(s);
		return TCL_ERROR;
	}
	Tk_Window tkwin = Tk_MainWindow(interp);
	if (!tkwin) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj("Tk_MainWindow() return null", -1));
		free(s);
		return TCL_ERROR;
	}
	Display *dpy = Tk_Display(tkwin);
	if (!dpy) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj("Tk_Display() return null", -1));
		free(s);
		return TCL_ERROR;
	}
	*keycode = XKeysymToKeycode(dpy, XStringToKeysym(p));

	free(s);
	return TCL_OK;
}

/* register hotkey with keystr and script */
static int Hotkey_RegisterCmd2(ClientData clientData,
                               Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
	/* fprintf(stderr, "Hotkey_RegisterCmd2: called with %d arguments\n", objc); */

	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "key script");
		return TCL_ERROR;
	}
	int keycode;
	unsigned int modifiers;
	if (keycodeFromKeystr(interp, Tcl_GetString(objv[1]), &keycode, &modifiers) == TCL_ERROR) {
		return TCL_ERROR;
	}
	struct  XGrabkeyInfo info;
	SetXGrabKeyInfo(interp, keycode, modifiers, &info);
	if (SetXGrabKeyInfo(interp, keycode, modifiers, &info) == TCL_ERROR) {
		return TCL_ERROR;
	}

	if (appendHotkey(interp, &info, objv[2]) == TCL_ERROR) {
		return TCL_ERROR;
	}

	return TCL_OK;
}

/* unregister hotkey with keystr */
static int Hotkey_UnregisterCmd2(ClientData clientData,
                                 Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
	/* fprintf(stderr, "Hotkey_UnregisterCmd2: called with %d arguments\n", objc); */

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "key");
		return TCL_ERROR;
	}

	int keycode;
	unsigned int modifiers;
	if (keycodeFromKeystr(interp, Tcl_GetString(objv[1]), &keycode, &modifiers) == TCL_ERROR) {
		return TCL_ERROR;
	}
	struct  XGrabkeyInfo info;
	SetXGrabKeyInfo(interp, keycode, modifiers, &info);
	if (SetXGrabKeyInfo(interp, keycode, modifiers, &info) == TCL_ERROR) {
		return TCL_ERROR;
	}

	if (removeHotkey(interp, &info) == TCL_ERROR) {
		return TCL_ERROR;
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

	if (Tcl_PkgProvide(interp, "hotkey", "1.1") == TCL_ERROR) {
		return TCL_ERROR;
	}

	Tcl_CreateObjCommand(interp, NS "::register", Hotkey_RegisterCmd, NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::unregister", Hotkey_UnregisterCmd, NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::register2", Hotkey_RegisterCmd2, NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::unregister2", Hotkey_UnregisterCmd2, NULL, NULL);

	/* store hotkey info as dict */
	hotkeyInfo = Tcl_NewDictObj();
	return TCL_OK;
}

