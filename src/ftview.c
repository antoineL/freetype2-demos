/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-2000 by                                                  */
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

  static
  FT_Error  Render_All( int  first_glyph )
  {
    FT_F26Dot6  start_x, start_y, step_x, step_y, x, y;
    int         i;
    grBitmap    bit3;

    start_x = 4;
    start_y = 16 + current_font.font.pix_height;

    error = FTC_Manager_Lookup_Size( manager, &current_font.font, &face, &size );
    if (error)
    {
      /* this should never happen !! */
      return 0;
    }

    step_x = size->metrics.x_ppem + 4;
    step_y = (size->metrics.height >> 6) + 4;

    x = start_x;
    y = start_y;

    i = first_glyph;

#if 0
    while ( i < first_glyph + 2 )
#else
    while ( i < num_glyphs )
#endif
    {
      FT_Glyph  glyph;
      
      error = FTC_Image_Cache_Lookup( image_cache,
                                      &current_font,
                                      i,
                                      &glyph );
      if (!error)
      {
        FT_BitmapGlyph  bitmap = (FT_BitmapGlyph)glyph;
        FT_Bitmap*      source = &bitmap->bitmap;
        FT_Pos          x_top, y_top;
        
        if (glyph->format != ft_glyph_format_bitmap)
          PanicZ( "invalid glyph format returned !!" );
          
        bit3.rows   = source->rows;
        bit3.width  = source->width;
        bit3.pitch  = source->pitch;
        bit3.buffer = source->buffer;

        switch (source->pixel_mode)
        {
          case ft_pixel_mode_mono:
            bit3.mode  = gr_pixel_mode_mono;
            break;

          case ft_pixel_mode_grays:
            bit3.mode  = gr_pixel_mode_gray;
            bit3.grays = source->num_grays;
            break;

          default:
            continue;
        }
    
        /* now render the bitmap into the display surface */
        x_top = x + bitmap->left;
        y_top = y - bitmap->top;
        grBlitGlyphToBitmap( &bit, &bit3, x_top, y_top, fore_color );

        x += ( glyph->advance.x >> 16 ) + 1;

        if ( x > bit.width )
        {
          x  = start_x;
          y += step_y;

          if ( y >= bit.rows )
            return FT_Err_Ok;
        }
      }
      else
        Fail++;

      i++;
    }

    return FT_Err_Ok;
  }



  static
  FT_Error  Render_Text( int  first_glyph )
  {
    FT_F26Dot6  start_x, start_y, step_x, step_y, x, y;
    int         i;
    grBitmap    bit3;

    const unsigned char*  p;

    start_x = 4;
    start_y = 16 + current_font.font.pix_height;

    error = FTC_Manager_Lookup_Size( manager, &current_font.font, &face, &size );
    if (error)
    {
      /* this should never happen */
      return 0;
    }

    step_x = size->metrics.x_ppem + 4;
    step_y = (size->metrics.height >> 6) + 4;

    x = start_x;
    y = start_y;

    i = first_glyph;
    p = Text;
    while ( i > 0 && *p )
    {
      p++;
      i--;
    }

    while ( *p )
    {
      FT_Glyph  glyph;
      
      error = FTC_Image_Cache_Lookup( image_cache,
                                      &current_font,
                                      FT_Get_Char_Index( face, *p ),
                                      &glyph );
      if (!error)
      {
        FT_BitmapGlyph  bitmap = (FT_BitmapGlyph)glyph;
        FT_Bitmap*      source = &bitmap->bitmap;
        FT_Pos          x_top, y_top;
        
        if (glyph->format != ft_glyph_format_bitmap)
          PanicZ( "invalid glyph format returned !!" );
          
        bit3.rows   = source->rows;
        bit3.width  = source->width;
        bit3.pitch  = source->pitch;
        bit3.buffer = source->buffer;

        switch (source->pixel_mode)
        {
          case ft_pixel_mode_mono:
            bit3.mode  = gr_pixel_mode_mono;
            break;

          case ft_pixel_mode_grays:
            bit3.mode  = gr_pixel_mode_gray;
            bit3.grays = source->num_grays;
            break;

          default:
            continue;
        }
    
        /* now render the bitmap into the display surface */
        x_top = x + bitmap->left;
        y_top = y - bitmap->top;
        grBlitGlyphToBitmap( &bit, &bit3, x_top, y_top, fore_color );

        x += ( glyph->advance.x >> 16 ) + 1;

        if ( x > bit.width )
        {
          x  = start_x;
          y += step_y;

          if ( y >= bit.rows )
            return FT_Err_Ok;
        }
      }
      else
        Fail++;

      i++;
      p++;
    }

    return FT_Err_Ok;
  }




 /*************************************************************************/
 /*************************************************************************/
 /*****                                                               *****/
 /*****                    REST OF THE APPLICATION/PROGRAM            *****/
 /*****                                                               *****/
 /*************************************************************************/
 /*************************************************************************/

  static
  void Help( void )
  {
    grEvent  dummy_event;


    Clear_Display();
    grGotoxy( 0, 0 );
    grSetMargin( 2, 1 );
    grGotobitmap( &bit );

    grWriteln( "FreeType Glyph Viewer - part of the FreeType test suite" );
    grLn();
    grWriteln( "This program is used to display all glyphs from one or" );
    grWriteln( "several font files, with the FreeType library." );
    grLn();
    grWriteln( "Use the following keys:" );
    grLn();
    grWriteln( "  F1 or ?   : display this help screen" );
    grWriteln( "  a         : toggle anti-aliasing" );
    grWriteln( "  h         : toggle outline hinting" );
    grWriteln( "  b         : toggle embedded bitmaps" );
    grWriteln( "  l         : toggle low precision rendering" );
    grWriteln( "  f         : toggle force auto-hinting" );
    grWriteln( "  space     : toggle rendering mode" );
    grLn();
    grWriteln( "  n         : next font" );
    grWriteln( "  p         : previous font" );
    grLn();
    grWriteln( "  Up        : increase pointsize by 1 unit" );
    grWriteln( "  Down      : decrease pointsize by 1 unit" );
    grWriteln( "  Page Up   : increase pointsize by 10 units" );
    grWriteln( "  Page Down : decrease pointsize by 10 units" );
    grLn();
    grWriteln( "  Right     : increment first glyph index" );
    grWriteln( "  Left      : decrement first glyph index" );
    grLn();
    grWriteln( "  F7        : decrement first glyph index by 10" );
    grWriteln( "  F8        : increment first glyph index by 10" );
    grWriteln( "  F9        : decrement first glyph index by 100" );
    grWriteln( "  F10       : increment first glyph index by 100" );
    grWriteln( "  F11       : decrement first glyph index by 1000" );
    grWriteln( "  F12       : increment first glyph index by 1000" );
    grLn();
    grWriteln( "press any key to exit this help screen" );

    grRefreshSurface( surface );
    grListenSurface( surface, gr_event_key, &dummy_event );
  }


  static
  int  Process_Event( grEvent*  event )
  {
    int  i;


    switch ( event->key )
    {
    case grKeyEsc:            /* ESC or q */
    case grKEY( 'q' ):
      return 0;

    case grKEY( 'a' ):
      antialias  = !antialias;
      new_header = antialias ? (char *)"anti-aliasing is now on"
                             : (char *)"anti-aliasing is now off";
      set_current_image_type();
      return 1;

    case grKEY( 'f' ):
      autohint = !autohint;
      new_header = autohint ? (char *)"forced auto-hinting is now on"
                            : (char *)"forced auto-hinting is now off";
      set_current_image_type();
      return 1;
      
    case grKEY( 'b' ):
      use_sbits  = !use_sbits;
      new_header = use_sbits
                     ? (char *)"embedded bitmaps are now used when available"
                     : (char *)"embedded bitmaps are now ignored";
      set_current_image_type();
      return 1;

    case grKEY( 'n' ):
    case grKEY( 'p' ):
      return (int)event->key;

    case grKEY( 'l' ):
      low_prec   = !low_prec;
      new_header = low_prec
                     ? (char *)"rendering precision is now forced to low"
                     : (char *)"rendering precision is now normal";
      break;

    case grKEY( 'h' ):
      hinted     = !hinted;
      new_header = hinted ? (char *)"glyph hinting is now active"
                          : (char *)"glyph hinting is now ignored";
      set_current_image_type();
      break;

    case grKEY( ' ' ):
      render_mode ^= 1;
      new_header   = render_mode ? (char *)"rendering all glyphs in font"
                                 : (char *)"rendering test text string";
      break;

    case grKeyF1:
    case grKEY( '?' ):
      Help();
      return 1;

    case grKeyPageUp:   i =    10; goto Do_Scale;
    case grKeyPageDown: i =   -10; goto Do_Scale;
    case grKeyUp:       i =     1; goto Do_Scale;
    case grKeyDown:     i =    -1; goto Do_Scale;

    case grKeyLeft:     i =    -1; goto Do_Glyph;
    case grKeyRight:    i =     1; goto Do_Glyph;
    case grKeyF7:       i =   -10; goto Do_Glyph;
    case grKeyF8:       i =    10; goto Do_Glyph;
    case grKeyF9:       i =  -100; goto Do_Glyph;
    case grKeyF10:      i =   100; goto Do_Glyph;
    case grKeyF11:      i = -1000; goto Do_Glyph;
    case grKeyF12:      i =  1000; goto Do_Glyph;

    default:
      ;
    }
    return 1;

  Do_Scale:
    ptsize += i;
    if ( ptsize < 1 )         ptsize = 1;
    if ( ptsize > MAXPTSIZE ) ptsize = MAXPTSIZE;
    return 1;

  Do_Glyph:
    Num += i;
    if ( Num < 0 )           Num = 0;
    if ( Num >= num_glyphs ) Num = num_glyphs - 1;
    return 1;
  }


  static
  void  usage( char*  execname )
  {
    fprintf( stderr,  "\n" );
    fprintf( stderr,  "ftview: simple glyph viewer -- part of the FreeType project\n" );
    fprintf( stderr,  "-----------------------------------------------------------\n" );
    fprintf( stderr,  "\n" );
    fprintf( stderr,  "Usage: %s [options below] ppem fontname[.ttf|.ttc] ...\n",
             execname );
    fprintf( stderr,  "\n" );
    fprintf( stderr,  "  -d        enable debugging messages\n" );
    fprintf( stderr,  "  -l N      set debugging trace level to N (default: 0, max: 7)\n" );
    fprintf( stderr,  "  -r R      use resolution R dpi (default: 72 dpi)\n" );
    fprintf( stderr,  "  -f index  specify first glyph index to display\n" );
    fprintf( stderr,  "\n" );

    exit( 1 );
  }


  int  main( int    argc,
             char*  argv[] )
  {
    int    old_ptsize, orig_ptsize, font_index;
    int    first_glyph = 0;
    int    XisSetup = 0;
    char*  execname;
    int    option;

    grEvent   event;


    execname = ft_basename( argv[0] );

    while ( 1 )
    {
      option = getopt( argc, argv, "df:l:r:" );

      if ( option == -1 )
        break;

      switch ( option )
      {
      case 'd':
        debug = 1;
        break;

      case 'f':
        first_glyph = atoi( optarg );
        break;

      case 'l':
        trace_level = atoi( optarg );
        if ( trace_level < 1 || trace_level > 7 )
          usage( execname );
        break;

      case 'r':
        res = atoi( optarg );
        if ( res < 1 )
          usage( execname );

        break;

      default:
        usage( execname );
        break;
      }
    }

    argc -= optind;
    argv += optind;

    if ( argc <= 1 )
      usage( execname );

    if ( sscanf( argv[0], "%d", &orig_ptsize ) != 1 )
      orig_ptsize = 64;

    if ( debug )
    {
      trace_level = 0;
    }

    /* Initialize engine */
    init_freetype();

    argc--;
    argv++;
    for ( ; argc > 0; argc--, argv++ )
      install_font_file( argv[0] );

    if (num_fonts == 0)
      PanicZ( "could not find/open any font file" );
      
    
    font_index = 0;
    ptsize     = orig_ptsize;
    
  NewFile:
    set_current_face( fonts[ font_index ] );
    set_current_pointsize( ptsize );
    set_current_image_type();
    num_glyphs = fonts[font_index]->num_glyphs;

    /* initialize graphics if needed */
    if ( !XisSetup )
    {
      XisSetup = 1;
      Init_Display();
    }

    grSetTitle( surface, "FreeType Glyph Viewer - press F1 for help" );
    old_ptsize = ptsize;

    if ( num_fonts >= 1 )
    {
      Fail = 0;
      Num  = first_glyph;

      if ( Num >= num_glyphs )
        Num = num_glyphs - 1;

      if ( Num < 0 )
        Num = 0;
    }

    for ( ;; )
    {
      int  key;


      Clear_Display();

      if ( num_fonts >= 1 )
      {
        switch (render_mode)
        {
          case 0:
            Render_Text( Num );
            break;
            
          default:
            Render_All( Num );
        }

        sprintf( Header, "%s %s (file `%s')",
                         face->family_name,
                         face->style_name,
                         ft_basename( ((PFont)current_font.font.face_id)->filepathname ) );

        if ( !new_header )
          new_header = Header;

        grWriteCellString( &bit, 0, 0, new_header, fore_color );
        new_header = 0;

        sprintf( Header, "at %d points, first glyph = %d",
                         ptsize,
                         Num );
      }

      grWriteCellString( &bit, 0, 8, Header, fore_color );
      grRefreshSurface( surface );

      grListenSurface( surface, 0, &event );
      if ( !( key = Process_Event( &event ) ) )
        goto End;

      if ( key == 'n' )
      {
        if ( font_index+1 < num_fonts )
          font_index++;

        goto NewFile;
      }

      if ( key == 'p' )
      {
        if ( font_index > 0 )
          font_index--;

        goto NewFile;
      }

      if ( ptsize != old_ptsize )
      {
        set_current_pointsize( ptsize );
        old_ptsize = ptsize;
      }
    }

  End:

    printf( "Execution completed successfully.\n" );
    printf( "Fails = %d\n", Fail );

    done_freetype();
    exit( 0 );      /* for safety reasons */
    return 0;       /* never reached */
}


/* End */
