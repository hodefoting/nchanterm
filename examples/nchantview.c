/* a rough terminal image viewer */

#include <stdio.h>  /* sprintf */
#include <math.h>
#include "stb_image.h"

#define NCHANTERM_HEADER_ONLY
#include "nchanterm.c"

#define NL         do {x = 2; y++;}while(0)
#define P(string)  do {x += nct_print (term, x, y, string, -1);}while(0)

static char *qblocks[]={" ", "▘", "▝", "▀", "▖", "▌", "▞", "▛", "▗", "▚", "▐", "▜", "▄", "▙", "▟", "█", NULL};
static char *utf8_gray_scale[]={" ","░","▒","▓","█","█", NULL};

void set_gray (Nchanterm *n, int x, int y, float value)
{
  int i = value * 5.0;
  if (i > 5)
    i = 5;
  if (i < 0)
    i = 0;
  nct_set (n, x, y, utf8_gray_scale[i]);
}

/* draws with 4 input subpixels per output subpixel, the routine is
 * written to deal with an input that has 2x2 input pixels per
 * output pixel.
 */
static void draw_rgb_cell (Nchanterm *n, int x, int y,
                           float r[4], float g[4], float b[4])
{
  float sum[3] = {0,0,0};
  int i;
  for (i=0; i<4; i++)
    {
      sum[0] += r[i]/4;
      sum[1] += g[i]/4;
      sum[2] += b[i]/4;
    }
  {
    /* go through all fg/bg combinations with all color mixtures and
     * compare with our desired average color output
     */
    int fg, bg;
    float mix;

    int best_fg = 7;
    int best_bg = 0;
    float best_mix = 1.0;
    float best_distance = 10000000.0;
    int use_geom = 0;

#define POW2(a) ((a)*(a))

    for (fg = 0; fg < 8; fg ++)
      for (bg = 0; bg < 8; bg ++)
        for (mix = 0.0; mix <= 1.0; mix+=0.25)
          {
            int frgb[3] = { (fg & 1)!=0,
                            (fg & 2)!=0,
                            (fg & 4)!=0};
            int brgb[3] = { (bg & 1)!=0,
                            (bg & 2)!=0,
                            (bg & 4)!=0};
            float resrgb[3];
            float distance;
            int c;
            for (c = 0; c < 3; c++)
              resrgb[c] = (frgb[c] * mix + brgb[c] * (1.0-mix)) ;

            distance = sqrt (POW2(resrgb[0] - sum[0])+
                             POW2(resrgb[1] - sum[1])+
                             POW2(resrgb[2] - sum[2]));
            if (distance < best_distance)
              {
                best_distance = distance;
                best_fg = fg;
                best_bg = bg;
                best_mix = mix;
              }
          }
    if (best_mix <= 0.0) /* prefer to draw blocks than to not do so */
      {
        int tmp = best_fg;
        best_fg = best_bg;
        best_bg = tmp;
        best_mix = 1.0-best_mix;
      }
    if (best_bg == 7 && best_fg == 0)
      {
        int tmp = best_fg;
        best_fg = best_bg;
        best_bg = tmp;
        best_mix = 1.0-best_mix;
      }
    if (best_bg == 0 && best_mix <= 0.0)
      {
        best_fg = 7;
        best_bg = 0;
        best_mix = 0;
      }

    //best_distance = 1000;
  int bestbits = 0;
  {
    int totbits;
    for (totbits = 0; totbits < 16; totbits++)
        {
          float br[4],bg[4],bb[4];
          int i;
          float distance = 0;

          for (i=0;i<4;i++)
            {
              br[i] = ((totbits >> (i))&1) ? ((best_fg & 1) != 0) :
                                             ((best_bg & 1) != 0);
              bg[i] = ((totbits >> (i))&1) ? ((best_fg & 2) != 0) :
                                             ((best_bg & 2) != 0);
              bb[i] = ((totbits >> (i))&1) ? ((best_fg & 4) != 0) :
                                             ((best_bg & 4) != 0);
            }

          for (i=0;i<4;i++)
            distance += sqrt (POW2(br[i] - r[i])+
                              POW2(bg[i] - g[i])+
                              POW2(bb[i] - b[i]));
#define GEOM_FACTOR  0.28
          if (distance/4 * GEOM_FACTOR < best_distance)
            {
              best_distance = distance/4 * GEOM_FACTOR;
              use_geom = 1;
              bestbits = totbits;
            }
        }
  }

  /* XXX:
   * do another pass checking 1/4th filled permutations,
   * these should give better large curves.
   */

  nct_fg_color (n, best_fg);
  nct_bg_color (n, best_bg);
  if (use_geom)
    nct_set (n, x, y, qblocks[bestbits]);
  else
    set_gray (n, x, y, best_mix);
  }
}

/*  draw a 32bit RGBA file,..
 */
void nct_buf (Nchanterm *n, int x0, int y0, int w, int h,
              unsigned char *buf, int rw, int rh)
{
  int u, v;

  if (!buf)
    return;

  for (u = 0; u < w; u++)
    for (v = 0; v < h; v++)
      {
        float r[4], g[4], b[4];
        float xo, yo;
        int no = 0;
        for (yo = 0.0; yo <= 0.5; yo += 0.5)
          for (xo = 0.0; xo <= 0.5; xo += 0.5, no++)
            {
              int c = 0;

              /* do a set of samplings to get a crude higher
               * quality box down-sampler?, do this crunching
               * based on a scaling factor.
               */

              float uo = 0.0, vo = 0.0;
              r[no]=g[no]=b[no]=0.0;
              //for (uo = 0.0 ; uo <= 0.5; uo+= 0.1)
              //for (vo = 0.0 ; vo <= 0.5; vo+= 0.1)
              //
              // using nearest neighbour is best for non photos..
              // we do not want the added AA for crisp edges
                  {
                    int x, y;
                    x = ((u+xo+uo) * 1.0 / w) * rw;
                    y = ((v+yo+vo) * 1.0 / h) * rh;
                    if (x<0) x = 0;
                    if (y<0) y = 0;
                    if (x>=rw) x = rw-1;
                    if (y>=rh) y = rh-1;
                      
                    r[no] += buf [(y * rw + x) * 4 + 0] / 255.0;
                    g[no] += buf [(y * rw + x) * 4 + 1] / 255.0;
                    b[no] += buf [(y * rw + x) * 4 + 2] / 255.0;
                    c++;
                  }
              r[no] /= c;
              g[no] /= c;
              b[no] /= c;
            }
        draw_rgb_cell (n, x0+u, y0+v, r, g, b);
      }
}

void nct_image (Nchanterm *n, int x0, int y0, int w, int h, const char *path)
{
  int rw, rh;
  unsigned char *image= NULL;
  image = stbi_load (path, &rw, &rh, NULL, 4);
  if (!image)
    return;
  nct_buf (n, x0, y0, w, h, image, rw, rh);
  free (image);
}

int main (int argc, char **argv)
{
  Nchanterm *term;
  int quit = 0;

  if (!argv[1])
    {
      printf ("usage:\nnchantview <imagefile>\n");
      return -1;
    }
  term = nct_new ();
  nct_clear (term);
  nct_image (term, 1, 1, nct_width (term), nct_height (term), argv[1]);
  nct_flush (term);

  while (!quit)
    {
      const char *event;
      event = nct_get_event (term, 1000, NULL, NULL);
      if (!strcmp (event, "control-c")||
          !strcmp (event, "esc"))
        quit = 1;
      else if (!strcmp (event, "size-changed"))
        {
          nct_set_size (term, nct_sys_terminal_width (), 
                              nct_sys_terminal_height ());
          nct_clear (term);
          nct_image (term, 1, 1, nct_width (term), nct_height (term), argv[1]);
          nct_flush (term);
        }
      else if (!strcmp (event, "control-l"))
        nct_reflush (term);
    }
  nct_destroy (term);
  return 0;
}
