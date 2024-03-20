/**
 * デバッグ表示座標定義
 */
#ifdef DEBUG_ST
#define ST_TX_TICK	4
#define ST_TY_TICK	24
#define ST_TX_PAT	0
#define ST_TY_PAT	1
#define ST_TX_MAP_DATA_BANKS	0
#define ST_TY_MAP_DATA_BANKS	21
#define ST_TX_CH_STATUS			0
#define ST_TY_CH_STATUS			3
#define ST_TX_ENVE_E			14
#define ST_TY_ENVE_E			11
#endif

struct st_header {
	unsigned char *patterns;
	unsigned char **wave_table;
	unsigned char **vol_table;
	unsigned char *chan_map;
};

extern unsigned char st_song_bank;
extern unsigned char **st_pattern_table;

extern unsigned char *st_chan_env[];
extern unsigned char st_chan_env_pos[];
extern unsigned char st_chan_len[];

extern unsigned char st_row_idx;
extern unsigned char st_pattern_idx;
extern unsigned char *st_chan_map;
extern unsigned char **st_wave_table;
extern unsigned char **st_vol_table;
extern unsigned char g_st_song_playing;

void st_init(void);
void st_reset(void);
void st_set_song(unsigned char song_bank, struct st_header *song_addr);

void st_set_env(unsigned char chan, unsigned char *env);
void st_set_vol(unsigned char chan, unsigned char left, unsigned char right);
void st_load_wave(unsigned char chan, unsigned char *wave);
void st_effect_wave(unsigned char chan, unsigned int freq, unsigned char len);
void st_effect_noise(unsigned char chan, unsigned char freq, unsigned char len);
