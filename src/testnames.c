# include <stdio.h>

#include <string.h>

#include <stdlib.h>

#include <math.h>

#include <freetype/freetype.h>

int main( void )
{
   FT_Library  font_library;
   FT_Face font_face;
   FT_Matrix matrix;
   FT_Bitmap bitmap;
   FT_GlyphSlot  cur_glyph;
   FT_Glyph_Metrics glyph_metrics;
   int glyph_ind;
   int num_chars;
   char char_name[256];

     
   if ( FT_Init_FreeType( &font_library ) )
     exit;
   
   if ( FT_New_Face( font_library , "c:/winnt/fonts/times.ttf" , 0 , &font_face ) )
     exit;
   if ( FT_Set_Char_Size( font_face , 0 , 768 , 300 , 300 ) )
     exit;
   num_chars = (int) font_face->num_glyphs;
   FT_Set_Transform( font_face , NULL , NULL );

   for ( glyph_ind = 0 ; glyph_ind < num_chars; glyph_ind++ )
     {
        if ( FT_Load_Glyph( font_face , glyph_ind , FT_LOAD_DEFAULT ) )
          exit;
        cur_glyph = font_face->glyph;
        if ( cur_glyph->format != ft_glyph_format_bitmap )
          if ( FT_Render_Glyph( font_face->glyph , ft_render_mode_mono ) )
            exit;
        if ( FT_Get_Glyph_Name( font_face , glyph_ind , char_name , 16 ) )
          exit;
        bitmap = cur_glyph->bitmap;
        glyph_metrics = cur_glyph->metrics;
        printf( "Glyph %d  name %s %ld %ld %ld %d %d\n" , glyph_ind , char_name ,
                glyph_metrics.horiBearingX / 64 ,
                glyph_metrics.horiBearingY / 64 ,
                glyph_metrics.horiAdvance / 64 ,
                bitmap.width , bitmap.rows );
     }
     
  return 0;
}