#include <stdio.h>  /* sprintf */

#define NCHANTERM_HEADER_ONLY
#include "nchanterm.c"

#define NL         do {x = 2; y++;}while(0)
#define P(string)  do {x += nct_print (term, x, y, string, -1);}while(0)

int main (int argc, char **argv)
{
  Nchanterm *term = nct_new ();
  const char *event = "";
  int ex = 0, ey = 0;
  int quit = 0;

  nct_mouse (term, NC_MOUSE_DRAG);
  while (!quit)
    {
      int x = 1 , y = 1;
      nct_clear (term);

      NL;
      P ("This is a tiny terminal abstraction");
      NL;
      NL;
      nct_set_attr (term, NCT_A_REVERSE);
      P ("event");
      x+=2;
      nct_set_attr (term, NCT_A_NORMAL);
      P (event);
      x++;
      if (ex >0) {
        char buf[100];
        sprintf (buf, "%i %i", ex, ey);
        P(buf);
      }
      NL;
      NL;


      nct_set_attr (term, NCT_A_BOLD);P("bold");
      nct_set_attr (term, NCT_A_UNDERLINE);P("underline");
      nct_set_attr (term, NCT_A_REVERSE);P("reverse");
      nct_set_attr (term, NCT_A_DIM);P("dim");

      NL;
      NL;

      nct_set_attr (term, NCT_A_DIM);
      { /* draw all fg/bg combinations */
        int fg, bg;
        for (fg = 0; fg < 8; fg++)
          for (bg = 0; bg < 8; bg++)
            {
              nct_fg_color (term, fg);
              nct_bg_color (term, bg);
              nct_set (term, fg+3 + 9 * 0, bg+y, "X");
            }
      }
      nct_set_attr (term, NCT_A_NORMAL);
      { /* draw all fg/bg combinations */
        int fg, bg;
        for (fg = 0; fg < 8; fg++)
          for (bg = 0; bg < 8; bg++)
            {
              nct_fg_color (term, fg);
              nct_bg_color (term, bg);
              nct_set (term, fg+3 + 9 * 1, bg+y, "X");
            }
      }
      nct_set_attr (term, NCT_A_BOLD);
      { /* draw all fg/bg combinations */
        int fg, bg;
        for (fg = 0; fg < 8; fg++)
          for (bg = 0; bg < 8; bg++)
            {
              nct_fg_color (term, fg);
              nct_bg_color (term, bg);
              nct_set (term, fg+3 + 9 * 2, bg+y, "X");
            }
      }
      nct_flush (term);

      do
      {
        event = nct_get_event (term, 5000, &ex, &ey);

        if (!strcmp (event, "control-c")||
            !strcmp (event, "esc"))
          quit = 1;
        else if (!strcmp (event, "size-changed"))
          nct_set_size (term, nct_sys_terminal_width (), 
                              nct_sys_terminal_height ());
        else if (!strcmp (event, "control-l"))
          {
            event = "forced redraw";
            nct_reflush (term);
          }
      } while (nct_has_event (term, 15));
    }
  nct_destroy (term);
  return 0;
}
