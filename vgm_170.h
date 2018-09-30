#pragma once

#include <stdint.h>

// http://www.smspower.org/uploads/Music/vgmspec170.txt (desfasadillo)
// http://vgmrips.net/wiki/VGM_Specification (current)
//
// Header
//
//    00  01  02  03   04  05  06  07   08  09  0A  0B  0C  0D  0E  0F
//    0x00 ["Vgm " ident   ][EoF offset     ][Version        ][SN76489 clock  ]
//    0x10 [YM2413 clock   ][GD3 offset     ][Total # samples][Loop offset    ]
//    0x20 [Loop # samples ][Rate           ][SN FB ][SNW][SF][YM2612 clock   ]
//    0x30 [YM2151 clock   ][VGM data offset][Sega PCM clock ][SPCM Interface ]
//    0x40 [RF5C68 clock   ][YM2203 clock   ][YM2608 clock   ][YM2610/B clock ]
//    0x50 [YM3812 clock   ][YM3526 clock   ][Y8950 clock    ][YMF262 clock   ]
//    0x60 [YMF278B clock  ][YMF271 clock   ][YMZ280B clock  ][RF5C164 clock  ]
//    0x70 [PWM clock      ][AY8910 clock   ][AYT][AY Flags  ][VM] *** [LB][LM]
//    0x80 [GB DMG clock   ][NES APU clock  ][MultiPCM clock ][uPD7759 clock  ]
//    0x90 [OKIM6258 clock ][OF][KF][CF] *** [OKIM6295 clock ][K051649 clock  ]
//    0xA0 [K054539 clock  ][HuC6280 clock  ][C140 clock     ][K053260 clock  ]
//    0xB0 [Pokey clock    ][QSound clock   ] *** *** *** *** [Extra Hdr ofs  ]
//    0xC0  *** *** *** ***  *** *** *** ***  *** *** *** ***  *** *** *** ***
//    ...
//    0xF0  *** *** *** ***  *** *** *** ***  *** *** *** ***  *** *** *** ***
//

// WARN: valores en little endian!
typedef struct {
   char ident[4];
   uint32_t eof;
   uint32_t version;
   uint32_t sn76489_clk;

   uint32_t ym2413_clk;
   uint32_t gd3_offset;
   uint32_t samples;
   uint32_t loop_offset;

   uint32_t loop_samples;
   uint32_t rate;
   uint32_t sn;
   uint32_t ym2612_clk;

   uint32_t ym2151_clk;
   uint32_t vgm_data_offset;
   uint32_t sega_pcm_clock;
   uint32_t spcm_interface;

   uint32_t rf5c68_clk;
   uint32_t ym2203_clk;
   uint32_t ym2608_clk;
   uint32_t ym2610_clk;

   uint32_t ym3812_clk;
   uint32_t iym3526_clk;;
   uint32_t y8950_clk;
   uint32_t ymf262_clk;

   uint32_t ymf278b_clk;
   uint32_t ymf271_clk;
   uint32_t ymz280b_clk;
   uint32_t rf5c164_clk;

     // 0x70
   uint32_t pwm_clk;
   uint32_t ay8910_clk;
   uint32_t ay_flags;
   uint8_t  volume_modifier;
   uint8_t  reserved;
   uint8_t  loop_base;
   uint8_t  loop_modifier;

   uint32_t dmg_clk;
   uint32_t apu_clk;
   uint32_t multipcm_clk;
   uint32_t upd7759_clk;

   uint32_t okim6258_clk;
   uint32_t of_kf_cf;
   uint32_t okim6295_clk;
   uint32_t k051649_clk;

   uint32_t k054539_clk;
   uint32_t huc6280_clk;
   uint32_t c140_clk;
   uint32_t k053260_clk;

   uint32_t pokey_clk;
   uint32_t qsound_clk;
   uint32_t _na1;
   uint32_t extra_hdr_offset;
} vgm170_hdr ;

