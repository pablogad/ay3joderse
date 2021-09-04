#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <zlib.h>

#include "vgm_170.h"
#include "ay-3-8910.h"
#include "gen_pwm.h"

#ifdef DEBUG
#define DEBUG 1
#else
#define DEBUG 0
#endif

// Descomprimir VGZ en memoria o leer VGM
static uint8_t* vg = NULL;
static long vg_size = 0;

static uint32_t cnt_samples = 0;

#define CHUNK_SIZE 32768
char chunk[ CHUNK_SIZE ];


// Asume vg apunta a datos. Al volver vg apunta a datos descomprimidos
int inflate_vgz(  ) {
   z_stream strm;
   strm.zalloc = Z_NULL;
   strm.zfree = Z_NULL;
   strm.opaque = Z_NULL;
   // Descomprimir todo el buffer
   strm.avail_in = vg_size;
   strm.next_in = vg;

   int ret = inflateInit2( &strm, 16+MAX_WBITS );
   if( ret == Z_OK ) {

      char* new_vg = NULL;
      long new_vg_size = 0L;

      while( 1 ) {
         strm.avail_out = CHUNK_SIZE;
	 strm.next_out = chunk;  // Leer siguiente porción de entrada
         ret = inflate( &strm, Z_NO_FLUSH );
	 if( ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR ) {
            printf( "Error %d en inflate, '%s'\n", ret, strm.msg ? strm.msg : "???" );
            break;
	 }
	 if( ret != Z_STREAM_ERROR ) {
            // Buffer inflado
	    char* ptr = new_vg;
	    long out_len = CHUNK_SIZE - strm.avail_out;

	    if( new_vg == NULL ) {
	       new_vg_size = out_len;
               new_vg = (char*)malloc( new_vg_size );
	       ptr = new_vg;
	    }
	    else {
               new_vg = (char*)realloc( new_vg, new_vg_size + out_len );
               ptr = &new_vg[ new_vg_size ];
               new_vg_size += out_len;
	    }
 
	    memcpy( ptr, chunk, out_len );
	 }
	 else {
            printf( "Stream error! :( \n" );
            break;
	 }

         if( ret == Z_STREAM_END ) {
            ret = 0;  // Fin, OK
	    free( vg );
	    vg = new_vg;
	    vg_size = new_vg_size;
	    printf( "-- Descomprimidos %lu bytes\n", vg_size );
	    break;
	 }
      }

      inflateEnd(&strm);

      if( new_vg && ret ) free( new_vg );
   }

   return ret;
}

// Esperar número de samples (cada sample dura 1/44100 seg)
void wait_samples( const uint32_t nsamples ) {
   const uint64_t t_1s = 1000000/44100;
   cnt_samples += nsamples;
#if DEBUG
   printf( " -- WAIT %d samples, van %d\n", nsamples, cnt_samples );
#endif

   delay_us_ay3( nsamples * t_1s );
}

// Devolver puntero al siguiente dato
static inline uint8_t* next_note( uint8_t* cur_note ) {
   uint8_t note = *cur_note;
#if DEBUG
   printf( ">> Data @ %p: %02X", cur_note, note );
#endif

   int adic_bytes = 0;

   if( ( note >= 0x4F && note <= 0x50 ) ||
       ( note >= 0x30 && note < 0x40 ) ) {
      adic_bytes = 1;
   }
   else if( ( note > 0x50 && note < 0x62 ) ||
            ( note > 0x9F && note < 0xC0 ) ||
            ( note > 0x3F && note < 0x4F ) ||
            ( note > 0xA0 && note < 0xB0 ) ||
            ( note > 0xBB && note < 0xC0 ) ) {
      adic_bytes = 2;
   }
   else if( ( note >= 0xC0 && note <= 0xD4 ) ||
            ( note >= 0xC5 && note < 0xD0 ) ||
            ( note >= 0xD5 && note <= 0xE0 ) ) {
      adic_bytes = 3;
   }
   else if( note >= 0xE1 && note <= 0xFF ) {
      adic_bytes = 4;
   }
   else if( note == 0x67 ) {
      if( cur_note[1] != 0x66 ) {
         printf( "ERROR - data block no tiene byte 0x66\n" );
	 exit(-1);
      }
      adic_bytes = 6 + ( ( ( cur_note[6] * 256 ) + cur_note[5] * 256 ) + cur_note[4] * 256 ) + cur_note[3];
   }
   else if( note == 0x68 ) {
      if( cur_note[1] != 0x66 ) {
         printf( "ERROR - PCM block no tiene byte 0x66\n" );
	 exit(-1);
      }
      adic_bytes = 8 + ( ( cur_note[9] * 256 ) + cur_note[10] * 256 ) + cur_note[11];
   }

#if DEBUG
   for( int i=1; i<=adic_bytes; i++ )
      printf( " %02X", cur_note[i] );
#endif

   // Imprimir interpretacion
#if DEBUG
   if( note == 0x61 ) {
      printf( "  -> WAIT %d SAMPLES", cur_note[2]*256 + cur_note[1] );
   }
   else if( note == 0x62 ) printf( "  -> WAIT 735 SAMPLES" );
   else if( note == 0x63 ) printf( "  -> WAIT 882 SAMPLES" );
   else if( note == 0x67 ) {
      printf( "  -> Data block con %lu bytes\n", adic_bytes-6 );
   }
   else if( note >= 0x70 && note <= 0x7F ) printf( "  -> WAIT %d SAMPLES", note - 0x6F );
   else if( note == 0xA0 ) {
      printf( "  -> AY3 CMD - write val(%d) a reg(%d)", cur_note[2], cur_note[1] );
   }
   printf( "\n" );
#endif

   return cur_note + adic_bytes + 1;
}

// Frecuencia de nuestro AY-3-8910 : 1Mhz
#define FREQ_AY3 1000000

// Ajustar los valores TP de AY-3-8910 para tener en cuenta la
// distinta frecuencia
static void adj_tones( uint8_t* cur_note, const uint8_t* end_data_ptr,
                      const uint32_t freq ) {

   float k = (float)FREQ_AY3/(float)freq;

   while( *cur_note != 0x66 && cur_note < end_data_ptr ) {
      // Escalar los valores de TPx para los canales A.B.C (registros R0 a R5)
      if( *cur_note == 0xA0 && cur_note[2] != 0 &&
             ( cur_note[1] >= 0 && cur_note[1] <= 5 ) ) {
         uint8_t tmp_note = (uint8_t)( (float)cur_note[2] * k );
#if DEBUG
         printf( "  -- NOTA ESCALADA %d --> %d\n", cur_note[2], tmp_note );
#endif
if( k>1 && tmp_note < cur_note[2] ) printf ("OVERFLOW\n");
if( k<1 && tmp_note > cur_note[2] ) printf ("OVERFLOW\n");
         cur_note[2] = tmp_note;
      }
      cur_note = next_note( cur_note );
   }
}

// Reproducir la siguiente nota
static uint8_t* play_note( uint8_t* cur_note ) {

   uint8_t n = *cur_note;

   if( n == 0xA0 ) {  // AY-3-8910 register
      ay3_write_reg( cur_note[1], cur_note[2] );
   }
   else if( ( n & 0xF0 ) == 0x70 ) {
      wait_samples( ( n&0xF ) + 1 );
   }
   else if( n == 0x61 ) {  // Wait N samples
      wait_samples( cur_note[2] * 256 + cur_note[1] );
   }
   else if( n == 0x62 ) {
      wait_samples( 735 );
   }
   else if( n == 0x63 ) {
      wait_samples( 882 );
   }
   else if( n == 0x67 ) {
      // Data block
      uint8_t data_type = cur_note[2];
      uint32_t data_size = ( ( ( cur_note[6] * 256 ) + cur_note[5] * 256 ) + cur_note[4] * 256 ) + cur_note[3];
      // TODO
   }

   return next_note( cur_note );
}


int main( int argc, char* argv[] ) {

   if( argc < 2 || argc > 3) {
      printf( "Args no validos\n" );
      exit(-2);
   }

   uint8_t gen_clk = 0;
   FILE* f = NULL;
   char* inputFileName;

   for (int i=1; i<argc; i++) {

      // Opcion para generar CLK del ay-3-8910 con PWM
      if( !strcmp( argv[i], "-p" ) ) {
         gen_clk = 1;
      }
      else { 
         // Cargar VGM
	 printf( "Loading %s\n", argv[i] );
         f = fopen( argv[i], "r" );
	 inputFileName = argv[i];
      }
   }

   if( !f ) {
      printf( "OMG - fichero no válido\n" );
      exit(-1);
   }

   // Leer primeros 4 bytes para ver si es un VGM o un VGZ
   char id[5];
   fread( id, 1, 4, f );

   // Obtener tamaño fichero
   fseek( f, 0L, SEEK_END );
   vg_size = ftell( f );
   fseek( f, 0L, SEEK_SET );

   // Si es "Vgm" leerlo a buffer
   vg = (uint8_t*)malloc( vg_size );
   long sz = fread( vg, 1, vg_size, f );

   printf( "Leidos %lu bytes del fichero %s\n", sz, inputFileName );

   // ¿VGZ?
   if( strncmp( id, "Vgm ", 4 ) ) {
      printf( "-- Descomprimiendo VGZ...\n" );
      int ret = inflate_vgz();
      if( ret != 0 ) {
         free( vg ); vg = NULL;
      }
   }

   fclose(f);

   if( vg != NULL ) {
      // Comprobar cosas de la cabesera
      // SAMPLES: a 44100 por segundo
      // Tiempo 1 sample = 1/44100 sg = 22,6757369615 uSeg
      vgm170_hdr* hdr = vg;
      uint8_t fin = 0;
      if( strncmp( hdr->ident, "Vgm ", 4 ) ) {
         printf( "Id erronea %s!\n", hdr->ident );
	 fin = 1;
      }
      if( !fin ) {

         uint8_t loop_count = 0;
	 uint32_t loop_entry = hdr->loop_offset + 0x1C;
	 uint32_t loop_start = hdr->samples - hdr->loop_samples;

         if( hdr->version >= 0x0151 ) {
            loop_count = (2 * hdr->loop_modifier + 8 ) / 16;
	    if( hdr->version >= 0x0160 ) {
               loop_count += hdr->loop_base;
	    }
	 }
	 if( loop_count < 1 && hdr->loop_offset != 0 )
            loop_count = 1;

         printf( "Version [BCD]     : %08Xh\n", hdr->version );
         printf( "Samples           : %d\nRate              : %d\n", hdr->samples, hdr->rate );
	 printf( "Loop entry/samples: %d/%d\n", loop_entry, hdr->loop_samples );
	 printf( "Loop count        : %d\n", loop_count );
	 printf( "Loop start        : @ %d samples\n", loop_start );
	 printf( "VGM data offset   : %08Xh\n", hdr->vgm_data_offset );
	 printf( "AY clock          : %d Hz\n", hdr->ay8910_clk );
	 printf( "AY flags          : %08Xh\n", hdr->ay_flags );

	 // En version anterior a 1.50, el offset es 0x40 fijo
	 if( hdr->version < 0x0150 )
            hdr->vgm_data_offset = 0x00000040;  // En estas versiones es 0 este campo
         // Reproducir cansión comenzando en vgm_data_offset (rel al registro, offset 0x34)
	 uint8_t* cur_note = &vg[ hdr->vgm_data_offset + 0x34 ];
	 uint8_t* loop_point = &vg[ hdr->loop_offset + 0x1C ];  // Valor en cabecera relativo a loop offset @ 0x1C
	 long data_size = vg_size - (cur_note - vg);
	 uint8_t* end_data_ptr = &vg[ vg_size ];  // Fin de datos por seguridad

	 // No funciona bien para agudos...
	 //printf( "-- Ajustando tonos de %lu Hz a %lu Hz\n", hdr->ay8910_clk, FREQ_AY3 );
         //adj_tones( cur_note, end_data_ptr, hdr->ay8910_clk );

	 init_ay3();

	 // Debe estar DESPUES de init_ay3, que inicializa la libreria bcm2835!!
         if( gen_clk ) {
            printf( "-- Generando reloj de %d Hz con PWM en el pin 12\n", hdr->ay8910_clk );
            init_pwm( hdr->ay8910_clk );
	 }

	 printf( "------- PLAY: START @ %08Xh -------\n", hdr->vgm_data_offset );

	 // Cuenta de samples para loops
	 cnt_samples = 0;
	 uint32_t loop_entry_samples = -1;  // Cuenta de samples en loop entry point
	 uint8_t* loop_entry_ptr = vg + loop_entry;

	 while( 1 ) {
            cur_note = play_note( cur_note );

	    // Si se han contado los samples del loop, salir en último loop
	    if( !loop_count && loop_entry_samples != -1 &&
	        //cnt_samples >= hdr->loop_samples ) {
	        cnt_samples == loop_entry_samples + hdr->loop_samples ) {

	       printf( "------ FIN de último loop --- %d samples -----\n", cnt_samples );
	       break;
	    }

	    // 0x66: end of sound data
	    if( *cur_note == 0x66 || cur_note >= end_data_ptr ) {
               if( loop_entry && loop_count ) {
                  cur_note = loop_entry_ptr;
	          cnt_samples = loop_entry_samples;
		  loop_count--;

	          printf( "  -> LOOP(%d) - salto a %08Xh, sample %d\n", loop_count+1, cur_note, cnt_samples );

		  continue;
	       }

	       printf( "------ FIN DE CANCION --- %d samples -------\n", cnt_samples );
               break;
	    }

	    // Comprobar loop start y guardar cuenta de samples al entrar en
	    // loop
	    if( cur_note == loop_entry_ptr && loop_count ) {
               loop_entry_samples = cnt_samples;
	       printf( "  -> Loop entry point @ sample %d\n", loop_entry_samples );
	    }
	 }

	 end_ay3();

         if( gen_clk ) end_pwm();
      }
   }
   // Fin
   
}
