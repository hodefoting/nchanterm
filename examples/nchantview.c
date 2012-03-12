/* a rough terminal image viewer */

#include <stdio.h>  /* sprintf */
#include <math.h>
#include "stb_image.h"

#define NCHANTERM_HEADER_ONLY
#include "nchanterm.c"

#define NL         do {x = 2; y++;}while(0)
#define P(string)  do {x += nct_print (term, x, y, string, -1);}while(0)

#if 1
static char *quarter_blocks[]=
  {" ","▘","▝","▀","▖","▌","▞","▛","▗","▚","▐","▜","▄","▙","▟","█",
    "▎","▊","▂","▆", 
    "▎","▊","▂","▆", /* repeated, for the reverse video case */
    NULL};
static char *utf8_gray_scale[]={" ","░","▒","▓","█","█", NULL};
#else
static char *quarter_blocks[]=
  {" ","'","`","~",",","B","/","P",".","\\","C","q","m","b","d","M",NULL};
static char *utf8_gray_scale[]={" ","^","X","W","%","M", NULL};
#endif

static float rowerror[2000][3];
static float error[3];

static int frame = 0;

static int do_dither = 1;
static int cfg_mono = 0;
static int cfg_temporal = 0;
static float cfg_crisp = 0.2;

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
                           float r[16], float g[16], float b[16],
                           int dither, float crispness)
{
  float sum[3] = {0,0,0};
  int i;
  for (i=0; i<16; i++)
    {
      sum[0] += r[i]/16;
      sum[1] += g[i]/16;
      sum[2] += b[i]/16;
    }
  if (dither)
    {
      int i;
      for (i = 0; i < 3; i++)
        {
          sum[i]+=error[i];
          sum[i]+=rowerror[x][i];
          rowerror[x][i]=0;
        }
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
    float best_rgb[3];
    int use_geom = 0;

#define POW2(a) ((a)*(a))

    for (fg = 0; fg < 8; fg ++)
      for (bg = 0; bg < 8; bg ++)
      {
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
            if (distance <= best_distance)
              {
                int i;
                best_distance = distance;
                best_fg = fg;
                best_bg = bg;
                best_mix = mix;
                for (i = 0; i<3; i++)
                  best_rgb[i] = resrgb[i];
              }
          }
      }
    /* change to other equivalents that seems to behave better than
     * their correspondent
     */
#if 1
    if(1)if (best_bg == 7 && best_fg == 7)
      {
        best_fg = 7;
        best_bg = 0;
        best_mix = 1.0;
      }
      if(1)if (best_fg == 0 && best_mix >=1.0 )
      {
        best_bg = 0;
        best_fg = 7;
        best_mix = 0.0;
      }
#endif
  if (dither)
    {
      int i;
      for (i = 0; i < 3; i++)
        {
          float delta = (sum[i] - best_rgb[i]);
          error[i] = delta * 0.25;
          rowerror[x][i] += delta * 0.25;
          if (x>0)
            rowerror[x-1][i] += delta * 0.25;
          rowerror[x+1][i] += delta * 0.25;
        }
    }

  int bestbits = 0;
  {
    int totbits;
    for (totbits = 0; totbits < 24; totbits++)
        {
          float br[16],bg[16],bb[16];
          int i, x, y;
          float distance = 0;

          if (totbits < 16)
            {
              for (i = 0, y = 0; y < 4; y ++)
                for (x = 0; x < 4; x ++, i++)
                  {
                    br[i] = (totbits>>((y/2)*2+(x/2)) &1) ? ((best_fg & 1) != 0) : ((best_bg & 1) != 0);
                    bg[i] = (totbits>>((y/2)*2+(x/2)) &1) ? ((best_fg & 2) != 0) : ((best_bg & 2) != 0);
                    bb[i] = (totbits>>((y/2)*2+(x/2)) &1) ? ((best_fg & 4) != 0) : ((best_bg & 4) != 0);
                  }
            }
          else
            {
              switch (totbits)
                {
                  case 16:
                    for (i = 0, y = 0; y < 4; y ++)
                      for (x = 0; x < 4; x ++, i++)
                        {
                    br[i] = (x==0) ? ((best_fg & 1) != 0) : ((best_bg & 1) != 0);
                    bg[i] = (x==0) ? ((best_fg & 2) != 0) : ((best_bg & 2) != 0);
                    bb[i] = (x==0) ? ((best_fg & 4) != 0) : ((best_bg & 4) != 0);
                        }
                    break;
                  case 17:
                    for (i = 0, y = 0; y < 4; y ++)
                      for (x = 0; x < 4; x ++, i++)
                        {
                    br[i] = (x!=3) ? ((best_fg & 1) != 0) : ((best_bg & 1) != 0);
                    bg[i] = (x!=3) ? ((best_fg & 2) != 0) : ((best_bg & 2) != 0);
                    bb[i] = (x!=3) ? ((best_fg & 4) != 0) : ((best_bg & 4) != 0);
                        }
                    break;
                  case 18:
                    for (i = 0, y = 0; y < 4; y ++)
                      for (x = 0; x < 4; x ++, i++)
                        {
                    br[i] = (y==3) ? ((best_fg & 1) != 0) : ((best_bg & 1) != 0);
                    bg[i] = (y==3) ? ((best_fg & 2) != 0) : ((best_bg & 2) != 0);
                    bb[i] = (y==3) ? ((best_fg & 4) != 0) : ((best_bg & 4) != 0);
                        }
                        
                    break;
                  case 19:
                    for (i = 0, y = 0; y < 4; y ++)
                      for (x = 0; x < 4; x ++, i++)
                        {
                    br[i] = (y!=0) ? ((best_fg & 1) != 0) : ((best_bg & 1) != 0);
                    bg[i] = (y!=0) ? ((best_fg & 2) != 0) : ((best_bg & 2) != 0);
                    bb[i] = (y!=0) ? ((best_fg & 4) != 0) : ((best_bg & 4) != 0);
                        }
                    break;


                  case 20:
                    for (i = 0, y = 0; y < 4; y ++)
                      for (x = 0; x < 4; x ++, i++)
                        {
                    br[i] = (x!=0) ? ((best_fg & 1) != 0) : ((best_bg & 1) != 0);
                    bg[i] = (x!=0) ? ((best_fg & 2) != 0) : ((best_bg & 2) != 0);
                    bb[i] = (x!=0) ? ((best_fg & 4) != 0) : ((best_bg & 4) != 0);
                        }
                    break;
                  case 21:
                    for (i = 0, y = 0; y < 4; y ++)
                      for (x = 0; x < 4; x ++, i++)
                        {
                    br[i] = (x==3) ? ((best_fg & 1) != 0) : ((best_bg & 1) != 0);
                    bg[i] = (x==3) ? ((best_fg & 2) != 0) : ((best_bg & 2) != 0);
                    bb[i] = (x==3) ? ((best_fg & 4) != 0) : ((best_bg & 4) != 0);
                        }
                    break;
                  case 22:
                    for (i = 0, y = 0; y < 4; y ++)
                      for (x = 0; x < 4; x ++, i++)
                        {
                    br[i] = (y!=3) ? ((best_fg & 1) != 0) : ((best_bg & 1) != 0);
                    bg[i] = (y!=3) ? ((best_fg & 2) != 0) : ((best_bg & 2) != 0);
                    bb[i] = (y!=3) ? ((best_fg & 4) != 0) : ((best_bg & 4) != 0);
                        }
                        
                    break;
                  case 23:
                    for (i = 0, y = 0; y < 4; y ++)
                      for (x = 0; x < 4; x ++, i++)
                        {
                    br[i] = (y==0) ? ((best_fg & 1) != 0) : ((best_bg & 1) != 0);
                    bg[i] = (y==0) ? ((best_fg & 2) != 0) : ((best_bg & 2) != 0);
                    bb[i] = (y==0) ? ((best_fg & 4) != 0) : ((best_bg & 4) != 0);
                        }
                    break;



                }
            }

          for (i=0;i<16;i++)
            distance += sqrt (POW2(br[i] - r[i])+
                              POW2(bg[i] - g[i])+
                              POW2(bb[i] - b[i]));

          float GEOM_FACTOR = (1.000001 - crispness) / 3;

          distance = distance * GEOM_FACTOR / 16;

          if (distance <= best_distance)
            {
              best_distance = distance;
              use_geom = 1;
              bestbits = totbits;
            }
        }
  }

  /* XXX:
   * do another pass checking 1/4th filled permutations with the hbar/vbar
   * bits.
   */

  if (use_geom && bestbits >= 20)
    {
      nct_fg_color (n, best_bg);
      nct_bg_color (n, best_fg);
    }
  else
    {
      nct_fg_color (n, best_fg);
      nct_bg_color (n, best_bg);
    }
  if (use_geom)
    nct_set (n, x, y, quarter_blocks[bestbits]);
  else
    set_gray (n, x, y, best_mix);
  }
}

/*  draw a 32bit RGBA file,..
 */
void nct_buf (Nchanterm *n, int x0, int y0, int w, int h,
              unsigned char *buf, int rw, int rh,
              int dither, float crispness, int mono)
{
  int u, v;

  if (!buf)
    return;

  memset (rowerror, 0, sizeof (rowerror));
  memset (error, 0, sizeof (error));

  for (v = 0; v < h; v++)
    for (u = 0; u < w; u++)
      {
        float r[16], g[16], b[16];
        float xo, yo;
        int no = 0;
        for (yo = 0.0; yo <= 0.75; yo += 0.25)
          for (xo = 0.0; xo <= 0.75; xo += 0.25, no++)
            {
              int c = 0;

              /* do a set of samplings to get a crude higher
               * quality box down-sampler?, do this crunching
               * based on a scaling factor.
               */

              float uo = 0.0, vo = 0.0;
              r[no]=g[no]=b[no]=0.0;
              for (uo = 0.0 ; uo <= 0.5; uo+= 0.2)   /* these increments should be adjusted */
                for (vo = 0.0 ; vo <= 0.5; vo+= 0.2) /* according to scaling factor */
                  {
                    int x, y;
                    x = ((u+xo+uo + ((frame^12345)%3)*(1.0/3)/3) * 1.0 / w) * rw;
                    y = ((v+yo+vo + ((frame^12345)%7)*(1.0/7)/3) * 1.0 / h) * rh;
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
              if (mono)
                {
                  float v = (r[no] + g[no] + b[no])/3;
                  r[no] = g[no] = b[no] = v;
                }
            }
        draw_rgb_cell (n, x0+u, y0+v, r, g, b, dither, crispness);
      }
}

void nct_set_dither (Nchanterm *n, int dither)
{
  do_dither = dither;
}

void nct_set_crispness (Nchanterm *n, float crispness)
{
  cfg_crisp = crispness;
}

void nct_set_mono (Nchanterm *n, int mono)
{
  cfg_mono = mono;
}

void nct_image (Nchanterm *n, int x0, int y0, int w, int h, const char *path)
{
  int rw, rh;
  unsigned char *image= NULL;
  image = stbi_load (path, &rw, &rh, NULL, 4);
  if (!image)
    return;
  nct_buf (n, x0, y0, w, h, image, rw, rh, do_dither, cfg_crisp, cfg_mono);
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

  if (argv[2])
    {
      nct_set_dither (NULL, atoi(argv[2]));
      if (argv[3])
        {
          nct_set_crispness (NULL, atof(argv[3]));
          if (argv[4])
            nct_set_mono (NULL, 1);
        }
    }


  term = nct_new ();
  nct_clear (term);
  nct_image (term, 1, 1, nct_width (term), nct_height (term), argv[1]);
  nct_flush (term);
  //return 0;

  while (!quit)
    {
      const char *event;
      if (cfg_temporal)
        event = nct_get_event (term, 10, NULL, NULL);
      else
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
      else if (cfg_temporal)
        {
          nct_clear (term);
          nct_image (term, 1, 1, nct_width (term), nct_height (term), argv[1]);
          nct_flush (term);
          frame ++;
        }
    }
  nct_destroy (term);
  return 0;
}
