#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/xpm.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <math.h>
#include <pthread.h>
#include "decorations/bottom-active.xpm"
#include "decorations/bottom-inactive.xpm"
#include "decorations/bottom-right-active.xpm"
#include "decorations/bottom-left-active.xpm"
#include "decorations/right-active.xpm"
#include "decorations/right-inactive.xpm"
#include "decorations/left-active.xpm"
#include "decorations/left-inactive.xpm"
#include "decorations/hide-active.xpm"
#include "decorations/hide-inactive.xpm"
#include "decorations/hide-prelight.xpm"
#include "decorations/close-active.xpm"
#include "decorations/close-inactive.xpm"
#include "decorations/close-prelight.xpm"
#include "decorations/maximize-active.xpm"
#include "decorations/maximize-inactive.xpm"
#include "decorations/maximize-prelight.xpm"
#include "decorations/title-1-active.xpm"
#include "decorations/title-1-inactive.xpm"
#include "decorations/top-left-active.xpm"
#include "decorations/top-right-active.xpm"
#include "decorations/top-left-inactive.xpm"
#include "decorations/top-right-inactive.xpm"

#define nWorkspaces 5
int topMargin = 0; // TODO test
int leftMargin = 0; // TODO test
int rightMargin = 0; // TODO test
int bottomMargin = 30;

typedef struct _Window {
    Window window;
    Window parentDec;
    Window hideButton;
    Window maxButton;
    Window closeButton;
    Window left;
    Window right;
    XWindowAttributes attribs;
} _Window;

void HandleExpose(XEvent *event);
void HandleMapRequest(XEvent *event);
void HandleButtonPress(XEvent *event);
void HandleButtonRelease(XEvent *event);
void HandleMotionNotify(XEvent *event);
void HandleEnterWindow(XEvent *event);
void HandleLeaveWindow(XEvent *event);
void HandleUnmapNotify(XEvent *event);
void HandlePropertyChange(XEvent *event);
void HandleConfigureRequest(XEvent *event);
void CreateWindow(Window window, Window parentDec); // TODO change to one param, only one window
void SendToWorkspace(int workspace);
void GoToWorkspace(int workspace);
void CreateDecorations(void);
void MaximizeWindow(int id);
void CloseWindow(int id);
void HideWindow(int id);
void MoveParentDec(int id);
void CenterWindow(int id);
void HideDecorations(void);
void ReDecorate(void);
void Decorate(int id);
void SetID(Window window, int id);
int GetID(Window window);
int NextID(void);
void Init(void);
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

Bool running = True;
Bool closing = False;
Bool decorations = True;

int currentWorkspace = 0;
int windowIDs[nWorkspaces][999];
_Window *workspaces[nWorkspaces][999]; // TODO malloc

GC gc;
Window root;
Display *display;
XButtonEvent buttonClick;
int screen, screenWidth, screenHeight, tempX, tempY, tempHeight, tempWidth, tempMotionX = 0, tempMotionY = 0;

Pixmap buttonsPixmap[23];
XpmAttributes xpmAttribs[23];

char **decorationFiles[] = {bottom_active_xpm,bottom_inactive_xpm,bottom_right_active_xpm,bottom_left_active_xpm,
                            right_active_xpm,left_active_xpm,right_inactive_xpm,left_inactive_xpm,
                            hide_active_xpm,hide_inactive_xpm,hide_prelight_xpm,close_active_xpm,close_inactive_xpm,
                            close_prelight_xpm,maximize_active_xpm,maximize_inactive_xpm,maximize_prelight_xpm,
                            title_1_active_xpm,title_1_inactive_xpm,top_left_active_xpm,top_right_active_xpm,
                            top_left_inactive_xpm,top_right_inactive_xpm};

enum {
    bottom_active,bottom_inactive,bottom_right_active,bottom_left_active,
    right_active,left_active,right_inactive,left_inactive,hide_active,
    hide_inactive,hide_prelight,close_active,close_inactive,close_prelight,
    maximize_active,maximize_inactive,maximize_prelight,title_1_active,title_1_inactive,
    top_left_active,top_right_active,top_left_inactive,top_right_inactive,last
};

void HandleExpose(XEvent *event)
{
    Window exposeWindow = event->xexpose.window;
    int id = GetID(exposeWindow);

    XWindowAttributes windowAttribs;
    XGetWindowAttributes(display, workspaces[currentWorkspace][id]->window, &windowAttribs);

    if (exposeWindow == workspaces[currentWorkspace][id]->closeButton) {
        XCopyArea(display, buttonsPixmap[close_inactive], workspaces[currentWorkspace][id]->closeButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
    else if (exposeWindow == workspaces[currentWorkspace][id]->hideButton) {
        XCopyArea(display, buttonsPixmap[hide_inactive], workspaces[currentWorkspace][id]->hideButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
    else if (exposeWindow == workspaces[currentWorkspace][id]->maxButton) {
        XCopyArea(display, buttonsPixmap[maximize_inactive], workspaces[currentWorkspace][id]->maxButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
    else if (exposeWindow == workspaces[currentWorkspace][id]->left) {
        for (int i=0 ; i<=(int)(windowAttribs.width/xpmAttribs[left_inactive].height); i++) {
            XCopyArea(display, buttonsPixmap[left_inactive], workspaces[currentWorkspace][id]->left, gc, 0, -windowAttribs.height +  xpmAttribs[left_inactive].height*i, -xpmAttribs[left_inactive].width, -xpmAttribs[left_inactive].height, 0, 0);
        }
    }
    else if (exposeWindow == workspaces[currentWorkspace][id]->right) {
        //for (int i=0 ; i<=(int)(windowAttribs.width/xpmAttribs[left_inactive].height); i++) {
            //XCopyArea(display, buttonsPixmap[left_inactive], workspaces[currentWorkspace][id]->left, gc, 0, -windowAttribs.height +  xpmAttribs[left_inactive].height*i, -xpmAttribs[left_inactive].width, -xpmAttribs[left_inactive].height, 0, 0);
        //}
    }
}

void HandleMapRequest(XEvent *event)
{
    Window mapWindow = event->xmaprequest.window; // TODO change all these to pointers

    for (int i=0 ; i<999 ; i++) {
        if (workspaces[currentWorkspace][i]->window == mapWindow) {
            return;
        }
    }

    XMapWindow(display, mapWindow);

    XWindowAttributes windowAttribs;
    XGetWindowAttributes(display, mapWindow, &windowAttribs);
    if (tempHeight || tempWidth) {
        XMoveResizeWindow(display, mapWindow, tempX, tempY, tempWidth, tempHeight);
    }
    else {
        XMoveResizeWindow(display, mapWindow, tempX, tempY, windowAttribs.width, windowAttribs.height);
    }

    unsigned long length, after;
    unsigned char *data = NULL;
    int format;
    Atom atom = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    if (XGetWindowProperty(display,mapWindow,atom,0L,1L,False,AnyPropertyType,&atom,&format,&length,&after,&data) == Success) {
        if (((Atom *)data)[0] == XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False)) {
            return;
        }
    }

    XSelectInput(display, mapWindow, ExposureMask|StructureNotifyMask|EnterWindowMask|LeaveWindowMask);
    XGrabButton(display, AnyButton, Mod1Mask, mapWindow, True, PointerMotionMask, GrabModeAsync, GrabModeAsync, 0, 0);

    Window parentDec = XCreateSimpleWindow(display, root, 0, 0, windowAttribs.width, windowAttribs.height, 0, 0x000000, 0xffffff);
    XMapWindow(display, parentDec);
    XGrabButton(display, AnyButton, AnyModifier, parentDec, True, PointerMotionMask|ButtonPressMask|ButtonReleaseMask,GrabModeAsync, GrabModeAsync, 0, 0);

    int id = NextID();
    SetID(mapWindow, id);
    SetID(parentDec, id);

    CreateWindow(mapWindow, parentDec); // TODO pointers // TODO change name
    if (decorations) {
        Decorate(id);
    }
    CenterWindow(id);

    XSetInputFocus(display, mapWindow, 0, CurrentTime);
}

void HandleButtonPress(XEvent *event)
{
    buttonClick = event->xbutton;
    Window clickWindow = event->xbutton.window;
    int id = GetID(clickWindow);

    if (clickWindow == workspaces[currentWorkspace][id]->closeButton) {
        XCopyArea(display, buttonsPixmap[close_active], workspaces[currentWorkspace][id]->closeButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
    else if (clickWindow == workspaces[currentWorkspace][id]->hideButton) {
        XCopyArea(display, buttonsPixmap[hide_active], workspaces[currentWorkspace][id]->hideButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
    else if (clickWindow == workspaces[currentWorkspace][id]->maxButton) {
        XCopyArea(display, buttonsPixmap[maximize_active], workspaces[currentWorkspace][id]->maxButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
    else if (clickWindow == workspaces[currentWorkspace][id]->window || clickWindow == workspaces[currentWorkspace][id]->parentDec) {
        XRaiseWindow(display, workspaces[currentWorkspace][id]->parentDec);
        XRaiseWindow(display, workspaces[currentWorkspace][id]->window);
        XSetInputFocus(display, workspaces[currentWorkspace][id]->window, 0, CurrentTime);
    }
}

void HandleButtonRelease(XEvent *event)
{
    Window releaseWindow = event->xbutton.subwindow;
    int id = GetID(event->xbutton.window); // to avoid crash when clicking on parentDec instead of a child

    if (releaseWindow == workspaces[currentWorkspace][id]->closeButton) {
        XCopyArea(display, buttonsPixmap[close_inactive], workspaces[currentWorkspace][id]->closeButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
        CloseWindow(id);
    }
    else if (releaseWindow == workspaces[currentWorkspace][id]->hideButton) {
        XCopyArea(display, buttonsPixmap[hide_inactive], workspaces[currentWorkspace][id]->hideButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
        HideWindow(id);
    }
    else if (releaseWindow == workspaces[currentWorkspace][id]->maxButton) {
        XCopyArea(display, buttonsPixmap[maximize_inactive], workspaces[currentWorkspace][id]->maxButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
        MaximizeWindow(id);
    }
}

void HandleMotionNotify(XEvent *event)
{
    int xd = event->xmotion.x - buttonClick.x;
    int yd = event->xmotion.y - buttonClick.y;
    int id = GetID(event->xmotion.window);

    XWindowAttributes windowAttrib;
    XGetWindowAttributes(display, workspaces[currentWorkspace][id]->window, &windowAttrib);

    if (windowAttrib.x + windowAttrib.width + xd + (int)xpmAttribs[right_inactive].width > screenWidth - 15 &&
        windowAttrib.x + windowAttrib.width + xd + (int)xpmAttribs[right_inactive].width <= screenWidth &&
        buttonClick.button == 1) {
        XMoveResizeWindow(display, workspaces[currentWorkspace][id]->window, screenWidth - windowAttrib.width - (int)xpmAttribs[right_inactive].width,
                                   windowAttrib.y,
                                   windowAttrib.width,
                                   windowAttrib.height);
    }
    else if (windowAttrib.x + xd + (int)xpmAttribs[left_inactive].width < 15 &&
             windowAttrib.x + xd + (int)xpmAttribs[left_inactive].width >= 0 &&
             buttonClick.button == 1) {
        XMoveResizeWindow(display, workspaces[currentWorkspace][id]->window, (int)xpmAttribs[right_inactive].width,
                                   windowAttrib.y,
                                   windowAttrib.width,
                                   windowAttrib.height);
    }
    else if (event->xbutton.x_root == screenWidth - 1 && buttonClick.button == 1) {
        XMoveResizeWindow(display, workspaces[currentWorkspace][id]->window, screenWidth / 2,
                                   xpmAttribs[title_1_inactive].height + topMargin,
                                   screenWidth / 2 - (xpmAttribs[right_inactive].width * 2),
                                   screenHeight - xpmAttribs[title_1_inactive].height - xpmAttribs[bottom_inactive].height - topMargin - bottomMargin);
    }
    else if (event->xbutton.x_root == 0 && buttonClick.button == 1) {
        XMoveResizeWindow(display, workspaces[currentWorkspace][id]->window, xpmAttribs[right_inactive].width,
                                   xpmAttribs[title_1_inactive].height + topMargin,
                                   screenWidth / 2 - (xpmAttribs[right_inactive].width * 2),
                                   screenHeight - xpmAttribs[title_1_inactive].height - xpmAttribs[bottom_inactive].height - topMargin - bottomMargin);
    }
    else if (event->xbutton.y_root == 0 && buttonClick.button == 1) {
        XMoveResizeWindow(display, workspaces[currentWorkspace][id]->window, xpmAttribs[right_inactive].width,
                                   xpmAttribs[title_1_inactive].height + topMargin,
                                   screenWidth - (xpmAttribs[right_inactive].width * 2),
                                   (screenHeight - topMargin - bottomMargin) / 2);
    }
    else if (event->xbutton.y_root == (screenHeight - 1) && buttonClick.button == 1) {
        XMoveResizeWindow(display, workspaces[currentWorkspace][id]->window, 0,
                                   (screenHeight - topMargin - bottomMargin) / 2,
                                   screenWidth - (xpmAttribs[right_inactive].width * 2),
                                   (screenHeight  - topMargin - bottomMargin)/ 2);
    }
    else if (workspaces[currentWorkspace][id]->attribs.width || workspaces[currentWorkspace][id]->attribs.height) { // TODO improve
        XMoveResizeWindow(display, workspaces[currentWorkspace][id]->window, windowAttrib.x + (buttonClick.button == 1 ? xd : 0),
                                                windowAttrib.y + (buttonClick.button == 1 ? yd : 0),
                                                workspaces[currentWorkspace][id]->attribs.width,
                                                workspaces[currentWorkspace][id]->attribs.height);
        workspaces[currentWorkspace][id]->attribs.width = 0;
        workspaces[currentWorkspace][id]->attribs.height = 0;
    }
    else {
        XMoveResizeWindow(display, workspaces[currentWorkspace][id]->window, windowAttrib.x + (buttonClick.button == 1 ? xd : 0),
                                                windowAttrib.y + (buttonClick.button == 1 ? yd : 0),
                                                MAX(windowAttrib.width + (buttonClick.button == 3 ? event->xmotion.x - tempMotionX : 0), 30),
                                                MAX(windowAttrib.height + (buttonClick.button == 3 ? event->xmotion.y - tempMotionY : 0), 30));
    }
    tempMotionX = event->xmotion.x;
    tempMotionY = event->xmotion.y;

    if (decorations) {
        MoveParentDec(id);
    }
}

void HandleEnterWindow(XEvent *event)
{
    Window enterWindow = event->xcrossing.window;
    int id = GetID(enterWindow);

    if (enterWindow == workspaces[currentWorkspace][id]->closeButton) {
        XCopyArea(display, buttonsPixmap[close_prelight], workspaces[currentWorkspace][id]->closeButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
    else if (enterWindow == workspaces[currentWorkspace][id]->hideButton) {
        XCopyArea(display, buttonsPixmap[hide_prelight], workspaces[currentWorkspace][id]->hideButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
    else if (enterWindow == workspaces[currentWorkspace][id]->maxButton) {
        XCopyArea(display, buttonsPixmap[maximize_prelight], workspaces[currentWorkspace][id]->maxButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
    else {
        XSetInputFocus(display, workspaces[currentWorkspace][id]->window, 0, CurrentTime);
    }
}

void HandleLeaveWindow(XEvent *event)
{
    if (closing) {
        closing = False;
        return;
    }
    Window leaveWindow = event->xcrossing.window;
    int id = GetID(leaveWindow);

    if (leaveWindow == workspaces[currentWorkspace][id]->closeButton) {
        XCopyArea(display, buttonsPixmap[close_inactive], workspaces[currentWorkspace][id]->closeButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
    else if (leaveWindow == workspaces[currentWorkspace][id]->hideButton) {
        XCopyArea(display, buttonsPixmap[hide_inactive], workspaces[currentWorkspace][id]->hideButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
    else if (leaveWindow == workspaces[currentWorkspace][id]->maxButton) {
        XCopyArea(display, buttonsPixmap[maximize_inactive], workspaces[currentWorkspace][id]->maxButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
}

void HandleUnmapNotify(XEvent *event)
{
    Window unmapWindow = event->xunmap.window;
    closing = True;
    for (int i=0; i<999; i++) {
        if (workspaces[currentWorkspace][i]->window == unmapWindow) {
            XDestroyWindow(display, workspaces[currentWorkspace][i]->parentDec);
            XSetInputFocus(display, root, 0, CurrentTime);
            workspaces[currentWorkspace][i]->window = 0;
            windowIDs[currentWorkspace][i] = 0;
            break;
        }
    }
}

void HandlePropertyChange(XEvent *event)
{
    Atom atom = event->xproperty.atom;
    Atom workspaceAtom = XInternAtom(display, "workspace", False);
    Atom decoAtom = XInternAtom(display, "decorations", False);
    Atom sendToWorkspaceAtom = XInternAtom(display, "sendToWorkspace", False);
    if (atom == workspaceAtom) {
        XTextProperty text;
        XGetTextProperty(display, root, &text, workspaceAtom);
        char *temp;
        closing = True;
        GoToWorkspace(strtol((const char *)text.value, &temp, 10));
    }
    else if (atom == sendToWorkspaceAtom) {
        XTextProperty text;
        XGetTextProperty(display, root, &text, sendToWorkspaceAtom);
        char *temp;
        closing = True;
        SendToWorkspace(strtol((const char *)text.value, &temp, 10));
    }
    else if (atom == decoAtom) {
        XTextProperty text;
        XGetTextProperty(display, root, &text, decoAtom);
        char *temp;
        if (!strtol((const char *)text.value, &temp, 10)) {
            HideDecorations();
            decorations = False;
        }
        else {
            ReDecorate();
            decorations = True;
        }
    }
}

void HandleConfigureRequest(XEvent *event)
{
    if (event->xconfigurerequest.x) {
        tempX = event->xconfigurerequest.x;
    }
    if (event->xconfigurerequest.y) {
        tempY = event->xconfigurerequest.y;
    }
    if (event->xconfigurerequest.width) {
        tempWidth = event->xconfigurerequest.width;
    }
    if (event->xconfigurerequest.height) {
        tempHeight = event->xconfigurerequest.height;
    }
}

void SendToWorkspace(int workspace) {
    Window focusedWindow;
    int revert;
    XGetInputFocus(display, &focusedWindow, &revert);

    int id = GetID(focusedWindow);
    _Window *_window = malloc(sizeof(_Window));
    _window->window = 0;

    workspaces[workspace][id] = workspaces[currentWorkspace][id];
    XUnmapWindow(display, workspaces[currentWorkspace][id]->window);
    XUnmapWindow(display, workspaces[currentWorkspace][id]->parentDec);
    workspaces[currentWorkspace][id] = _window;
    windowIDs[currentWorkspace][id] = 0;
    windowIDs[workspace][id] = 1;
    XSetInputFocus(display, root, 0, CurrentTime);
}

void GoToWorkspace(int workspace) {
    if (currentWorkspace == workspace) {
        return;
    }
    for (int i=0; i<999; i++) {
        if (windowIDs[currentWorkspace][i]) {
            XUnmapWindow(display, workspaces[currentWorkspace][i]->parentDec);
            XUnmapWindow(display, workspaces[currentWorkspace][i]->window);
        }
        if (windowIDs[workspace][i]) {
            XMapWindow(display, workspaces[workspace][i]->parentDec);
            XMapWindow(display, workspaces[workspace][i]->window);
        }
    }
    currentWorkspace = workspace;
    XSetInputFocus(display, root, 0, CurrentTime); // TODO change?
}

void MoveParentDec(int id)
{
    XWindowAttributes windowAttribs;
    XGetWindowAttributes(display, workspaces[currentWorkspace][id]->window, &windowAttribs);
    XMoveResizeWindow(display, workspaces[currentWorkspace][id]->parentDec,
                               windowAttribs.x - xpmAttribs[left_active].width,
                               windowAttribs.y - xpmAttribs[title_1_active].height,
                               windowAttribs.width + (xpmAttribs[left_active].width * 2),
                               windowAttribs.height + xpmAttribs[bottom_active].height + xpmAttribs[title_1_active].height);
    XMoveResizeWindow(display, workspaces[currentWorkspace][id]->closeButton,
                               windowAttribs.width - xpmAttribs[close_inactive].width,
                               xpmAttribs[bottom_inactive].height,
                               xpmAttribs[close_inactive].width,
                               xpmAttribs[close_inactive].height);
    XMoveResizeWindow(display, workspaces[currentWorkspace][id]->maxButton,
                               windowAttribs.width - xpmAttribs[maximize_inactive].width * 2,
                               xpmAttribs[bottom_inactive].height,
                               xpmAttribs[maximize_inactive].width,
                               xpmAttribs[maximize_inactive].height);
    XMoveResizeWindow(display, workspaces[currentWorkspace][id]->hideButton,
                               windowAttribs.width - xpmAttribs[hide_inactive].width * 3,
                               xpmAttribs[bottom_inactive].height,
                               xpmAttribs[hide_inactive].width,
                               xpmAttribs[hide_inactive].height);
    XMoveResizeWindow(display, workspaces[currentWorkspace][id]->left,
                               0,
                               xpmAttribs[top_left_inactive].height,
                               xpmAttribs[left_inactive].width,
                               windowAttribs.height);
    XMoveResizeWindow(display, workspaces[currentWorkspace][id]->right,
                               windowAttribs.width + xpmAttribs[right_inactive].width,
                               xpmAttribs[top_right_inactive].height,
                               xpmAttribs[right_inactive].width,
                               windowAttribs.height);
}

void CenterWindow(int id) {
    XWindowAttributes windowAttrib;
    XGetWindowAttributes(display, workspaces[currentWorkspace][id]->window, &windowAttrib);
    if (windowAttrib.width < 100 || windowAttrib.height < 100) {
        XMoveResizeWindow(display, workspaces[currentWorkspace][id]->window, (screenWidth - windowAttrib.width - 400)/2,
                                   (screenHeight - windowAttrib.height - 400)/2,
                                   windowAttrib.width + 400, windowAttrib.height + 400);
    }
    else {
        XMoveResizeWindow(display, workspaces[currentWorkspace][id]->window, (screenWidth - windowAttrib.width)/2,
                                   (screenHeight - windowAttrib.height)/2, windowAttrib.width, windowAttrib.height);
    }
    MoveParentDec(id);
    XRaiseWindow(display, workspaces[currentWorkspace][id]->window);
}

void HideDecorations() {
    for (int i=0; i<999; i++) {
        if (windowIDs[currentWorkspace][i]) {
            XUnmapWindow(display, workspaces[currentWorkspace][i]->parentDec);
        }
    }
}

void ReDecorate() {
    for (int i=0; i<999; i++) {
        if (windowIDs[currentWorkspace][i]) {
            XMapWindow(display, workspaces[currentWorkspace][i]->parentDec);
        }
    }
}

void Decorate(int id)
{
    XWindowAttributes parentDecAttribs;
    XGetWindowAttributes(display, workspaces[currentWorkspace][id]->parentDec, &parentDecAttribs);
    XWindowAttributes windowAttribs;
    XGetWindowAttributes(display, workspaces[currentWorkspace][id]->window, &windowAttribs);

    workspaces[currentWorkspace][id]->closeButton = XCreateSimpleWindow(display, workspaces[currentWorkspace][id]->parentDec,
        parentDecAttribs.width - xpmAttribs[close_inactive].width - xpmAttribs[right_inactive].width, xpmAttribs[bottom_inactive].height,
        xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0x000000, 0xffffff);
    XMapWindow(display, workspaces[currentWorkspace][id]->closeButton);
    XCopyArea(display, buttonsPixmap[close_inactive], workspaces[currentWorkspace][id]->closeButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    XGrabButton(display, AnyButton, AnyModifier, workspaces[currentWorkspace][id]->closeButton, True, ButtonPressMask|ButtonReleaseMask,GrabModeAsync, GrabModeAsync, 0, 0);
    XSelectInput(display, workspaces[currentWorkspace][id]->closeButton, ExposureMask|StructureNotifyMask|EnterWindowMask|LeaveWindowMask);
    SetID(workspaces[currentWorkspace][id]->closeButton, id);

    workspaces[currentWorkspace][id]->maxButton = XCreateSimpleWindow(display, workspaces[currentWorkspace][id]->parentDec,
        parentDecAttribs.width - xpmAttribs[maximize_inactive].width * 2 - xpmAttribs[right_inactive].width, xpmAttribs[bottom_inactive].height,
        xpmAttribs[maximize_inactive].width, xpmAttribs[maximize_inactive].height, 0, 0x000000, 0xffffff);
    XMapWindow(display, workspaces[currentWorkspace][id]->maxButton);
    XCopyArea(display, buttonsPixmap[maximize_inactive], workspaces[currentWorkspace][id]->maxButton, gc, 0, 0, xpmAttribs[maximize_inactive].width, xpmAttribs[maximize_inactive].height, 0, 0);
    XGrabButton(display, AnyButton, AnyModifier, workspaces[currentWorkspace][id]->maxButton, True, ButtonPressMask|ButtonReleaseMask,GrabModeAsync, GrabModeAsync, 0, 0);
    XSelectInput(display, workspaces[currentWorkspace][id]->maxButton, ExposureMask|StructureNotifyMask|EnterWindowMask|LeaveWindowMask);
    SetID(workspaces[currentWorkspace][id]->maxButton, id);

    workspaces[currentWorkspace][id]->hideButton = XCreateSimpleWindow(display, workspaces[currentWorkspace][id]->parentDec,
        parentDecAttribs.width - xpmAttribs[hide_inactive].width * 3 - xpmAttribs[right_inactive].width, xpmAttribs[bottom_inactive].height,
        xpmAttribs[hide_inactive].width, xpmAttribs[hide_inactive].height, 0, 0x000000, 0xffffff);
    XMapWindow(display, workspaces[currentWorkspace][id]->hideButton);
    XCopyArea(display, buttonsPixmap[hide_inactive], workspaces[currentWorkspace][id]->hideButton, gc, 0, 0, xpmAttribs[hide_inactive].width, xpmAttribs[hide_inactive].height, 0, 0);
    XGrabButton(display, AnyButton, AnyModifier, workspaces[currentWorkspace][id]->hideButton, True, ButtonPressMask|ButtonReleaseMask,GrabModeAsync, GrabModeAsync, 0, 0);
    XSelectInput(display, workspaces[currentWorkspace][id]->hideButton, ExposureMask|StructureNotifyMask|EnterWindowMask|LeaveWindowMask);
    SetID(workspaces[currentWorkspace][id]->hideButton, id);

    workspaces[currentWorkspace][id]->left = XCreateSimpleWindow(display, workspaces[currentWorkspace][id]->parentDec,
                                                                 0, 0, xpmAttribs[left_inactive].width, -parentDecAttribs.height, 0, 0x000000, 0xffffff);
    XMapWindow(display, workspaces[currentWorkspace][id]->left);
    for (int i=0 ; i<=(int)(windowAttribs.width/xpmAttribs[left_inactive].height); i++) {
        XCopyArea(display, buttonsPixmap[left_inactive], workspaces[currentWorkspace][id]->left, gc, 0, -windowAttribs.height +  xpmAttribs[left_inactive].height*i, -xpmAttribs[left_inactive].width, -xpmAttribs[left_inactive].height, 0, 0);
    }
    XGrabButton(display, AnyButton, AnyModifier, workspaces[currentWorkspace][id]->left, True, ButtonPressMask|ButtonReleaseMask,GrabModeAsync, GrabModeAsync, 0, 0);
    XSelectInput(display, workspaces[currentWorkspace][id]->left, ExposureMask);
    SetID(workspaces[currentWorkspace][id]->left, id);

    workspaces[currentWorkspace][id]->right = XCreateSimpleWindow(display, workspaces[currentWorkspace][id]->parentDec,
                                                                  windowAttribs.width + xpmAttribs[right_inactive].width, 0, xpmAttribs[right_inactive].width, -parentDecAttribs.height, 0, 0x000000, 0xffffff);
    XMapWindow(display, workspaces[currentWorkspace][id]->right);
    for (int i=0 ; i<=(int)(windowAttribs.width/xpmAttribs[right_inactive].height); i++) {
        XCopyArea(display, buttonsPixmap[left_inactive], workspaces[currentWorkspace][id]->right, gc, 0, -windowAttribs.height +  xpmAttribs[left_inactive].height*i, -xpmAttribs[left_inactive].width, -xpmAttribs[left_inactive].height, 0, 0);
    }
    XGrabButton(display, AnyButton, AnyModifier, workspaces[currentWorkspace][id]->right, True, ButtonPressMask|ButtonReleaseMask,GrabModeAsync, GrabModeAsync, 0, 0);
    XSelectInput(display, workspaces[currentWorkspace][id]->right, ExposureMask);
    SetID(workspaces[currentWorkspace][id]->right, id);
}

void CloseWindow(int id) // XKillClient closes multiple instances of the same program
{
    XEvent event;
    event.type = ClientMessage;
    event.xclient.window = workspaces[currentWorkspace][id]->window;
    event.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", False);
    event.xclient.format = 32;
    event.xclient.data.l[0] = XInternAtom(display, "WM_DELETE_WINDOW", False);
    event.xclient.data.l[1] = CurrentTime;
    XSendEvent(display, workspaces[currentWorkspace][id]->window, False, NoEventMask, &event);
}

void MaximizeWindow(int id) {
    XGetWindowAttributes(display, workspaces[currentWorkspace][id]->window, &workspaces[currentWorkspace][id]->attribs);
    XMoveResizeWindow(display, workspaces[currentWorkspace][id]->window, xpmAttribs[left_active].width,
                               xpmAttribs[title_1_active].height - xpmAttribs[bottom_active].height,
                               screenWidth - (xpmAttribs[left_active].width * 2) - rightMargin - leftMargin,
                               screenHeight - xpmAttribs[title_1_active].height - xpmAttribs[bottom_active].height - bottomMargin - topMargin);
    MoveParentDec(id);
}

void HideWindow(int id) {
    XUnmapWindow(display, workspaces[currentWorkspace][id]->window);
}

void CreateWindow(Window window, Window parentDec)
{
    int id = GetID(window);
    _Window *_window = malloc(sizeof(_Window));
    _window->window = window;
    _window->parentDec = parentDec;
    workspaces[currentWorkspace][id] = _window;
}

void SetID(Window window, int id)
{
    Atom idAtom = XInternAtom(display, "ID", False);
    char n[4];
    sprintf(n, "%i", id);
    int nOfDigits =(id==0)?1:log10(id)+1;
    XChangeProperty(display, window, idAtom, XA_STRING, 8, PropModeReplace, (const unsigned char *)n, nOfDigits);
    XSync(display, 0);
}

int GetID(Window window)
{
    XTextProperty text;
    Atom idAtom = XInternAtom(display, "ID", False);
    XGetTextProperty(display, window, &text, idAtom);
    char *temp;
    return strtol((const char *)text.value, &temp, 10);
}

int NextID()
{
    int id = 0;
    while (windowIDs[currentWorkspace][id]) {
        id++;
    }
    windowIDs[currentWorkspace][id] = 1;
    return id;
}

void CreateDecorations()
{
    XGCValues values;
    XImage *img;
    XpmAttributes attributes;
    for (int i=0; i<last; i++) {
        XpmCreateImageFromData(display, decorationFiles[i], &img, NULL, &attributes);
        buttonsPixmap[i] = XCreatePixmap(display, root, img->width, img->height, img->depth);
        XChangeGC(display, gc, GCForeground | GCBackground, &values);
        XPutImage(display, buttonsPixmap[i], gc, img, 0, 0, 0, 0, img->width, img->height);
        xpmAttribs[i] = attributes;
    }
}

void Init()
{
    screenWidth = XDisplayWidth(display, screen);
    screenHeight = XDisplayHeight(display, screen);
    XSelectInput(display, root, PropertyChangeMask|SubstructureRedirectMask|SubstructureNotifyMask);

    XGCValues values;
    gc = XCreateGC(display, root, 0, &values);

    _Window *_window = malloc(sizeof(_Window));
    _window->window = 0;
    for (int i=0 ; i<999 ; i++) {
        for (int j=0 ; j<nWorkspaces ; j++) {
            windowIDs[j][i] = 0;
            workspaces[j][i] = _window;
        }
    }
}

int main(int argc, char **argv)
{
    display = XOpenDisplay("");
    screen = DefaultScreen(display);
    root = RootWindow(display, screen);

    if (argc > 2) {
        Atom hideAtom = XInternAtom(display, "hide", False); // TODO global
        Atom closeAtom = XInternAtom(display, "close", False); // TODO global
        Atom decoAtom = XInternAtom(display, "decorations", False); // TODO global
        Atom workspaceAtom = XInternAtom(display, "workspace", False); // TODO global
        Atom sendToWorkspaceAtom = XInternAtom(display, "sendToWorkspace", False); // TODO global
        if (!strcmp(argv[1],"--send")) {
            XChangeProperty(display, root, sendToWorkspaceAtom, XA_STRING, 8, PropModeReplace, (const unsigned char*)argv[2], 1);
        }
        else if (!strcmp(argv[1],"--go")) {
            XChangeProperty(display, root, workspaceAtom, XA_STRING, 8, PropModeReplace, (const unsigned char*)argv[2], 1);
        }
        else if (!strcmp(argv[1],"--deco")) {
            XChangeProperty(display, root, decoAtom, XA_STRING, 8, PropModeReplace, (const unsigned char*)argv[2], 1);
        }
        else if (!strcmp(argv[1],"--hide")) {
            XChangeProperty(display, root, hideAtom, XA_STRING, 8, PropModeReplace, (const unsigned char*)argv[2], 1);
        }
        else if (!strcmp(argv[1],"--unhide")) {
            XChangeProperty(display, root, hideAtom, XA_STRING, 8, PropModeReplace, (const unsigned char*)argv[2], 1);
        }
        else if (!strcmp(argv[1],"--close")) {
            XChangeProperty(display, root, closeAtom, XA_STRING, 8, PropModeReplace, (const unsigned char*)argv[2], 1);
        }
        else {
            printf("blah\n");
        }
        XSync(display, 0);
        return 0;
    }

    Init();
    CreateDecorations();

    XEvent event;
    while (running) {
        XNextEvent(display, &event);
        //FILE *log = fopen("/tmp/DecoWMLog", "a");
        //fprintf(log, "event: %i\n", event.type);
        //fclose(log);
        pthread_t threadID;
        switch (event.type) {
            case(Expose):
                pthread_create(&threadID, NULL, (void *(*)(void *))HandleExpose, &event);
                pthread_join(threadID, NULL);
                break;
            case(MapRequest):
                HandleMapRequest(&event);
                break;
            case(ButtonPress):
                pthread_create(&threadID, NULL, (void *(*)(void *))HandleButtonPress, &event);
                pthread_join(threadID, NULL);
                break;
            case(ButtonRelease):
                pthread_create(&threadID, NULL, (void *(*)(void *))HandleButtonRelease, &event);
                pthread_join(threadID, NULL);
                break;
            case(MotionNotify):
                pthread_create(&threadID, NULL, (void *(*)(void *))HandleMotionNotify, &event);
                pthread_join(threadID, NULL);
                break;
            case(EnterNotify):
                pthread_create(&threadID, NULL, (void *(*)(void *))HandleEnterWindow, &event);
                pthread_join(threadID, NULL);
                break;
            case(LeaveNotify):
                pthread_create(&threadID, NULL, (void *(*)(void *))HandleLeaveWindow, &event);
                pthread_join(threadID, NULL);
                break;
            case(UnmapNotify):
                pthread_create(&threadID, NULL, (void *(*)(void *))HandleUnmapNotify, &event);
                pthread_join(threadID, NULL);
                break;
            case(PropertyNotify):
                pthread_create(&threadID, NULL, (void *(*)(void *))HandlePropertyChange, &event);
                pthread_join(threadID, NULL);
                break;
            case(ConfigureRequest):
                pthread_create(&threadID, NULL, (void *(*)(void *))HandleConfigureRequest, &event);
                pthread_join(threadID, NULL);
                break;
            default:
                break;
        }
    }
    return 0;
}
// TODO malloc array de windows and percorra ate o final com biggestId
// TODO fix fullscreen add option to keep how it is
// TODO multimonitor
// TODO alt tab
// TODO other decorations
