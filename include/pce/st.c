/* SimpleTracker
   Plays MOD files converted to ST format by mod2mml.
   Copyright (c) 2014, Ulrich Hecht
   All rights reserved.
   See LICENSE for details on use and redistribution. */

#include <huc.h>
#include <st.h>

const unsigned char *psg_ch = 0x800;
const unsigned char *psg_bal = 0x801;
const unsigned char *psg_freqlo = 0x802;
const unsigned char *psg_freqhi = 0x803;
const unsigned char *psg_ctrl = 0x804;
const unsigned char *psg_chbal = 0x805;
const unsigned char *psg_data = 0x806;
const unsigned char *psg_noise = 0x807;
const unsigned char *psg_lfofreq = 0x808;
const unsigned char *psg_lfoctrl = 0x809;

unsigned char current_wave[6];
unsigned char *st_chan_env[6];
unsigned char st_chan_env_pos[6];
unsigned char st_chan_len[6];
unsigned char st_chan_disable_tune[6];
#ifdef DEBUG_ST
unsigned int st_chan_freq[6];
unsigned char st_chan_chbal[6];
#endif

unsigned char st_row_idx;
unsigned char st_pattern_idx;
unsigned int st_song_banks;
unsigned char **st_pattern_table;
unsigned char *st_chan_map;
unsigned char **st_wave_table;
unsigned char **st_vol_table;

static unsigned char st_tick;
unsigned char g_st_song_playing;

void st_set_env(unsigned char chan, unsigned char *env)
{
	st_chan_env[chan] = env;
}

void st_set_vol(unsigned char chan, unsigned char left, unsigned char right)
{
#ifdef DEBUG_ST
	st_chan_chbal[chan] = (left << 4) | right;
#endif
	__sei();
	*psg_ch = chan;
	*psg_chbal = (left << 4) | right;
	__cli();
}

void st_load_wave(unsigned char chan, unsigned char *wave)
{
	unsigned char i;
	__sei();
	*psg_ch = chan;
	*psg_ctrl = 0;
	for (i = 0; i < 32; i++) {
		*psg_data = *wave;
		wave++;
	}
	__cli();
	current_wave[chan] = 0xff; /* force reload when tune plays */
}

void st_effect_wave(unsigned char chan, unsigned int freq, unsigned char len)
{
	st_chan_disable_tune[chan] = 1;
	st_chan_env_pos[chan] = 0;
	st_chan_len[chan] = len;
#ifdef DEBUG_ST
	st_chan_freq[chan] = freq;
#endif
	__sei();
	*psg_ch = chan;
	*psg_freqlo = freq & 0xff;
	*psg_freqhi = freq >> 8;
	*psg_noise = 0;
	__cli();
}

void st_effect_noise(unsigned char chan, unsigned char freq, unsigned char len)
{
	st_chan_disable_tune[chan] = 1;
	st_chan_env_pos[chan] = 0;
	st_chan_len[chan] = len;
#ifdef DEBUG_ST
	st_chan_freq[chan] = freq;
#endif
	__sei();
	*psg_ch = chan;
	*psg_noise = 0x80 | (freq ^ 31);
	__cli();
}

void st_play_song()
{
	g_st_song_playing = 1;
}

void st_stop_song()
{
	g_st_song_playing = 0;
}

void st_reset(void)
{
	unsigned char j, i;
	*psg_bal = 0xff;
	*psg_lfoctrl = 0;
	for (j = 0; j < 6; j++) {
		*psg_ch = j;
		*psg_ctrl = 0;
		*psg_chbal = 0;
		*psg_freqlo = 0;
		*psg_freqhi = 0;
		*psg_noise = 0;
		for (i = 0; i < 15; i++) {
			*psg_data = (i > 7 ? 0x1f : 0);
		}
	}
	memset(current_wave, 0xff, sizeof(current_wave));
	memset(st_chan_env_pos, 0, sizeof(st_chan_env_pos));
	memset(st_chan_disable_tune, 0, sizeof(st_chan_disable_tune));
#ifdef DEBUG_ST
	memset(st_chan_freq, 0, sizeof(st_chan_freq));
	memset(st_chan_chbal, 0, sizeof(st_chan_chbal));
#endif
	st_pattern_idx = 0;
	st_row_idx = 0;
	st_tick = 0;
	g_st_song_playing = 0;
	irq_enable_user(IRQ_VSYNC);
}

static void load_ins(unsigned char ins)
{
	unsigned char i;
	unsigned char *data;
	data = st_wave_table[ins];
	*psg_ctrl = 0;
	for (i = 0; i < 32; i++) {
		*psg_data = *data;
		data++;
	}
}

static void vsync_handler(void) __mapcall
#ifdef DEBUG_ST
			 __sirq
#else
			 __irq
#endif
{
	static unsigned char *pat;
	unsigned int freq;
	unsigned char chan;
	unsigned char *chv;
	unsigned char ins;
	unsigned char j;
	unsigned char dummy;
	unsigned char l, m;
	unsigned char is_drum;
	unsigned int save_banks;
#ifdef DEBUG_ST
	char * str;
	unsigned char chan_len;
	unsigned char chbal;
#endif

#ifdef PROFILE_ST
	timer_stop();
	irq_disable(IRQ_TIMER);
	timer_set(127);
	timer_start();
#endif
	save_banks = mem_mapdatabanks(st_song_banks);
	if ((st_tick & 7) == 0 && g_st_song_playing) {
		if (st_row_idx == 0) {
			pat = st_pattern_table[st_pattern_idx];
			if (!pat) {
				st_reset();
				pat = st_pattern_table[0];
			}
			st_pattern_idx++;
		}
#ifdef DEBUG_ST
		// PAT:
		put_hex(pat, 4, ST_TX_PAT+4, ST_TY_PAT);
#endif
		for (j = 0; j < 4; j++) {
			*psg_ch = chan = st_chan_map[j];
			chv = st_chan_env[chan];
			ins = *pat;
			if (ins == 0xff) {
				/* rest */
				pat++;
			} else if (st_chan_disable_tune[chan]) {
				/* channel paused for sound effect */
				pat += 4;
			} else {
				is_drum = (ins & 0xe0) == 0xe0;
				if (!is_drum
				    && ins != current_wave[chan]) {
					load_ins(ins);
					current_wave[chan] = ins;
				}
#ifdef DEBUG_ST
				// wav
				put_hex(ins, 2, ST_TX_CH_STATUS+12, ST_TY_CH_STATUS+1 +chan);
#endif
				pat++;
				st_chan_len[chan] = (*pat++) * 8;
				freq = *pat++;
				freq |= (*pat++) << 8;
#ifdef DEBUG_ST
				st_chan_freq[chan] = freq;
#endif
				st_chan_env_pos[chan] = 0;
				chv = st_chan_env[chan] =
				    st_vol_table[ins & 0x1f];
				if (is_drum && chan >= 4) {
#ifndef DISABLE_DRUMS
					*psg_noise =
					    0x80 | ((freq & 0xf) ^ 31);
#ifdef DEBUG_ST
					// drum or note
					put_string("drum", ST_TX_CH_STATUS+15, ST_TY_CH_STATUS+1 +chan);
#endif
					current_wave[chan] = -1;
					st_chan_env_pos[chan] = 0;
					chv = st_chan_env[chan] =
					    st_vol_table[ins &
								 0x1f];
#ifdef DEBUG_ST
					st_chan_chbal[chan] = 0xaa;
#endif
					*psg_chbal = 0xaa;
#else
#ifdef DEBUG_ST
					st_chan_chbal[chan] = 0;
#endif
					*psg_chbal = 0;
#endif
				} else {
					*psg_noise = 0;
					*psg_freqlo = freq & 0xff;
					*psg_freqhi = freq >> 8;
#ifdef DEBUG_ST
					// drum or note
					put_string("note", ST_TX_CH_STATUS+15, ST_TY_CH_STATUS+1 +chan);
					st_chan_chbal[chan] = 0xff;
#endif
					*psg_chbal = 0xff;
				}
#ifdef DEBUG_ST
#endif
			}
		}
		st_row_idx = (st_row_idx + 1) & 0x3f;
	}

	for (chan = 0; chan < 6; chan++) {
		*psg_ch = chan;
		chv = st_chan_env[chan];
		if (!st_chan_len[chan]) {
			st_chan_env_pos[chan] = 0xff;
			st_chan_disable_tune[chan] = 0;
			*psg_ctrl = 0x80;
#ifdef DEBUG_ST
			put_string("  ",  ST_TX_CH_STATUS+20, ST_TY_CH_STATUS+1 +chan);
			put_string("--",  ST_TX_CH_STATUS+23, ST_TY_CH_STATUS+1 +chan);
			put_string("--",  ST_TX_CH_STATUS+26, ST_TY_CH_STATUS+1 +chan);
			put_string("---", ST_TX_CH_STATUS+29, ST_TY_CH_STATUS+1 +chan);
#endif
		} else {
			st_chan_len[chan]--;
		}
		l = st_chan_env_pos[chan];
		if (l != 0xff) {
			*psg_ctrl = 0x80 | chv[l];
#ifdef DEBUG_ST
			if (chv[l] > 31)
			{
				// ENVE-E: (エラー表示?)
				put_string("s",     ST_TX_ENVE_E+ 7, ST_TY_ENVE_E);
				put_hex(chv, 4,     ST_TX_ENVE_E+ 9, ST_TY_ENVE_E);
				put_hex(st_tick, 2, ST_TX_ENVE_E+14, ST_TY_ENVE_E);
			}
			// freq
			freq = st_chan_freq[chan];
			put_number(freq,     5, ST_TX_CH_STATUS+ 2, ST_TY_CH_STATUS+1 +chan);
			chbal = st_chan_chbal[chan];
			put_hex(chbal >> 4,  1, ST_TX_CH_STATUS+ 8, ST_TY_CH_STATUS+1 +chan);
			put_hex(chbal & 15,  1, ST_TX_CH_STATUS+10, ST_TY_CH_STATUS+1 +chan);
			// SE
			str = st_chan_disable_tune[chan] ? "SE" : "  ";
			put_string(str, 		ST_TX_CH_STATUS+20, ST_TY_CH_STATUS+1 +chan);
			// enve vol
			put_number(chv[l],   2, ST_TX_CH_STATUS+23, ST_TY_CH_STATUS+1 +chan);
			// enve pos
			put_number(l,        2, ST_TX_CH_STATUS+26, ST_TY_CH_STATUS+1 +chan);
			// len
			chan_len = st_chan_len[chan];
			put_number(chan_len, 3, ST_TX_CH_STATUS+29, ST_TY_CH_STATUS+1 +chan);
#endif
			if (l < 15)
				st_chan_env_pos[chan] = l + 1;
		}
	}
	mem_mapdatabanks(save_banks);
#ifdef DEBUG_ST
	// TICK:
	put_hex(st_tick, 2, ST_TX_TICK+5, ST_TY_TICK);
#endif
	++st_tick;
#ifdef PROFILE_ST
	j = timer_get();
	timer_stop();
	put_number(127 - j, 4, 5, 1);
#endif
}

void st_init(void)
{
	st_pattern_table = 0;
	irq_add_vsync_handler(vsync_handler);
}

void st_set_song(unsigned char song_bank, struct st_header *song_addr)
{
	unsigned int save;
	st_song_banks = ((song_bank + 1) << 8) | song_bank;
	save = mem_mapdatabanks(st_song_banks);
#ifdef DEBUG_ST
	// MAP DATA BANKS
	put_hex(save, 4, ST_TX_MAP_DATA_BANKS, ST_TY_MAP_DATA_BANKS+1);
#endif
	st_pattern_table = song_addr->patterns;
	st_chan_map = song_addr->chan_map;
	st_wave_table = song_addr->wave_table;
	st_vol_table = song_addr->vol_table;
#ifdef DEBUG_ST
	put_hex(mem_mapdatabanks(save), 4, ST_TX_MAP_DATA_BANKS+5, ST_TY_MAP_DATA_BANKS+1);
#else
	mem_mapdatabanks(save);
#endif
	st_reset();
}
