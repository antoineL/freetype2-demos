/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-2000 by                                                  */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  ftcommin.i - common routines for the FreeType demo programs.            */
/*                                                                          */
/****************************************************************************/


#include <freetype/freetype.h>
#include <freetype/ftcache.h>

  /* the following header shouldn't be used in normal programs */
#include <freetype/internal/ftdebug.h>

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


  /* forward declarations */
  extern void  PanicZ( const char*  message );

  static FT_Error  error;


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                 DISPLAY-SPECIFIC DEFINITIONS                  *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


#include "graph.h"
#include "grfont.h"


#define DIM_X      500
#define DIM_Y      400

#define CENTER_X   ( bit.width / 2 )
#define CENTER_Y   ( bit.rows / 2 )

#define MAXPTSIZE  500                 /* dtp */


  char   Header[128];
  char*  new_header = 0;

  const unsigned char*  Text = (unsigned char*)
    "The quick brown fox jumped over the lazy dog 0123456789 "
    "\342\352\356\373\364\344\353\357\366\374\377\340\371\351\350\347 "
    "&#~\"\'(-`_^@)=+\260 ABCDEFGHIJKLMNOPQRSTUVWXYZ "
    "$\243^\250*\265\371%!\247:/;.,?<>";

  grSurface*  surface;      /* current display surface */
  grBitmap    bit;          /* current display bitmap  */

  static grColor  fore_color = { 255 };

  int  Fail;

  int  graph_init  = 0;

  int  render_mode = 1;
  int  debug       = 0;
  int  trace_level = 0;


#define RASTER_BUFF_SIZE   32768
  char  raster_buff[RASTER_BUFF_SIZE];


#undef  NODEBUG

#ifndef NODEBUG

#define LOG( x )  LogMessage##x


  void  LogMessage( const char*  fmt, ... )
  {
    va_list  ap;


    va_start( ap, fmt );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
  }

#else /* !DEBUG */

#define LOG(x)  /* */

#endif


  /* PanicZ */
  void  PanicZ( const char*  message )
  {
    fprintf( stderr, "%s\n  error = 0x%04x\n", message, error );
    exit( 1 );
  }


  /* clear the `bit' bitmap/pixmap */
  static
  void  Clear_Display( void )
  {
    long  image_size = (long)bit.pitch * bit.rows;


    if ( image_size < 0 )
      image_size = -image_size;
    memset( bit.buffer, 0, image_size );
  }


  /* initialize the display bitmap `bit' */
  static
  void  Init_Display( void )
  {
    grInitDevices();

    bit.mode  = gr_pixel_mode_gray;
    bit.width = DIM_X;
    bit.rows  = DIM_Y;
    bit.grays = 256;

    surface = grNewSurface( 0, &bit );
    if ( !surface )
      PanicZ( "could not allocate display surface\n" );

    graph_init = 1;
  }


#define MAX_BUFFER  300000


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****               FREETYPE-SPECIFIC DEFINITIONS                   *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  FT_Library       library;      /* the FreeType library            */
  FTC_Manager      manager;      /* the cache manager               */
  FTC_Image_Cache  image_cache;

  FT_Face          face;         /* the font face                   */
  FT_Size          size;         /* the font size                   */
  FT_GlyphSlot     glyph;        /* the glyph slot                  */

  FTC_Image_Desc   current_font;

  int  num_glyphs;            /* number of glyphs   */
  int  ptsize;                /* current point size */

  int  hinted    = 1;         /* is glyph hinting active?    */
  int  antialias = 1;         /* is anti-aliasing active?    */
  int  use_sbits = 1;         /* do we use embedded bitmaps? */
  int  low_prec  = 0;         /* force low precision         */
  int  autohint  = 0;         /* force auto-hinting          */
  int  Num;                   /* current first glyph index   */

  int  res = 72;


#define MAX_GLYPH_BYTES  150000   /* 150 Kb for the glyph image cache */


#define FLOOR( x )  (   (x)        & -64 )
#define CEIL( x )   ( ( (x) + 63 ) & -64 )
#define TRUNC( x )  (   (x) >> 6 )


  /* this simple record is used to model a given `installed' face */
  typedef struct  TFont_
  {
    const char*  filepathname;
    int          face_index;
    
    int          num_glyphs;
  
  } TFont, *PFont;

  static PFont*  fonts;
  static int     num_fonts;
  static int     max_fonts = 0;

  static
  const char*  file_suffixes[] =
  {
    ".ttf",
    ".ttc",
    ".pfa",
    ".pfb",
    0
  };


  static
  int  install_font_file( char*  filepath )
  {
    static char   filename[1024 + 5];
    int           i, len, suffix_len, num_faces;
    const char**  suffix;
    

    len = strlen( filepath );
    if ( len > 1024 )
      len = 1024;
      
    strncpy( filename, filepath, len );
    filename[len] = 0;
    
    error = FT_New_Face( library, filename, 0, &face );
    if ( !error )
      goto Success;
    
    /* could not open the file directly; we will know try various */
    /* suffixes like `.ttf' or `.pfb'                             */
    
#ifndef macintosh

    suffix     = file_suffixes;
    suffix_len = 0;
    i          = len - 1;

    while ( i > 0 && filename[i] != '\\' && filename[i] != '/' )
    {
      if ( filename[i] == '.' )
      {
        suffix_len = i;
        break;
      }
      i--;
    }
    if ( suffix_len == 0 )
    {
      for ( suffix = file_suffixes; suffix[0]; suffix++ )
      {
        /* try with current suffix */
        strcpy( filename + len, suffix[0] );
        
        error = FT_New_Face( library, filename, 0, &face );
        if ( !error )
          goto Success;
      }
    }

#endif /* !macintosh */
    
    /* really couldn't open this file */
    return -1;    
    
  Success:

    /* allocate new font object */
    num_faces = face->num_faces;
    for ( i = 0; i < num_faces; i++ )
    {
      PFont  font;


      if ( i > 0 )
      {
        error = FT_New_Face( library, filename, i, &face );
        if ( error )
          continue;
      }
      
      font = (PFont)malloc( sizeof ( *font ) );
      font->filepathname = (char*)malloc( strlen( filename ) + 1 );
      font->face_index   = i;
      font->num_glyphs   = face->num_glyphs;

      strcpy( (char*)font->filepathname, filename );
      
      FT_Done_Face( face );
              
      if ( max_fonts == 0 )
      {
        max_fonts = 16;
        fonts = (PFont*)malloc( max_fonts * sizeof ( PFont ) );
      }
      else if ( num_fonts >= max_fonts )
      {
        max_fonts *= 2;
        fonts = (PFont*)realloc( fonts, max_fonts * sizeof ( PFont ) );
      }
      
      fonts[num_fonts++] = font;
    }

    return 0;
  }



  /*************************************************************************/
  /*                                                                       */
  /* The face requester is a function provided by the client application   */
  /* to the cache manager, whose role is to translate an `abstract' face   */
  /* ID into a real FT_Face object.                                        */
  /*                                                                       */
  /* In this program, the face IDs are simply pointers to TFont objects.   */
  /*                                                                       */
  static
  FT_Error  my_face_requester( FTC_FaceID  face_id,
                               FT_Library  library,
                               FT_Pointer  request_data,
                               FT_Face*    aface )
  {
    PFont  font = (PFont)face_id;
    FT_UNUSED( request_data );


    return FT_New_Face( library,
                        font->filepathname,
                        font->face_index,
                        aface );
  }                               
  
  
  static
  void  init_freetype( void )
  {
    error = FT_Init_FreeType( &library );
    if ( error )
      PanicZ( "could not initialize FreeType" );
    
    error = FTC_Manager_New( library, 0, 0, 0,
                             my_face_requester, 0, &manager );
    if ( error )
      PanicZ( "could not initialize cache manager" );
    
    error = FTC_Image_Cache_New( manager, &image_cache );
    if ( error )
      PanicZ( "could not initialize glyph image cache" );
  }


  static
  void  done_freetype( void )
  {
    FTC_Manager_Done( manager );
    FT_Done_FreeType( library );
  }  


  static
  void  set_current_face( PFont  font )
  {
    current_font.font.face_id = (FTC_FaceID)font;
  }  


  static
  void  set_current_size( int  pixel_size )
  {
    current_font.font.pix_width  = pixel_size;
    current_font.font.pix_height = pixel_size;
  }


  static
  void  set_current_pointsize( int  point_size )
  {
    set_current_size( ( point_size * res + 36 ) / 72 );
  }


  static
  void  set_current_image_type( void )
  {
    current_font.image_type = antialias ? ftc_image_grays : ftc_image_mono;
    
    if ( !hinted )
      current_font.image_type |= ftc_image_flag_unhinted;
      
    if ( autohint )
      current_font.image_type |= ftc_image_flag_autohinted;

    if ( !use_sbits )
      current_font.image_type |= ftc_image_flag_no_sbits;
  }   


/* End */
