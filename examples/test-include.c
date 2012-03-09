#include <stdio.h>  /* sprintf */

#include "nchanterm.c"

int main (int argc, char **argv)
{
  Nchanterm *term = nct_new ();
  const char *event = "";
  int ex = 0, ey = 0;
  int quit = 0;

  nct_mouse (term, NC_MOUSE_DRAG);
  while (!quit)
    {
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


        int x = 3 , y = 3;
        nct_clear (term);

        x += nct_print (term, x, y, "event:", -1);
        x ++;
        x += nct_print (term, x, y, event, -1);

        if (ex >0) {
          char buf[100];
          sprintf (buf, "%i %i", ex, ey);
          x += nct_print (term, x, y, buf, -1);
        }
      }
      while (nct_has_event (term, 0));
    }
  nct_destroy (term);
  return 0;
}
