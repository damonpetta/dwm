#include "gaplessgrid.c"
#include "selfrestart.c"
#include "zoomswap.c"
#include "moveresize.c"

/* mbp-mappings */
#define XF86AudioMute           0x1008ff12
#define XF86AudioLowerVolume    0x1008ff11
#define XF86AudioRaiseVolume    0x1008ff13
#define XF86TouchpadToggle      0x1008ffa9

static const char dmenufont[]             = "Inconsolata:pixelsize=18";
static const char *fonts[] = {
    "Inconsolata:pixelsize=14",
    "FontAwesome:size=10",
    "Siji:size=14",
};

static const unsigned int borderpx          = 1;    /* border pixel of windows */
static const unsigned int snap              = 10;   /* snap pixel */
static const unsigned int tagpadding        = 13;
static const unsigned int tagspacing        = 5;
static const unsigned int gappx             = 12;   /* gap pixel between windows */
static const unsigned int systraypinning    = 0;    /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayspacing    = 2;    /* systray spacing */
static const int systraypinningfailfirst    = 1;    /* 1: if pinning fails, display systray on the first monitor, 0: display systray on the last monitor*/
static const int showsystray                = 1;    /* 0 means no systray */
static const int showbar                    = 1;    /* 0 means no bar */
static const int topbar                     = 1;    /* 0 means bottom bar */

#define NUMCOLORS 9
static const char colors[NUMCOLORS][MAXCOLORS][9] = {
    // border    foreground background
    { "#f9f9f9", "#f5f5f5", "#03070b" }, // 0 = normal
    { "#000000", "#f5f5f5", "#4b1a2c" }, // 1 = selected
    { "#b43030", "#f5f5f5", "#b23450" }, // 2 = red / urgent
    { "#212121", "#f5f5f5", "#14161A" }, // 3 = green / occupied
    { "#212121", "#ab7438", "#0b0606" }, // 4 = yellow
    { "#212121", "#475971", "#0b0606" }, // 5 = blue
    { "#212121", "#694255", "#0b0606" }, // 6 = cyan
    { "#212121", "#3e6868", "#0b0606" }, // 7 = magenta
    { "#212121", "#cfa696", "#0b0606" }, // 8 = grey
};

static const char *tags[] = {
  " ",
  " ",
  " ",
  " ",
  " ",
  " ",
  " ",
  " ",
  " ",
  " "
};

static const Rule rules[] = {
    /* xprop(1):
     *  WM_CLASS(STRING) = instance, class
     *  WM_NAME(STRING) = title
     */
    /* class          instance  title                                tags mask  * isfloating      monitor */
    { "st",           NULL,     NULL,                                 1 << 1,         0,          -1 },
    { "MAIL",         NULL,     NULL,                                 1 << 2,         0,          -1 },
    { "Slack",        NULL,     NULL,                                 1 << 9,         0,          -1 },
    { NULL,           NULL,     "Google Hangouts",                    1 << 5,         1,          -1 },
    { "Lxappearance", NULL,     NULL,                                 0,              1,          -1 },
};

/* layout(s) */
static const float mfact        = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster        = 1;    /* number of clients in master area */
static const int resizehints    = 0;    /* 1 means respect size hints in tiled resizals */

static const Layout layouts[] = {
    /* symbol     arrange function */
    { "\ue002",      tile },    /* first entry is default */
    { "\ue006",      NULL },    /* no layout function means floating behavior */
    { "\ue000",      monocle },
    { "\ue003",      htile },
    { "\ue00a",      gaplessgrid },
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
    { MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
    { MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2]             = "0"; /* component of dmenucmd, manipulated in spawn() */
//static const char *dmenucmd[]       = { "dmenu_run", NULL };
//-nb '#03070b' -sb '#4b1a2c'
static const char *dmenucmd[]       = { "dmenu_run", "-fn", dmenufont, "-nb", "#03070b", "-nf", "#f5f5f5", "-sb", "#4b1a2c", "-sf", "#f5f5f5", NULL };
static const char *termcmd[]        = { "st", NULL };
static const char *volup[]          = { "pulseaudio-ctl", "up", NULL };
static const char *voldown[]        = { "pulseaudio-ctl", "down", NULL };
static const char *voltoggle[]      = { "pulseaudio-ctl", "mute", NULL };
static const char *togtouchpad[]    = { "touchpad", NULL };
static const char *screenshot[]     = { "screenshot-selection", NULL};
static const char scratchpadname[]  = "Google Hangouts - damon@petta.org";
static const char *scratchpadcmd[]  = { "hangout_personal", NULL };


static Key keys[] = {
    /* modifier           key                    function            * * * argument */
    { MODKEY,             XK_r,                  spawn,              {.v = dmenucmd } },
    { 0|ControlMask,        XK_space,              spawn,              {.v = dmenucmd } },
    { MODKEY,             XK_Return,             spawn,              {.v = termcmd } },
    { MODKEY|ShiftMask,   XK_b,                  togglebar,          {0} },
    { MODKEY,             XK_minus,              togglescratch,      {.v = scratchpadcmd } },
    { MODKEY,             XK_Right,              focusstack,         {.i = +1 } },
    { MODKEY,             XK_Left,               focusstack,         {.i = -1 } },
    { MODKEY,             XK_i,                  incnmaster,         {.i = +1 } },
    { MODKEY,             XK_d,                  incnmaster,         {.i = -1 } },
    { MODKEY|ShiftMask,   XK_Left,               setmfact,           {.f = -0.05} },
    { MODKEY|ShiftMask,   XK_Right,              setmfact,           {.f = +0.05} },
    { MODKEY|ShiftMask,   XK_Up,                 setcfact,           {.f = +0.25} },
    { MODKEY|ShiftMask,   XK_Down,               setcfact,           {.f = -0.25} },
    { MODKEY|ShiftMask,   XK_o,                  setcfact,           {.f =  0.00} },
    { MODKEY,             XK_Down,               pushdown,           {0} },
    { MODKEY,             XK_Up,                 pushup,             {0} },
    { MODKEY|ShiftMask,   XK_Return,             zoom,               {0} },
    { MODKEY,             XK_Tab,                view,               {0} },
    { MODKEY|ShiftMask,   XK_w,                  killclient,         {0} },
    { MODKEY,             XK_t,                  setlayout,          {.v = &layouts[0]} },
    { MODKEY,             XK_f,                  setlayout,          {.v = &layouts[1]} },
    { MODKEY,             XK_m,                  setlayout,          {.v = &layouts[2]} },
    { MODKEY,             XK_b,                  setlayout,          {.v = &layouts[3]} },
    { MODKEY,             XK_g,                  setlayout,          {.v = &layouts[4]} },
    { MODKEY,             XK_space,              setlayout,          {0} },
    { MODKEY,             XK_u,                  togglefullscreen,   {0} },
    { MODKEY|ShiftMask,   XK_space,              togglefloating,     {0} },
    { MODKEY,             XK_equal,              view,               {.ui = ~0 } },
    { MODKEY|ShiftMask,   XK_equal,              tag,                {.ui = ~0 } },
    { MODKEY,             XK_comma,              focusmon,           {.i = -1 } },
    { MODKEY,             XK_period,             focusmon,           {.i = +1 } },
    { MODKEY|ShiftMask,   XK_comma,              tagmon,             {.i = -1 } },
    { MODKEY|ShiftMask,   XK_period,             tagmon,             {.i = +1 } },
    { Mod4Mask,           XK_Up,                 moveresize,         {.v = "0x -25y 0w 0h"} },
    { Mod4Mask,           XK_Down,               moveresize,         {.v = "0x 25y 0w 0h"} },
    { Mod4Mask,           XK_Left,               moveresize,         {.v = "-25x 0y 0w 0h"} },
    { Mod4Mask,           XK_Right,              moveresize,         {.v = "25x 0y 0w 0h"} },
    { Mod4Mask|ShiftMask, XK_Up,                 moveresize,         {.v = "0x 0y 0w -25h"} },
    { Mod4Mask|ShiftMask, XK_Down,               moveresize,         {.v = "0x 0y 0w 25h"} },
    { Mod4Mask|ShiftMask, XK_Left,               moveresize,         {.v = "0x 0y -25w 0h"} },
    { Mod4Mask|ShiftMask, XK_Right,              moveresize,         {.v = "0x 0y 25w 0h"} },
    TAGKEYS(              XK_1,                  0)
    TAGKEYS(              XK_2,                  1)
    TAGKEYS(              XK_3,                  2)
    TAGKEYS(              XK_4,                  3)
    TAGKEYS(              XK_5,                  4)
    TAGKEYS(              XK_6,                  5)
    TAGKEYS(              XK_7,                  6)
    TAGKEYS(              XK_8,                  7)
    TAGKEYS(              XK_9,                  8)
    TAGKEYS(              XK_0,                  9)
    { Mod1Mask|ShiftMask, XK_4,             spawn,              {.v = screenshot } },
    { MODKEY|ShiftMask,   XK_r,                  self_restart,       {0} },
    { MODKEY|ShiftMask,   XK_q,                  quit,               {0} },
    { 0,                  XK_Print,              spawn,              {.v = screenshot } },
    { 0,                  XF86AudioRaiseVolume,  spawn,              {.v = volup } },
    { 0,                  XF86AudioLowerVolume,  spawn,              {.v = voldown } },
    { 0,                  XF86AudioMute,         spawn,              {.v = voltoggle } },
    { 0,                  XF86TouchpadToggle,    spawn,              {.v = togtouchpad } },
};

/* button definitions */
/* click can be ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
    /* click              event mask  button          function            * * argument */
    { ClkLtSymbol,        0,          Button1,        setlayout,          {0} },
    { ClkLtSymbol,        0,          Button3,        setlayout,          {.v = &layouts[2]} },
    { ClkStatusText,      0,          Button2,        spawn,              {.v = termcmd } },
    { ClkClientWin,       MODKEY,     Button1,        movemouse,          {0} },
    { ClkClientWin,       MODKEY,     Button2,        togglefloating,     {0} },
    { ClkClientWin,       MODKEY,     Button3,        resizemouse,        {0} },
    { ClkTagBar,          0,          Button1,        view,               {0} },
    { ClkTagBar,          0,          Button3,        toggleview,         {0} },
    { ClkTagBar,          MODKEY,     Button1,        tag,                {0} },
    { ClkTagBar,          MODKEY,     Button3,        toggletag,          {0} },
};
