/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-2000 by                                                  */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/****************************************************************************/


#include <freetype/freetype.h>

  /* the following header shouldn't be used in normal programs */
#include <freetype/internal/ftdebug.h>

  /* showing driver name */
#include <freetype/ftmodule.h>
#include <freetype/internal/ftobjs.h>
#include <freetype/internal/ftdriver.h>

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


  FT_Library    library;      /* the FreeType library            */
  FT_Face       face;         /* the font face                   */

  FT_Error      error;

  int  comma_flag = 0;

#if 0
  int  debug       = 0;
  int  trace_level = 0;
#endif


  /* PanicZ */
  static
  void  PanicZ( const char*  message )
  {
    fprintf( stderr, "%s\n  error = 0x%04x\n", message, error );
    exit( 1 );
  }


  void  Print_Comma( const char*  message )
  {
    if ( comma_flag )
      printf(", ");

    printf("%s", message);
    comma_flag = 1;
  }


  static
  void  usage( char*  execname )
  {
    fprintf( stderr,  "\n" );
    fprintf( stderr,  "ftview: simple font dumper -- part of the FreeType project\n" );
    fprintf( stderr,  "-----------------------------------------------------------\n" );
    fprintf( stderr,  "\n" );
    fprintf( stderr,  "Usage: %s fontname[.ttf|.ttc]\n", execname );

    exit( 1 );
  }


  void  Print_Name( FT_Face  face )
  {
    printf( "font name entries\n" );

    /* XXX: Foundry? Copyright? Version? ... */

    printf( "   family: %s\n", face->family_name );
    printf( "   style:  %s\n", face->style_name );
  }


  void  Print_Type( FT_Face  face )
  {
    FT_ModuleRec*  module;


    printf( "font type entries\n" );

    module = &face->driver->root;
    printf( "   FreeType driver: %s\n", module->clazz->module_name );

    /* Is it better to dump all sfnt tag names? */
    printf( "   sfnt wrapped: %s\n",
            FT_IS_SFNT( face ) ? (char *)"yes" : (char *)"no" );

    /* isScalable? */
    comma_flag = 0;
    printf( "   type: " );
    if ( FT_IS_SCALABLE( face ) )
      Print_Comma( "scalable" );
    if ( FT_HAS_MULTIPLE_MASTERS( face ) )
      Print_Comma( "Multiple Master" );
    if ( FT_HAS_FIXED_SIZES( face ) )
      Print_Comma( "fixed size" );
    printf( "\n" );

    /* Direction */
    comma_flag = 0;
    printf( "   direction: " );
    if ( FT_HAS_HORIZONTAL( face ) )
      Print_Comma( "horizontal" );

    if ( FT_HAS_VERTICAL( face ) )
      Print_Comma( "vertical" );

    printf( "\n" );

    printf( "   fixed width: %s\n",
            FT_IS_FIXED_WIDTH( face ) ? (char *)"yes" : (char *)"no" );

    printf( "   fast glyphs: %s\n",
            FT_HAS_FAST_GLYPHS( face ) ? (char *)"yes" : (char *)"no" );

    printf( "   glyph names: %s\n",
            FT_HAS_GLYPH_NAMES( face ) ? (char *)"yes" : (char *)"no" );
  }


  void  Print_Fixed( FT_Face  face )
  {
    int  i;


    /* num_fixed_size */
    printf( "fixed size\n" );

    /* available size */
    for( i = 0; i < face->num_fixed_sizes; i++ )
    {
      printf( "   %d: height %d, width %d\n",
              i,
              face->available_sizes[i].height,
              face->available_sizes[i].width );
    }
  }


  void  Print_Charmaps( FT_Face  face )
  {
    int  i;


    /* CharMaps */
    printf("charmaps\n");

    for( i = 0; i < face->num_charmaps; i++ )
    {
      printf( "   %d: platform: %d, encoding: %d\n",
              i,
              face->charmaps[i]->platform_id,
              face->charmaps[i]->encoding_id );
    }
  }


  int  main( int    argc,
             char*  argv[] )
  {
    int    i, file;
    char   filename[128 + 4];
    char   alt_filename[128 + 4];
    char*  execname;
    int    num_faces;


    execname = ft_basename( argv[0] );

    if ( argc != 2 )
      usage( execname );

    file = 1;

#if 0
    if ( debug )
    {
#ifdef FT_DEBUG_LEVEL_TRACE
      FT_SetTraceLevel( trace_any, (FT_Byte)trace_level );
#else
      trace_level = 0;
#endif
    }
#endif /* 0 */

    /* Initialize engine */
    error = FT_Init_FreeType( &library );
    if ( error )
      PanicZ( "Could not initialize FreeType library" );

    filename[128] = '\0';
    alt_filename[128] = '\0';

    strncpy( filename, argv[file], 128 );
    strncpy( alt_filename, argv[file], 128 );

    /* try to load the file name as is, first */
    error = FT_New_Face( library, argv[file], 0, &face );
    if ( !error )
      goto Success;

#ifndef macintosh
    i = strlen( argv[file] );
    while ( i > 0 && argv[file][i] != '\\' && argv[file][i] != '/' )
    {
      if ( argv[file][i] == '.' )
        i = 0;
      i--;
    }

    if ( i >= 0 )
    {
      strncpy( filename + strlen( filename ), ".ttf", 4 );
      strncpy( alt_filename + strlen( alt_filename ), ".ttc", 4 );
    }
#endif

    /* Load face */
    error = FT_New_Face( library, filename, 0, &face );

  Success:
    num_faces = face->num_faces;
    FT_Done_Face( face );

    printf( "There %s %d %s in this file.\n",
            num_faces == 1 ? (char *)"is" : (char *)"are",
            num_faces,
            num_faces == 1 ? (char *)"face" : (char *)"faces" );

    for( i = 0; i < num_faces; i++ )
    {
      error = FT_New_Face( library, filename, i, &face );

      printf("\n----- Face number: %d -----\n\n", i);
      Print_Name( face );
      printf("\n");
      Print_Type( face );

      if ( face->num_fixed_sizes )
      {
        printf( "\n" );
        Print_Fixed( face );
      }

      if( face->num_charmaps )
      {
        printf("\n");
        Print_Charmaps( face );
      }

      FT_Done_Face( face );
    }

    FT_Done_FreeType( library );

    exit( 0 );      /* for safety reasons */
    return 0;       /* never reached */
  }


/* End */
