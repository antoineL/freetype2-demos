/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-2000, 2003, 2004 by                                      */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  FTView - a simple font viewer.                                          */
/*                                                                          */
/*  This is a new version using the MiGS graphics subsystem for             */
/*  blitting and display.                                                   */
/*                                                                          */
/*  Press F1 when running this program to have a list of key-bindings       */
/*                                                                          */
/****************************************************************************/


#include "ftcommon.i"
#include <math.h>



static void
do_rect( int  x,
         int  y,
         int  w,
         int  h,
         int  gray )
{
  unsigned char*  line = bit.buffer + y*bit.pitch;

  if ( bit.pitch < 0 )
    line -= bit.pitch*(bit.rows-1);

  line += 3*x;

  if ( gray >= 0 )
  {
    for ( ; h > 0; h--, line += bit.pitch )
      memset( line, gray, 3*w );
  }
  else
  {
    for ( ; h > 0; h--, line+= bit.pitch )
    {
      int             w2 = w;
      unsigned char*  dst = line;

      for ( ; w2 > 0; w2--, dst += 3 )
      {
        int  color = ((w2+h) & 1)*255;

        dst[0] = dst[1] = dst[2] = (unsigned char)color;
      }
    }
  }
}


static FT_Error
Render_GammaGrid( void )
{
  int   g;
  int   xmargin = 10;
  int   levels  = 1;
  int   gamma_first = 16;
  int   gamma_last  = 26;
  int   gammas      = gamma_last - gamma_first + 1;
  int   xside       = (bit.width-100)/gammas - xmargin;
  int   yside       = (bit.rows-100)/2;
  int   yrepeat     = 1;

  int   x0      = (bit.width - gammas*(xside+xmargin)+xmargin)/2;
  int   y0      = (bit.rows  - (8+yside*2*yrepeat))/2;
  int   pitch   = bit.pitch;


  if ( pitch < 0 )
    pitch = -pitch;

#if 1
  memset( bit.buffer, 255, pitch*bit.rows );
#else
 /* fill the background with a simple pattern corresponding to 50%
  * linear gray from a reasonnable viewing distance
  */
  {
    int             nx, ny;
    unsigned char*  line = bit.buffer;
    if ( bit.pitch < 0 )
      line -= (bit.pitch*(bit.rows-1));

    for ( ny = 0; ny < bit.rows; ny++, line += bit.pitch )
    {
      unsigned char*  dst = line;
      int             nx;

      for ( nx = 0; nx < bit.width; nx++, dst += 3 )
      {
        int  color = ((nx+ny) & 1)*255;

        dst[0] = dst[1] = dst[2] = (unsigned char)color;
      }
    }
  }
#endif

  grGotobitmap( &bit );

  for ( g = gamma_first; g <= gamma_last; g += 1 )
  {
    double gamma = g/10.0;
    char   temp[6];
    int    x = x0 + (xside+xmargin)*(g-gamma_first);
    int    y = y0;
    int    nx, ny;

    grSetPixelMargin( x, y0-8 );
    grGotoxy( 0, 0 );

    sprintf( temp, "%.1f", gamma );
    grWrite( temp );

    for ( ny = 0; ny < yrepeat; ny++, y += 2*yside )
    {
      do_rect( x, y, xside, yside, (int)255.0*pow( 0.5, 1.0/gamma ) );
      do_rect( x, y+yside, xside, yside, -1 );
    }
  }
  return 0;
}



int
main( int    argc,
      char*  argv[] )
{
  grEvent  event;

  /* Initialize engine */
  /* initialize graphics if needed */
  Init_Display();

  grSetTitle( surface, "FreeType Gamma Matcher" );

  Clear_Display();

  Render_GammaGrid();
  grRefreshSurface( surface );
  grListenSurface( surface, 0, &event );

  exit( 0 );      /* for safety reasons */
  return 0;       /* never reached */
}


/* End */
