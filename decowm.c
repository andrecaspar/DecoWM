#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/xpm.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <math.h>
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

typedef struct _Window {
    Window window;
    Window parentDec;
    Window minButton;
    Window maxButton;
    Window closeButton;
    struct _Window *up;
    struct _Window *down;
    struct _Window *left;
    struct _Window *right;
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
void CloseWindow(int id);
void MaximizeWindow(int id);
void CreateWindow(Window window, Window parentDec); // change to one param, only one window
void Decorate(int id);
void SetID(Window window, int id);
int GetID(Window window);
int NextID(void);
void CreateDecorations(void);
void MoveParentDec(int id);
void CenterWindow(int id);
void Init(void);

Bool running = True;
Bool closing = False;

int currentWorkspace = 0;
int windowIDs[nWorkspaces][999];
_Window *workspaces[nWorkspaces][999]; // malloc

GC gc;
Window root;
Display *display;
XButtonEvent buttonClick;
int screen, screenWidth, screenHeight, tempX, tempY, tempMotionX = 0, tempMotionY = 0;

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

    if (exposeWindow == workspaces[currentWorkspace][id]->closeButton) {
        XCopyArea(display, buttonsPixmap[close_inactive], workspaces[currentWorkspace][id]->closeButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
    else if (exposeWindow == workspaces[currentWorkspace][id]->minButton) {
        XCopyArea(display, buttonsPixmap[hide_inactive], workspaces[currentWorkspace][id]->minButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
    else if (exposeWindow == workspaces[currentWorkspace][id]->maxButton) {
        XCopyArea(display, buttonsPixmap[maximize_inactive], workspaces[currentWorkspace][id]->maxButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
}

void HandleMapRequest(XEvent *event)
{
    Window mapWindow = event->xmaprequest.window; // change all these to pointers

    for (int i=0 ; i<999 ; i++) {
        if (workspaces[currentWorkspace][i]->window == mapWindow) {
            return;
        }
    }

    XMapWindow(display, mapWindow);

    XWindowAttributes windowAttrib;
    XGetWindowAttributes(display, mapWindow, &windowAttrib);
    XMoveResizeWindow(display, mapWindow, tempX, tempY, windowAttrib.width, windowAttrib.height);

    unsigned long length, after;
    unsigned char *data;
    int format;
    Atom atom = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    XGetWindowProperty(display,mapWindow,atom,0L,1L,False,AnyPropertyType,&atom,&format,&length,&after,&data);
    char *value = XGetAtomName(display, ((Atom *)data)[0]);
    if (!strcmp(value, "_NET_WM_WINDOW_TYPE_DOCK")) {
        return;
    }

    XSelectInput(display, mapWindow, ExposureMask|StructureNotifyMask|EnterWindowMask|LeaveWindowMask);
    XGrabButton(display, AnyButton, Mod1Mask, mapWindow, True, PointerMotionMask, GrabModeAsync, GrabModeAsync, 0, 0);

    Window parentDec = XCreateSimpleWindow(display, root, 0, 0, 1, 1, 0, BlackPixel(display, screen), 0x86d6b2);
    XMapWindow(display, parentDec);
    XGrabButton(display, AnyButton, AnyModifier, parentDec, True, PointerMotionMask|ButtonPressMask|ButtonReleaseMask,GrabModeAsync, GrabModeAsync, 0, 0);

    int id = NextID();
    SetID(mapWindow, id);
    SetID(parentDec, id);
    CreateWindow(mapWindow, parentDec); // pointers // change name

    Decorate(id);

    XMoveResizeWindow(display, mapWindow, (screenWidth - windowAttrib.width)/2, (screenHeight - windowAttrib.height)/2, windowAttrib.width, windowAttrib.height);
    MoveParentDec(id);
    XRaiseWindow(display, mapWindow);
}

void HandleButtonPress(XEvent *event)
{
    buttonClick = event->xbutton;
    Window clickWindow = event->xbutton.window;
    int id = GetID(clickWindow);

    if (clickWindow == workspaces[currentWorkspace][id]->closeButton) {
        XCopyArea(display, buttonsPixmap[close_active], workspaces[currentWorkspace][id]->closeButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
    else if (clickWindow == workspaces[currentWorkspace][id]->minButton) {
        XCopyArea(display, buttonsPixmap[hide_active], workspaces[currentWorkspace][id]->minButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
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
    else if (releaseWindow == workspaces[currentWorkspace][id]->minButton) {
        XCopyArea(display, buttonsPixmap[hide_inactive], workspaces[currentWorkspace][id]->minButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
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
    _Window *window = workspaces[currentWorkspace][id];

    XWindowAttributes windowAttrib;
    XGetWindowAttributes(display, window->window, &windowAttrib);

    XMoveResizeWindow(display, window->window, windowAttrib.x + (buttonClick.button == 1 ? xd : 0),
                                               windowAttrib.y + (buttonClick.button == 1 ? yd : 0),
                                               windowAttrib.width + (buttonClick.button == 3 ? event->xmotion.x - tempMotionX : 0),
                                               windowAttrib.height + (buttonClick.button == 3 ? event->xmotion.y - tempMotionY : 0));
    tempMotionX = event->xmotion.x;
    tempMotionY = event->xmotion.y;

    MoveParentDec(id);
}

void HandleEnterWindow(XEvent *event)
{
    Window enterWindow = event->xcrossing.window;
    int id = GetID(enterWindow);

    if (enterWindow == workspaces[currentWorkspace][id]->closeButton) {
        XCopyArea(display, buttonsPixmap[close_prelight], workspaces[currentWorkspace][id]->closeButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
    else if (enterWindow == workspaces[currentWorkspace][id]->minButton) {
        XCopyArea(display, buttonsPixmap[hide_prelight], workspaces[currentWorkspace][id]->minButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
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
    else if (leaveWindow == workspaces[currentWorkspace][id]->minButton) {
        XCopyArea(display, buttonsPixmap[hide_inactive], workspaces[currentWorkspace][id]->minButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
    else if (leaveWindow == workspaces[currentWorkspace][id]->maxButton) {
        XCopyArea(display, buttonsPixmap[maximize_inactive], workspaces[currentWorkspace][id]->maxButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    }
}

void HandleUnmapNotify(XEvent *event)
{
    Window unmapWindow = event->xunmap.window;
    for (int i=0; i<999; i++) {
        if (workspaces[currentWorkspace][i]->window == unmapWindow) {
            XDestroyWindow(display, workspaces[currentWorkspace][i]->parentDec);
            XSetInputFocus(display, root, 0, CurrentTime);
            workspaces[currentWorkspace][i]->window = 0;
            windowIDs[currentWorkspace][i] = 0;
            closing = True;
            break;
        }
    }
}

void HandlePropertyChange(XEvent *event)
{
}

void HandleConfigureRequest(XEvent *event)
{
    if (event->xconfigurerequest.x) {
        tempX = event->xconfigurerequest.x;
    }
    if (event->xconfigurerequest.y) {
        tempY = event->xconfigurerequest.y;
    }
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
    XMoveResizeWindow(display, workspaces[currentWorkspace][id]->minButton,
                               windowAttribs.width - xpmAttribs[hide_inactive].width * 3,
                               xpmAttribs[bottom_inactive].height,
                               xpmAttribs[hide_inactive].width,
                               xpmAttribs[hide_inactive].height);
}

void Decorate(int id)
{
    //FILE *log = fopen("/tmp/DecoWMLog", "a");
    //fprintf(log, "id: %i\n", id);
    //fclose(log);
    XWindowAttributes parentDecAttribs;
    XGetWindowAttributes(display, workspaces[currentWorkspace][id]->parentDec, &parentDecAttribs);

    workspaces[currentWorkspace][id]->closeButton = XCreateSimpleWindow(display, workspaces[currentWorkspace][id]->parentDec,
        parentDecAttribs.width - xpmAttribs[close_inactive].width - xpmAttribs[right_inactive].width, xpmAttribs[bottom_inactive].height,
        xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, BlackPixel(display, screen), WhitePixel(display, screen));
    XMapWindow(display, workspaces[currentWorkspace][id]->closeButton);
    XCopyArea(display, buttonsPixmap[close_inactive], workspaces[currentWorkspace][id]->closeButton, gc, 0, 0, xpmAttribs[close_inactive].width, xpmAttribs[close_inactive].height, 0, 0);
    XGrabButton(display, AnyButton, AnyModifier, workspaces[currentWorkspace][id]->closeButton, True, ButtonPressMask|ButtonReleaseMask,GrabModeAsync, GrabModeAsync, 0, 0);
    XSelectInput(display, workspaces[currentWorkspace][id]->closeButton, ExposureMask|StructureNotifyMask|EnterWindowMask|LeaveWindowMask);
    SetID(workspaces[currentWorkspace][id]->closeButton, id);

    workspaces[currentWorkspace][id]->maxButton = XCreateSimpleWindow(display, workspaces[currentWorkspace][id]->parentDec,
        parentDecAttribs.width - xpmAttribs[maximize_inactive].width * 2 - xpmAttribs[right_inactive].width, xpmAttribs[bottom_inactive].height,
        xpmAttribs[maximize_inactive].width, xpmAttribs[maximize_inactive].height, 0, BlackPixel(display, screen), WhitePixel(display, screen));
    XMapWindow(display, workspaces[currentWorkspace][id]->maxButton);
    XCopyArea(display, buttonsPixmap[maximize_inactive], workspaces[currentWorkspace][id]->maxButton, gc, 0, 0, xpmAttribs[maximize_inactive].width, xpmAttribs[maximize_inactive].height, 0, 0);
    XGrabButton(display, AnyButton, AnyModifier, workspaces[currentWorkspace][id]->maxButton, True, ButtonPressMask|ButtonReleaseMask,GrabModeAsync, GrabModeAsync, 0, 0);
    XSelectInput(display, workspaces[currentWorkspace][id]->maxButton, ExposureMask|StructureNotifyMask|EnterWindowMask|LeaveWindowMask);
    SetID(workspaces[currentWorkspace][id]->maxButton, id);

    workspaces[currentWorkspace][id]->minButton = XCreateSimpleWindow(display, workspaces[currentWorkspace][id]->parentDec,
        parentDecAttribs.width - xpmAttribs[hide_inactive].width * 3 - xpmAttribs[right_inactive].width, xpmAttribs[bottom_inactive].height,
        xpmAttribs[hide_inactive].width, xpmAttribs[hide_inactive].height, 0, BlackPixel(display, screen), WhitePixel(display, screen));
    XMapWindow(display, workspaces[currentWorkspace][id]->minButton);
    XCopyArea(display, buttonsPixmap[hide_inactive], workspaces[currentWorkspace][id]->minButton, gc, 0, 0, xpmAttribs[hide_inactive].width, xpmAttribs[hide_inactive].height, 0, 0);
    XGrabButton(display, AnyButton, AnyModifier, workspaces[currentWorkspace][id]->minButton, True, ButtonPressMask|ButtonReleaseMask,GrabModeAsync, GrabModeAsync, 0, 0);
    XSelectInput(display, workspaces[currentWorkspace][id]->minButton, ExposureMask|StructureNotifyMask|EnterWindowMask|LeaveWindowMask);
    SetID(workspaces[currentWorkspace][id]->minButton, id);
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
    XMoveResizeWindow(display, workspaces[currentWorkspace][id]->window, xpmAttribs[left_active].width,
                               xpmAttribs[title_1_active].height - xpmAttribs[bottom_active].height,
                               screenWidth - (xpmAttribs[left_active].width * 2),
                               screenHeight - xpmAttribs[title_1_active].height - xpmAttribs[bottom_active].height);
    MoveParentDec(id);
}

void CreateWindow(Window window, Window parentDec)
{
    int id = GetID(window);
    _Window *_window = malloc(sizeof(_Window));
    _window->window = window;
    _window->up = 0;
    _window->down = 0;
    _window->left = 0;
    _window->right = 0;
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
        for (int j=0 ; j < nWorkspaces ; j++) {
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
        Atom workspaceAtom = XInternAtom(display, "workspace", False); // global
        Atom sendToWorkspaceAtom = XInternAtom(display, "sendToWorkspace", False);
        Atom decoAtom = XInternAtom(display, "decorations", False);
        if (!strcmp(argv[1],"--send")) {
            XChangeProperty(display, root, sendToWorkspaceAtom, XA_STRING, 8, PropModeReplace, (const unsigned char*)argv[2], 1);
        }
        else if (!strcmp(argv[1],"--go")) {
            XChangeProperty(display, root, workspaceAtom, XA_STRING, 8, PropModeReplace, (const unsigned char*)argv[2], 1);
        }
        else if (!strcmp(argv[1],"--deco")) {
            XChangeProperty(display, root, decoAtom, XA_STRING, 8, PropModeReplace, (const unsigned char*)argv[2], 1);
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
        switch (event.type) {
            case(Expose):
                HandleExpose(&event);
                break;
            case(MapRequest):
                HandleMapRequest(&event);
                break;
            case(ButtonPress):
                HandleButtonPress(&event);
                break;
            case(ButtonRelease):
                HandleButtonRelease(&event);
                break;
            case(MotionNotify):
                HandleMotionNotify(&event);
                break;
            case(EnterNotify):
                HandleEnterWindow(&event);
                break;
            case(LeaveNotify):
                HandleLeaveWindow(&event);
                break;
            case(UnmapNotify):
                HandleUnmapNotify(&event);
                break;
            case(PropertyNotify):
                HandlePropertyChange(&event);
                break;
            case(ConfigureRequest):
                HandleConfigureRequest(&event);
                break;
            default:
                break;
        }
    }
    return 0;
}
// malloc array de windows and percorra ate o final com biggestId
