/* cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
#include "shared.h"
#include "client.h"
#include "ui.h"

qbool		scr_initialized;	/* ready to draw */

Cvar		*cl_timegraph;
Cvar		*cl_debuggraph;
Cvar		*cl_graphheight;
Cvar		*cl_graphscale;
Cvar		*cl_graphshift;

/* SCR_DrawNamedPic: Coordinates are 640*480 virtual values */
void
SCR_DrawNamedPic(float x, float y, float width, float height,
		 const char *picname)
{
	Handle hShader;

	assert(width != 0);

	hShader = re.RegisterShader(picname);
	SCR_AdjustFrom640(&x, &y, &width, &height);
	re.DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

/* SCR_AdjustFrom640: Adjusted for resolution and screen aspect ratio */
void
SCR_AdjustFrom640(float *x, float *y, float *w, float *h)
{
	float	xscale;
	float	yscale;
#if 0
	/* adjust for wide screens */
	if(cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640)
		*x += 0.5 *
		      (cls.glconfig.vidWidth -
		       (cls.glconfig.vidHeight * 640 / 480));

#endif
	/* scale for screen sizes */
	xscale	= cls.glconfig.vidWidth / 640.0;
	yscale	= cls.glconfig.vidHeight / 480.0;
	if(x)
		*x *= xscale;
	if(y)
		*y *= yscale;
	if(w)
		*w *= xscale;
	if(h)
		*h *= yscale;
}

/* SCR_FillRect: Coordinates are 640*480 virtual values */
void
SCR_FillRect(float x, float y, float width, float height, const float *color)
{
	re.SetColor(color);

	SCR_AdjustFrom640(&x, &y, &width, &height);
	re.DrawStretchPic(x, y, width, height, 0, 0, 0, 0, cls.whiteShader);

	re.SetColor(NULL);
}

/* SCR_DrawPic: Coordinates are 640*480 virtual values */
void
SCR_DrawPic(float x, float y, float width, float height, Handle hShader)
{
	SCR_AdjustFrom640(&x, &y, &width, &height);
	re.DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

/* SCR_DrawChar: chars are drawn at 640*480 virtual screen size */
static void
SCR_DrawChar(int x, int y, float size, int ch)
{
	int row, col;
	float	frow, fcol;
	float	ax, ay, aw, ah;

	ch &= 255;

	if(ch == ' ')
		return;

	if(y < -size)
		return;

	ax	= x;
	ay	= y;
	aw	= size;
	ah	= size;
	SCR_AdjustFrom640(&ax, &ay, &aw, &ah);

	row	= ch >> 4;
	col	= ch & 15;

	frow	= row * 0.0625;
	fcol	= col * 0.0625;
	size	= 0.0625;

	re.DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + size, frow + size,
		cls.charSetShader);
}

/* SCR_DrawSmallChar: small chars are drawn at native screen resolution */
void
SCR_DrawSmallChar(int x, int y, int ch)
{
	int row, col;
	float	frow, fcol;
	float	size;

	ch &= 255;

	if(ch == ' ')
		return;

	if(y < -SMALLCHAR_HEIGHT)
		return;

	row = ch >> 4;
	col = ch & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

	re.DrawStretchPic(x, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, fcol, frow,
		fcol + size, frow + size, cls.charSetShader);
}

/*
 * SCR_DrawBigString[Color]: Draws a multi-colored string with a drop shadow,
 * optionally forcing to a fixed color. Coordinates are at 640 by 480 virtual
 * resolution
 */
void
SCR_DrawStringExt(int x, int y, float size, const char *string, float *setColor,
		  qbool forceColor, qbool noColorEscape)
{
	Vec4	color;
	const char      *s;
	int	xx;

	/* draw the drop shadow */
	color[0] =
		color[1] =
			color[2] = 0;
	color[3] = setColor[3];
	re.SetColor(color);
	s = string;
	xx = x;
	while(*s){
		if(!noColorEscape && Q_IsColorString(s)){
			s += 2;
			continue;
		}
		SCR_DrawChar(xx+2, y+2, size, *s);
		xx += size;
		s++;
	}

	/* draw the colored text */
	s = string;
	xx = x;
	re.SetColor(setColor);
	while(*s){
		if(!noColorEscape && Q_IsColorString(s)){
			if(!forceColor){
				Q_Memcpy(color, g_color_table[ColorIndex(
									*(s+1))],
					sizeof(color));
				color[3] = setColor[3];
				re.SetColor(color);
			}
			s += 2;
			continue;
		}
		SCR_DrawChar(xx, y, size, *s);
		xx += size;
		s++;
	}
	re.SetColor(NULL);
}


void
SCR_DrawBigString(int x, int y, const char *s, float alpha,
		  qbool noColorEscape)
{
	float color[4];

	color[0] =
		color[1] =
			color[2] = 1.0;
	color[3] = alpha;
	SCR_DrawStringExt(x, y, BIGCHAR_WIDTH, s, color, qfalse, noColorEscape);
}

void
SCR_DrawBigStringColor(int x, int y, const char *s, Vec4 color,
		       qbool noColorEscape)
{
	SCR_DrawStringExt(x, y, BIGCHAR_WIDTH, s, color, qtrue, noColorEscape);
}

/*
 * SCR_DrawSmallString[Color]: Draws a multi-colored string with a drop shadow,
 * optionally forcing to a fixed color.
 */
void
SCR_DrawSmallStringExt(int x, int y, const char *string, float *setColor,
		       qbool forceColor, qbool noColorEscape)
{
	Vec4	color;
	const char      *s;
	int	xx;

	/* draw the colored text */
	s = string;
	xx = x;
	re.SetColor(setColor);
	while(*s){
		if(Q_IsColorString(s)){
			if(!forceColor){
				Q_Memcpy(color,
					g_color_table[ColorIndex(*(s+1))],
					sizeof(color));
				color[3] = setColor[3];
				re.SetColor(color);
			}
			if(!noColorEscape){
				s += 2;
				continue;
			}
		}
		SCR_DrawSmallChar(xx, y, *s);
		xx += SMALLCHAR_WIDTH;
		s++;
	}
	re.SetColor(NULL);
}

/* SCR_Strlen: skips color escape codes */
static int
SCR_Strlen(const char *str)
{
	const char *s = str;
	int count = 0;

	while(*s){
		if(Q_IsColorString(s))
			s += 2;
		else{
			count++;
			s++;
		}
	}
	return count;
}

int
SCR_GetBigStringWidth(const char *str)
{
	return SCR_Strlen(str) * BIGCHAR_WIDTH;
}

void
SCR_DrawDemoRecording(void)
{
	char	string[1024];
	int	pos;

	if(!clc.demorecording)
		return;
	if(clc.spDemoRecording)
		return;

	pos = fsftell(clc.demofile);
	sprintf(string, "RECORDING %s: %ik", clc.demoName, pos / 1024);

	SCR_DrawStringExt(320 - strlen(
			string) * 4, 20, 8, string, g_color_table[7], qtrue,
		qfalse);
}

#ifdef USE_VOIP
void
SCR_DrawVoipMeter(void)
{
	char	buffer[16];
	char	string[256];
	int	limit, i;

	if(!cl_voipShowMeter->integer)
		return;		/* player doesn't want to show meter at all. */
	else if(!cl_voipSend->integer)
		return;		/* not recording at the moment. */
	else if(clc.state != CA_ACTIVE)
		return;		/* not connected to a server. */
	else if(!clc.voipEnabled)
		return;		/* server doesn't support VoIP. */
	else if(clc.demoplaying)
		return;		/* playing back a demo. */
	else if(!cl_voip->integer)
		return;		/* client has VoIP support disabled. */

	limit = (int)(clc.voipPower * 10.0f);
	if(limit > 10)
		limit = 10;

	for(i = 0; i < limit; i++)
		buffer[i] = '*';
	while(i < 10)
		buffer[i++] = ' ';
	buffer[i] = '\0';

	sprintf(string, "VoIP: [%s]", buffer);
	SCR_DrawStringExt(320 - strlen(string) * 4, 10, 8, string,
		g_color_table[7], qtrue, qfalse);
}
#endif

/*
 * DEBUG GRAPH
 */
static int current;
static float values[1024];

void
scrdebuggraph(float value)
{
	values[current] = value;
	current = (current + 1) % ARRAY_LEN(values);
}

void
SCR_DrawDebugGraph(void)
{
	int a, x, y, w, i, h;
	float v;

	/* draw the graph */
	w	= cls.glconfig.vidWidth;
	x	= 0;
	y	= cls.glconfig.vidHeight;
	re.SetColor(g_color_table[0]);
	re.DrawStretchPic(x, y - cl_graphheight->integer,
		w, cl_graphheight->integer, 0, 0, 0, 0, cls.whiteShader);
	re.SetColor(NULL);

	for(a = 0; a < w; ++a){
		i = (ARRAY_LEN(values) + current-1 - (a % ARRAY_LEN(values)))
		    % ARRAY_LEN(values);
		v	= values[i];
		v	= v * cl_graphscale->integer + cl_graphshift->integer;

		if(v < 0)
			v += cl_graphheight->integer *
			     (1 + (int)(-v / cl_graphheight->integer));
		h = (int)v % cl_graphheight->integer;
		re.DrawStretchPic(x+w-1-a, y - h, 1, h, 0, 0, 0, 0,
			cls.whiteShader);
	}
}

void
SCR_Init(void)
{
	cl_timegraph = cvarget ("timegraph", "0", CVAR_CHEAT);
	cl_debuggraph	= cvarget ("debuggraph", "0", CVAR_CHEAT);
	cl_graphheight	= cvarget ("graphheight", "32", CVAR_CHEAT);
	cl_graphscale	= cvarget ("graphscale", "1", CVAR_CHEAT);
	cl_graphshift	= cvarget ("graphshift", "0", CVAR_CHEAT);

	scr_initialized = qtrue;
}

/* SCR_DrawScreenField: This will be called twice if rendering in stereo mode */
void
SCR_DrawScreenField(Stereoframe stereoFrame)
{
	qbool uiFullscreen;

	re.BeginFrame(stereoFrame);

	uiFullscreen = (uivm && vmcall(uivm, UI_IS_FULLSCREEN));

	/*
	 * wide aspect ratio screens need to have the sides cleared unless they
	 * are displaying game renderings
	 */
	if(uiFullscreen || (clc.state != CA_ACTIVE && clc.state != CA_CINEMATIC))
		if(cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640){
			re.SetColor(g_color_table[0]);
			re.DrawStretchPic(0, 0, cls.glconfig.vidWidth,
				cls.glconfig.vidHeight, 0, 0, 0, 0,
				cls.whiteShader);
			re.SetColor(NULL);
		}

	/*
	 * if the menu is going to cover the entire screen, we don't need to
	 * render anything under it
	 * */
	if(uivm && !uiFullscreen){
		switch(clc.state){
		default:
			comerrorf(ERR_FATAL, "SCR_DrawScreenField: bad clc.state");
			break;
		case CA_CINEMATIC:
			SCR_DrawCinematic();
			break;
		case CA_DISCONNECTED:
			/* force menu up */
			sndstopall();
			vmcall(uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN);
			break;
		case CA_CONNECTING:
		case CA_CHALLENGING:
		case CA_CONNECTED:
			/* connecting clients will only show the connection dialog
			 * refresh to update the time */
			vmcall(uivm, UI_REFRESH, cls.realtime);
			vmcall(uivm, UI_DRAW_CONNECT_SCREEN, qfalse);
			break;
		case CA_LOADING:
		case CA_PRIMED:
			/* draw the game information screen and loading progress */
			CL_CGameRendering(stereoFrame);

			/* also draw the connection information, so it doesn't
			 * flash away too briefly on local or lan games
			 * refresh to update the time */
			vmcall(uivm, UI_REFRESH, cls.realtime);
			vmcall(uivm, UI_DRAW_CONNECT_SCREEN, qtrue);
			break;
		case CA_ACTIVE:
			/* always supply STEREO_CENTER as vieworg offset is now done by the engine. */
			CL_CGameRendering(stereoFrame);
			SCR_DrawDemoRecording();
#ifdef USE_VOIP
			SCR_DrawVoipMeter();
#endif
			break;
		}
	}

	/* the menu draws next */
	if(Key_GetCatcher() & KEYCATCH_UI && uivm)
		vmcall(uivm, UI_REFRESH, cls.realtime);

	/* console draws next */
	Con_DrawConsole();

	/* debug graph can be drawn on top of anything */
	if(cl_debuggraph->integer || cl_timegraph->integer || cl_debugMove->integer)
		SCR_DrawDebugGraph ();
}

/*
 * SCR_UpdateScreen: This is called every frame, and can also be called
 * explicitly to flush text to the screen.
 */
void
SCR_UpdateScreen(void)
{
	static int recursive;

	if(!scr_initialized)
		return;		/* not initialized yet */

	if(++recursive > 2)
		comerrorf(ERR_FATAL, "SCR_UpdateScreen: recursively called");
	recursive = 1;

	/* If there is no VM, there are also no rendering commands issued. Stop the renderer in
	 * that case. */
	if(uivm || com_dedicated->integer){
		/* XXX */
		int in_anaglyphMode = cvargeti("r_anaglyphMode");
		/* if running in stereo, we need to draw the frame twice */
		if(cls.glconfig.stereoEnabled || in_anaglyphMode){
			SCR_DrawScreenField(STEREO_LEFT);
			SCR_DrawScreenField(STEREO_RIGHT);
		}else
			SCR_DrawScreenField(STEREO_CENTER);

		if(com_speeds->integer)
			re.EndFrame(&time_frontend, &time_backend);
		else
			re.EndFrame(NULL, NULL);
	}
	recursive = 0;
}
