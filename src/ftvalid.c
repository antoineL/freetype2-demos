/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality font engine         */
/*                                                                          */
/*  Copyright 2005 by                                                       */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*  ftvalid: Validates layout related tables of OpenType.  This program     */
/*           calls `FT_OpenType_Validate' on a given file, and reports      */
/*           the validation result.                                         */
/*                                                                          */
/*  written by YAMATO Masatake and SUZUKI Toshiya.                          */
/*                                                                          */
/****************************************************************************/


#include <ft2build.h>

#include FT_FREETYPE_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_VALIDATE_H
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H
#include FT_INTERNAL_MEMORY_H
#include FT_INTERNAL_OBJECTS_H

#include FT_OPENTYPE_VALIDATE_H


#include "common.h"


#include <stdio.h>
#include <string.h>
#include <stdlib.h>


  static char*  execname;
  
  typedef struct  TableSpecRec_
  {
    FT_UInt  tag;
    FT_UInt  validation_flag;

  } TableSpecRec, *TableSpec;


  typedef enum 
  { 
#define OT_VALIDATOR_SYMBOL "ot" 
    OT_VALIDATE = 0,

  } ValidatorType;

  static const char*  validator_symbols[] = { OT_VALIDATOR_SYMBOL, };

  static ValidatorType  validator;

  
#define MAKE_TABLE_SPEC( x ) { TTAG_##x, FT_VALIDATE_##x }

  static const TableSpecRec  ot_table_spec[] = 
  {
    MAKE_TABLE_SPEC( BASE ),
    MAKE_TABLE_SPEC( GDEF ),
    MAKE_TABLE_SPEC( GPOS ),
    MAKE_TABLE_SPEC( GSUB ),
    MAKE_TABLE_SPEC( JSTF ),
  };
#define N_OT_TABLE_SPEC  (sizeof ( ot_table_spec ) / sizeof ( TableSpecRec ) )


  static void
  panic( int          error,
         const char*  message )
  {
    fprintf( stderr, "%s\n  error = 0x%04x\n", message, error );
    exit( 1 );
  }


  static char*
  make_tag_chararray ( char     chararray[4],
                       FT_UInt  tag )
  {
    chararray[0] = (char)( ( tag >> 24 ) & 0xFF );
    chararray[1] = (char)( ( tag >> 16 ) & 0xFF );
    chararray[2] = (char)( ( tag >> 8  ) & 0xFF );
    chararray[3] = (char)( ( tag >> 0  ) & 0xFF );
    return chararray;
  }


  static void
  print_tag ( FILE*    stream,
              FT_UInt  tag )
  {
    char  buffer[5];


    buffer[4] = '\0';
    fprintf( stream, "%s", make_tag_chararray( buffer, tag ) );
  }


  static void
  print_usage( void )
  {
    unsigned int i;


    fprintf( stderr, "\n" );
    fprintf( stderr, "ftvalid: layout table validator -- part of the FreeType project\n" );
    fprintf( stderr, "---------------------------------------------------------------\n" );
    fprintf( stderr, "\n" );
    fprintf( stderr, "Usage: %s [options] fontfile\n", execname );
    fprintf( stderr, "\n" );
    fprintf( stderr, "  -t validator              select validator. \n");
    fprintf( stderr, "                            Currently only \"ot\" is available.\n" );
    fprintf( stderr, "\n" );
    fprintf( stderr, "  -T \"sfnt:tabl:enam:es  \"  select snft table names to be validated.\n" );
    fprintf( stderr, "                            `:' is for separating table names.\n" );
    fprintf( stderr, "                            Supported tables in ot validator are:\n" );

    fprintf( stderr, "                            " );

    for ( i = 0; i < N_OT_TABLE_SPEC; i++ )
    {
      print_tag( stderr, ot_table_spec[i].tag );
      fprintf( stderr, " " );
    }

    fprintf( stderr, "\n" );
    fprintf( stderr, "\n" );
    fprintf( stderr, "  -L                        list the layout related SFNT tables\n" );
    fprintf( stderr, "                            available in the font file. Choice of\n" );
    fprintf( stderr, "                            validator with -t option affects on the\n" );
    fprintf( stderr, "                            listing.\n" );
    fprintf( stderr, "\n" );
    fprintf( stderr, "  -v validation_level       validation level. \n" );
    fprintf( stderr, "                            validation_level = 0...2\n" );
    fprintf( stderr, "                            (0: default, 1: tight, 2: paranoid)\n" );

#if 0
    fprintf( stderr, "  -l trace_level            trace level for debug information.\n" );
    fprintf( stderr, "                            trace_level = 1...7\n" );
#endif /* 0 */

    fprintf( stderr, "-------------------------------------------------------------------\n" );
    fprintf( stderr, "\n" );
    fprintf( stderr, "Environment variable\n" );
    fprintf( stderr, "FT2_DEBUG: You can specify trace components and their levels[1-7]\n" );
    fprintf( stderr, "           to it like FT2_DEBUG=\"module1:level module2:level...\".\n" );
    fprintf( stderr, "           Available components for ot validator:\n" );
    fprintf( stderr, "           otvmodule otvcommon otvbase otvgdef otvgpos otvgsub otvjstf\n" );
    fprintf( stderr, "\n" );

    exit( 1 );
  }


  static FT_Error
  try_load( FT_Face   face,
            FT_ULong  tag )
  {
    FT_ULong  length;


    length = 0;
    return FT_Load_Sfnt_Table( face, tag, 0, NULL, &length );
  }


  static FT_UInt
  find_validation_flag( FT_UInt             tag,
                        const TableSpecRec  spec[],
                        int                 spec_count )
  {
    int i;


    for ( i = 0; i < spec_count; i++ )
    {
      if ( tag == spec[i].tag )
        return spec[i].validation_flag;
    }
    
    fprintf( stderr, "*** Wrong table name: " );
    print_tag( stderr, tag );
    fprintf( stderr, "\n" );

    print_usage();

    return 0;
  }


  static FT_UInt
  parse_table_specs( const char*         tables,
                     const TableSpecRec  spec[],
                     int                 spec_count )
  {
    FT_UInt  validation_flags;
    size_t   len;

    unsigned int  i;
    char          tag[4];
    

    validation_flags = 0;

    len = strlen( tables );
    if (( len % 5 ) != 4 )
    {
      fprintf( stderr, "*** Wrong length of table names\n" );
      print_usage();
    }
    
    for ( i = 0; i < len; i++ )
    {
      if ( ( ( i % 5 ) == 4 ) )
      {
        if ( tables[i] != ':' )
        {
          fprintf( stderr, "*** Wrong table separator: %c\n", tables[i] );
          print_usage();
        }
        i++;
      }

      tag[i % 5] = tables[i];
      if ( ( i % 5 ) == 3 )
        validation_flags |= find_validation_flag( FT_MAKE_TAG( tag[0],
                                                               tag[1],
                                                               tag[2],
                                                               tag[3] ), 
                                                  spec, 
                                                  spec_count );
    }

    return validation_flags;
  }


  static FT_UInt
  list_face_tables( FT_Face             face,
                    const TableSpecRec  spec[],
                    int                 spec_count )
  {
    FT_Error  error;

    FT_UInt  validation_flags;
    int      i;
    FT_UInt  tag;
    

    validation_flags = 0;
    
    for ( i = 0; i < spec_count; i++ )
    {
      tag   = spec[i].tag;
      error = try_load( face, tag );
      if ( error == 0 )
        validation_flags |= spec[i].validation_flag;
    }
    return validation_flags;
  }


  static FT_UInt
  make_table_specs( FT_Face             face,
                    const char*         request,
                    const TableSpecRec  spec[],
                    int                 spec_count )
  {
    if ( request == NULL || request[0] == '\0' )
      return list_face_tables ( face, spec, spec_count );
    else
      return parse_table_specs ( request, spec, spec_count );
  }


  static int
  print_tables( FILE*               stream,
                FT_UInt             validation_flags,
                const TableSpecRec  spec[],
                int                 spec_count )
  {
    int i;
    int n_print;
    

    for ( i = 0, n_print = 0; i < spec_count; i++ )
    {
      if ( spec[i].validation_flag & validation_flags )
      {
        if ( n_print != 0 )
          fprintf( stream, "%c", ':' );
        print_tag( stream, spec[i].tag );
        n_print++;
      }
    }
    
    fprintf( stream, "\n" );

    return !n_print;
  }


  static void
  report_header( FT_UInt             validation_flags,
                 const TableSpecRec  spec[],
                 int                 spec_count )
  {
    printf( "[%s:%s] validation targets: ",
            execname, validator_symbols[validator] );
    print_tables( stdout, validation_flags, spec, spec_count );
    printf( "-------------------------------------------------------------------\n" );
  }


  static void
  report_result( FT_Bytes            data[],
                 FT_UInt             validation_flags,
                 const TableSpecRec  spec[],
                 int                 spec_count )
  {
    int  i;
    int  n_passes;
    

    for ( i = 0, n_passes = 0; i < spec_count; i++ )
    {
      if ( ( spec[i].validation_flag & validation_flags ) &&
             data[i] != NULL                              )
      {
        printf( "[%s:%s] ", execname, validator_symbols[validator] );
        print_tag( stdout, spec[i].tag );
        fprintf( stdout, "...pass\n" );
        n_passes++;
      }
    }

    if ( n_passes == 0 )
    {
      printf( "[%s:%s] layout tables are not invalid.\n",
              execname, validator_symbols[validator] );
      printf( "[%s:%s] set FT2_DEBUG environment variable to \n",
              execname, validator_symbols[validator] );
      printf( "[%s:%s] know the validation detail. \n",
              execname, validator_symbols[validator] );
    }
  }


  /* 
   * OpenType related funtions
   */
  static int
  run_ot_validator( FT_Face      face,
                    const char*  tables,
                    int          validation_level )
  {
    FT_UInt       validation_flags;
    FT_Error      error;
    FT_Bytes      data[N_OT_TABLE_SPEC];
    unsigned int  i;
    FT_Memory     memory = FT_FACE_MEMORY( face );
    

    validation_flags  = validation_level;
    validation_flags |= make_table_specs( face, tables, ot_table_spec,
                                          N_OT_TABLE_SPEC );

    for ( i = 0; i < N_OT_TABLE_SPEC; i++ )
      data[i] = NULL;

    report_header( validation_flags, ot_table_spec, N_OT_TABLE_SPEC );

    error = FT_OpenType_Validate(
              face, 
              validation_flags, 
              &data[0], &data[1], &data[2], &data[3], &data[4] );
      
    report_result( data, validation_flags, ot_table_spec, N_OT_TABLE_SPEC );

    for ( i = 0; i < N_OT_TABLE_SPEC; i++ )
      FT_FREE( data[i] );

    return (int)error;
  }


  static int
  list_ot_tables( FT_Face  face )
  {
    FT_UInt  validation_flags;


    validation_flags = list_face_tables( face, ot_table_spec,
                                         N_OT_TABLE_SPEC );
    return print_tables( stdout, validation_flags, ot_table_spec,
                         N_OT_TABLE_SPEC );
  }


  /*
   * Main driver
   */

  int
  main( int     argc, 
        char**  argv )
  {
    char*  fontfile;
    int    option;
    int    status;

    char*  tables;
    int    dump_table_list;

    FT_ValidationLevel  validation_level;
#if 0
    int  trace_level;
#endif  /* 0 */
    
    execname = ft_basename( argv[0] );

    /*
     * Parsing options
     */
    validator        = OT_VALIDATE;
    tables           = NULL;
    dump_table_list  = 0;
    validation_level = FT_VALIDATE_DEFAULT;
#if 0
    trace_level      = 0;
#endif  /* 0 */

    while ( 1 )
    {
      option = getopt( argc, argv, "t:T:Lv:l:" );

      if ( option == -1 )
        break;
    
      switch ( option )
      {
      case 't':
        if ( strcmp( optarg, OT_VALIDATOR_SYMBOL ) == 0 )
          validator = OT_VALIDATE;
        else
        {
          fprintf( stderr, "*** Unknown validator name: %s\n", optarg );
          print_usage();
        }
        break;

      case 'T':
        tables = optarg;
        break;

      case 'L':
        dump_table_list = 1;
        break;

      case 'v':
        validation_level = atoi( optarg );
        if ( validation_level > FT_VALIDATE_PARANOID )
        {
          fprintf( stderr, "*** Validation level is out of range: %d\n",
                           validation_level );
          print_usage();
        }
        break;

      default:
        print_usage();
        break;
      }
    }

    argc -= optind;
    argv += optind;

    if ( argc == 0 )
    {
      fprintf(stderr, "*** Font file is not specified.\n");
      print_usage();
    }
    else if ( argc > 1 )
    {
      fprintf(stderr, "*** Too many font files.\n");
      print_usage();
    }

    fontfile = argv[0];

#if 0
    printf( "fontfile: %s\n",
            fontfile);
    printf( "validator type: %s\n",
            ( validator == OT_VALIDATE ) ? OT_VALIDATOR_SYMBOL : "unknown" );
    printf( "tables: %s\n",
            ( tables != NULL ) ? tables : "unspecified" );
    printf( "action: %s\n",
            ( dump_table_list == 1 ) ? "list" : "validate" );
    printf( "validation level: %d\n",
            validation_level );
#if 0
    printf( "trace level: %d\n", trace_level );
#endif /* 0 */
#endif /* 0 */


    /*
     * Run a validator
     */
    {
      FT_Library  library;
      FT_Face     face;
      FT_Error    error;


      status = 0;

      error = FT_Init_FreeType( &library );
      if ( error )
        panic ( error, "Could not initialize FreeType library" );

      /* TODO: Multiple faces in a font file? */
      error = FT_New_Face( library, fontfile, 0, &face );
      if ( error )
        panic( error, "Could not open face." );
      
      if ( validator == OT_VALIDATE )
      {
        if ( dump_table_list == 0 )
          status = run_ot_validator( face, tables, validation_level );
        else
          status = list_ot_tables  ( face );
      }
      /* ...More validator here */
      
      FT_Done_FreeType( library );
    }

    return status;
  }


/* End */
