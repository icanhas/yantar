/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */


#include <SDL/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../client/client.h"
#include "../../client/keycodes.h"
#include "../../client/keys.h"
#include "../local.h"

#ifdef MACOS_X
/* Mouse acceleration needs to be disabled */
#define MACOS_X_ACCELERATION_HACK
/* Cursor needs hack to hide */
#define MACOS_X_CURSOR_HACK
#endif

#ifdef MACOS_X_ACCELERATION_HACK
#include <IOKit/IOTypes.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#include <IOKit/hidsystem/event_status_driver.h>
#endif

static Cvar *in_keyboardDebug = NULL;

static SDL_Joystick *stick = NULL;

static qbool mouseAvailable = qfalse;
static qbool mouseActive = qfalse;
static qbool keyRepeatEnabled = qfalse;

static Cvar	*in_mouse = NULL;
#ifdef MACOS_X_ACCELERATION_HACK
static Cvar	*in_disablemacosxmouseaccel = NULL;
static double	originalMouseSpeed = -1.0;
#endif
static Cvar	*in_nograb;

static Cvar	*in_joystick = NULL;
static Cvar	*in_joystickDebug = NULL;
static Cvar	*in_joystickThreshold	= NULL;
static Cvar	*in_joystickNo		= NULL;
static Cvar	*in_joystickUseAnalog	= NULL;

static int vidRestartTime = 0;

#define CTRL(a) ((a)-'a'+1)

static void
IN_PrintKey(const SDL_keysym *keysym, keyNum_t key, qbool down)
{
	if(down)
		comprintf("+ ");
	else
		comprintf("  ");

	comprintf("0x%02x \"%s\"", keysym->scancode,
		SDL_GetKeyName(keysym->sym));

	if(keysym->mod & KMOD_LSHIFT) comprintf(" KMOD_LSHIFT");
	if(keysym->mod & KMOD_RSHIFT) comprintf(" KMOD_RSHIFT");
	if(keysym->mod & KMOD_LCTRL) comprintf(" KMOD_LCTRL");
	if(keysym->mod & KMOD_RCTRL) comprintf(" KMOD_RCTRL");
	if(keysym->mod & KMOD_LALT) comprintf(" KMOD_LALT");
	if(keysym->mod & KMOD_RALT) comprintf(" KMOD_RALT");
	if(keysym->mod & KMOD_LMETA) comprintf(" KMOD_LMETA");
	if(keysym->mod & KMOD_RMETA) comprintf(" KMOD_RMETA");
	if(keysym->mod & KMOD_NUM) comprintf(" KMOD_NUM");
	if(keysym->mod & KMOD_CAPS) comprintf(" KMOD_CAPS");
	if(keysym->mod & KMOD_MODE) comprintf(" KMOD_MODE");
	if(keysym->mod & KMOD_RESERVED) comprintf(" KMOD_RESERVED");

	comprintf(" Q:0x%02x(%s)", key, Key_KeynumToString(key));

	if(keysym->unicode){
		comprintf(" U:0x%02x", keysym->unicode);

		if(keysym->unicode > ' ' && keysym->unicode < '~')
			comprintf("(%c)", (char)keysym->unicode);
	}

	comprintf("\n");
}

#define MAX_CONSOLE_KEYS 16

static qbool
IN_IsConsoleKey(keyNum_t key, const unsigned char character)
{
	typedef struct consoleKey_s {
		enum {
			KEY,
			CHARACTER
		} type;

		union {
			keyNum_t	key;
			unsigned char	character;
		} u;
	} consoleKey_t;

	static consoleKey_t consoleKeys[ MAX_CONSOLE_KEYS ];
	static int numConsoleKeys = 0;
	int i;

	/* Only parse the variable when it changes */
	if(cl_consoleKeys->modified){
		char *text_p, *token;

		cl_consoleKeys->modified = qfalse;
		text_p = cl_consoleKeys->string;
		numConsoleKeys = 0;

		while(numConsoleKeys < MAX_CONSOLE_KEYS){
			consoleKey_t *c = &consoleKeys[ numConsoleKeys ];
			int charCode = 0;

			token = Q_readtok(&text_p);
			if(!token[ 0 ])
				break;

			if(strlen(token) == 4)
				charCode = Q_hexstr2int(token);

			if(charCode > 0){
				c->type = CHARACTER;
				c->u.character = (unsigned char)charCode;
			}else{
				c->type		= KEY;
				c->u.key	= Key_StringToKeynum(token);

				/* 0 isn't a key */
				if(c->u.key <= 0)
					continue;
			}

			numConsoleKeys++;
		}
	}

	/* If the character is the same as the key, prefer the character */
	if(key == character)
		key = 0;

	for(i = 0; i < numConsoleKeys; i++){
		consoleKey_t *c = &consoleKeys[ i ];

		switch(c->type){
		case KEY:
			if(key && c->u.key == key)
				return qtrue;
			break;

		case CHARACTER:
			if(c->u.character == character)
				return qtrue;
			break;
		}
	}

	return qfalse;
}

static const char *
IN_TranslateSDLToQ3Key(SDL_keysym *keysym,
		       keyNum_t *key, qbool down)
{
	static unsigned char buf[ 2 ] = { '\0', '\0' };

	*buf	= '\0';
	*key	= 0;

	if(keysym->sym >= SDLK_SPACE && keysym->sym < SDLK_DELETE){
		/* These happen to match the ASCII chars */
		*key = (int)keysym->sym;
	}else{
		switch(keysym->sym){
		case SDLK_PAGEUP:       *key	= K_PGUP;          break;
		case SDLK_KP9:          *key	= K_KP_PGUP;       break;
		case SDLK_PAGEDOWN:     *key	= K_PGDN;          break;
		case SDLK_KP3:          *key	= K_KP_PGDN;       break;
		case SDLK_KP7:          *key	= K_KP_HOME;       break;
		case SDLK_HOME:         *key	= K_HOME;          break;
		case SDLK_KP1:          *key	= K_KP_END;        break;
		case SDLK_END:          *key	= K_END;           break;
		case SDLK_KP4:          *key	= K_KP_LEFTARROW;  break;
		case SDLK_LEFT:         *key	= K_LEFTARROW;     break;
		case SDLK_KP6:          *key	= K_KP_RIGHTARROW; break;
		case SDLK_RIGHT:        *key	= K_RIGHTARROW;    break;
		case SDLK_KP2:          *key	= K_KP_DOWNARROW;  break;
		case SDLK_DOWN:         *key	= K_DOWNARROW;     break;
		case SDLK_KP8:          *key	= K_KP_UPARROW;    break;
		case SDLK_UP:           *key	= K_UPARROW;       break;
		case SDLK_ESCAPE:       *key	= K_ESCAPE;        break;
		case SDLK_KP_ENTER:     *key	= K_KP_ENTER;      break;
		case SDLK_RETURN:       *key	= K_ENTER;         break;
		case SDLK_TAB:          *key	= K_TAB;           break;
		case SDLK_F1:           *key	= K_F1;            break;
		case SDLK_F2:           *key	= K_F2;            break;
		case SDLK_F3:           *key	= K_F3;            break;
		case SDLK_F4:           *key	= K_F4;            break;
		case SDLK_F5:           *key	= K_F5;            break;
		case SDLK_F6:           *key	= K_F6;            break;
		case SDLK_F7:           *key	= K_F7;            break;
		case SDLK_F8:           *key	= K_F8;            break;
		case SDLK_F9:           *key	= K_F9;            break;
		case SDLK_F10:          *key	= K_F10;           break;
		case SDLK_F11:          *key	= K_F11;           break;
		case SDLK_F12:          *key	= K_F12;           break;
		case SDLK_F13:          *key	= K_F13;           break;
		case SDLK_F14:          *key	= K_F14;           break;
		case SDLK_F15:          *key	= K_F15;           break;

		case SDLK_BACKSPACE:    *key	= K_BACKSPACE;     break;
		case SDLK_KP_PERIOD:    *key	= K_KP_DEL;        break;
		case SDLK_DELETE:       *key	= K_DEL;           break;
		case SDLK_PAUSE:        *key	= K_PAUSE;         break;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT:       *key = K_SHIFT;         break;

		case SDLK_LCTRL:
		case SDLK_RCTRL:        *key = K_CTRL;          break;

		case SDLK_RMETA:
		case SDLK_LMETA:        *key = K_COMMAND;       break;

		case SDLK_RALT:
		case SDLK_LALT:         *key = K_ALT;           break;

		case SDLK_LSUPER:
		case SDLK_RSUPER:       *key = K_SUPER;         break;

		case SDLK_KP5:          *key	= K_KP_5;          break;
		case SDLK_INSERT:       *key	= K_INS;           break;
		case SDLK_KP0:          *key	= K_KP_INS;        break;
		case SDLK_KP_MULTIPLY:  *key	= K_KP_STAR;       break;
		case SDLK_KP_PLUS:      *key	= K_KP_PLUS;       break;
		case SDLK_KP_MINUS:     *key	= K_KP_MINUS;      break;
		case SDLK_KP_DIVIDE:    *key	= K_KP_SLASH;      break;

		case SDLK_MODE:         *key	= K_MODE;          break;
		case SDLK_COMPOSE:      *key	= K_COMPOSE;       break;
		case SDLK_HELP:         *key	= K_HELP;          break;
		case SDLK_PRINT:        *key	= K_PRINT;         break;
		case SDLK_SYSREQ:       *key	= K_SYSREQ;        break;
		case SDLK_BREAK:        *key	= K_BREAK;         break;
		case SDLK_MENU:         *key	= K_MENU;          break;
		case SDLK_POWER:        *key	= K_POWER;         break;
		case SDLK_EURO:         *key	= K_EURO;          break;
		case SDLK_UNDO:         *key	= K_UNDO;          break;
		case SDLK_SCROLLOCK:    *key	= K_SCROLLOCK;     break;
		case SDLK_NUMLOCK:      *key	= K_KP_NUMLOCK;    break;
		case SDLK_CAPSLOCK:     *key	= K_CAPSLOCK;      break;

		default:
			if(keysym->sym >= SDLK_WORLD_0 && keysym->sym <= SDLK_WORLD_95)
				*key = (keysym->sym - SDLK_WORLD_0) + K_WORLD_0;
			break;
		}
	}

	if(down && keysym->unicode && !(keysym->unicode & 0xFF00)){
		unsigned char ch = (unsigned char)keysym->unicode & 0xFF;

		switch(ch){
		case 127:	/* ASCII delete */
			if(*key != K_DEL){
				/* ctrl-h */
				*buf = CTRL('h');
				break;
			}
		/* fallthrough */

		default: *buf = ch; break;
		}
	}

	if(in_keyboardDebug->integer)
		IN_PrintKey(keysym, *key, down);

	/* Keys that have ASCII names but produce no character are probably
	 * dead keys -- ignore them */
	if(down && strlen(Key_KeynumToString(*key)) == 1 &&
	   keysym->unicode == 0){
		if(in_keyboardDebug->integer)
			comprintf("  Ignored dead key '%c'\n", *key);

		*key = 0;
	}

	if(IN_IsConsoleKey(*key, *buf)){
		/* Console keys can't be bound or generate characters */
		*key	= K_CONSOLE;
		*buf	= '\0';
	}

	/* Don't allow extended ASCII to generate characters */
	if(*buf & 0x80)
		*buf = '\0';

	return (char*)buf;
}

#ifdef MACOS_X_ACCELERATION_HACK
static io_connect_t
IN_GetIOHandle(void)	/* mac os x mouse accel hack */
{
	io_connect_t	iohandle = MACH_PORT_NULL;
	kern_return_t	status;
	io_service_t	iohidsystem = MACH_PORT_NULL;
	mach_port_t	masterport;

	status = IOMasterPort(MACH_PORT_NULL, &masterport);
	if(status != KERN_SUCCESS)
		return 0;

	iohidsystem = IORegistryEntryFromPath(masterport, kIOServicePlane ":/IOResources/IOHIDSystem");
	if(!iohidsystem)
		return 0;

	status = IOServiceOpen(iohidsystem, mach_task_self(), kIOHIDParamConnectType, &iohandle);
	IOObjectRelease(iohidsystem);

	return iohandle;
}
#endif

static void
IN_GobbleMotionEvents(void)
{
	SDL_Event dummy[ 1 ];

	/* Gobble any mouse motion events */
	SDL_PumpEvents( );
	while(SDL_PeepEvents(dummy, 1, SDL_GETEVENT,
		      SDL_EVENTMASK(SDL_MOUSEMOTION))){
	}
}

static void
IN_ActivateMouse(void)
{
	if(!mouseAvailable || !SDL_WasInit(SDL_INIT_VIDEO))
		return;

#ifdef MACOS_X_ACCELERATION_HACK
	if(!mouseActive){	/* mac os x mouse accel hack */
		/* Save the status of mouse acceleration */
		originalMouseSpeed = -1.0;	/* in case of error */
		if(in_disablemacosxmouseaccel->integer){
			io_connect_t mouseDev = IN_GetIOHandle();
			if(mouseDev != 0){
				if(IOHIDGetAccelerationWithKey(mouseDev, CFSTR(kIOHIDMouseAccelerationType),
					   &originalMouseSpeed) == kIOReturnSuccess){
					comprintf("previous mouse acceleration: %f\n", originalMouseSpeed);
					if(IOHIDSetAccelerationWithKey(mouseDev,
						   CFSTR(kIOHIDMouseAccelerationType),
						   -1.0) != kIOReturnSuccess){
						comprintf(
							"Could not disable mouse acceleration (failed at IOHIDSetAccelerationWithKey).\n");
						cvarsetstr ("in_disablemacosxmouseaccel", 0);
					}
				}else{
					comprintf(
						"Could not disable mouse acceleration (failed at IOHIDGetAccelerationWithKey).\n");
					cvarsetstr ("in_disablemacosxmouseaccel", 0);
				}
				IOServiceClose(mouseDev);
			}else{
				comprintf(
					"Could not disable mouse acceleration (failed at IO_GetIOHandle).\n");
				cvarsetstr ("in_disablemacosxmouseaccel", 0);
			}
		}
	}
#endif

	if(!mouseActive){
		SDL_ShowCursor(0);
#ifdef MACOS_X_CURSOR_HACK
		/* This is a bug in the current SDL/macosx...have to toggle it a few
		 *  times to get the cursor to hide. */
		SDL_ShowCursor(1);
		SDL_ShowCursor(0);
#endif
		SDL_WM_GrabInput(SDL_GRAB_ON);

		IN_GobbleMotionEvents( );
	}

	/* in_nograb makes no sense in fullscreen mode */
	if(!cvargeti("r_fullscreen")){
		if(in_nograb->modified || !mouseActive){
			if(in_nograb->integer)
				SDL_WM_GrabInput(SDL_GRAB_OFF);
			else
				SDL_WM_GrabInput(SDL_GRAB_ON);

			in_nograb->modified = qfalse;
		}
	}

	mouseActive = qtrue;
}

static void
IN_DeactivateMouse(void)
{
	if(!SDL_WasInit(SDL_INIT_VIDEO))
		return;

	/* Always show the cursor when the mouse is disabled,
	 * but not when fullscreen */
	if(!cvargeti("r_fullscreen"))
		SDL_ShowCursor(1);

	if(!mouseAvailable)
		return;

#ifdef MACOS_X_ACCELERATION_HACK
	if(mouseActive){/* mac os x mouse accel hack */
		if(originalMouseSpeed != -1.0){
			io_connect_t mouseDev = IN_GetIOHandle();
			if(mouseDev != 0){
				comprintf("restoring mouse acceleration to: %f\n", originalMouseSpeed);
				if(IOHIDSetAccelerationWithKey(mouseDev, CFSTR(kIOHIDMouseAccelerationType),
					   originalMouseSpeed) != kIOReturnSuccess)
					comprintf(
						"Could not re-enable mouse acceleration (failed at IOHIDSetAccelerationWithKey).\n");
				IOServiceClose(mouseDev);
			}else
				comprintf(
					"Could not re-enable mouse acceleration (failed at IO_GetIOHandle).\n");
		}
	}
#endif

	if(mouseActive){
		IN_GobbleMotionEvents( );

		SDL_WM_GrabInput(SDL_GRAB_OFF);

		/* Don't warp the mouse unless the cursor is within the window */
		if(SDL_GetAppState( ) & SDL_APPMOUSEFOCUS)
			SDL_WarpMouse(cls.glconfig.vidWidth / 2, cls.glconfig.vidHeight / 2);

		mouseActive = qfalse;
	}
}

/* We translate axes movement into keypresses */
static int joy_keys[16] = {
	K_LEFTARROW, K_RIGHTARROW,
	K_UPARROW, K_DOWNARROW,
	K_JOY17, K_JOY18,
	K_JOY19, K_JOY20,
	K_JOY21, K_JOY22,
	K_JOY23, K_JOY24,
	K_JOY25, K_JOY26,
	K_JOY27, K_JOY28
};

/* translate hat events into keypresses
 * the 4 highest buttons are used for the first hat ... */
static int hat_keys[16] = {
	K_JOY29, K_JOY30,
	K_JOY31, K_JOY32,
	K_JOY25, K_JOY26,
	K_JOY27, K_JOY28,
	K_JOY21, K_JOY22,
	K_JOY23, K_JOY24,
	K_JOY17, K_JOY18,
	K_JOY19, K_JOY20
};


struct {
	qbool		buttons[16];	/* !!! FIXME: these might be too many. */
	unsigned int	oldaxes;
	int		oldaaxes[MAX_JOYSTICK_AXIS];
	unsigned int	oldhats;
} stick_state;

static void
IN_InitJoystick(void)
{
	int	i = 0;
	int	total = 0;
	char	buf[16384] = "";

	if(stick != NULL)
		SDL_JoystickClose(stick);

	stick = NULL;
	memset(&stick_state, '\0', sizeof(stick_state));

	if(!SDL_WasInit(SDL_INIT_JOYSTICK)){
		comdprintf("Calling SDL_Init(SDL_INIT_JOYSTICK)...\n");
		if(SDL_Init(SDL_INIT_JOYSTICK) == -1){
			comdprintf("SDL_Init(SDL_INIT_JOYSTICK) failed: %s\n", SDL_GetError());
			return;
		}
		comdprintf("SDL_Init(SDL_INIT_JOYSTICK) passed.\n");
	}

	total = SDL_NumJoysticks();
	comdprintf("%d possible joysticks\n", total);

	/* Print list and build cvar to allow ui to select joystick. */
	for(i = 0; i < total; i++){
		Q_strcat(buf, sizeof(buf), SDL_JoystickName(i));
		Q_strcat(buf, sizeof(buf), "\n");
	}

	cvarget("in_availableJoysticks", buf, CVAR_ROM);

	if(!in_joystick->integer){
		comdprintf("Joystick is not active.\n");
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		return;
	}

	in_joystickNo = cvarget("in_joystickNo", "0", CVAR_ARCHIVE);
	if(in_joystickNo->integer < 0 || in_joystickNo->integer >= total)
		cvarsetstr("in_joystickNo", "0");

	in_joystickUseAnalog = cvarget("in_joystickUseAnalog", "0", CVAR_ARCHIVE);

	stick = SDL_JoystickOpen(in_joystickNo->integer);

	if(stick == NULL){
		comdprintf("No joystick opened.\n");
		return;
	}

	comdprintf("Joystick %d opened\n", in_joystickNo->integer);
	comdprintf("Name:       %s\n", SDL_JoystickName(in_joystickNo->integer));
	comdprintf("Axes:       %d\n", SDL_JoystickNumAxes(stick));
	comdprintf("Hats:       %d\n", SDL_JoystickNumHats(stick));
	comdprintf("Buttons:    %d\n", SDL_JoystickNumButtons(stick));
	comdprintf("Balls:      %d\n", SDL_JoystickNumBalls(stick));
	comdprintf("Use Analog: %s\n", in_joystickUseAnalog->integer ? "Yes" : "No");

	SDL_JoystickEventState(SDL_QUERY);
}

static void
IN_ShutdownJoystick(void)
{
	if(stick){
		SDL_JoystickClose(stick);
		stick = NULL;
	}

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

static void
IN_JoyMove(void)
{
	qbool joy_pressed[ARRAY_LEN(joy_keys)];
	unsigned int	axes	= 0;
	unsigned int	hats	= 0;
	int	total = 0;
	int	i = 0;

	if(!stick)
		return;

	SDL_JoystickUpdate();

	memset(joy_pressed, '\0', sizeof(joy_pressed));

	/* update the ball state. */
	total = SDL_JoystickNumBalls(stick);
	if(total > 0){
		int	balldx = 0;
		int	balldy = 0;
		for(i = 0; i < total; i++){
			int	dx	= 0;
			int	dy	= 0;
			SDL_JoystickGetBall(stick, i, &dx, &dy);
			balldx	+= dx;
			balldy	+= dy;
		}
		if(balldx || balldy){
			/* !!! FIXME: is this good for stick balls, or just mice?
			 * Scale like the mouse input... */
			if(abs(balldx) > 1)
				balldx *= 2;
			if(abs(balldy) > 1)
				balldy *= 2;
			comqueueevent(0, SE_MOUSE, balldx, balldy, 0, NULL);
		}
	}

	/* now query the stick buttons... */
	total = SDL_JoystickNumButtons(stick);
	if(total > 0){
		if(total > ARRAY_LEN(stick_state.buttons))
			total = ARRAY_LEN(stick_state.buttons);
		for(i = 0; i < total; i++){
			qbool pressed = (SDL_JoystickGetButton(stick, i) != 0);
			if(pressed != stick_state.buttons[i]){
				comqueueevent(0, SE_KEY, K_JOY1 + i, pressed, 0, NULL);
				stick_state.buttons[i] = pressed;
			}
		}
	}

	/* look at the hats... */
	total = SDL_JoystickNumHats(stick);
	if(total > 0){
		if(total > 4) total = 4;
		for(i = 0; i < total; i++)
			((Uint8*)&hats)[i] = SDL_JoystickGetHat(stick, i);
	}

	/* update hat state */
	if(hats != stick_state.oldhats){
		for(i = 0; i < 4; i++)
			if(((Uint8*)&hats)[i] != ((Uint8*)&stick_state.oldhats)[i]){
				/* release event */
				switch(((Uint8*)&stick_state.oldhats)[i]){
				case SDL_HAT_UP:
					comqueueevent(0, SE_KEY, hat_keys[4*i + 0], qfalse, 0, NULL);
					break;
				case SDL_HAT_RIGHT:
					comqueueevent(0, SE_KEY, hat_keys[4*i + 1], qfalse, 0, NULL);
					break;
				case SDL_HAT_DOWN:
					comqueueevent(0, SE_KEY, hat_keys[4*i + 2], qfalse, 0, NULL);
					break;
				case SDL_HAT_LEFT:
					comqueueevent(0, SE_KEY, hat_keys[4*i + 3], qfalse, 0, NULL);
					break;
				case SDL_HAT_RIGHTUP:
					comqueueevent(0, SE_KEY, hat_keys[4*i + 0], qfalse, 0, NULL);
					comqueueevent(0, SE_KEY, hat_keys[4*i + 1], qfalse, 0, NULL);
					break;
				case SDL_HAT_RIGHTDOWN:
					comqueueevent(0, SE_KEY, hat_keys[4*i + 2], qfalse, 0, NULL);
					comqueueevent(0, SE_KEY, hat_keys[4*i + 1], qfalse, 0, NULL);
					break;
				case SDL_HAT_LEFTUP:
					comqueueevent(0, SE_KEY, hat_keys[4*i + 0], qfalse, 0, NULL);
					comqueueevent(0, SE_KEY, hat_keys[4*i + 3], qfalse, 0, NULL);
					break;
				case SDL_HAT_LEFTDOWN:
					comqueueevent(0, SE_KEY, hat_keys[4*i + 2], qfalse, 0, NULL);
					comqueueevent(0, SE_KEY, hat_keys[4*i + 3], qfalse, 0, NULL);
					break;
				default:
					break;
				}
				/* press event */
				switch(((Uint8*)&hats)[i]){
				case SDL_HAT_UP:
					comqueueevent(0, SE_KEY, hat_keys[4*i + 0], qtrue, 0, NULL);
					break;
				case SDL_HAT_RIGHT:
					comqueueevent(0, SE_KEY, hat_keys[4*i + 1], qtrue, 0, NULL);
					break;
				case SDL_HAT_DOWN:
					comqueueevent(0, SE_KEY, hat_keys[4*i + 2], qtrue, 0, NULL);
					break;
				case SDL_HAT_LEFT:
					comqueueevent(0, SE_KEY, hat_keys[4*i + 3], qtrue, 0, NULL);
					break;
				case SDL_HAT_RIGHTUP:
					comqueueevent(0, SE_KEY, hat_keys[4*i + 0], qtrue, 0, NULL);
					comqueueevent(0, SE_KEY, hat_keys[4*i + 1], qtrue, 0, NULL);
					break;
				case SDL_HAT_RIGHTDOWN:
					comqueueevent(0, SE_KEY, hat_keys[4*i + 2], qtrue, 0, NULL);
					comqueueevent(0, SE_KEY, hat_keys[4*i + 1], qtrue, 0, NULL);
					break;
				case SDL_HAT_LEFTUP:
					comqueueevent(0, SE_KEY, hat_keys[4*i + 0], qtrue, 0, NULL);
					comqueueevent(0, SE_KEY, hat_keys[4*i + 3], qtrue, 0, NULL);
					break;
				case SDL_HAT_LEFTDOWN:
					comqueueevent(0, SE_KEY, hat_keys[4*i + 2], qtrue, 0, NULL);
					comqueueevent(0, SE_KEY, hat_keys[4*i + 3], qtrue, 0, NULL);
					break;
				default:
					break;
				}
			}
	}

	/* save hat state */
	stick_state.oldhats = hats;

	/* finally, look at the axes... */
	total = SDL_JoystickNumAxes(stick);
	if(total > 0){
		if(in_joystickUseAnalog->integer){
			if(total > MAX_JOYSTICK_AXIS)
				total = MAX_JOYSTICK_AXIS;
			for(i = 0; i < total; i++){
				Sint16 axis = SDL_JoystickGetAxis(stick, i);
				float f = ((float)abs(axis)) / 32767.0f;
				
				if(f < in_joystickThreshold->value)
					axis = 0;
				if(axis != stick_state.oldaaxes[i]){
					comqueueevent(0, SE_JOYSTICK_AXIS, i, axis, 0, NULL);
					stick_state.oldaaxes[i] = axis;
				}
			}
		}else{
			if(total > 16)
				total = 16;
			for(i = 0; i < total; i++){
				Sint16 axis = SDL_JoystickGetAxis(stick, i);
				float f = ((float)axis) / 32767.0f;
				if(f < -in_joystickThreshold->value){
					axes |= (1 << (i * 2));
				}else if( f > in_joystickThreshold->value ){
					axes |= (1 << ((i * 2) + 1));
				}
			}
		}
	}

	/* Time to update axes state based on old vs. new. */
	if(axes != stick_state.oldaxes){
		for(i = 0; i < 16; i++){
			if((axes & (1 << i)) && !(stick_state.oldaxes & (1 << i))){
				comqueueevent(0, SE_KEY, joy_keys[i], qtrue, 0, NULL);
			}

			if(!(axes & (1 << i)) && (stick_state.oldaxes & (1 << i))){
				comqueueevent(0, SE_KEY, joy_keys[i], qfalse, 0, NULL);
			}
		}
	}

	/* Save for future generations. */
	stick_state.oldaxes = axes;
}

static void
IN_ProcessEvents(void)
{
	SDL_Event	e;
	const char *character	= NULL;
	keyNum_t	key	= 0;

	if(!SDL_WasInit(SDL_INIT_VIDEO))
		return;

	if(Key_GetCatcher( ) == 0 && keyRepeatEnabled){
		SDL_EnableKeyRepeat(0, 0);
		keyRepeatEnabled = qfalse;
	}else if(!keyRepeatEnabled){
		SDL_EnableKeyRepeat(300, 31);
		keyRepeatEnabled = qtrue;
	}

	while(SDL_PollEvent(&e)){
		switch(e.type){
		case SDL_KEYDOWN:
			character = IN_TranslateSDLToQ3Key(&e.key.keysym, &key, qtrue);
			if(key)
				comqueueevent(0, SE_KEY, key, qtrue, 0, NULL);

			if(character)
				comqueueevent(0, SE_CHAR, *character, 0, 0, NULL);
			break;

		case SDL_KEYUP:
			IN_TranslateSDLToQ3Key(&e.key.keysym, &key, qfalse);

			if(key)
				comqueueevent(0, SE_KEY, key, qfalse, 0, NULL);
			break;

		case SDL_MOUSEMOTION:
			if(mouseActive)
				comqueueevent(0, SE_MOUSE, e.motion.xrel, e.motion.yrel, 0, NULL);
			break;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		{
			unsigned char b;
			switch(e.button.button){
			case 1:   b	= K_MOUSE1;     break;
			case 2:   b	= K_MOUSE3;     break;
			case 3:   b	= K_MOUSE2;     break;
			case 4:   b	= K_MWHEELUP;   break;
			case 5:   b	= K_MWHEELDOWN; break;
			case 6:   b	= K_MOUSE4;     break;
			case 7:   b	= K_MOUSE5;     break;
			default:  b	= K_AUX1 + (e.button.button - 8) % 16; break;
			}
			comqueueevent(0, SE_KEY, b,
				(e.type == SDL_MOUSEBUTTONDOWN ? qtrue : qfalse), 0, NULL);
		}
		break;

		case SDL_QUIT:
			cbufexecstr(EXEC_NOW, "quit Closed window\n");
			break;

		case SDL_VIDEORESIZE:
		{
			char width[32], height[32];
			Q_sprintf(width, sizeof(width), "%d", e.resize.w);
			Q_sprintf(height, sizeof(height), "%d", e.resize.h);
			cvarsetstr("r_customwidth", width);
			cvarsetstr("r_customheight", height);
			cvarsetstr("r_mode", "-1");
			/* wait until user stops dragging for 1 second, so
			 * we aren't constantly recreating the GL context while
			 * he tries to drag...*/
			vidRestartTime = sysmillisecs() + 1000;
		}
		break;
		case SDL_ACTIVEEVENT:
			if(e.active.state & SDL_APPINPUTFOCUS){
				cvarsetf("com_unfocused", !e.active.gain);
			}
			if(e.active.state & SDL_APPACTIVE){
				cvarsetf("com_minimized", !e.active.gain);
			}
			break;

		default:
			break;
		}
	}
}

void
IN_Frame(void)
{
	qbool loading;

	IN_JoyMove( );
	IN_ProcessEvents( );

	/* If not DISCONNECTED (main menu) or ACTIVE (in game), we're loading */
	loading = !!(clc.state != CA_DISCONNECTED && clc.state != CA_ACTIVE);

	if(!cvargeti("r_fullscreen") && (Key_GetCatcher( ) & KEYCATCH_CONSOLE)){
		/* Console is down in windowed mode */
		IN_DeactivateMouse( );
	}else if(!cvargeti("r_fullscreen") && loading){
		/* Loading in windowed mode */
		IN_DeactivateMouse( );
	}else if(!(SDL_GetAppState() & SDL_APPINPUTFOCUS)){
		/* Window not got focus */
		IN_DeactivateMouse( );
	}else
		IN_ActivateMouse( );

	/* in case we had to delay actual restart of video system... */
	if((vidRestartTime != 0) && (vidRestartTime < sysmillisecs())){
		vidRestartTime = 0;
		cbufaddstr("vid_restart");
	}
}

void
IN_InitKeyLockStates(void)
{
	unsigned char *keystate = SDL_GetKeyState(NULL);

	keys[K_SCROLLOCK].down	= keystate[SDLK_SCROLLOCK];
	keys[K_KP_NUMLOCK].down = keystate[SDLK_NUMLOCK];
	keys[K_CAPSLOCK].down	= keystate[SDLK_CAPSLOCK];
}

void
IN_Init(void)
{
	int appState;

	if(!SDL_WasInit(SDL_INIT_VIDEO)){
		comerrorf(ERR_FATAL, "IN_Init called before SDL_Init( SDL_INIT_VIDEO )");
		return;
	}

	comdprintf("\n------- Input Initialization -------\n");

	in_keyboardDebug = cvarget("in_keyboardDebug", "0", CVAR_ARCHIVE);

	/* mouse variables */
	in_mouse	= cvarget("in_mouse", "1", CVAR_ARCHIVE);
	in_nograb	= cvarget("in_nograb", "0", CVAR_ARCHIVE);

	in_joystick = cvarget("in_joystick", "0", CVAR_ARCHIVE|CVAR_LATCH);
	in_joystickDebug = cvarget("in_joystickDebug", "0", CVAR_TEMP);
	in_joystickThreshold = cvarget("joy_threshold", "0.15", CVAR_ARCHIVE);

#ifdef MACOS_X_ACCELERATION_HACK
	in_disablemacosxmouseaccel = cvarget("in_disablemacosxmouseaccel", "1", CVAR_ARCHIVE);
#endif

	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	keyRepeatEnabled = qtrue;

	mouseAvailable = (in_mouse->value != 0);
	IN_DeactivateMouse( );

	appState = SDL_GetAppState( );
	cvarsetf("com_unfocused", !(appState & SDL_APPINPUTFOCUS));
	cvarsetf("com_minimized", !(appState & SDL_APPACTIVE));

	IN_InitKeyLockStates( );

	IN_InitJoystick( );
	comdprintf("------------------------------------\n");
}

void
IN_Shutdown(void)
{
	IN_DeactivateMouse( );
	mouseAvailable = qfalse;

	IN_ShutdownJoystick( );
}

void
IN_Restart(void)
{
	IN_ShutdownJoystick( );
	IN_Init( );
}
