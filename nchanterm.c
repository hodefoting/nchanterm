/* nchanterm, a minimal ANSI based utf8 terminal control abstraction.
 *
 * Copyright (c) 2012 Øyvind Kolås <pippin@gimp.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef NCHANTERM_H
#define NCHANTERM_H

typedef struct _Nchanterm Nchanterm;

/* create a new terminal instance, automatically sized to initial size
 * of terminal.
 */
Nchanterm  *nct_new      (void);
/* free up a terminal instance */
void        nct_destroy  (Nchanterm *term);
/* set the size of the terminal, you need to do this in resize-callbacks */
void        nct_set_size (Nchanterm *term, int width, int height);
/* query size of terminal */
int         nct_width    (Nchanterm *term);
int         nct_height   (Nchanterm *term);
/* clear terminal contents to ' ', reset attributes and colors */
void        nct_clear    (Nchanterm *term);
/* set a utf8 char from the buffer str at location x, y with current style */
void        nct_set      (Nchanterm *term, int x, int y, const char *str);
/* read back the utf8 string in a character cell (XXX: style readback missing)*/
const char *nct_get      (Nchanterm *term, int x, int y);
/* print a string at given coordinates, only num_chars first chars printed */
int         nct_print    (Nchanterm *term, int x, int y,
                          const char *string, int num_chars);
enum {
  NCT_COLOR_BLACK  = 0,
  NCT_COLOR_RED    = 1,
  NCT_COLOR_GREEN  = 2,
  NCT_COLOR_YELLOW = 3,
  NCT_COLOR_BLUE   = 4,
  NCT_COLOR_MAGENTA= 5,
  NCT_COLOR_CYAN   = 6,
  NCT_COLOR_WHITE  = 7
};
/* set the foreground/background color used */
void nct_fg_color (Nchanterm *term, int color);
void nct_bg_color (Nchanterm *term, int color);

enum {
  NCT_A_NORMAL     = 0,
  NCT_A_BOLD       = 1 << 0,
  NCT_A_DIM        = 1 << 1,
  NCT_A_UNDERLINE  = 1 << 2,
  NCT_A_REVERSE    = 1 << 3,
};

/* set the attribute mode, this is a bitmask combination of the above modes. */
void nct_set_attr            (Nchanterm *term, int attr);
/* get current attribute mode.  */
int  nct_get_attr            (Nchanterm *term);

/* update what is visible to the user */
void nct_flush               (Nchanterm *term);

/* force a full redraw */
void nct_reflush             (Nchanterm *term);

/* get the width and height of the terminal querying the system */
int  nct_sys_terminal_width  (void);
int  nct_sys_terminal_height (void);

/* toggle visibility of the cursor */
void nct_show_cursor         (Nchanterm *term);
void nct_hide_cursor         (Nchanterm *term);

/* set position of cursor */
void nct_set_cursor_pos      (Nchanterm *term, int  x, int  y);

/* get position of cursor */
void nct_get_cursor_pos      (Nchanterm *term, int *x, int *y);

/* input handling
 *   nchanteds input handling reports keyboard, window resizing and mouse
 *   events in a uniform string based API. UTF8 input is passed through
 *   as the strings of the glyphs as the event string.
 *
 *  "size-changed"   - the geometry of the terminal has changed
 *  "mouse-pressed"  - the left mouse button was clicked
 *  "mouse-motion"   - motion events (level of motion event reporting
 *                    should be configurable)
 *  "idle"           - timeout elapsed.
 *  keys: "up" "down" "space" "control-left" "return" "esc" "a" "A"
 *        "control-a" "F1" "control-c" ...
 */

/* get an event as a string from the terminal, if it is a mouse event coords
 * will be returned in x and y if not NULL, block for up to timeout_ms  */
const char *nct_get_event     (Nchanterm *n, int timeout_ms, int *x, int *y);

/* check if there is pending events, with a timeout */
int         nct_has_event (Nchanterm *n, int timeout_ms);

/* get a human readable/suited for ui string representing a keybinding */
const char *nct_key_get_label (Nchanterm *n, const char *nick);

enum {
  NC_MOUSE_NONE  = 0,
  NC_MOUSE_PRESS = 1,  /* "mouse-pressed", "mouse-released" */
  NC_MOUSE_DRAG  = 2,  /* + "mouse-drag"   (motion with pressed button) */
  NC_MOUSE_ALL   = 3   /* + "mouse-motion" (also delivered for release) */
};
/* set which mouse events to report, defaults to report none */
void nct_mouse           (Nchanterm *term, int nct_mouse_state);
#endif

/************************** end of header ***********************/
#ifndef NCHANTERM_HEADER_ONLY

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <locale.h>

int nct_sys_terminal_width (void)
{
  struct winsize ws; 
  if (ioctl(0,TIOCGWINSZ,&ws)!=0)
    return 80;
  return ws.ws_col;
} 

int nct_sys_terminal_height (void)
{
  struct winsize ws; 
  if (ioctl(0,TIOCGWINSZ,&ws)!=0)
    return 25;
  return ws.ws_row;
}

typedef struct NcCell
{ 
  char  str[8]; /* utf8 representation of char */
  int   attr;   /* attributes */
  int   color;  /* both fg and bg */
} NcCell;

struct _Nchanterm {
  NcCell  *cells_front;
  NcCell  *cells_back;
  int      cells_width;
  int      cells_height;
  int      mode;
  int      color;
  int      mode_set;
  int      color_set;
  int      width;
  int      height;
  int      cursor_x;
  int      cursor_y;
  int      utf8;
  int      is_st;
};

/* a quite minimal core set of terminal escape sequences are used to do all
 * things nchanterm can do */
#define ANSI_RESET_DEVICE        "\ec"
#define ANSI_YX                  "\e[%d;%dH"
#define ANSI_CURSOR_FORWARD      "\e[%dC"
#define ANSI_CURSOR_FORWARD1     "\e[C"
#define ANSI_STYLE_RESET         "\e[m"
#define ANSI_STYLE_START         "\e["
#define ANSI_STYLE_END           "m"
#define DECTCEM_CURSOR_SHOW      "\e[?25h"
#define DECTCEM_CURSOR_HIDE      "\e[?25l"
#define TERMINAL_MOUSE_OFF       "\e[?1000l"
#define TERMINAL_MOUSE_ON_BASIC  "\e[?1000h"
#define TERMINAL_MOUSE_ON_DRAG   "\e[?1000h\e[?1003h" /* +ON_BASIC for wider */
#define TERMINAL_MOUSE_ON_FULL   "\e[?1000h\e[?1004h" /* compatibility */

void nct_show_cursor (Nchanterm *t)
{
  printf ("%s", DECTCEM_CURSOR_SHOW);
}
void nct_hide_cursor (Nchanterm *t)
{
  printf ("%s", DECTCEM_CURSOR_HIDE);
}

static int nct_utf8_len (const unsigned char first_byte)
{
  if      ((first_byte & 0x80) == 0)
    return 1; /* ASCII */
  else if ((first_byte & 0xE0) == 0xC0)
    return 2;
  else if ((first_byte & 0xF0) == 0xE0)
    return 3;
  else if ((first_byte & 0xF8) == 0xF0)
    return 4;
  return 1;
}

static int nct_utf8_strlen (const char *s)
{
   int count;
   if (!s)
     return 0;
   for (count = 0; *s; s++)
     if ((*s & 0xC0) != 0x80)
       count++;
   return count;
}

void nct_set_size      (Nchanterm *n, int width, int height)
{
  n->width = width;
  n->height = height;
  nct_set_cursor_pos (n, width, height);
}

int nct_width (Nchanterm *n)
{
  return n->width;
} 
int nct_height (Nchanterm *n)
{
  return n->height;
} 

#define COLOR_PAIR(f,b)       (f * 8 + b)
#define COLOR_FG(p)           (((int)p)/8)
#define COLOR_BG(p)           (((int)p)%8)
#define NCHANT_DEFAULT_COLORS (COLOR_PAIR(NCT_COLOR_WHITE, NCT_COLOR_BLACK))

static void nct_ensure_mode (Nchanterm *n)
{
  int first = 1;
  int data = 0;

  if (n->mode_set == n->mode && n->color_set == n->color)
    return;
  n->mode_set  = n->mode;
  n->color_set = n->color;

  printf(ANSI_STYLE_RESET);
#define SEPERATOR  { if (!data) { printf(ANSI_STYLE_START); data = 1;} \
                     if (first) first=0; else printf (";");}
  if (n->mode & NCT_A_BOLD)     { SEPERATOR; printf ("1"); }
  if ((n->mode & NCT_A_DIM) && !n->is_st) { SEPERATOR; printf ("2");}
  if (n->mode & NCT_A_REVERSE)  { SEPERATOR; printf ("7"); }
  if (n->mode & NCT_A_UNDERLINE){ SEPERATOR; printf ("4"); }
  if (n->color != NCHANT_DEFAULT_COLORS)
    { SEPERATOR; 
      if (COLOR_BG(n->color) == 0)
      printf ("%i", COLOR_FG(n->color)+30);
      else
      printf ("%i;%i", COLOR_FG(n->color)+30,
                       COLOR_BG(n->color)+40); }
  if (data)
    printf(ANSI_STYLE_END);
#undef SEPERATOR
}

void nct_set_attr (Nchanterm *n, int attr)
{
  if (n->mode != attr)
      n->mode = attr;
}

int nct_get_attr (Nchanterm *n)
{
  return n->mode;
}

void nct_fg_color (Nchanterm *n, int ncolor)
{
  n->color -= COLOR_FG (n->color) * 8;
  n->color += ncolor * 8;
}

void nct_bg_color (Nchanterm *n, int ncolor)
{
  n->color -= COLOR_BG (n->color);
  n->color += ncolor;
}

int nct_print (Nchanterm *n, int x, int y, const char *string, int utf8_length)
{
  const char *s;
  int len;
  int pos = 0;
  if (!string)
    return 0;
  if (utf8_length < 0)
    utf8_length = nct_utf8_strlen (string);

  for (s = string; pos < utf8_length && *s; s += nct_utf8_len (*s))
    {
      int c;
      len = nct_utf8_len (*s);
      nct_set (n, x++, y, s);
      for (c = 0; c < len; c++)
        if (!s[c])
          return pos;
      pos++;
    }
  return pos;
}

static void nct_cells_cls (Nchanterm *n)
{
  int i;
  for (i = 0; i < n->cells_width * n->cells_height; i ++)
    {
      n->cells_back[i].str[0]=' ';
      n->cells_back[i].str[1]='\0';
      n->cells_back[i].attr = 0;
      n->cells_back[i].color = NCHANT_DEFAULT_COLORS;
    }
}

void nct_clear (Nchanterm *n)
{
  n->color = NCHANT_DEFAULT_COLORS;
  nct_set_attr (n, NCT_A_NORMAL);
  nct_cells_cls (n);
}

static void nct_cells_cls_front (Nchanterm *n)
{
  int i;
  for (i = 0; i < n->cells_width * n->cells_height; i ++)
    n->cells_front[i].str[0]='\2'; 
}

static void nct_cells_ensure (Nchanterm *n)
{
  int w = n->width;
  int h = n->height;
  if (w != n->cells_width || h != n->cells_height)
    {
      if (n->cells_front)
        free (n->cells_front);
      if (n->cells_back)
        free (n->cells_back);
      n->cells_width = w;
      n->cells_height = h;
      n->cells_front = calloc (sizeof (NcCell) * n->cells_width * n->cells_height, 1);
      n->cells_back  = calloc (sizeof (NcCell) * n->cells_width * n->cells_height, 1);
      nct_cells_cls (n);
      nct_cells_cls_front (n);
    }
}

static NcCell *nct_get_cell (Nchanterm *n, int x, int y)
{
  nct_cells_ensure (n);
  if (x < 1) x = 1;
  if (y < 1) y = 1;
  if (x > n->cells_width) x = n->cells_width;
  if (y > n->cells_height) y = n->cells_height;
  return &n->cells_back[(y-1) * n->cells_width + (x-1)];
}

static NcCell *nct_get_front_cell (Nchanterm *n, int x, int y)
{
  nct_cells_ensure (n);
  if (x < 1) x = 1;
  if (y < 1) y = 1;
  if (x > n->cells_width) x = n->cells_width;
  if (y > n->cells_height) y = n->cells_height;
  return &n->cells_front[(y-1) * n->cells_width + (x-1)];
}

static void _nct_set_attr (Nchanterm *n, int x, int y, int attr, int color)
{
  NcCell *cell = nct_get_cell (n, x, y);
  cell->attr   = attr;
  cell->color  = color;
}

void nct_set (Nchanterm *n, int x, int y, const char *str)
{
  int     i, bytes = nct_utf8_len (*str);
  NcCell *cell     = nct_get_cell (n, x, y);

  if (x <= 0 || y <= 0 ||
      x > n->cells_width || y > n->cells_height)
    return;

  for (i = 0; i < bytes; i ++)
    cell->str[i] = str[i];
  cell->str[bytes] = 0;

  if (!n->utf8) /* strip down to ascii if utf8 */
    {
      if (cell->str[0] & 0x80)
          cell->str[0] = '?';
      cell->str[1] = 0;
    }
  _nct_set_attr (n, x, y, n->mode, n->color);
}

const char *nct_get (Nchanterm *n, int x, int y)
{
  NcCell *cell = nct_get_cell (n, x, y);
  return cell->str;
}

void nct_flush (Nchanterm *n)
{
  int x, y;
  int w = n->cells_width;
  int h = n->cells_height;
  static int had_full = -1;
  int cx=-1, cy=-1;
  nct_cells_ensure (n);

  if (had_full <=0)
    {
      had_full = 40; // do a full refresh every now and then.
      nct_cells_cls_front (n);
    }
  else
    {
      had_full--;
    }

  for (y = 1; y <= h; y ++)
    for (x = 1; x <= w; x ++)
      {
        NcCell *back  = nct_get_cell (n, x, y);
        NcCell *front = nct_get_front_cell (n, x, y);

        if (memcmp (back, front, sizeof (NcCell)))
          {
            n->color = back->color;
            nct_set_attr (n, back->attr);
            nct_ensure_mode (n);

            if (y != cy)
              printf (ANSI_YX, y, x);
            else if (x > cx)
              {
                if (x - cx == 1)
                  printf (ANSI_CURSOR_FORWARD1);
                else
                  printf (ANSI_CURSOR_FORWARD, (x - cx));
              }
            else 
              printf (ANSI_YX, y, x);

            printf ("%s", back->str);
            cx = x + 1;
            cy = y;
            memcpy (front, back, sizeof (NcCell));
          }
      }
  /* reset to defaults, to be in a better terminal state for any
   * potential ctrl+c or similar
   */
  n->color = NCHANT_DEFAULT_COLORS;
  nct_set_attr (n, NCT_A_NORMAL);
  nct_ensure_mode (n);
  printf (ANSI_YX, n->cursor_y, n->cursor_x);
  fflush (NULL);
}

void nct_reflush             (Nchanterm *n)
{
  nct_cells_cls_front (n);
  nct_flush (n);
}

Nchanterm *nct_new  (void)
{
  Nchanterm *term = calloc (sizeof (Nchanterm), 1);
  const char *locale = setlocale (LC_ALL, "");
  const char *term_env  = getenv ("TERM");
  if (strstr (locale, "utf8")  || strstr (locale, "UTF8")  ||
      strstr (locale, "UTF-8") || strstr (locale, "utf-8"))
    term->utf8 = 1;
  if (strstr (term_env,  "Eterm"))
    term->utf8 = 0;

  /* some special casing to avoid sending unknown commands to st  */
  if (!strcmp (term_env, "st-256color") || !strcmp (term_env, "st"))
    term->is_st = 1;
  nct_set_size (term, nct_sys_terminal_width (), nct_sys_terminal_height ());
  return term;
}

void nct_destroy  (Nchanterm *n)
{
  free (n);
}

void nct_set_cursor_pos (Nchanterm *n,  int x, int  y)
{
  n->cursor_x = x;
  n->cursor_y = y;
}

void nct_get_cursor_pos (Nchanterm *n, int *x, int *y)
{
  if (x) *x = n->cursor_x;
  if (y) *y = n->cursor_y;
}
/*************************** input handling *************************/

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define DELAY_MS  100  /* internal select timeout for inputs, this
                         is the minimal resolution possible to use -
                         this also causes this many wakeups per second
                         for the process..
                       */

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

static int  size_changed = 0;       /* XXX: global state */
static int  signal_installed = 0;   /* XXX: global state */

static const char *mouse_modes[]=
{TERMINAL_MOUSE_OFF,
 TERMINAL_MOUSE_ON_BASIC,
 TERMINAL_MOUSE_ON_DRAG,
 TERMINAL_MOUSE_ON_FULL,
 NULL};

/* note that a nick can have multiple occurences, the labels
 * should be kept the same for all occurences of a combination. */
typedef struct NcKeyCode {
  char *nick;          /* programmers name for key */
  char *label;         /* utf8 label for key */
    char  sequence[10];  /* terminal sequence */
} NcKeyCode;
static const NcKeyCode keycodes[]={  
  {"up",                  "↑",     "\e[A"},
  {"down",                "↓",     "\e[B"},
  {"right",               "→",     "\e[C"},
  {"left",                "←",     "\e[D"},

  {"shift-up",            "⇧↑",    "\e[1;2A"},
  {"shift-down",          "⇧↓",    "\e[1;2B"},
  {"shift-right",         "⇧→",    "\e[1;2C"},
  {"shift-left",          "⇧←",    "\e[1;2D"},

  {"alt-up",              "^↑",    "\e[1;3A"},
  {"alt-down",            "^↓",    "\e[1;3B"},
  {"alt-right",           "^→",    "\e[1;3C"},
  {"alt-left",            "^←",    "\e[1;3D"},

  {"alt-shift-up",        "alt-s↑", "\e[1;4A"},
  {"alt-shift-down",      "alt-s↓", "\e[1;4B"},
  {"alt-shift-right",     "alt-s→", "\e[1;4C"},
  {"alt-shift-left",      "alt-s←", "\e[1;4D"},

  {"control-up",          "^↑",    "\e[1;5A"},
  {"control-down",        "^↓",    "\e[1;5B"},
  {"control-right",       "^→",    "\e[1;5C"},
  {"control-left",        "^←",    "\e[1;5D"},

  /* putty */
  {"control-up",          "^↑",    "\eOA"},
  {"control-down",        "^↓",    "\eOB"},
  {"control-right",       "^→",    "\eOC"},
  {"control-left",        "^←",    "\eOD"},

  {"control-shift-up",    "^⇧↑",   "\e[1;6A"},
  {"control-shift-down",  "^⇧↓",   "\e[1;6B"},
  {"control-shift-right", "^⇧→",   "\e[1;6C"},
  {"control-shift-left",  "^⇧←",   "\e[1;6D"},

  {"control-up",          "^↑",    "\eOa"},
  {"control-down",        "^↓",    "\eOb"},
  {"control-right",       "^→",    "\eOc"},
  {"control-left",        "^←",    "\eOd"},

  {"shift-up",            "⇧↑",    "\e[a"},
  {"shift-down",          "⇧↓",    "\e[b"},
  {"shift-right",         "⇧→",    "\e[c"},
  {"shift-left",          "⇧←",    "\e[d"},

  {"insert",              "ins",   "\e[2~"},
  {"delete",              "del",   "\e[3~"},
  {"page-up",             "PgUp",  "\e[5~"},
  {"page-down",           "PdDn",  "\e[6~"},
  {"home",                "Home",  "\eOH"},
  {"end",                 "End",   "\eOF"},
  {"home",                "Home",  "\e[H"},
  {"end",                 "End",   "\e[F"},
  {"control-delete",      "^del",  "\e[3;5~"},
  {"shift-delete",        "⇧del",  "\e[3;2~"},
  {"control-shift-delete","^⇧del", "\e[3;6~"},

  {"F1",        "F1",  "\e[11~"},
  {"F2",        "F2",  "\e[12~"},
  {"F3",        "F3",  "\e[13~"},
  {"F4",        "F4",  "\e[14~"},
  {"F1",        "F1",  "\eOP"},
  {"F2",        "F2",  "\eOQ"},
  {"F3",        "F3",  "\eOR"},
  {"F4",        "F4",  "\eOS"},
  {"F5",        "F5",  "\e[15~"},
  {"F6",        "F6",  "\e[16~"},
  {"F7",        "F7",  "\e[17~"},
  {"F8",        "F8",  "\e[18~"},
  {"F9",        "F9",  "\e[19~"},
  {"F9",        "F9",  "\e[20~"},
  {"F10",       "F10", "\e[21~"},
  {"F11",       "F11", "\e[22~"},
  {"F12",       "F12", "\e[23~"},
  {"tab",       "↹",  {9, '\0'}},
  {"backspace", "⌫",  {127, '\0'}},
  {"space",     "␣",   " "},
  {"esc",        "␛",  "\e"},
  {"return",    "⏎",  {10,0}},
  {"return",    "⏎",  {13,0}},
  /* this section could be autogenerated by code */
  {"control-a", "^A",  {1,0}},
  {"control-b", "^B",  {2,0}},
  {"control-c", "^C",  {3,0}},
  {"control-d", "^D",  {4,0}},
  {"control-e", "^E",  {5,0}},
  {"control-f", "^F",  {6,0}},
  {"control-g", "^G",  {7,0}},
  {"control-h", "^H",  {8,0}}, /* backspace? */
  {"control-i", "^I",  {9,0}},
  {"control-j", "^J",  {10,0}},
  {"control-k", "^K",  {11,0}},
  {"control-l", "^L",  {12,0}},
  {"control-n", "^N",  {14,0}},
  {"control-o", "^O",  {15,0}},
  {"control-p", "^P",  {16,0}},
  {"control-q", "^Q",  {17,0}},
  {"control-r", "^R",  {18,0}},
  {"control-s", "^S",  {19,0}},
  {"control-t", "^T",  {20,0}},
  {"control-u", "^U",  {21,0}},
  {"control-v", "^V",  {22,0}},
  {"control-w", "^W",  {23,0}},
  {"control-x", "^X",  {24,0}},
  {"control-y", "^Y",  {25,0}},
  {"control-z", "^Z",  {26,0}},
  {"alt-0",     "%0",  "\e0"},
  {"alt-1",     "%1",  "\e1"},
  {"alt-2",     "%2",  "\e2"},
  {"alt-3",     "%3",  "\e3"},
  {"alt-4",     "%4",  "\e4"},
  {"alt-5",     "%5",  "\e5"},
  {"alt-6",     "%6",  "\e6"},
  {"alt-7",     "%7",  "\e7"}, /* backspace? */
  {"alt-8",     "%8",  "\e8"},
  {"alt-9",     "%9",  "\e9"},
  {"alt-+",     "%+",  "\e+"},
  {"alt--",     "%-",  "\e-"},
  {"alt-/",     "%/",  "\e/"},
  {"alt-a",     "%A",  "\ea"},
  {"alt-b",     "%B",  "\eb"},
  {"alt-c",     "%C",  "\ec"},
  {"alt-d",     "%D",  "\ed"},
  {"alt-e",     "%E",  "\ee"},
  {"alt-f",     "%F",  "\ef"},
  {"alt-g",     "%G",  "\eg"},
  {"alt-h",     "%H",  "\eh"}, /* backspace? */
  {"alt-i",     "%I",  "\ei"},
  {"alt-j",     "%J",  "\ej"},
  {"alt-k",     "%K",  "\ek"},
  {"alt-l",     "%L",  "\el"},
  {"alt-n",     "%N",  "\em"},
  {"alt-n",     "%N",  "\en"},
  {"alt-o",     "%O",  "\eo"},
  {"alt-p",     "%P",  "\ep"},
  {"alt-q",     "%Q",  "\eq"},
  {"alt-r",     "%R",  "\er"},
  {"alt-s",     "%S",  "\es"},
  {"alt-t",     "%T",  "\et"},
  {"alt-u",     "%U",  "\eu"},
  {"alt-v",     "%V",  "\ev"},
  {"alt-w",     "%W",  "\ew"},
  {"alt-x",     "%X",  "\ex"},
  {"alt-y",     "%Y",  "\ey"},
  {"alt-z",     "%Z",  "\ez"},
  /* Linux Console  */
  {"home",      "Home", "\e[1~"},
  {"end",       "End",  "\e[4~"},
  {"F1",        "F1",   "\e[[A"},
  {"F2",        "F2",   "\e[[B"},
  {"F3",        "F3",   "\e[[C"},
  {"F4",        "F4",   "\e[[D"},
  {"F5",        "F5",   "\e[[E"},
  {"F6",        "F6",   "\e[[F"},
  {"F7",        "F7",   "\e[[G"},
  {"F8",        "F8",   "\e[[H"},
  {"F9",        "F9",   "\e[[I"},
  {"F10",       "F10",  "\e[[J"},
  {"F11",       "F11",  "\e[[K"},
  {"F12",       "F12",  "\e[[L"}, 
  {NULL, }
};

static struct termios orig_attr; /* in order to restore at exit */
static int    nc_is_raw = 0;
static int    atexit_registered = 0;
static int    mouse_mode = NC_MOUSE_NONE;

static void _nc_noraw (void)
{
  if (nc_is_raw && tcsetattr (STDIN_FILENO, TCSAFLUSH, &orig_attr) != -1)
    nc_is_raw = 0;
}

static void
nc_at_exit (void)
{
  printf(TERMINAL_MOUSE_OFF);
  _nc_noraw();
  if (mouse_mode)
    printf(TERMINAL_MOUSE_OFF);
}

static int _nc_raw (void)
{
  struct termios raw;
  if (!isatty (STDIN_FILENO))
    return -1;
  if (!atexit_registered)
    {
      atexit (nc_at_exit);
      atexit_registered = 1;
    }
  if (tcgetattr (STDIN_FILENO, &orig_attr) == -1)
    return -1;
  raw = orig_attr;  /* modify the original mode */
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */
  if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &raw) < 0)
    return -1;
  nc_is_raw = 1;
  fflush (NULL);
  return 0;
}

static int match_keycode (const char *buf, int length, const NcKeyCode **ret)
{
  int i;
  int matches = 0;

  if (!strncmp (buf, "\e[M", MIN(length,3)))
    {
      if (length >= 6)
        return 9001;
      return 2342;
    }
  for (i = 0; keycodes[i].nick; i++)
    if (!strncmp (buf, keycodes[i].sequence, length))
      {
        matches ++;
        if (strlen (keycodes[i].sequence) == length && ret)
          {
            *ret = &keycodes[i];
            return 1;
          }
      }
  if (matches != 1 && ret)
    *ret = NULL;
  return matches==1?2:matches;
}

static void nc_resize_term (int dummy)
{
  size_changed = 1;
}

int nct_has_event (Nchanterm *n, int delay_ms)
{
  struct timeval tv;
  int retval;
  fd_set rfds;

  if (size_changed)
    return 1;
  FD_ZERO (&rfds);
  FD_SET (STDIN_FILENO, &rfds);
  tv.tv_sec = 0; tv.tv_usec = delay_ms * 1000; 
  retval = select (1, &rfds, NULL, NULL, &tv);
  if (size_changed)
    return 1;
  return retval == 1 && retval != -1;
}

const char *nct_get_event (Nchanterm *n, int timeoutms, int *x, int *y)
{
  unsigned char buf[20];
  int length;

  if (x) *x = -1;
  if (y) *y = -1;

  if (!signal_installed)
    {
      _nc_raw ();
      signal_installed = 1;
      signal (SIGWINCH, nc_resize_term);
    }
  if (mouse_mode)
    printf(mouse_modes[mouse_mode]);

  {
    int elapsed = 0;
    int got_event = 0;

    do {
      if (size_changed)
        {
          size_changed = 0;
          return "size-changed";
        }
      got_event = nct_has_event (n, MIN(DELAY_MS, timeoutms-elapsed));
      if (size_changed)
        {
          size_changed = 0;
          return "size-changed";
        }
      /* only do this if the client has asked for idle events,
       * and perhaps programmed the ms timer?
       */
      elapsed += MIN(DELAY_MS, timeoutms-elapsed);
      if (!got_event && timeoutms && elapsed >= timeoutms)
        return "idle";
    } while (!got_event);
  }

  for (length = 0; length < 10; length ++)
    if (read (STDIN_FILENO, &buf[length], 1) != -1)
      {
        const NcKeyCode *match = NULL;

        /* special case ESC, so that we can use it alone in keybindings */
        if (length == 0 && buf[0] == 27)
          {
            struct timeval tv;
            fd_set rfds;
            FD_ZERO (&rfds);
            FD_SET (STDIN_FILENO, &rfds);
            tv.tv_sec = 0;
            tv.tv_usec = 1000 * DELAY_MS;
            if (select (1, &rfds, NULL, NULL, &tv) == 0)
              return "esc";
          }

        switch (match_keycode ((void*)buf, length + 1, &match))
          {
            case 1: /* unique match */
              if (!match)
                return NULL;
              return match->nick;
              break;
            case 9001: /* mouse event */
              if (x) *x = ((unsigned char)buf[4]-32)*1.0;
              if (y) *y = ((unsigned char)buf[5]-32)*1.0;
              switch (buf[3])
                {
                  case 32: return "mouse-press";
                  case 33: return "mouse1-press";
                  case 34: return "mouse2-press";
                  case 40: return "alt-mouse-press";
                  case 41: return "alt-mouse1-press";
                  case 42: return "alt-mouse2-press";
                  case 48: return "control-mouse-press";
                  case 49: return "control-mouse1-press";
                  case 50: return "control-mouse2-press";
                  case 56: return "alt-control-mouse-press";
                  case 57: return "alt-control-mouse1-press";
                  case 58: return "alt-control-mouse2-press";
                  case 64: return "mouse-drag";
                  case 65: return "mouse1-drag";
                  case 66: return "mouse2-drag";
                  case 71: return "mouse-motion"; /* shift+motion */
                  case 72: return "alt-mouse-drag";
                  case 73: return "alt-mouse1-drag";
                  case 74: return "alt-mouse2-drag";
                  case 75: return "mouse-motion"; /* alt+motion */
                  case 80: return "control-mouse-drag";
                  case 81: return "control-mouse1-drag";
                  case 82: return "control-mouse2-drag";
                  case 83: return "mouse-motion"; /* ctrl+motion */
                  case 91: return "mouse-motion"; /* ctrl+alt+motion */
                  case 95: return "mouse-motion"; /* ctrl+alt+shift+motion */
                  case 96: return "scroll-up";
                  case 97: return "scroll-down";
                  case 100: return "shift-scroll-up";
                  case 101: return "shift-scroll-down";
                  case 104: return "alt-scroll-up";
                  case 105: return "alt-scroll-down";
                  case 112: return "control-scroll-up";
                  case 113: return "control-scroll-down";
                  case 116: return "control-shift-scroll-up";
                  case 117: return "control-shift-scroll-down";
                  case 35: /* (or release) */
                  case 51: /* (or ctrl-release) */
                  case 43: /* (or alt-release) */
                  case 67: return "mouse-motion";
                           /* have a separate mouse-drag ? */
                  default: {
                             static char rbuf[100];
                             sprintf (rbuf, "mouse (unhandled state: %i)", buf[3]);
                             return rbuf;
                           }
                }
            case 0: /* no matches, bail*/
              { 
                static char ret[256];
                if (length == 0 && nct_utf8_len (buf[0])>1) /* single unicode
                                                               char */
                  {
                    read (STDIN_FILENO, &buf[length+1], nct_utf8_len(buf[0])-1);
                    buf[nct_utf8_len(buf[0])]=0;
                    strcpy (ret, (void*)buf);
                    return ret;
                  }
                if (length == 0) /* ascii */
                  {
                    buf[1]=0;
                    strcpy (ret, (void*)buf);
                    return ret;
                  }
                sprintf (ret, "unhandled %i:'%c' %i:'%c' %i:'%c' %i:'%c' %i:'%c' %i:'%c' %i:'%c'",
                    length >=0 ? buf[0] : 0, 
                    length >=0 ? buf[0]>31?buf[0]:'?' : ' ', 
                    length >=1 ? buf[1] : 0, 
                    length >=1 ? buf[1]>31?buf[1]:'?' : ' ', 
                    length >=2 ? buf[2] : 0, 
                    length >=2 ? buf[2]>31?buf[2]:'?' : ' ', 
                    length >=3 ? buf[3] : 0, 
                    length >=3 ? buf[3]>31?buf[3]:'?' : ' ',
                    length >=4 ? buf[4] : 0, 
                    length >=4 ? buf[4]>31?buf[4]:'?' : ' ',
                    length >=5 ? buf[5] : 0, 
                    length >=5 ? buf[5]>31?buf[5]:'?' : ' ',
                    length >=6 ? buf[6] : 0, 
                    length >=6 ? buf[6]>31?buf[6]:'?' : ' '
                    );
                return ret;
              }
              return NULL;
            default: /* continue */
              break;
          }
      }
    else
      return "key read eek";
  return "fail";
}

const char *nct_key_get_label (Nchanterm *n, const char *nick)
{
  int j;
  int found = -1;
  for (j = 0; keycodes[j].nick; j++)
    if (found == -1 && !strcmp (keycodes[j].nick, nick))
      return keycodes[j].label;
  return NULL;
}

void nct_mouse (Nchanterm *term, int mode)
{
  if (term->is_st && mode > 1)
    mode = 1;
  if (mode != mouse_mode)
    printf (mouse_modes[mode]);
  mouse_mode = mode;
}
#endif
