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



  static FT_Error
  Render_GammaGrid( void )
  {
    int   g;
    int   yside  = 11;
    int   xside  = 10;
    int   levels = 17;
    int   gammas = 30;
    int   x_0    = (bit.width - levels*xside)/2;
    int   y_0    = (bit.rows  - gammas*(yside+1))/2;
    int   pitch  = bit.pitch;

    if ( pitch < 0 )
      pitch = -pitch;

    memset( bit.buffer, 100, pitch*bit.rows );

    grGotobitmap( &bit );

    for ( g = 1; g <= gammas; g += 1 )
    {
      double ggamma = g/10.0;
      char   temp[6];
      int    y = y_0 + (yside+1)*(g-1);
      int    nx, ny;

      unsigned char*  line = bit.buffer + y*bit.pitch;

      if ( bit.pitch < 0 )
        line -= bit.pitch*(bit.rows-1);

      line += x_0*3;

      grSetPixelMargin( x_0-32, y + (yside-8)/2 );
      grGotoxy( 0, 0 );

      sprintf( temp, "%.1f", ggamma );
      grWrite( temp );

      for ( ny = 0; ny < yside; ny++, line += bit.pitch )
      {
        unsigned char*  dst = line;

        for ( nx = 0; nx < levels; nx++, dst += 3*xside )
        {
          double  p   = nx/(double)(levels-1);
          int     gm  = 255.0*pow( p, ggamma );

          memset( dst, gm, xside*3 );
        }
      }
    }
    return 0;
  }


  static FT_Error
  Render_Stroke( int  first_index )
  {
    FT_F26Dot6     start_x, start_y, step_x, step_y, x, y;
    FTC_ScalerRec  scaler;
    FT_Stroker     stroker = NULL;
    int            i;
    grBitmap       bit3;


    start_x = 4;
    start_y = 16 + current_font.height;


    scaler.face_id = current_font.face_id;
    scaler.width   = current_font.width;
    scaler.height  = current_font.height;
    scaler.pixel   = 1;

    error = FTC_Manager_LookupSize( cache_manager, &scaler, &size );
    if ( error )
      goto Exit;

    error = FT_Stroker_New( size->face->memory, &stroker );
    if ( error )
      goto Exit;

    FT_Stroker_Set( stroker,
                    64,
                    FT_STROKER_LINECAP_ROUND,
                    FT_STROKER_LINEJOIN_ROUND,
                    0 );

    step_x = size->metrics.x_ppem + 4;
    step_y = ( size->metrics.height >> 6 ) + 4;

    x = start_x;
    y = start_y;

    i = first_index;
    i = first_index;

    while ( i < face->num_glyphs )
    {
      int           left, top, x_advance, y_advance, x_top, y_top;
      FT_GlyphSlot  slot = size->face->glyph;
      FT_Glyph      glyphp;

#if 0
      gindex = *(unsigned char*)p;
      if ( encoding == FT_ENCODING_NONE )
        gindex = get_glyph_index( gindex );
#endif
      error = FT_Load_Glyph( size->face, i, FT_LOAD_NO_BITMAP );
      if ( !error && slot->format == FT_GLYPH_FORMAT_OUTLINE )
      {
        FT_Glyph  glyphb;

        error = FT_Get_Glyph( slot, &glyphp );
        if ( error )
          goto Next;

        error = FT_Glyph_Stroke( &glyphp, stroker, 1 );
        if ( error )
        {
          FT_Done_Glyph( glyphp );
          goto Next;
        }

        error = glyph_to_bitmap( glyphp, &bit3, &left, &top,
                                 &x_advance, &y_advance, (FT_Pointer*)&glyphb );
        if ( !error )
        {
          /* now render the bitmap into the display surface */
          x_top = x + left;
          y_top = y - top;
          grBlitGlyphToBitmap( &bit, &bit3, x_top, y_top, fore_color );

          FT_Done_Glyph( glyphb );
          FT_Done_Glyph( glyphp );

          x += x_advance + 1;

          if ( x + size->metrics.x_ppem > bit.width )
          {
            x  = start_x;
            y += step_y;

            if ( y >= bit.rows )
              goto Exit;
          }
        }
        else
          FT_Done_Glyph( glyphp );

        if ( error )
          goto Next;
      }
      else
      {
      Next:
        Fail++;
      }

      i++;
    }

  Exit:
    if ( stroker )
      FT_Stroker_Done( stroker );

    return error;
  }

  static FT_Error
  Render_All( int  first_index )
  {
    FT_F26Dot6     start_x, start_y, step_x, step_y, x, y;
    FTC_ScalerRec  scaler;
    FT_Pointer     glyf;
    int            i;
    grBitmap       bit3;


    start_x = 4;
    start_y = 16 + current_font.height;

    scaler.face_id = current_font.face_id;
    scaler.width   = current_font.width;
    scaler.height  = current_font.height;
    scaler.pixel   = 1;

    error = FTC_Manager_LookupSize( cache_manager, &scaler, &size );
    if ( error )
    {
      /* probably a non-existent bitmap font size */
      return error;
    }

    step_x = size->metrics.x_ppem + 4;
    step_y = ( size->metrics.height >> 6 ) + 4;

    x = start_x;
    y = start_y;

    i = first_index;

#if 0
    while ( i < first_index + 1 )
#else
    while ( i < num_indices )
#endif
    {
      int  x_top, y_top, left, top, x_advance, y_advance;


      error = get_glyph_bitmap( i, &bit3, &left, &top,
                                &x_advance, &y_advance, &glyf );
      if ( !error )
      {
        /* now render the bitmap into the display surface */
        x_top = x + left;
        y_top = y - top;
        grBlitGlyphToBitmap( &bit, &bit3, x_top, y_top, fore_color );

        if ( glyf )
          done_glyph_bitmap( glyf );

        x += x_advance + 1;

        if ( x + size->metrics.x_ppem > bit.width )
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


  static FT_Error
  Render_Text( int  first_index )
  {
    FT_F26Dot6     start_x, start_y, step_x, step_y, x, y;
    FTC_ScalerRec  scaler;
    FT_Pointer     glyf;
    int            i;
    grBitmap       bit3;

    const unsigned char*  p;


    start_x = 4;
    start_y = 16 + current_font.height;

    scaler.face_id = current_font.face_id;
    scaler.width   = current_font.width;
    scaler.height  = current_font.height;
    scaler.pixel   = 1;

    error = FTC_Manager_LookupSize( cache_manager, &scaler, &size );
    if ( error )
    {
      /* probably a non-existent bitmap font size */
      return error;
    }

    step_x = size->metrics.x_ppem + 4;
    step_y = ( size->metrics.height >> 6 ) + 4;

    x = start_x;
    y = start_y;

    i = first_index;
    p = Text;
    while ( i > 0 && *p )
    {
      p++;
      i--;
    }

    while ( *p )
    {
      int      left, top, x_advance, y_advance, x_top, y_top;
      FT_UInt  gindex;


      gindex = *(unsigned char*)p;
      if ( encoding == FT_ENCODING_NONE )
        gindex = get_glyph_index( gindex );

      /* if a cmap is active, `get_glyph_bitmap' will convert the */
      /* char code in `gindex' to a real glyph index              */
      error = get_glyph_bitmap( gindex, &bit3, &left, &top,
                                &x_advance, &y_advance, &glyf );
      if ( !error )
      {
        /* now render the bitmap into the display surface */
        x_top = x + left;
        y_top = y - top;
        grBlitGlyphToBitmap( &bit, &bit3, x_top, y_top, fore_color );

        if ( glyf )
          done_glyph_bitmap( glyf );

        x += x_advance + 1;

        if ( x + size->metrics.x_ppem > bit.width )
        {
          x  = start_x;
          y += step_y;

          if ( y >= bit.rows )
            return FT_Err_Ok;
        }
      }
      else
        Fail++;

      p++;
    }

    return FT_Err_Ok;
  }


  static FT_Error
  Render_Waterfall( int  first_size )
  {
    FT_F26Dot6  start_x, start_y, step_x, step_y, x, y;
    FT_Pointer  glyf;
    int         pix_size, max_size = 100000;
    grBitmap    bit3;

    unsigned char         text[256];
    const unsigned char*  p;


    start_x = 4;
    start_y = 16;

    pix_size = first_size;

    {
      error = FTC_Manager_LookupFace( cache_manager, current_font.face_id, &face );
      if ( error )
      {
        /* can't access the font file. do not render anything */
        fprintf( stderr, "can't access font file %p\n", current_font.face_id );
        return 0;
      }

      if ( !FT_IS_SCALABLE( face ) )
      {
        int  i;


        max_size = 0;
        for ( i = 0; i < face->num_fixed_sizes; i++ )
          if ( face->available_sizes[i].height >= max_size )
            max_size = face->available_sizes[i].height;
      }
    }

    for (;;)
    {
      FTC_ScalerRec  scaler;


      sprintf( (char*)text,
                "%d: the quick brown fox jumps over the lazy dog "
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", pix_size );

      p = text;

      set_current_size( pix_size );

      if ( pix_size > max_size )
        break;
      pix_size++;

      scaler.face_id = current_font.face_id;
      scaler.width   = current_font.width;
      scaler.height  = current_font.height;
      scaler.pixel   = 1;

      error = FTC_Manager_LookupSize( cache_manager, &scaler, &size );
      if ( error )
      {
        /* probably a non-existent bitmap font size */
        continue;
      }

      step_x = size->metrics.x_ppem + 4;
      step_y = ( size->metrics.height >> 6 ) + 1;

      x = start_x;
      y = start_y + ( size->metrics.ascender >> 6 );

      start_y += step_y;

      if ( y >= bit.rows )
        break;

      while ( *p )
      {
        int      left, top, x_advance, y_advance, x_top, y_top;
        FT_UInt  gindex;


        gindex = *(unsigned char*)p;
        if ( encoding == FT_ENCODING_NONE )
          gindex = get_glyph_index( gindex );

        /* if a cmap is active, `get_glyph_bitmap' will convert the */
        /* char code in `gindex' to a real glyph index              */
        error = get_glyph_bitmap( gindex, &bit3, &left, &top,
                                  &x_advance, &y_advance, &glyf );
        if ( !error )
        {
          /* now render the bitmap into the display surface */
          x_top = x + left;
          y_top = y - top;
          grBlitGlyphToBitmap( &bit, &bit3, x_top, y_top, fore_color );

          if ( glyf )
            done_glyph_bitmap( glyf );

          x += x_advance + 1;

          if ( x + size->metrics.x_ppem > bit.width )
            break;
        }
        else
          Fail++;

        p++;
      }
    }

    set_current_size( first_size );

    return FT_Err_Ok;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                REST OF THE APPLICATION/PROGRAM                *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  static void
  Help( void )
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
    grWriteln( "  L         : cycle through rendering modes" );
    grWriteln( "  h         : toggle outline hinting" );
    grWriteln( "  b         : toggle embedded bitmaps" );
    grWriteln( "  l         : toggle low precision rendering" );
    grWriteln( "  f         : toggle forced auto-hinting" );
    grWriteln( "  space     : toggle rendering mode" );
    grLn();
    grWriteln( "  c         : toggle between cache modes" );
    grLn();
    grWriteln( "  n         : next font" );
    grWriteln( "  p         : previous font" );
    grLn();
    grWriteln( "  g         : increase gamma by 0.1" );
    grWriteln( "  v         : decrease gamma by 0.1" );
    grLn();
    grWriteln( "  K         : show gamma ramp" );
    grLn();
    grWriteln( "  Up        : increase pointsize by 1 unit" );
    grWriteln( "  Down      : decrease pointsize by 1 unit" );
    grWriteln( "  Page Up   : increase pointsize by 10 units" );
    grWriteln( "  Page Down : decrease pointsize by 10 units" );
    grLn();
    grWriteln( "  Right     : increment index" );
    grWriteln( "  Left      : decrement index" );
    grLn();
    grWriteln( "  F7        : decrement index by 10" );
    grWriteln( "  F8        : increment index by 10" );
    grWriteln( "  F9        : decrement index by 100" );
    grWriteln( "  F10       : increment index by 100" );
    grWriteln( "  F11       : decrement index by 1000" );
    grWriteln( "  F12       : increment index by 1000" );
    grLn();
    grWriteln( "press any key to exit this help screen" );

    grRefreshSurface( surface );
    grListenSurface( surface, gr_event_key, &dummy_event );
  }


  static int
  Process_Event( grEvent*  event )
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

    case grKEY( 'g' ):
      if ( the_gamma >= 3.0 )
        the_gamma = 3.0;
      else
        the_gamma += 0.1;

      goto SetGamma;

    case grKEY( 'v' ):
      if ( the_gamma <= 0 )
        the_gamma = 0;
      else
        the_gamma -= 0.1;

      goto SetGamma;

    SetGamma:
      grSetGlyphGamma( the_gamma );
      sprintf( Header, "gamma changed to %.1f %s", the_gamma, the_gamma == 0.0 ? "(sRGB mode)" : "" );
      new_header = Header;
      break;

    case grKEY( 'L' ):
      lcd_mode = ( lcd_mode + 1 ) % 6;

      switch ( lcd_mode )
      {
      case 0:
        new_header = (char *)"normal anti-aliased rendering on";
        break;
      case 1:
        new_header = (char *)"light anti-aliased rendering on";
        break;
      case 2:
        new_header = (char *)"horizontal LCD-optimized rendering on (RGB)";
        break;
      case 3:
        new_header = (char *)"horizontal LCD-optimized rendering on (BGR)";
        break;
      case 4:
        new_header = (char *)"vertical LCD-optimized rendering on (RGB)";
        break;
      case 5:
        new_header = (char *)"vertical LCD-optimized rendering on (BGR)";
        break;
      default:
        ;
      }
      set_current_image_type();
      return 1;

    case grKEY( 'c' ):
      use_sbits_cache = !use_sbits_cache;
      new_header = use_sbits_cache ? (char *)"now using sbits cache"
                                   : (char *)"now using normal cache";
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

    case grKEY( 'K' ):
      render_mode = 4;
      new_header = (char*)"rendering gamma grid";
      break;

    case grKEY( ' ' ):
      render_mode = ( render_mode + 1 ) % 4;
      switch ( render_mode )
      {
        case 0:
          new_header = (char*)"rendering all glyphs in font";
          break;
        case 1:
          new_header = (char*)"rendering test text string";
          break;
        case 2:
          new_header = (char*)"rendering stroked text";
          break;

        default:
          new_header = (char*)"rendering glyph waterfall";
      }
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
    if ( Num < 0 )            Num = 0;
    if ( Num >= num_indices ) Num = num_indices - 1;

    return 1;
  }


  static void
  usage( char*  execname )
  {
    fprintf( stderr,  "\n" );
    fprintf( stderr,  "ftview: simple glyph viewer -- part of the FreeType project\n" );
    fprintf( stderr,  "-----------------------------------------------------------\n" );
    fprintf( stderr,  "\n" );
    fprintf( stderr,  "Usage: %s [options below] ppem fontname[.ttf|.ttc] ...\n",
             execname );
    fprintf( stderr,  "\n" );
    fprintf( stderr,  "  -r R      use resolution R dpi (default: 72 dpi)\n" );
    fprintf( stderr,  "  -f index  specify first index to display\n" );
    fprintf( stderr,  "  -e enc    specify encoding tag (default: no encoding)\n" );
    fprintf( stderr,  "  -D        dump cache usage statistics\n" );
    fprintf( stderr,  "\n" );

    exit( 1 );
  }


  int
  main( int    argc,
        char*  argv[] )
  {
    int          old_ptsize, orig_ptsize, font_index;
    int          first_index = 0;
    int          XisSetup = 0;
    char*        execname;
    int          option;
    const char*  Header_format;

    grEvent      event;


    execname = ft_basename( argv[0] );

    while ( 1 )
    {
      option = getopt( argc, argv, "Dde:f:l:r:" );

      if ( option == -1 )
        break;

      switch ( option )
      {
      case 'd':
        debug = 1;
        break;

      case 'D':
        dump_cache_stats = 1;
        break;

      case 'e':
        encoding = (FT_Encoding)make_tag( optarg );
        break;

      case 'f':
        first_index = atoi( optarg );
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

    Header_format = encoding != FT_ENCODING_NONE
                      ? "at %d points, first char code = 0x%x"
                      : "at %d points, first glyph index = %d";

    if ( argc <= 1 )
      usage( execname );

    if ( sscanf( argv[0], "%d", &orig_ptsize ) != 1 )
      orig_ptsize = 64;

#if FREETYPE_MAJOR == 2 && FREETYPE_MINOR == 0 && FREETYPE_PATCH <= 8
    if ( debug )
    {
#  ifdef FT_DEBUG_LEVEL_TRACE
      FT_SetTraceLevel( trace_any, (FT_Byte)trace_level );
#  else
      trace_level = 0;
#  endif
    }
#elif 0
       /* "setenv/putenv" is not ANSI and I don't want to mess */
       /* with this portability issue right now..              */
    if ( debug )
    {
      char  temp[32];

      sprintf( temp, "any=%d", trace_level );
      setenv( "FT2_DEBUG", temp );
    }
#endif

    /* Initialize engine */
    init_freetype();

    argc--;
    argv++;
    for ( ; argc > 0; argc--, argv++ )
      install_font_file( argv[0] );

    if ( num_fonts == 0 )
      PanicZ( "could not find/open any font file" );

    font_index = 0;
    ptsize     = orig_ptsize;

  NewFile:
    set_current_face( fonts[font_index] );
    set_current_pointsize( ptsize );
    set_current_image_type();
    num_indices = fonts[font_index]->num_indices;

    error = FTC_Manager_LookupFace( cache_manager,
                                    current_font.face_id, &face );
    if ( error )
    {
      /* can't access the font file; do not render anything */
      fprintf( stderr, "can't access font file %p\n",
               current_font.face_id );
      exit( 1 );
    }

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
      Num  = first_index;

      if ( Num >= num_indices )
        Num = num_indices - 1;

      if ( Num < 0 )
        Num = 0;
    }

    for ( ;; )
    {
      int  key;


      Clear_Display();

      if ( num_fonts >= 1 )
      {
        error = FT_Err_Ok;


        switch ( render_mode )
        {
        case 0:
          error = Render_All( Num );
          break;

        case 1:
          error = Render_Text( Num );
          break;

        case 2:
          error = Render_Stroke( Num );
          break;

        case 4:
          error = Render_GammaGrid();
          render_mode = 0;
          break;

        default:
          error = Render_Waterfall( ptsize );
        }

        if ( !new_header )
        {
          if ( size )
            sprintf( Header, "%s %s (file `%s')",
              face->family_name,
              face->style_name,
              ft_basename( ((PFont)current_font.face_id)->filepathname ) );
          else
          {
            if ( error == FT_Err_Invalid_Pixel_Size )
              sprintf( Header, "Invalid pixel size (file `%s')",
                ft_basename( ((PFont)current_font.face_id)->filepathname ) );
            else if ( error == FT_Err_Invalid_PPem )
              sprintf( Header, "Invalid ppem value (file `%s')",
                ft_basename( ((PFont)current_font.face_id)->filepathname ) );
            else
              sprintf( Header, "File `%s': error 0x%04x",
                ft_basename( ((PFont)current_font.face_id)->filepathname ),
                (FT_UShort)error );
          }

          new_header = Header;
        }

        grWriteCellString( &bit, 0, 0, new_header, fore_color );
        new_header = 0;

        sprintf( Header, Header_format, ptsize, Num );
      }

      grWriteCellString( &bit, 0, 8, Header, fore_color );
      grRefreshSurface( surface );

      if ( dump_cache_stats )
      {
        /* dump simple cache manager statistics */
        fprintf( stderr, "cache manager [ nodes, bytes, average ] = "
                         " [ %d, %ld, %f ]\n",
                         cache_manager->num_nodes,
                         cache_manager->cur_weight,
                         cache_manager->num_nodes > 0
                           ? cache_manager->cur_weight * 1.0 /
                               cache_manager->num_nodes
                           : 0.0 );
      }

      grListenSurface( surface, 0, &event );
      if ( !( key = Process_Event( &event ) ) )
        goto End;

      if ( key == 'n' )
      {
        if ( font_index + 1 < num_fonts )
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
