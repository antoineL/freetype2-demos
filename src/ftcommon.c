/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 2005 by                                                       */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  ftcommon.c - common routines for the FreeType demo programs.            */
/*                                                                          */
/****************************************************************************/


#include <ft2build.h>
#include FT_FREETYPE_H

#include FT_CACHE_H
#include FT_CACHE_MANAGER_H

#include FT_BITMAP_H

#include "ftcommon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

  FT_Error   error;


#undef  NODEBUG

#ifndef NODEBUG

  void
  LogMessage( const char*  fmt, ... )
  {
    va_list  ap;


    va_start( ap, fmt );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
  }

#endif /* NODEBUG */


  /* PanicZ */
  void
  PanicZ( const char*  message )
  {
    fprintf( stderr, "%s\n  error = 0x%04x\n", message, error );
    exit( 1 );
  }



  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                 DISPLAY-SPECIFIC DEFINITIONS                  *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


#define DIM_X      500
#define DIM_Y      400


  FTDemo_Display*
  FTDemo_Display_New( grPixelMode  mode )
  {
    FTDemo_Display*  display;
    grSurface*       surface;
    grBitmap         bit;


    display = malloc( sizeof( FTDemo_Display ) );
    if ( !display )
      return NULL;

    if ( mode != gr_pixel_mode_gray  &&
         mode != gr_pixel_mode_rgb24 )
      return NULL;

    grInitDevices();

    bit.mode  = mode;
    bit.width = DIM_X;
    bit.rows  = DIM_Y;
    bit.grays = 256;

    surface = grNewSurface( 0, &bit );

    if ( !surface )
    {
      free( display );

      return NULL;
    }

    display->surface = surface;
    display->bitmap  = &surface->bitmap;

    grSetGlyphGamma( 1.0 );

    memset( &display->fore_color, 0, sizeof( grColor ) );
    memset( &display->back_color, 0xff, sizeof( grColor ) );

    return display;
  }


  void
  FTDemo_Display_Done( FTDemo_Display*  display )
  {
    grDoneBitmap( display->bitmap );
    free( display->surface );
    free( display );
  }


  void
  FTDemo_Display_Clear( FTDemo_Display*  display )
  {
    grBitmap*  bit   = display->bitmap;
    int        pitch = bit->pitch;


    if ( pitch < 0 )
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


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****               FREETYPE-SPECIFIC DEFINITIONS                   *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


#define FLOOR( x )  (   (x)        & -64 )
#define CEIL( x )   ( ( (x) + 63 ) & -64 )
#define ROUND( x )  ( ( (x) + 32 ) & -64 )
#define TRUNC( x )  (   (x) >> 6 )



  static
  const char*  file_suffixes[] =
  {
    ".ttf",
    ".ttc",
    ".otf",
    ".pfa",
    ".pfb",
    0
  };


  /*************************************************************************/
  /*                                                                       */
  /* The face requester is a function provided by the client application   */
  /* to the cache manager, whose role is to translate an `abstract' face   */
  /* ID into a real FT_Face object.                                        */
  /*                                                                       */
  /* In this program, the face IDs are simply pointers to TFont objects.   */
  /*                                                                       */
  static FT_Error
  my_face_requester( FTC_FaceID  face_id,
                     FT_Library  lib,
                     FT_Pointer  request_data,
                     FT_Face*    aface )
  {
    PFont  font = (PFont)face_id;


    FT_UNUSED( request_data );

    error = FT_New_Face( lib,
                         font->filepathname,
                         font->face_index,
                         aface );

    if ( !error && (*aface)->charmaps )
      (*aface)->charmap = (*aface)->charmaps[font->cmap_index];

    return error;
  }


  FTDemo_Handle*
  FTDemo_New( FT_Encoding encoding )
  {
    FTDemo_Handle*  handle;


    handle = malloc( sizeof( FTDemo_Handle ) );
    if ( !handle )
      return NULL;

    memset( handle, 0, sizeof( FTDemo_Handle ) );

    error = FT_Init_FreeType( &handle->library );
    if ( error )
      PanicZ( "could not initialize FreeType" );

    error = FTC_Manager_New( handle->library, 0, 0, 0,
                             my_face_requester, 0, &handle->cache_manager );
    if ( error )
      PanicZ( "could not initialize cache manager" );

    error = FTC_SBitCache_New( handle->cache_manager, &handle->sbits_cache );
    if ( error )
      PanicZ( "could not initialize small bitmaps cache" );

    error = FTC_ImageCache_New( handle->cache_manager, &handle->image_cache );
    if ( error )
      PanicZ( "could not initialize glyph image cache" );

    error = FTC_CMapCache_New( handle->cache_manager, &handle->cmap_cache );
    if ( error )
      PanicZ( "could not initialize charmap cache" );


    FT_Bitmap_New( &handle->bitmap );

    handle->encoding = encoding;

    handle->hinted    = 1;
    handle->antialias = 1;
    handle->use_sbits = 1;
    handle->low_prec  = 0;
    handle->autohint  = 0;
    handle->lcd_mode  = 0;

    handle->use_sbits_cache  = 1;

    /* string_init */
    memset( handle->string, 0, sizeof( TGlyph ) * MAX_GLYPHS );
    handle->string_length = 0;
    handle->kerning_mode = KERNING_MODE_NORMAL;
    handle->vertical = 0;
    handle->string_reload = 1;

    return handle;
  }


  void
  FTDemo_Done( FTDemo_Handle*  handle )
  {
    int  i;


    for ( i = 0; i < handle->max_fonts; i++ )
    {
      if ( handle->fonts[i] )
      {
        if ( handle->fonts[i]->filepathname )
          free( (void*)handle->fonts[i]->filepathname );
        free( handle->fonts[i] );
      }
    }
    free( handle->fonts );

    /* string_done */
    for ( i = 0; i < MAX_GLYPHS; i++ )
    {
      PGlyph  glyph = handle->string + i;


      if ( glyph->image )
        FT_Done_Glyph( glyph->image );
    }

    FT_Bitmap_Done( handle->library, &handle->bitmap );
    FTC_Manager_Done( handle->cache_manager );
    FT_Done_FreeType( handle->library );
  }


  FT_Error
  FTDemo_Install_Font( FTDemo_Handle*  handle,
                       const char*     filepath )
  {
    static char  filename[1024 + 5];
    int          i, len, num_faces;
    FT_Face      face;


    len = strlen( filepath );
    if ( len > 1024 )
      len = 1024;

    strncpy( filename, filepath, len );
    filename[len] = 0;

    error = FT_New_Face( handle->library, filename, 0, &face );

#ifndef macintosh
    /* could not open the file directly; we will now try various */
    /* suffixes like `.ttf' or `.pfb'                            */
    if ( error )
    {
      const char**  suffix;
      char*         p;
      int           found = 0;

      suffix = file_suffixes;
      p      = filename + len - 1;

      while ( p >= filename && *p != '\\' && *p != '/' )
      {
        if ( *p == '.' )
          break;

        p--;
      }

      /* no suffix found */
      if ( p < filename || *p != '.' )
        p = filename + len;

      for ( suffix = file_suffixes; suffix[0]; suffix++ )
      {
        /* try with current suffix */
        strcpy( p, suffix[0] );

        error = FT_New_Face( handle->library, filename, 0, &face );
        if ( !error )
        {
          found = 1;

          break;
        }
      }

      /* really couldn't open this file */
      if ( !found )
        return error;
    }
#endif /* !macintosh */

    /* allocate new font object */
    num_faces = face->num_faces;
    for ( i = 0; i < num_faces; i++ )
    {
      PFont  font;


      if ( i > 0 )
      {
        error = FT_New_Face( handle->library, filename, i, &face );
        if ( error )
          continue;
      }

      if ( handle->encoding != FT_ENCODING_NONE )
      {
        error = FT_Select_Charmap( face, handle->encoding );
        if ( error )
        {
          FT_Done_Face( face );
          return error;
        }
      }

      font = (PFont)malloc( sizeof ( *font ) );

      font->filepathname = (char*)malloc( strlen( filename ) + 1 );
      strcpy( (char*)font->filepathname, filename );

      font->face_index   = i;
      font->cmap_index   = face->charmap ? FT_Get_Charmap_Index( face->charmap ) : 0;

      switch ( handle->encoding )
      {
      case FT_ENCODING_NONE:
        font->num_indices = face->num_glyphs;
        break;

      case FT_ENCODING_UNICODE:
        font->num_indices = 0x110000L;
        break;

      case FT_ENCODING_MS_SYMBOL:
      case FT_ENCODING_ADOBE_LATIN_1:
      case FT_ENCODING_ADOBE_STANDARD:
      case FT_ENCODING_ADOBE_EXPERT:
      case FT_ENCODING_ADOBE_CUSTOM:
      case FT_ENCODING_APPLE_ROMAN:
        font->num_indices = 0x100L;
        break;

      default:
        font->num_indices = 0x10000L;
      }

      FT_Done_Face( face );
      face = NULL;

      if ( handle->max_fonts == 0 )
      {
        handle->max_fonts = 16;
        handle->fonts     = (PFont*)calloc( handle->max_fonts, sizeof ( PFont ) );
      }
      else if ( handle->num_fonts >= handle->max_fonts )
      {
        handle->max_fonts *= 2;
        handle->fonts      = (PFont*)realloc( handle->fonts, handle->max_fonts * sizeof ( PFont ) );

        memset( &handle->fonts[handle->num_fonts], 0,
                ( handle->max_fonts - handle->num_fonts ) * sizeof ( PFont ) );
      }

      handle->fonts[handle->num_fonts++] = font;
    }

    return FT_Err_Ok;
  }


  void
  FTDemo_Set_Current_Font( FTDemo_Handle*  handle,
                           PFont           font )
  {
    handle->current_font = font;
    handle->image_type.face_id = (FTC_FaceID)font;

    handle->string_reload = 1;
  }


  void
  FTDemo_Set_Current_Size( FTDemo_Handle*  handle,
                           int             pixel_size )
  {
    if ( pixel_size > 0xFFFF )
      pixel_size = 0xFFFF;

    handle->image_type.width  = (FT_UShort)pixel_size;
    handle->image_type.height = (FT_UShort)pixel_size;

    handle->string_reload = 1;
  }


  void
  FTDemo_Set_Current_Pointsize( FTDemo_Handle*  handle,
                                int             point_size,
                                int             res )
  {
    FTDemo_Set_Current_Size( handle, ( point_size * res + 36 ) / 72 );
  }


  void
  FTDemo_Update_Current_Flags( FTDemo_Handle*  handle )
  {
    handle->image_type.flags = handle->antialias ? FT_LOAD_DEFAULT : FT_LOAD_TARGET_MONO;

    handle->image_type.flags |= FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH;

    if ( !handle->hinted )
      handle->image_type.flags |= FT_LOAD_NO_HINTING;

    if ( handle->autohint )
      handle->image_type.flags |= FT_LOAD_FORCE_AUTOHINT;

    if ( !handle->use_sbits )
      handle->image_type.flags |= FT_LOAD_NO_BITMAP;

    if ( handle->antialias && handle->lcd_mode > 0 )
    {
      if ( handle->lcd_mode <= 1 )
        handle->image_type.flags |= FT_LOAD_TARGET_LIGHT;
      else if ( handle->lcd_mode <= 3 )
        handle->image_type.flags |= FT_LOAD_TARGET_LCD;
      else
        handle->image_type.flags |= FT_LOAD_TARGET_LCD_V;
    }

    handle->string_reload = 1;
  }


  FT_UInt
  FTDemo_Get_Index( FTDemo_Handle*  handle,
                    FT_UInt32       charcode )
  {
    FTC_FaceID  face_id = handle->image_type.face_id;
    PFont       font    = handle->current_font;


    return FTC_CMapCache_Lookup( handle->cmap_cache, face_id,
                                 font->cmap_index, charcode );
  }


  FT_Error
  FTDemo_Get_Size( FTDemo_Handle*  handle,
                   FT_Size*        asize)
  {
    FTC_ScalerRec  scaler;
    FT_Size        size;


    scaler.face_id = handle->image_type.face_id;
    scaler.width   = handle->image_type.width;
    scaler.height  = handle->image_type.height;
    scaler.pixel   = 1;

    error = FTC_Manager_LookupSize( handle->cache_manager, &scaler, &size );

    if ( !error )
      *asize = size;

    return error;
  }


  FT_Error
  FTDemo_Glyph_To_Bitmap( FTDemo_Handle*  handle,
                          FT_Glyph        glyf,
                          grBitmap*       target,
                          int*            left,
                          int*            top,
                          int*            x_advance,
                          int*            y_advance,
                          FT_Glyph*       aglyf )
  {
    FT_BitmapGlyph  bitmap;
    FT_Bitmap*      source;


    *aglyf = NULL;

    error = FT_Err_Ok;

    if ( glyf->format == FT_GLYPH_FORMAT_OUTLINE )
    {
      FT_Render_Mode  render_mode = FT_RENDER_MODE_MONO;


      if ( handle->antialias )
      {
        if ( handle->lcd_mode == 0 )
          render_mode = FT_RENDER_MODE_NORMAL;
        else if ( handle->lcd_mode == 1 )
          render_mode = FT_RENDER_MODE_LIGHT;
        else if ( handle->lcd_mode <= 3 )
          render_mode = FT_RENDER_MODE_LCD;
        else
          render_mode = FT_RENDER_MODE_LCD_V;
      }

      /* render the glyph to a bitmap, don't destroy original */
      error = FT_Glyph_To_Bitmap( &glyf, render_mode, NULL, 0 );
      if ( error )
        return error;

      *aglyf = glyf;
    }

    if ( glyf->format != FT_GLYPH_FORMAT_BITMAP )
      PanicZ( "invalid glyph format returned!" );

    bitmap = (FT_BitmapGlyph)glyf;
    source = &bitmap->bitmap;

    target->rows   = source->rows;
    target->width  = source->width;
    target->pitch  = source->pitch;
    target->buffer = source->buffer;
    target->grays  = source->num_grays;

    switch ( source->pixel_mode )
    {
    case FT_PIXEL_MODE_MONO:
      target->mode = gr_pixel_mode_mono;
      break;

    case FT_PIXEL_MODE_GRAY:
      target->mode  = gr_pixel_mode_gray;
      target->grays = source->num_grays;
      break;

    case FT_PIXEL_MODE_GRAY2:
    case FT_PIXEL_MODE_GRAY4:
      (void)FT_Bitmap_Convert( handle->library, source, &handle->bitmap, 1 );
      target->pitch  = handle->bitmap.pitch;
      target->buffer = handle->bitmap.buffer;
      target->mode   = gr_pixel_mode_gray;
      target->grays  = handle->bitmap.num_grays;
      break;

    case FT_PIXEL_MODE_LCD:
      target->mode  = handle->lcd_mode == 2 ? gr_pixel_mode_lcd
                                            : gr_pixel_mode_lcd2;
      target->grays = source->num_grays;
      break;

    case FT_PIXEL_MODE_LCD_V:
      target->mode  = handle->lcd_mode == 4 ? gr_pixel_mode_lcdv
                                            : gr_pixel_mode_lcdv2;
      target->grays = source->num_grays;
      break;

    default:
      return FT_Err_Invalid_Glyph_Format;
    }

    *left = bitmap->left;
    *top  = bitmap->top;

    *x_advance = ( glyf->advance.x + 0x8000 ) >> 16;
    *y_advance = ( glyf->advance.y + 0x8000 ) >> 16;

    return error;
  }


  FT_Error
  FTDemo_Index_To_Bitmap( FTDemo_Handle*  handle,
                          FT_ULong        Index,
                          grBitmap*       target,
                          int*            left,
                          int*            top,
                          int*            x_advance,
                          int*            y_advance,
                          FT_Glyph*       aglyf )
  {
    *aglyf = NULL;

    /* use the SBits cache to store small glyph bitmaps; this is a lot */
    /* more memory-efficient                                           */
    /*                                                                 */
    if ( handle->use_sbits_cache        &&
         handle->image_type.width  < 48 &&
         handle->image_type.height < 48 )
    {
      FTC_SBit   sbit;
      FT_Bitmap  source;


      error = FTC_SBitCache_Lookup( handle->sbits_cache,
                                    &handle->image_type,
                                    Index,
                                    &sbit,
                                    NULL );
      if ( error )
        goto Exit;

      if ( sbit->buffer )
      {
        target->rows   = sbit->height;
        target->width  = sbit->width;
        target->pitch  = sbit->pitch;
        target->buffer = sbit->buffer;
        target->grays  = sbit->max_grays + 1;

        switch ( sbit->format )
        {
        case FT_PIXEL_MODE_MONO:
          target->mode = gr_pixel_mode_mono;
          break;

        case FT_PIXEL_MODE_GRAY:
          target->mode  = gr_pixel_mode_gray;
          target->grays = sbit->max_grays + 1;
          break;

        case FT_PIXEL_MODE_GRAY2:
        case FT_PIXEL_MODE_GRAY4:
          source.rows       = sbit->height;
          source.width      = sbit->width;
          source.pitch      = sbit->pitch;
          source.buffer     = sbit->buffer;
          source.pixel_mode = sbit->format;
          (void)FT_Bitmap_Convert( handle->library, &source, &handle->bitmap, 1 );

          target->pitch  = handle->bitmap.pitch;
          target->buffer = handle->bitmap.buffer;
          target->mode   = gr_pixel_mode_gray;
          target->grays  = handle->bitmap.num_grays;
          break;

        case FT_PIXEL_MODE_LCD:
          target->mode  = handle->lcd_mode == 2 ? gr_pixel_mode_lcd
                                                 : gr_pixel_mode_lcd2;
          target->grays = sbit->max_grays + 1;
          break;

        case FT_PIXEL_MODE_LCD_V:
          target->mode  = handle->lcd_mode == 4 ? gr_pixel_mode_lcdv
                                                 : gr_pixel_mode_lcdv2;
          target->grays = sbit->max_grays + 1;
          break;

        default:
          return FT_Err_Invalid_Glyph_Format;
        }

        *left      = sbit->left;
        *top       = sbit->top;
        *x_advance = sbit->xadvance;
        *y_advance = sbit->yadvance;

        goto Exit;
      }
    }

    /* otherwise, use an image cache to store glyph outlines, and render */
    /* them on demand. we can thus support very large sizes easily..     */
    {
      FT_Glyph   glyf;

      error = FTC_ImageCache_Lookup( handle->image_cache,
                                     &handle->image_type,
                                     Index,
                                     &glyf,
                                     NULL );

      if ( !error )
        error = FTDemo_Glyph_To_Bitmap( handle, glyf, target, left, top, x_advance, y_advance, aglyf );
    }

  Exit:
    /* don't accept a `missing' character with zero or negative width */
    if ( Index == 0 && *x_advance <= 0 )
      *x_advance = 1;

    return error;
  }


  FT_Error
  FTDemo_Draw_Index( FTDemo_Handle*   handle,
                     FTDemo_Display*  display,
                     int              gindex,
                     int*             pen_x,
                     int*             pen_y )
  {
    int       left, top, x_advance, y_advance;
    grBitmap  bit3;
    FT_Glyph  glyf;

    error = FTDemo_Index_To_Bitmap( handle, gindex, &bit3, &left, &top,
                                    &x_advance, &y_advance, &glyf );
    if ( error )
      return error;

    /* now render the bitmap into the display surface */
    grBlitGlyphToBitmap( display->bitmap, &bit3, *pen_x + left,
                         *pen_y - top, display->fore_color );

    if ( glyf )
      FT_Done_Glyph( glyf );

    *pen_x += x_advance + 1;

    return FT_Err_Ok;
  }


  FT_Error
  FTDemo_Draw_Glyph( FTDemo_Handle*   handle,
                     FTDemo_Display*  display,
                     FT_Glyph         glyph,
                     int*             pen_x,
                     int*             pen_y )
  {
    int       left, top, x_advance, y_advance;
    grBitmap  bit3;
    FT_Glyph  glyf;


    error = FTDemo_Glyph_To_Bitmap( handle, glyph, &bit3, &left, &top,
                                    &x_advance, &y_advance, &glyf );
    if ( error )
    {
      FT_Done_Glyph( glyph );

      return error;
    }

    /* now render the bitmap into the display surface */
    grBlitGlyphToBitmap( display->bitmap, &bit3, *pen_x + left,
                         *pen_y - top, display->fore_color );

    if ( glyf )
      FT_Done_Glyph( glyf );

    *pen_x += x_advance + 1;

    return FT_Err_Ok;
  }


  FT_Error
  FTDemo_Draw_Slot( FTDemo_Handle*   handle,
                    FTDemo_Display*  display,
                    FT_GlyphSlot     slot,
                    int*             pen_x,
                    int*             pen_y )
  {
    FT_Glyph   glyph;


    error = FT_Get_Glyph( slot, &glyph );
    if ( error )
      return error;

    error = FTDemo_Draw_Glyph( handle, display, glyph, pen_x, pen_y );

    FT_Done_Glyph( glyph );

    return error;
  }


  void
  FTDemo_String_Set_Kerning( FTDemo_Handle*  handle,
                             int             kerning_mode )
  {
    handle->kerning_mode  = kerning_mode;
    handle->string_reload = 1;
  }


  void
  FTDemo_String_Set_Vertical( FTDemo_Handle*  handle,
                              int             vertical )
  {
    handle->vertical      = vertical;
    handle->string_reload = 1;
  }


  void
  FTDemo_String_Set( FTDemo_Handle*        handle,
                     const unsigned char*  string )
  {
    const unsigned char*  p = string;
    unsigned long         codepoint;
    unsigned char         in_code;
    int                   expect;
    PGlyph                glyph = handle->string;


    handle->string_length = 0;
    codepoint = expect = 0;

    while ( *p )
    {
      in_code = *p++ ;

      if ( in_code >= 0xC0 )
      {
        if ( in_code < 0xE0 )           /*  U+0080 - U+07FF   */
        {
          expect = 1;
          codepoint = in_code & 0x1F;
        }
        else if ( in_code < 0xF0 )      /*  U+0800 - U+FFFF   */
        {
          expect = 2;
          codepoint = in_code & 0x0F;
        }
        else if ( in_code < 0xF8 )      /* U+10000 - U+10FFFF */
        {
          expect = 3;
          codepoint = in_code & 0x07;
        }
        continue;
      }
      else if ( in_code >= 0x80 )
      {
        --expect;

        if ( expect >= 0 )
        {
          codepoint <<= 6;
          codepoint  += in_code & 0x3F;
        }
        if ( expect >  0 )
          continue;

        expect = 0;
      }
      else                              /* ASCII, U+0000 - U+007F */
        codepoint = in_code;

      if ( handle->encoding != FT_ENCODING_NONE )
        glyph->glyph_index = FTDemo_Get_Index( handle, codepoint );
      else
        glyph->glyph_index = codepoint;

      glyph++;
      handle->string_length++;

      if ( handle->string_length >= MAX_GLYPHS )
        break;
    }

    handle->string_reload = 1;
  }


  static FT_Error
  string_load( FTDemo_Handle*  handle )
  {
    int        n;
    FT_UInt    prev_index     = 0;
    FT_Pos     prev_rsb_delta = 0;
    FT_Vector  pen, delta;
    FT_Size    size;
    FT_Face    face;


    error = FTDemo_Get_Size( handle, &size );
    if ( error )
      return error;

    face = size->face;

    pen.x = 0;
    pen.y = 0;
    for ( n = 0; n < handle->string_length; n++ )
    {
      PGlyph  glyph = handle->string + n;


      /* clear existing image if there is one */
      if ( glyph->image )
      {
        FT_Done_Glyph( glyph->image );
        glyph->image = NULL;
      }

      /* load the glyph and get the image */
      if ( FT_Load_Glyph( face, glyph->glyph_index, handle->image_type.flags ) ||
           FT_Get_Glyph( face->glyph, &glyph->image ) )
        continue;

      /* adjust pen position */
      if ( !handle->vertical && handle->kerning_mode )
      {
        if ( n > 0 )
        {
          FT_Vector  kern;


          FT_Get_Kerning( face, prev_index, glyph->glyph_index,
                          handle->hinted ? FT_KERNING_DEFAULT : FT_KERNING_UNFITTED,
                          &kern );

          pen.x += kern.x;
          pen.y += kern.y;
        }
        prev_index = glyph->glyph_index;

        if ( handle->kerning_mode > KERNING_MODE_NORMAL )
        {
          if ( n > 0 )
          {
            if ( prev_rsb_delta - face->glyph->lsb_delta >= 32 )
              pen.x -= 64;
            else if ( prev_rsb_delta - face->glyph->lsb_delta < -32 )
              pen.x += 64;
          }
          prev_rsb_delta = face->glyph->rsb_delta;
        }
      }

      if ( handle->vertical )
      {
        FT_Glyph_Metrics*  metrics = &face->glyph->metrics;


        delta.x = metrics->vertBearingX - metrics->horiBearingX;
        delta.y = -metrics->vertBearingY - metrics->horiBearingY;

        delta.x = ROUND( delta.x );
        delta.y = ROUND( delta.y );

        /* hack pen position */
        pen.x += delta.x;
        pen.y += delta.y;
      }

      if ( glyph->image->format != FT_GLYPH_FORMAT_BITMAP )
      {
        error = FT_Glyph_Transform( glyph->image, NULL, &pen );

        if ( error )
        {
          FT_Done_Glyph( glyph->image );
          glyph->image = NULL;
          continue;
        }
      }
      else
      {
        FT_BitmapGlyph  bitmap;


        bitmap = (FT_BitmapGlyph)glyph->image;

        bitmap->left += pen.x >> 6;
        bitmap->top  += pen.y >> 6;
      }

      if ( handle->vertical )
      {
        /* restore pen position */
        pen.x -= delta.x;
        pen.y -= delta.y;

        pen.x = 0;
        if ( face->glyph->metrics.vertAdvance == 0 )
          pen.y -= ( face->size->metrics.y_ppem * 11 / 10 ) << 6;
        else
          pen.y -= CEIL( face->glyph->metrics.vertAdvance );
      }
      else
      {
        pen.x += face->glyph->advance.x;
        pen.y = 0;
      }
    }

    handle->string_extent.x = pen.x;
    handle->string_extent.y = pen.y;

    return FT_Err_Ok;
  }


  static void
  gamma_ramp_apply( FT_Byte    gamma_ramp[256],
                    grBitmap*  bitmap )
  {
    int       i, j;
    FT_Byte*  p = (FT_Byte*)bitmap->buffer;

    if ( bitmap->pitch < 0 )
      p += -bitmap->pitch * ( bitmap->rows - 1 );

    for ( i = 0; i < bitmap->rows; i++ )
    {
      for ( j = 0; j < bitmap->width; j++ )
        p[j] = gamma_ramp[p[j]];

      p += bitmap->pitch;
    }
  }


  FT_Error
  FTDemo_String_Draw( FTDemo_Handle*   handle,
                      FTDemo_Display*  display,
                      int              center_x,
                      int              center_y,
                      FT_Matrix*       matrix,
                      FT_Byte          gamma_ramp[256] )
  {
    int        n;
    FT_Vector  start;
    FT_Size    size;
    FT_Face    face;


    if ( center_x < 0 ||
         center_y < 0 ||
         center_x > display->bitmap->width ||
         center_y > display->bitmap->rows )
      return FT_Err_Invalid_Argument;

    error = FTDemo_Get_Size( handle, &size );
    if ( error )
      return error;

    face = size->face;

    if ( handle->string_reload )
    {
      error = string_load( handle );
      if ( error )
        return error;

      handle->string_reload = 0;
    }

    /* get start position */
    start = handle->string_extent;
    /* XXX sbits */
    if ( matrix && FT_IS_SCALABLE( face ) )
    {
      FT_Vector_Transform( &start, matrix );
      start.x = ( center_x << 6 ) - start.x / 2;
      start.y = ( center_y << 6 ) + start.y / 2;
    }
    else
    {
      start.x = ROUND( ( center_x << 6 ) - start.x / 2 );
      start.y = ROUND( ( center_y << 6 ) + start.y / 2 );
    }

    /* change to Cartesian coordinates */
    start.y = ( display->bitmap->rows << 6 ) - start.y;

    for ( n = 0; n < handle->string_length; n++ )
    {
      PGlyph     glyph = handle->string + n;
      FT_Glyph   image;
      FT_BBox    bbox;


      if ( !glyph->image )
        continue;

      /* copy image */
      error = FT_Glyph_Copy( glyph->image, &image );
      if ( error )
        continue;

      if ( image->format != FT_GLYPH_FORMAT_BITMAP )
      {
        error = FT_Glyph_Transform( image, matrix, &start );

        if ( error )
        {
          FT_Done_Glyph( image );
          continue;
        }
      }
      else
      {
        FT_BitmapGlyph  bitmap = (FT_BitmapGlyph)image;


        bitmap->left += start.x >> 6;
        bitmap->top  += start.y >> 6;
      }

      FT_Glyph_Get_CBox( image, FT_GLYPH_BBOX_PIXELS, &bbox );

#if 0
      if ( n == 0 )
      {
        fprintf( stderr, "bbox = [%ld %ld %ld %ld]\n",
            bbox.xMin, bbox.yMin, bbox.xMax, bbox.yMax );
      }
#endif

      /* check bounding box; if it is completely outside the */
      /* display surface, we don't need to render it         */
      if ( bbox.xMax > 0                      &&
           bbox.yMax > 0                      &&
           bbox.xMin < display->bitmap->width &&
           bbox.yMin < display->bitmap->rows )
      {
        int       left, top, dummy1, dummy2;
        grBitmap  bit3;
        FT_Glyph  glyf;


        error = FTDemo_Glyph_To_Bitmap( handle, image, &bit3, &left, &top,
                                        &dummy1, &dummy2, &glyf );
        if ( !error )
        {
          if ( gamma_ramp )
            gamma_ramp_apply( gamma_ramp, &bit3 );

          /* change back to the usual coordinates */
          top = display->bitmap->rows - top;

          /* now render the bitmap into the display surface */
          grBlitGlyphToBitmap( display->bitmap, &bit3, left, top,
                               display->fore_color );

          if ( glyf )
            FT_Done_Glyph( glyf );
        }
      }

      FT_Done_Glyph( image );
    }

    return error;
  }


  FT_Encoding
  FTDemo_Make_Encoding_Tag( const char*  s )
  {
    int            i;
    unsigned long  l = 0;


    for ( i = 0; i < 4; i++ )
    {
      if ( !s[i] )
        break;
      l <<= 8;
      l  += (unsigned long)s[i];
    }

    return l;
  }


/* End */
