#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static void panic( const char*  fmt, ... )
{
  va_list  args;

  va_start( args, fmt );
  vfprintf( stderr, fmt, args );
  va_end( args );
  exit(1);
}

/** DISPLAY ABSTRACTION
 **/

typedef enum {
  DISPLAY_MODE_MONO = 0,
  DISPLAY_MODE_GRAY
  
} DisplayMode;

typedef void  (*Display_drawFunc)( void*        disp,
                                   DisplayMode  mode,
                                   int          x,
                                   int          y,
                                   int          width,
                                   int          height,
                                   int          pitch,
                                   void*        buffer );

typedef struct {
  void*             disp;
  Display_drawFunc  disp_draw;

} DisplayRec, *Display;

static const unsigned char*  default_text =
"Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Cras sit amet dui. "
"Nam sapien. Fusce vestibulum ornare metus. Maecenas ligula orci, consequat "
"vitae, dictum nec, lacinia non, elit. Aliquam iaculis molestie neque. Maecenas"
" suscipit felis ut pede convallis malesuada. Aliquam erat volutpat. Nunc "
"pulvinar condimentum nunc. Donec ac sem vel leo bibendum aliquam. Pellentesque"
" habitant morbi tristique senectus et netus et malesuada fames ac turpis "
"egestas.\n\n"

"Sed commodo. Nulla ut libero sit amet justo varius blandit. Mauris vitae nulla"
" eget lorem pretium ornare. Proin vulputate erat porta risus. Vestibulum "
"malesuada, odio at vehicula lobortis, nisi metus hendrerit est, vitae feugiat"
" quam massa a ligula. Aenean in tellus. Praesent convallis. Nullam vel lacus."
" Aliquam congue erat non urna mollis faucibus. Morbi vitae mauris faucibus "
"quam condimentum ornare. Quisque sit amet augue. Morbi ullamcorper mattis "
"enim. Aliquam erat volutpat. Morbi nec felis non enim pulvinar lobortis. "
"Ut libero. Nullam id orci quis nisl dapibus rutrum. Suspendisse consequat "
"vulputate leo. Aenean non orci non tellus iaculis vestibulum. Sed neque.\n\n"
;

/***********************************************************************/
/***********************************************************************/
/*****                                                             *****/
/*****               T E X T   R E N D E R I N G                   *****/
/*****                                                             *****/
/***********************************************************************/
/***********************************************************************/

typedef enum {
  RENDER_MODE_AUTOHINT,
  RENDER_MODE_BYTECODE,
  RENDER_MODE_WINDOWS,
  RENDER_MODE_AUTOHINT_NOADJUST,
  RENDER_MODE_UNHINTED
} RenderMode;


/** RENDER STATE
 **
 **/

typedef struct {
  FT_Library            library;
  const unsigned char*  text;
  int                   resolution;
  float                 char_size;
  int                   need_rescale;
  int                   use_kerning;
  const char*           filepath;
  const char*           filename;
  FT_Face               face;
  FT_Size               size;
  char**                files;
  int                   file_index;
  int                   file_count;
  char*                 message;
  DisplayRec            display;
  char                  filepath0[1024];
  char                  message0[1024];

} RenderStateRec, *RenderState;


static void
render_state_init( RenderState       state,
                   Display           display )
{
  memset( state, 0, sizeof(*state) );
  state->text         = default_text;
  state->filepath     = state->filepath0;
  state->filename     = "<none>";
  state->filepath0[0] = 0;
  state->resolution   = 72;
  state->char_size    = 16;
  state->display      = display[0];

  if ( FT_Init_FreeType( &state->library ) != 0 )
    panic( "could not initialize FreeType library. Check your code\n" );
}

static void
render_state_done( RenderState  state )
{
  if ( state->filepath != state->filepath0 ) {
    free((char*)state->filepath);
    state->filepath = state->filepath0;
  }
  state->filepath0[0] = 0;
  state->filename     = 0;

  if ( state->face ) {
    FT_Done_Face( state->face );
    state->face = NULL;
    state->size = NULL;
  }

  if ( state->library ) {
    FT_Done_FreeType( state->library );
    state->library = NULL;
  }
}


static void
render_state_set_resolution( RenderState  state,
                             int          resolution )
{
  state->resolution   = resolution;
  state->need_rescale = 1;
}


static void
render_state_set_size( RenderState  state,
                       float        char_size )
{
  state->char_size    = char_size;
  state->need_rescale = 1;
}

static void
_render_state_rescale( RenderState  state )
{
  if (state->need_rescale && state->size)
  {
    FT_Set_Char_Size( state->face, 0, state->char_size*64, 0, state->resolution );
    state->need_rescale = 0;
  }
}

static int
render_state_set_files( RenderState   state,
                        char**        files )
{
  int   count = 0;

  state->files = files;
  for ( ; files[0] != NULL; files++ )
    count++;

  state->file_count = count;
  state->file_index = 0;
}

static int
render_state_set_file( RenderState   state,
                       int           index )
{
  char*  filepath;

  if (index < 0)
    index = state->file_count-1;
  else if (index >= state->file_count)
    index = 0;

  if (index >= state->file_count)
    return -2;

  state->file_index = index;
  filepath = state->files[index];

  if ( state->face ) {
    FT_Done_Face( state->face );
    state->face         = NULL;
    state->size         = NULL;
    state->need_rescale = 1;
  }
  if ( filepath != NULL && filepath[0] != 0 ) {
    FT_Error  error;

    error = FT_New_Face( state->library, filepath, 0, &state->face );
    if (error) return -1;

    {
      int    len = strlen(filepath);
      char*  p;

      if (len+!1> sizeof(state->filepath0)) {
        state->filepath = malloc( len+1 );
        if (state->filepath == NULL) {
          state->filepath = state->filepath0;
          return -1;
        }
      }
      memcpy( (char*)state->filepath, filepath, len+1 );
      p = strrchr( state->filepath, '\\' );
      if ( p == NULL )
        p = strrchr( state->filepath, '/' );

      state->filename = p ? p+1 : state->filepath;
    }

    state->size         = state->face->size;
    state->need_rescale = 1;
  }
  return 0;
}


/** RENDERING
 **/
 
static int
render_state_draw( RenderState           state,
                   const unsigned char*  text,
                   RenderMode            rmode,
                   int                   x,
                   int                   y,
                   int                   width,
                   int                   height )
{
  const unsigned char*  p          = text;
  long                  load_flags = FT_LOAD_DEFAULT;
  FT_Face               face       = state->face;
  int                   left       = x;
  int                   right      = x + width;
  int                   bottom     = y + height;
  int                   line_height;
  FT_UInt               prev_glyph = 0;
  FT_Pos                prev_rsb_delta = 0;
  FT_Pos                x_origin       = x << 6;

  if (!face)
    return -2;

  _render_state_rescale(state);

  y += state->size->metrics.ascender/64;
  line_height = state->size->metrics.height/64;

  if (rmode == RENDER_MODE_AUTOHINT || rmode == RENDER_MODE_AUTOHINT_NOADJUST)
    load_flags = FT_LOAD_FORCE_AUTOHINT;

  if (rmode == RENDER_MODE_UNHINTED)
      load_flags |= FT_LOAD_NO_HINTING;

  for ( ; *p; p++ )
  {
    FT_UInt       gindex;
    FT_Error      error;
    FT_GlyphSlot  slot   = face->glyph;
    FT_Bitmap*    map    = &slot->bitmap;
    int           xmax;
    DisplayMode   mode;

    /* handle newlines */
    if ( *p == 0x0A )
    {
      if ( p[1] == 0x0D )
        p++;
      x_origin = left << 6;
      y       += line_height;
      prev_rsb_delta = 0;
      if (y >= bottom)
        break;
      continue;
    }
    else if ( *p == 0x0D ) {
      if ( p[1] == 0x0A )
        p++;
      x_origin = left << 6;
      y       += line_height;
      prev_rsb_delta = 0;
      if ( y >= bottom )
        break;
      continue;
    }

    gindex = FT_Get_Char_Index( state->face, p[0] );
    error  = FT_Load_Glyph( face, gindex, load_flags );

    if (error)
      continue;

    if ( state->use_kerning && gindex != 0 && prev_glyph != 0 ) {
      FT_Vector  vec;
      FT_Int     mode = FT_KERNING_DEFAULT;

      if (rmode == RENDER_MODE_UNHINTED)
          mode = FT_KERNING_UNFITTED;

      FT_Get_Kerning( face, prev_glyph, gindex, mode, &vec );

      x_origin += vec.x;
    }

    if (rmode != RENDER_MODE_AUTOHINT_NOADJUST && rmode != RENDER_MODE_UNHINTED) {
        if ( prev_rsb_delta - face->glyph->lsb_delta >= 32 )
            x_origin -= 64;
        else if ( prev_rsb_delta - face->glyph->lsb_delta < -32 )
            x_origin += 64;
    }
    prev_rsb_delta = face->glyph->rsb_delta;

    if (rmode == RENDER_MODE_UNHINTED && slot->format == FT_GLYPH_FORMAT_OUTLINE)
    {
        FT_Pos  shift = (x_origin & 63);

        FT_Outline_Translate( &slot->outline, shift, 0 );
    }
    FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);

    xmax = ((x_origin+63) >> 6) + slot->bitmap_left + map->width;
    if (xmax >= right) {
      x  = left;
      y += line_height;
      if (y >= bottom)
        break;

      x_origin       = x << 6;
      prev_rsb_delta = 0;
    }

    mode = DISPLAY_MODE_MONO;
    if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
      mode = DISPLAY_MODE_GRAY;

    state->display.disp_draw( state->display.disp, mode,
                              (x_origin >> 6) + slot->bitmap_left,
                              y - slot->bitmap_top,
                              map->width, map->rows,
                              map->pitch, map->buffer );

    if (rmode == RENDER_MODE_UNHINTED)
        x_origin += slot->linearHoriAdvance >> 10;
    else
        x_origin += slot->advance.x;

    prev_glyph   = gindex;
  }
}

/***********************************************************************/
/***********************************************************************/
/*****                                                             *****/
/*****                D I S P L A Y                                *****/
/*****                                                             *****/
/***********************************************************************/
/***********************************************************************/

#include "graph.h"
#include "grobjs.h"
#include "grfont.h"

typedef struct {
  grSurface*   surface;
  grBitmap*    bitmap;
  grColor      fore_color;
  grColor      back_color;
  double       gamma;

} ADisplayRec, *ADisplay;

#define  DIM_X   640
#define  DIM_Y   480

static int
adisplay_init( ADisplay     display,
               grPixelMode  mode )
{
  grSurface*       surface;
  grBitmap         bit;

  if ( mode != gr_pixel_mode_gray  &&
       mode != gr_pixel_mode_rgb24 )
    return -1;

  grInitDevices();

  bit.mode  = mode;
  bit.width = DIM_X;
  bit.rows  = DIM_Y;
  bit.grays = 256;

  surface = grNewSurface( 0, &bit );

  if ( !surface )
  {
    free( display );
    return -1;
  }

  display->surface = surface;
  display->bitmap  = &surface->bitmap;
  display->gamma   = 1.0;

  grSetGlyphGamma( display->gamma );

  memset( &display->fore_color, 0, sizeof( grColor ) );
  memset( &display->back_color, 0xff, sizeof( grColor ) );

  return 0;
}


static void
adisplay_clear( ADisplay  display )
{
  grBitmap*  bit   = display->bitmap;
  int        pitch = bit->pitch;

  if (pitch < 0)
    pitch = -pitch;

  if ( bit->mode == gr_pixel_mode_gray )
    memset( bit->buffer, display->back_color.value, pitch * bit->rows );
  else
  {
    unsigned char*  p = bit->buffer;
    int             i, j;

    for ( i = 0; i < bit->rows; i++ )
    {
      for ( j = 0; j < bit->width; j++ )
        memcpy( p + 3 * j, display->back_color.chroma, 3 );

      p += pitch;
    }
  }
}


static void
adisplay_done( ADisplay  display )
{
  grDoneBitmap( display->bitmap );
  grDoneSurface( display->surface );

  display->bitmap  = NULL;
  display->surface = NULL;
  
  grDoneDevices();
}


static void
adisplay_draw_glyph( void*        _display,
                     DisplayMode  mode,
                     int          x,
                     int          y,
                     int          width,
                     int          height,
                     int          pitch,
                     void*        buffer )
{
  ADisplay  display = _display;
  grBitmap  glyph;
  
  glyph.width  = width;
  glyph.rows   = height;
  glyph.pitch  = pitch;
  glyph.buffer = buffer;
  glyph.grays  = 256;
  glyph.mode   = gr_pixel_mode_mono;
  if (mode == DISPLAY_MODE_GRAY)
    glyph.mode = gr_pixel_mode_gray;
    
  grBlitGlyphToBitmap( display->bitmap, &glyph,
                       x, y, display->fore_color );
}


static void
adisplay_change_gamma( ADisplay   display,
                       double     delta )
{
  display->gamma += delta;
  if ( display->gamma > 3.0 )
    display->gamma = 3.0;
  else if ( display->gamma < 0. )
    display->gamma = 0.;

  grSetGlyphGamma( display->gamma );
}


static void
event_help( RenderState  state )
{
  ADisplay  display = state->display.disp;
  grEvent   dummy_event;


  adisplay_clear( display );
  grGotoxy( 0, 0 );
  grSetMargin( 2, 1 );
  grGotobitmap( display->bitmap );

  grWriteln( "Text Viewer - Simple text/font proofer for the FreeType project" );
  grLn();
  grWriteln( "This program is used to display text using two distinct algorithms" );
  grWriteln( "On the left, text is rendered by the TrueType bytecode interpreter" );
  grWriteln( "On the right, text is rendered through the FreeType auto-hinter" );
  grLn();
  grWriteln( "Use the following keys:" );
  grLn();
  grWriteln( "  F1 or ?    : display this help screen" );
  grLn();
  grWriteln( "  n          : jump to next font file in arguments list" );
  grWriteln( "  p          : jump to previous font file in arguments list" );
  grLn();
  grWriteln( "  g          : increase gamma by 0.1" );
  grWriteln( "  v          : decrease gamma by 0.1" );
  grLn();
  grWriteln( "  Up         : increase pointsize by 0.5 unit" );
  grWriteln( "  Down       : decrease pointsize by 0.5 unit" );
  grWriteln( "  Page Up    : increase pointsize by 5 units" );
  grWriteln( "  Page Down  : decrease pointsize by 5 units" );
  grLn();
  grWriteln( "press any key to exit this help screen" );

  grRefreshSurface( display->surface );
  grListenSurface( display->surface, gr_event_key, &dummy_event );
}

static void
event_change_gamma( RenderState  state,
                    double     delta )
{
  ADisplay  display = state->display.disp;

  adisplay_change_gamma( display, delta );
  if ( display->gamma == 0. )
    sprintf( state->message0, "gamma set to sRGB" );
  else
    sprintf( state->message0, "gamma set to %.1f", display->gamma );

  state->message = state->message0;
}


static void
event_change_size( RenderState  state,
                   double       delta )
{
  ADisplay  display = state->display.disp;
  double    char_size = state->char_size;

  char_size += delta;
  if (char_size < 6.)
    char_size = 6.;
  else if (char_size > 300)
    char_size = 300;

  render_state_set_size( state, char_size );
}


static int
process_event( RenderState  state,
               grEvent*     event )
{
  int  ret = 0;

  switch (event->key)
  {
  case grKeyEsc:
  case grKEY('q'):
    ret = 1;
    break;

  case grKeyF1:
    event_help( state );
    break;

  case grKEY('n'):
    render_state_set_file( state, state->file_index+1 );
    break;

  case grKEY('p'):
    render_state_set_file( state, state->file_index-1 );
    break;

  case grKEY( 'g' ):
    event_change_gamma( state, +0.1 );
    break;

  case grKEY( 'v' ):
    event_change_gamma( state, -0.1 );
    break;

  case grKeyUp:
    event_change_size( state, +0.5 );
    break;

  case grKeyDown:
    event_change_size( state, -0.5 );
    break;

  case grKeyPageUp:
    event_change_size( state, +5. );
    break;

  case grKeyPageDown:
    event_change_size( state, -5. );
  }
  return ret;
}


static void usage( void )
{
  fprintf( stderr,
           "ftdiff: a simple program to proof several text hinting modes\n"
           "usage:   aview  [options] fontfile [fontfile2 ...]\n\n"
           "   options:   -r NNN      : change resolution in dpi (default is 72)\n"
           "              -s NNN      : change character size in points (default is 16)\n"
           "              -f TEXTFILE : change displayed text\n\n" );
  exit(1);
}

static char*
get_option_arg( char*  option, char** *pargv, char**  argend )
{
  if ( option[2] == 0 ) {
    char**  argv = *pargv;
    if ( ++argv >= argend )
      usage();
    option = argv[0];
    *pargv = argv;
  }
  else
    option += 2;

  return option;
}


static void
write_message( RenderState  state )
{
  ADisplay  adisplay = state->display.disp;

  if ( state->message == NULL ) {
    state->message = state->message0;
    sprintf( state->message0, "%s @ %5.1fpt", state->filename, state->char_size );
  }

  grWriteCellString( adisplay->bitmap, 0, DIM_Y - 20, state->message,
                     adisplay->fore_color );

  state->message = NULL;
}


int  main(int  argc, char**  argv)
{
  char**          argend = argv + argc;
  ADisplayRec     adisplay[1];
  RenderStateRec  state[1];
  DisplayRec      display[1];
  int             resolution = -1;
  double          size       = -1;
  const char*     textfile   = NULL;
  unsigned char*  text       = (unsigned char*)default_text;

  /* Read Options */
  ++argv;
  while ( argv < argend && argv[0][0] == '-' ) {
    char*  arg = argv[0];
    switch (arg[1])
    {
    case 'r':
      arg = get_option_arg( arg, &argv, argend );
      resolution = atoi( arg );
      break;

    case 's':
      arg = get_option_arg( arg, &argv, argend );
      size = atof( arg );
      break;

    case 'f':
      arg      = get_option_arg( arg, &argv, argend );
      textfile = arg;
      break;

    default:
      usage();
    }
    argv++;
  }

  if ( argv >= argend )
    usage();

  /* Read Text File, if any */
  if ( textfile != NULL ) {
    FILE*   tfile = fopen( textfile, "r" );
    if ( tfile == NULL ) {
      fprintf( stderr, "could not read textfile '%s'\n", textfile );
    } else {
      long   tsize;
      fseek( tfile, 0, SEEK_END );
      tsize = ftell( tfile );
      fseek( tfile, 0, SEEK_SET );
      text = malloc( tsize+1 );
      if (text != NULL) {
        fread( text, tsize, 1, tfile );
        text[tsize] = 0;
      } else {
        fprintf( stderr, "not enough memory to read '%s'\n", textfile );
        text = (unsigned char*)default_text;
      }
      fclose( tfile );
    }
  }

  /* Initialize display */
  adisplay_init( adisplay, gr_pixel_mode_rgb24 );
  display->disp      = adisplay;
  display->disp_draw = adisplay_draw_glyph;

  render_state_init( state, display );

  if (resolution > 0)
    render_state_set_resolution( state, resolution );

  if (size > 0.)
    render_state_set_size( state, size );

  render_state_set_files( state, argv );
  render_state_set_file( state, 0 );

  grSetTitle( adisplay->surface, "FreeType Text Proofer, press F1 for help" );

  for (;;)
  {
    grEvent  event;

    adisplay_clear( adisplay );

    render_state_draw( state, text, RENDER_MODE_BYTECODE, 10, 10, DIM_X/3-15, DIM_Y-40 );

    render_state_draw( state, text, RENDER_MODE_AUTOHINT, DIM_X/3+5, 10, DIM_X/3-15, DIM_Y-40 );

    render_state_draw( state, text, RENDER_MODE_UNHINTED, DIM_X*2/3+5, 10, DIM_X/3-15, DIM_Y-40 );

    write_message( state );
    grRefreshSurface( adisplay->surface );
    grListenSurface( adisplay->surface, 0, &event );
    if ( process_event( state, &event ) )
      break;
  }

  render_state_done( state );
  adisplay_done( adisplay );
  return 0;
}

