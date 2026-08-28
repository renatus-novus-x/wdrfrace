#ifndef NDP_H_INCLUDED
#define NDP_H_INCLUDED

/*
 * NDP v1.07 compatible music-data player for the X68000 YM2151.
 *
 * This is a clean C implementation based on the public NDP driver
 * specification and the MIT-licensed NDP driver sources.  It does not need
 * Human68k, dynamic allocation, file I/O, or an interrupt hook.
 *
 * Define NDP_IMPLEMENTATION in exactly one C file before including ndp.h.
 * Call ndp_update() once per 60 Hz VSync.  A 30 Hz or 20 Hz application may
 * call ndp_update_ticks(2) or ndp_update_ticks(3), respectively.  This keeps
 * musical time but collapses intermediate register writes, so a real 60 Hz
 * call remains preferable for envelopes and effects.
 */

#include <stddef.h>
#include <stdint.h>

/*
 * Runtime profiles.  INTERPRETER decodes NDP/KSS data on the target.
 * STREAM replays a host-generated 60 Hz YM2151 register stream and is
 * intended for games with a tight CPU budget.
 */
#define NDP_PROFILE_INTERPRETER 0
#define NDP_PROFILE_STREAM 1

#ifndef NDP_PROFILE
#define NDP_PROFILE NDP_PROFILE_INTERPRETER
#endif

#if NDP_PROFILE == NDP_PROFILE_STREAM
#define NDP_PROFILE_NAME "STREAM"
#else
#define NDP_PROFILE_NAME "INTERPRETER"
#endif

typedef struct NdpSeBank {
    const uint8_t *data;
    size_t size;
    unsigned int count;
} NdpSeBank;

/*
 * Playback quality presets.  Define NDP_QUALITY before including ndp.h.
 * Every feature can also be overridden independently for game builds.
 */
#define NDP_QUALITY_FAST 0
#define NDP_QUALITY_BALANCED 1
#define NDP_QUALITY_ACCURATE 2

#ifndef NDP_QUALITY
#define NDP_QUALITY NDP_QUALITY_BALANCED
#endif

#if NDP_QUALITY == NDP_QUALITY_FAST
#define NDP_QUALITY_NAME "FAST"
#elif NDP_QUALITY == NDP_QUALITY_ACCURATE
#define NDP_QUALITY_NAME "ACCURATE"
#else
#define NDP_QUALITY_NAME "BALANCED"
#endif

#ifndef NDP_FEATURE_OPM_SHADOW
#define NDP_FEATURE_OPM_SHADOW 1
#endif
#ifndef NDP_FEATURE_NOISE_SUM
#define NDP_FEATURE_NOISE_SUM (NDP_QUALITY >= NDP_QUALITY_BALANCED)
#endif
#ifndef NDP_FEATURE_PITCH_DITHER
#define NDP_FEATURE_PITCH_DITHER 0
#endif
#ifndef NDP_FEATURE_VOLUME_DITHER
#define NDP_FEATURE_VOLUME_DITHER 0
#endif
#ifndef NDP_HARD_ENVELOPE_SAMPLES
#if NDP_QUALITY == NDP_QUALITY_FAST
#define NDP_HARD_ENVELOPE_SAMPLES 1
#elif NDP_QUALITY == NDP_QUALITY_ACCURATE
#define NDP_HARD_ENVELOPE_SAMPLES 3
#else
#define NDP_HARD_ENVELOPE_SAMPLES 2
#endif
#endif
#if NDP_HARD_ENVELOPE_SAMPLES < 1 || NDP_HARD_ENVELOPE_SAMPLES > 3
#error NDP_HARD_ENVELOPE_SAMPLES must be between 1 and 3
#endif
#ifndef NDP_FEATURE_EMBEDDED_SE
#define NDP_FEATURE_EMBEDDED_SE 1
#endif
#ifndef NDP_FEATURE_DIRECT_PSG
#define NDP_FEATURE_DIRECT_PSG 1
#endif
#ifndef NDP_FEATURE_FAST_FORWARD
#define NDP_FEATURE_FAST_FORWARD 1
#endif
#ifndef NDP_FAST_FORWARD_MAX_TICKS
#define NDP_FAST_FORWARD_MAX_TICKS 4096U
#endif
#ifndef NDP_STREAM_MAX_EVENTS_PER_UPDATE
#define NDP_STREAM_MAX_EVENTS_PER_UPDATE 256U
#endif
#if NDP_STREAM_MAX_EVENTS_PER_UPDATE < 1
#error NDP_STREAM_MAX_EVENTS_PER_UPDATE must be at least 1
#endif
#ifndef NDP_STREAM_SE_CHANNEL_BASE
#define NDP_STREAM_SE_CHANNEL_BASE 3U
#endif
#ifndef NDP_STREAM_SE_NOISE_CHANNEL
#define NDP_STREAM_SE_NOISE_CHANNEL 6U
#endif
#if NDP_STREAM_SE_CHANNEL_BASE + 2U >= 8U
#error NDP_STREAM_SE_CHANNEL_BASE must leave room for three channels
#endif
#if NDP_STREAM_SE_NOISE_CHANNEL >= 8U
#error NDP_STREAM_SE_NOISE_CHANNEL must be between 0 and 7
#endif
#if NDP_STREAM_SE_NOISE_CHANNEL >= NDP_STREAM_SE_CHANNEL_BASE && \
    NDP_STREAM_SE_NOISE_CHANNEL <= NDP_STREAM_SE_CHANNEL_BASE + 2U
#error NDP_STREAM_SE_NOISE_CHANNEL must not overlap tone channels
#endif
#ifndef NDP_DATA_ADDRESS
#define NDP_DATA_ADDRESS 0x4000U
#endif
#ifndef NDP_TONE_MODULATOR_TL
#define NDP_TONE_MODULATOR_TL 30
#endif
#ifndef NDP_FEATURE_TIMBRE_ADAPTATION
#define NDP_FEATURE_TIMBRE_ADAPTATION (NDP_QUALITY >= NDP_QUALITY_ACCURATE)
#endif
#ifndef NDP_TIMBRE_BASS_PERIOD
#define NDP_TIMBRE_BASS_PERIOD 0x0500U
#endif
#ifndef NDP_TONE_BASS_MODULATOR_TL
#define NDP_TONE_BASS_MODULATOR_TL 36
#endif
#ifndef NDP_TONE_PERCUSSIVE_MODULATOR_TL
#define NDP_TONE_PERCUSSIVE_MODULATOR_TL 22
#endif
#ifndef NDP_VOLUME_TL_TABLE
#define NDP_VOLUME_TL_TABLE \
    127,64,60,56,52,48,44,40,36,32,28,24,20,16,12,8
#endif
#ifndef NDP_PSG_AMPLITUDE_TABLE
#define NDP_PSG_AMPLITUDE_TABLE \
    0,1,2,3,4,6,9,13,18,26,37,52,74,105,149,211
#endif
#ifndef NDP_NOISE_TL_TABLE
#define NDP_NOISE_TL_TABLE \
    127,126,125,124,123,122,121,120,116,112,105,96,82,64,37,0
#endif
#ifndef NDP_NOISE_PERIOD_SCALE_Q8
#define NDP_NOISE_PERIOD_SCALE_Q8 256U
#endif
#ifndef NDP_NOISE_PERIOD_OFFSET
#define NDP_NOISE_PERIOD_OFFSET 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

int ndp_initialize(void);
void ndp_finalize(void);
int ndp_start(const void *data, size_t size);
int ndp_start_at(const void *data, size_t size, unsigned int data_address);
void ndp_stop(void);
void ndp_update(void);
void ndp_update_ticks(unsigned int ticks);
void ndp_set_master_volume(unsigned int attenuation);
void ndp_fade_out(unsigned int frames_per_step);
void ndp_fade_in(unsigned int frames_per_step);
void ndp_mute_channel(unsigned int channel, unsigned int frames);
unsigned int ndp_master_volume(void);
unsigned int ndp_channel_mute_frames(unsigned int channel);
int ndp_se_open(NdpSeBank *bank, const void *data, size_t size);
int ndp_se_play(const NdpSeBank *bank, unsigned int effect);
void ndp_se_stop(void);
int ndp_se_is_playing(void);
int ndp_is_playing(void);
unsigned int ndp_loop_count(void);
unsigned int ndp_end_tracks(void);

#ifdef __cplusplus
}
#endif

#ifdef NDP_IMPLEMENTATION

#define NDP_TRACKS 3
#define NDP_REPEAT_DEPTH 4
#define NDP_NONE 0xffffffffUL

#ifndef NDP_TONE_FEEDBACK
#define NDP_TONE_FEEDBACK 7
#endif
#ifndef NDP_TONE_MODULATOR_MUL
#define NDP_TONE_MODULATOR_MUL 2
#endif
#define NDP_TONE_CONTROL ((uint8_t)(((NDP_TONE_FEEDBACK & 7) << 3) | 4))

typedef struct NdpRepeat {
    uint32_t start;
    uint32_t escape;
    uint8_t remaining;
} NdpRepeat;

typedef struct NdpTrack {
    const uint8_t *sequence_data;
    uint32_t sequence_size;
    uint32_t sequence_end;
    uint32_t pc;
    uint16_t duration;
    uint16_t gate;
    uint8_t enabled;
    uint8_t ended;
    uint8_t note;
    uint8_t old_note;
    uint8_t volume;
    uint8_t saved_volume;
    uint8_t voice;
    uint8_t mix;
    uint8_t noise;
    uint8_t legato;
    uint8_t next_legato;
    uint8_t q;
    uint8_t q_sub;
    uint8_t fixed_gate;
    uint8_t more_length;
    uint8_t pitch_env;
    uint8_t pitch_delay;
    uint8_t note_env;
    uint8_t release_delay;
    uint8_t release_pitch_delay;
    uint8_t release_pitch_count;
    uint8_t release_pitch_skip;
    uint8_t release_count;
    uint8_t release_level;
    uint8_t release_volume;
    uint8_t release_active;
    uint8_t hard_enabled;
    uint8_t hard_shape;
    uint8_t hard_interval;
    uint8_t hard_count;
    uint32_t hard_phase;
    uint8_t env_level;
    uint8_t env_wait;
    uint8_t env_done;
    uint8_t repeat_depth;
    uint8_t mute_frames;
    uint8_t is_effect;
    int8_t note_delta;
    uint8_t volume_interval;
    uint8_t volume_interval_count;
    uint8_t volume_interval_amount;
    uint8_t volume_interval_target;
    uint8_t voice_pitch_zero;
    int16_t detune;
    int16_t pitch_delta;
    int16_t portamento_step;
    int32_t period;
    int32_t target_period;
    int32_t release_pitch_period;
    uint32_t env_pc;
    uint32_t pitch_pc;
    uint32_t note_pc;
    NdpRepeat repeat[NDP_REPEAT_DEPTH];
} NdpTrack;

typedef struct NdpState {
    const uint8_t *data;
    uint32_t size;
    uint16_t data_address;
    uint8_t playing;
    uint8_t no_infinite_loop;
    uint8_t master_volume;
    uint8_t fade_interval;
    uint8_t fade_count;
    uint8_t fade_loop_count;
    int8_t fade_direction;
    uint8_t slow_mask;
    uint8_t slow_count;
    uint8_t fast_forward;
    uint8_t ended_mask;
    uint8_t loop_count;
    uint16_t hard_period;
    uint32_t hard_step;
    uint32_t hard_substep;
    uint8_t hard_last_shape;
    uint8_t psg_noise;
    uint8_t frame_noise;
    uint8_t volume_interval_span;
    uint8_t rhythm_enabled;
    uint8_t rhythm_more_length;
    uint8_t rhythm_repeat_depth;
    uint8_t legacy_commands;
    uint8_t rhythm_note;
    uint8_t rhythm_active;
    uint8_t rhythm_end_pending;
    uint8_t rhythm_mix;
    uint8_t rhythm_level;
    uint8_t rhythm_noise;
    uint8_t rhythm_channel;
    uint16_t rhythm_duration;
    uint16_t rhythm_period;
    uint32_t rhythm_pc;
    uint32_t rhythm_env_pc;
    NdpRepeat rhythm_repeat[NDP_REPEAT_DEPTH];
    uint8_t rhythm_attenuation[32];
    uint32_t voice[16];
    uint32_t rhythm_voice[32];
    uint8_t rhythm_voice_modern[32];
    uint32_t pitch_env[16];
    uint32_t note_env[16];
    uint8_t pitch_env_delay[16];
    uint16_t pitch_env_delay_valid;
    uint16_t periods[96];
    uint8_t opm_shadow[256];
    uint8_t opm_valid[256];
    uint8_t tone_on[NDP_TRACKS];
    uint8_t se_tone_on[NDP_TRACKS];
    uint8_t se_noise_on;
    uint8_t se_hardware_ready;
    uint8_t noise_on;
    NdpTrack track[NDP_TRACKS];
    NdpTrack se_track[NDP_TRACKS];
    const uint8_t *se_data;
    uint32_t se_size;
    uint32_t se_effect_end;
    uint8_t se_playing;
    uint8_t se_priority;
} NdpState;

typedef struct NdpStreamState {
    const uint8_t *data;
    uint32_t size;
    uint32_t frame_count;
    uint32_t frames_left;
    uint32_t event_count;
    uint32_t event_index;
    uint32_t loop_event_index;
    uint16_t wait_frames;
    uint16_t loop_frame;
    uint16_t loop_wait_frames;
    uint16_t carrier_valid;
    uint8_t carrier_tl[16];
    uint8_t flags;
    volatile uint8_t active;
} NdpStreamState;

static NdpState ndp_s;

static const uint16_t ndp_default_periods[96] = {
    0x0d5d,0x0c9c,0x0be7,0x0b3c,0x0a9b,0x0a02,0x0973,0x08eb,0x086b,0x07f2,0x0780,0x0714,
    0x06af,0x064e,0x05f4,0x059e,0x054e,0x0501,0x04ba,0x0476,0x0436,0x03f9,0x03c0,0x038a,
    0x0357,0x0327,0x02fa,0x02cf,0x02a7,0x0281,0x025d,0x023b,0x021b,0x01fd,0x01e0,0x01c5,
    0x01ac,0x0194,0x017d,0x0168,0x0153,0x0140,0x012e,0x011d,0x010d,0x00fe,0x00f0,0x00e3,
    0x00d6,0x00ca,0x00be,0x00b4,0x00aa,0x00a0,0x0097,0x008f,0x0087,0x007f,0x0078,0x0071,
    0x006b,0x0065,0x005f,0x005a,0x0055,0x0050,0x004c,0x0047,0x0043,0x0040,0x003c,0x0039,
    0x0035,0x0032,0x0030,0x002d,0x002a,0x0028,0x0026,0x0024,0x0022,0x0020,0x001e,0x001c,
    0x001b,0x0019,0x0018,0x0016,0x0015,0x0014,0x0013,0x0012,0x0011,0x0010,0x000f,0x000e
};

static const uint8_t ndp_note_code[12] = {
    0x00,0x01,0x02,0x04,0x05,0x06,0x08,0x09,0x0a,0x0c,0x0d,0x0e
};

/*
 * The YM2151 in an X68000 is clocked at 4 MHz.  Its key-code formula is
 * conventionally expressed for a 3.579545 MHz clock, so an uncorrected
 * PSG period sounds about 1.92 semitones sharp.  The default includes the
 * calibrated correction for the quantized NDP period table.  Units are
 * 1/64 semitone.
 */
#ifndef NDP_OPM_PITCH_OFFSET64
#define NDP_OPM_PITCH_OFFSET64 119
#endif

static const uint8_t ndp_volume_tl[16] = {
    NDP_VOLUME_TL_TABLE
};

#if NDP_FEATURE_VOLUME_DITHER
/* Quarter-TL targets used only by temporal volume interpolation. */
static const uint16_t ndp_volume_tl_q2[16] = {
    508,282,257,239,218,199,178,161,143,129,113,97,81,65,49,32
};
#endif

#if NDP_FEATURE_NOISE_SUM || NDP_HARD_ENVELOPE_SAMPLES > 1
/* Relative PSG amplitude used for noise sums and envelope averaging. */
static const uint16_t ndp_noise_amplitude[16] = {
    NDP_PSG_AMPLITUDE_TABLE
};

static unsigned int ndp_amplitude_to_level(unsigned int amplitude)
{
    unsigned int level;
    for (level = 0; level < 15; ++level) {
        unsigned int sum = ndp_noise_amplitude[level] +
                           ndp_noise_amplitude[level + 1];
        if (amplitude * 2U < sum) return level;
    }
    return 15;
}
#endif

static uint8_t ndp_pitch_error[4];
static uint8_t ndp_volume_error[4];
static uint16_t ndp_psg_period_state[NDP_TRACKS];
static uint8_t ndp_psg_volume_state[NDP_TRACKS];
static uint8_t ndp_psg_mix_state[NDP_TRACKS];
static uint8_t ndp_psg_hard_state[NDP_TRACKS];
static uint8_t ndp_timbre_state[8];

static const uint8_t ndp_noise_tl[16] = {
    NDP_NOISE_TL_TABLE
};

/*
 * Additional YM2151 noise attenuation; one TL step is approximately 0.75 dB.
 * The default was calibrated against NDP DemoSongs 13 PSG output.  Define this
 * as 0 before including the implementation for the unmodified MS.X mapping.
 */
#ifndef NDP_NOISE_TL_BIAS
#define NDP_NOISE_TL_BIAS 28
#endif

static void ndp_zero(void *dst, size_t size)
{
    uint8_t *p = (uint8_t *)dst;
    while (size-- != 0) *p++ = 0;
}

static void ndp_set_hard_period(unsigned int period)
{
    uint32_t divisor;
    ndp_s.hard_period = (uint16_t)(period != 0 ? period : 1U);
    divisor = 60UL * ndp_s.hard_period;
    ndp_s.hard_step = 458181888UL / divisor;
    if (ndp_s.hard_step == 0) ndp_s.hard_step = 1;
    ndp_s.hard_substep = (ndp_s.hard_step + 3U) / 6U;
}

static uint16_t ndp_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int ndp_can_read(uint32_t at, uint32_t count)
{
    return at <= ndp_s.size && count <= ndp_s.size - at;
}

static uint8_t ndp_read8(uint32_t *pc)
{
    if (!ndp_can_read(*pc, 1)) return 0xff;
    return ndp_s.data[(*pc)++];
}

static uint16_t ndp_read16(uint32_t *pc)
{
    uint16_t v;
    if (!ndp_can_read(*pc, 2)) return 0;
    v = ndp_le16(ndp_s.data + *pc);
    *pc += 2;
    return v;
}

static void ndp_opm_raw(uint8_t reg, uint8_t value)
{
#ifdef NDP_OPM_WRITE
    NDP_OPM_WRITE(reg, value);
#else
    volatile uint8_t *address = (volatile uint8_t *)0x00e90001UL;
    volatile uint8_t *data = (volatile uint8_t *)0x00e90003UL;
    (void)*address;
    while ((*data & 0x80) != 0) { }
    *address = reg;
    (void)*address;
    while ((*data & 0x80) != 0) { }
    *data = value;
#endif
}

static void ndp_opm(uint8_t reg, uint8_t value)
{
#if NDP_FEATURE_OPM_SHADOW
    if (ndp_s.opm_valid[reg] && ndp_s.opm_shadow[reg] == value) return;
#endif
    ndp_s.opm_valid[reg] = 1;
    ndp_s.opm_shadow[reg] = value;
    ndp_opm_raw(reg, value);
}

static void ndp_opm_tracked(uint8_t reg, uint8_t value)
{
    ndp_s.opm_valid[reg] = 1;
    ndp_s.opm_shadow[reg] = value;
    ndp_opm_raw(reg, value);
}

static void ndp_slot(unsigned int slot, const uint8_t *v)
{
    ndp_opm((uint8_t)(0x40 + slot), (uint8_t)((v[8] << 4) | v[7]));
    ndp_opm((uint8_t)(0x60 + slot), v[5]);
    ndp_opm((uint8_t)(0x80 + slot), (uint8_t)((v[6] << 6) | v[0]));
    ndp_opm((uint8_t)(0xa0 + slot), (uint8_t)((v[10] << 7) | v[1]));
    ndp_opm((uint8_t)(0xc0 + slot), (uint8_t)((v[9] << 6) | v[2]));
    ndp_opm((uint8_t)(0xe0 + slot), (uint8_t)((v[4] << 4) | v[3]));
}

static void ndp_setup_channel(unsigned int ch)
{
    static const uint8_t m1[11] = {31,0,0,0,15,NDP_TONE_MODULATOR_TL,0,NDP_TONE_MODULATOR_MUL,0,0,0};
    static const uint8_t c1[11] = {31,0,0,0,15,127,0,1,0,0,0};
    static const uint8_t m2[11] = {31,0,0,15,0,127,0,2,0,0,0};
    static const uint8_t c2[11] = {31,0,0,15,0,127,0,1,0,0,0};
    ndp_slot(ch, m1);
    ndp_slot(ch + 16, c1);
    ndp_slot(ch + 8, m2);
    ndp_slot(ch + 24, c2);
    ndp_opm((uint8_t)(0x20 + ch), NDP_TONE_CONTROL);
    ndp_opm((uint8_t)(0x38 + ch), 0);
    ndp_opm((uint8_t)(0x70 + ch), 127);
    ndp_opm(0x08, (uint8_t)(0x18 | ch));
    if (ch < 8) ndp_timbre_state[ch] = 0;
}

static void ndp_setup_noise(void)
{
    static const uint8_t off[11] = {31,0,0,15,0,127,0,1,0,0,0};
    static const uint8_t carrier[11] = {31,0,0,0,0,127,0,0,0,0,0};
    ndp_slot(7, off);
    ndp_slot(15, off);
    ndp_slot(23, off);
    ndp_slot(31, carrier);
    ndp_opm(0x27, 0x3c);
    ndp_opm(0x7f, 127);
    ndp_opm(0x0f, 0x81);
    ndp_opm(0x08, 0x47);
}

static void ndp_key(unsigned int ch, int on)
{
    uint8_t state = (uint8_t)(on != 0);
    if (ndp_s.tone_on[ch] == state) return;
    ndp_s.tone_on[ch] = state;
    ndp_opm((uint8_t)(0x20 + ch),
            (uint8_t)(NDP_TONE_CONTROL | (state ? 0xc0 : 0)));
}

static unsigned int ndp_track_opm_channel(const NdpTrack *t,
                                          unsigned int channel)
{
#if NDP_PROFILE == NDP_PROFILE_STREAM
    if (t->is_effect) return NDP_STREAM_SE_CHANNEL_BASE + channel;
#else
    (void)t;
#endif
    return channel;
}

static void ndp_track_key(NdpTrack *t, unsigned int channel, int on)
{
#if NDP_PROFILE == NDP_PROFILE_STREAM
    if (t->is_effect) {
        unsigned int opm_channel = NDP_STREAM_SE_CHANNEL_BASE + channel;
        uint8_t state = (uint8_t)(on != 0);
        if (ndp_s.se_tone_on[channel] == state) return;
        ndp_s.se_tone_on[channel] = state;
        ndp_opm((uint8_t)(0x20 + opm_channel),
                (uint8_t)(NDP_TONE_CONTROL | (state ? 0xc0 : 0)));
        return;
    }
#else
    (void)t;
#endif
    ndp_key(channel, on);
}

static void ndp_noise_key(int on)
{
    uint8_t state = (uint8_t)(on != 0);
    if (ndp_s.noise_on == state) return;
    ndp_s.noise_on = state;
    ndp_opm(0x27, state ? 0xfc : 0x3c);
}

static void ndp_apply_timbre(unsigned int channel, unsigned int period,
                             unsigned int mix)
{
#if NDP_FEATURE_TIMBRE_ADAPTATION
    unsigned int timbre;
    unsigned int tl;
    if (channel >= 8) return;
    if ((mix & 2U) != 0) timbre = 2;
    else if (period >= NDP_TIMBRE_BASS_PERIOD) timbre = 1;
    else timbre = 0;
    if (ndp_timbre_state[channel] == timbre) return;
    ndp_timbre_state[channel] = (uint8_t)timbre;
    if (timbre == 2) tl = NDP_TONE_PERCUSSIVE_MODULATOR_TL;
    else if (timbre == 1) tl = NDP_TONE_BASS_MODULATOR_TL;
    else tl = NDP_TONE_MODULATOR_TL;
    if (tl > 127) tl = 127;
    ndp_opm((uint8_t)(0x60 + channel), (uint8_t)tl);
#else
    (void)channel;
    (void)period;
    (void)mix;
#endif
}

static void ndp_period_to_opm(int period, unsigned int channel, uint8_t *kc, uint8_t *kf)
{
    unsigned int i;
    unsigned int frac = 0;
    int pitch64;
#if !NDP_FEATURE_PITCH_DITHER
    (void)channel;
#endif
    if (period >= ndp_s.periods[0]) i = 0;
    else if (period <= ndp_s.periods[95]) i = 95;
    else {
        for (i = 0; i < 95; ++i) {
            int hi = ndp_s.periods[i];
            int lo = ndp_s.periods[i + 1];
            if (period <= hi && period > lo) {
                int span = hi - lo;
                if (span > 0) {
#if NDP_FEATURE_PITCH_DITHER
                    unsigned int frac256 = (unsigned int)((hi - period) * 256 / span);
                    frac = frac256 >> 2;
                    ndp_pitch_error[channel] = (uint8_t)(ndp_pitch_error[channel] +
                                                        (frac256 & 3));
                    if (ndp_pitch_error[channel] >= 4) {
                        ndp_pitch_error[channel] = (uint8_t)(ndp_pitch_error[channel] - 4);
                        ++frac;
                    }
#else
                    frac = (unsigned int)((hi - period) * 64 / span);
#endif
                }
                break;
            }
        }
    }
    pitch64 = (int)(i * 64 + frac) - NDP_OPM_PITCH_OFFSET64;
    if (pitch64 < 0) pitch64 = 0;
    if (pitch64 > 95 * 64 + 63) pitch64 = 95 * 64 + 63;
    i = (unsigned int)pitch64 >> 6;
    frac = (unsigned int)pitch64 & 63;
    *kc = (uint8_t)(((i / 12) << 4) | ndp_note_code[i % 12]);
    *kf = (uint8_t)(frac << 2);
}

static uint8_t ndp_tone_total_level(unsigned int volume, unsigned int channel)
{
#if NDP_FEATURE_VOLUME_DITHER
    unsigned int target = ndp_volume_tl_q2[volume > 15 ? 15 : volume];
    unsigned int level = target >> 2;
    ndp_volume_error[channel] = (uint8_t)(ndp_volume_error[channel] + (target & 3));
    if (ndp_volume_error[channel] >= 4) {
        ndp_volume_error[channel] = (uint8_t)(ndp_volume_error[channel] - 4);
        ++level;
    }
    return (uint8_t)(level > 127 ? 127 : level);
#else
    (void)channel;
    return ndp_volume_tl[volume > 15 ? 15 : volume];
#endif
}

static unsigned int ndp_noise_add(unsigned int current, unsigned int added)
{
#if NDP_FEATURE_NOISE_SUM
    unsigned int amplitude;
    if (added > 15) added = 15;
    if (current > 15) current = 15;
    amplitude = ndp_noise_amplitude[current] + ndp_noise_amplitude[added];
    return ndp_amplitude_to_level(amplitude);
#else
    return added > current ? added : current;
#endif
}

static void ndp_noise_merge(unsigned int *current_volume,
                            unsigned int *current_frequency,
                            unsigned int added_volume,
                            unsigned int added_frequency)
{
    unsigned int previous = *current_volume;
    if (added_volume == 0) return;
#if NDP_FEATURE_NOISE_SUM
    *current_volume = ndp_noise_add(previous, added_volume);
    if (added_volume >= previous) *current_frequency = added_frequency;
#else
    if (added_volume > previous) {
        *current_volume = added_volume;
        *current_frequency = added_frequency;
    }
#endif
}

static uint8_t ndp_noise_period_register(unsigned int period)
{
    unsigned int mapped;
    int adjusted;
    if (period == 0) period = 1;
    mapped = (period * NDP_NOISE_PERIOD_SCALE_Q8 + 128U) >> 8;
    adjusted = (int)mapped + NDP_NOISE_PERIOD_OFFSET;
    if (adjusted < 1) adjusted = 1;
    if (adjusted > 31) adjusted = 31;
    return (uint8_t)(31 - adjusted);
}

static void ndp_direct_apply_noise(void)
{
    unsigned int channel;
    unsigned int volume = 0;
    for (channel = 0; channel < NDP_TRACKS; ++channel) {
        if ((ndp_psg_mix_state[channel] & 2) != 0)
            volume = ndp_noise_add(volume, ndp_psg_volume_state[channel]);
    }
    if (volume != 0) {
        unsigned int tl = ndp_noise_tl[volume > 15 ? 15 : volume] +
                          NDP_NOISE_TL_BIAS;
        if (tl > 127) tl = 127;
        ndp_s.frame_noise = ndp_s.psg_noise;
        ndp_opm(0x0f, (uint8_t)(0x80 |
                ndp_noise_period_register(ndp_s.psg_noise)));
        ndp_opm(0x7f, (uint8_t)tl);
        ndp_noise_key(1);
    } else {
        ndp_opm(0x7f, 127);
        ndp_noise_key(0);
    }
}

static void ndp_direct_apply_channel(unsigned int channel)
{
    unsigned int volume = ndp_psg_hard_state[channel]
        ? 15U : ndp_psg_volume_state[channel];
    int period = ndp_psg_period_state[channel];
    uint8_t kc, kf;
    if (period == 0) period = 1;
    ndp_period_to_opm(period, channel, &kc, &kf);
    ndp_opm((uint8_t)(0x28 + channel), kc);
    ndp_opm((uint8_t)(0x30 + channel), kf);
    ndp_apply_timbre(channel, (unsigned int)period,
                     ndp_psg_mix_state[channel]);
    ndp_opm((uint8_t)(0x70 + channel),
            ndp_tone_total_level(volume, channel));
    ndp_key(channel, volume != 0 && (ndp_psg_mix_state[channel] & 1) != 0);
}

static void ndp_psg_direct_write(uint8_t reg, uint8_t value)
{
#if NDP_FEATURE_DIRECT_PSG
    unsigned int channel;
    if (reg > 13) return;
#ifdef NDP_PSG_WRITE
    NDP_PSG_WRITE(reg, value);
#endif
    if (reg <= 5) {
        channel = reg >> 1;
        if ((reg & 1) == 0)
            ndp_psg_period_state[channel] = (uint16_t)
                ((ndp_psg_period_state[channel] & 0x0f00U) | value);
        else
            ndp_psg_period_state[channel] = (uint16_t)
                ((ndp_psg_period_state[channel] & 0x00ffU) |
                 ((uint16_t)(value & 15U) << 8));
        ndp_direct_apply_channel(channel);
    } else if (reg == 6) {
        ndp_s.psg_noise = (uint8_t)(value & 31U);
        ndp_s.frame_noise = ndp_s.psg_noise;
        ndp_direct_apply_noise();
    } else if (reg == 7) {
        for (channel = 0; channel < NDP_TRACKS; ++channel) {
            ndp_psg_mix_state[channel] = (uint8_t)
                (((value >> channel) & 1U ? 0U : 1U) |
                 ((value >> (channel + 3U)) & 1U ? 0U : 2U));
            ndp_direct_apply_channel(channel);
        }
        ndp_direct_apply_noise();
    } else if (reg <= 10) {
        channel = reg - 8U;
        ndp_psg_volume_state[channel] = (uint8_t)(value & 15U);
        ndp_psg_hard_state[channel] = (uint8_t)((value & 16U) != 0);
        ndp_direct_apply_channel(channel);
        ndp_direct_apply_noise();
    } else if (reg == 11) {
        ndp_s.hard_period = (uint16_t)((ndp_s.hard_period & 0xff00U) | value);
    } else if (reg == 12) {
        ndp_s.hard_period = (uint16_t)((ndp_s.hard_period & 0x00ffU) |
                                       ((uint16_t)value << 8));
    } else {
        ndp_s.hard_last_shape = (uint8_t)(value & 15U);
    }
    if (reg == 11 || reg == 12) {
        ndp_set_hard_period(ndp_s.hard_period);
    }
#else
    (void)reg;
    (void)value;
#endif
}

static uint8_t ndp_rhythm_voice_valid(uint32_t pc, uint8_t length, uint8_t modern)
{
    uint32_t end = pc + length;
    while (pc < end) {
        uint8_t command = ndp_s.data[pc++];
        unsigned int arguments;
        if (command == 0xff) return 1;
        if (command == 0x10 || (command >= 0x20 && command <= 0x23)) continue;
        if (modern) {
            if (command == 1) arguments = 1;
            else if (command < 6) arguments = 2;
            else if (command <= 15) arguments = 1;
            else return 0;
        } else {
            if (command > 15) return 0;
            arguments = 1;
        }
        if ((uint32_t)arguments > end - pc) return 0;
        pc += arguments;
    }
    return 0;
}

static void ndp_parse_definitions(uint32_t pc)
{
    unsigned int guard = 0;
    while (ndp_can_read(pc, 1) && guard++ < 256) {
        uint8_t id = ndp_read8(&pc);
        uint8_t length;
        uint32_t body;
        if (id == 0xff) break;
        length = ndp_read8(&pc);
        body = pc;
        if (!ndp_can_read(body, length)) break;
        if (id < 16) ndp_s.voice[id] = body;
        else if (id < 48) {
            unsigned int rhythm = id - 16;
            if (ndp_rhythm_voice_valid(body, length, 1)) {
                ndp_s.rhythm_voice[rhythm] = body;
                ndp_s.rhythm_voice_modern[rhythm] = 1;
            } else if (ndp_rhythm_voice_valid(body, length, 0)) {
                ndp_s.rhythm_voice[rhythm] = body;
                ndp_s.rhythm_voice_modern[rhythm] = 0;
            }
        }
        else if (id < 64) ndp_s.pitch_env[id - 48] = body;
        else if (id < 80) ndp_s.note_env[id - 64] = body;
        pc += length;
    }
}

static void ndp_reset_voice(NdpTrack *t)
{
    t->env_pc = t->voice < 16 ? ndp_s.voice[t->voice] : NDP_NONE;
    t->voice_pitch_zero = 0;
    t->env_level = 15;
    t->env_wait = 0;
    t->env_done = 0;
}

static void ndp_reset_pitch_env(NdpTrack *t)
{
    uint8_t selector = (uint8_t)(t->pitch_env & 31);
    t->pitch_pc = (selector != 0 && selector <= 16) ?
                  ndp_s.pitch_env[selector - 1] : NDP_NONE;
    t->pitch_delta = 0;
    t->pitch_delay = 0;
    if (t->pitch_pc != NDP_NONE) {
        t->pitch_delay = ndp_read8(&t->pitch_pc);
        if (selector <= 16 &&
            (ndp_s.pitch_env_delay_valid & (uint16_t)(1U << (selector - 1))) != 0)
            t->pitch_delay = ndp_s.pitch_env_delay[selector - 1];
    }
}

static void ndp_reset_note_env(NdpTrack *t)
{
    uint8_t selector = (uint8_t)(t->note_env & 0x7f);
    t->note_pc = (selector != 0 && selector <= 16) ?
                 ndp_s.note_env[selector - 1] : NDP_NONE;
    t->note_delta = 0;
}

static void ndp_voice_tick(NdpTrack *t)
{
    unsigned int guard = 0;
    if (t->env_pc == NDP_NONE || t->env_done) return;
    if (t->env_wait != 0) {
        --t->env_wait;
        return;
    }
    while (guard++ < 32 && t->env_pc != NDP_NONE) {
        uint8_t c = ndp_read8(&t->env_pc);
        if (c < 0xa0) {
            t->env_level = (uint8_t)(c & 15);
            t->env_wait = (uint8_t)(c >> 4);
            return;
        }
        if (c == 0xa0) { t->env_wait = 1; return; }
        if (c == 0xa1) { t->voice_pitch_zero = 1; continue; }
        if (c == 0xa2) { t->pitch_env = ndp_read8(&t->env_pc); ndp_reset_pitch_env(t); continue; }
        if (c == 0xa3) { t->voice_pitch_zero = 0; t->pitch_delta = 0; continue; }
        if (c == 0xa4) { t->note_env = ndp_read8(&t->env_pc); ndp_reset_note_env(t); continue; }
        if (c == 0xa5) {
            t->volume_interval = ndp_read8(&t->env_pc);
            t->volume_interval_count = (uint8_t)(t->volume_interval & 0x7f);
            continue;
        }
        if (c >= 0xb0 && c < 0xc0) {
            t->hard_enabled = 1;
            t->hard_shape = (uint8_t)(c & 15);
            continue;
        }
        if (c >= 0xc0 && c < 0xc4) { t->mix = (uint8_t)(c & 3); continue; }
        if (c >= 0xd0 && c < 0xf0) { t->noise = (uint8_t)(c - 0xd0); continue; }
        if (c >= 0xf0) {
            uint8_t back = (uint8_t)(c & 15);
            if (back == 0 || back > t->env_pc) {
                t->env_done = 1;
                return;
            }
            t->env_pc -= back;
            continue;
        }
    }
}

static void ndp_pitch_tick(NdpTrack *t)
{
    uint8_t c;
    if (t->pitch_pc == NDP_NONE) return;
    if (t->pitch_delay != 0) { --t->pitch_delay; return; }
    if ((t->pitch_env & 0x40) != 0) return;
    if ((t->pitch_env & 0x20) != 0) t->pitch_delta = 0;
    c = ndp_read8(&t->pitch_pc);
    if (c == 0x80) {
        uint8_t back = ndp_read8(&t->pitch_pc);
        if (back != 0 && back <= t->pitch_pc) t->pitch_pc -= back;
        return;
    }
    t->pitch_delta = (int16_t)(t->pitch_delta + (int8_t)c);
}

static void ndp_note_tick(NdpTrack *t)
{
    uint8_t c;
    if (t->note_pc == NDP_NONE) return;
    c = ndp_read8(&t->note_pc);
    if (c == 0x80) {
        uint8_t back = ndp_read8(&t->note_pc);
        if (back == 0) t->note_pc = NDP_NONE;
        else if (back <= t->note_pc) t->note_pc -= back;
        return;
    }
    t->note_delta = (int8_t)c;
}

static uint8_t ndp_hard_level_at(unsigned int shape, uint32_t phase)
{
    unsigned int step = (unsigned int)(phase >> 16);
    unsigned int cycle = step >> 4;
    unsigned int position = step & 15;
    unsigned int attack = (shape & 4) != 0;
    unsigned int level;
    if ((shape & 8) == 0 && cycle != 0) {
        level = 0;
    } else if ((shape & 1) != 0 && cycle != 0) {
        if ((shape & 2) != 0) attack ^= 1;
        level = attack ? 15 : 0;
    } else {
        if ((shape & 2) != 0 && (cycle & 1) != 0) attack ^= 1;
        level = attack ? position : 15 - position;
    }
    return (uint8_t)level;
}

static uint8_t ndp_hard_level(NdpTrack *t)
{
#if NDP_HARD_ENVELOPE_SAMPLES <= 1
    uint8_t level = ndp_hard_level_at(t->hard_shape & 15, t->hard_phase);
#else
    uint32_t amplitude = 0;
    uint8_t level = 15;
#if NDP_HARD_ENVELOPE_SAMPLES >= 3
    uint32_t substep = ndp_s.hard_substep;
    amplitude += ndp_noise_amplitude[ndp_hard_level_at(t->hard_shape & 15,
        t->hard_phase + substep)];
    amplitude += ndp_noise_amplitude[ndp_hard_level_at(t->hard_shape & 15,
        t->hard_phase + substep * 3U)];
    amplitude += ndp_noise_amplitude[ndp_hard_level_at(t->hard_shape & 15,
        t->hard_phase + substep * 5U)];
    amplitude = ((amplitude + 1U) * 683U) >> 11;
#else
    uint32_t quarter = ndp_s.hard_step >> 2;
    amplitude += ndp_noise_amplitude[ndp_hard_level_at(t->hard_shape & 15,
        t->hard_phase + quarter)];
    amplitude += ndp_noise_amplitude[ndp_hard_level_at(t->hard_shape & 15,
        t->hard_phase + quarter * 3U)];
    amplitude = (amplitude + 1U) >> 1;
#endif
    level = (uint8_t)ndp_amplitude_to_level(amplitude);
#endif
    t->hard_phase += ndp_s.hard_step;
    return level;
}

static void ndp_repeat_start(NdpTrack *t)
{
    if (t->repeat_depth >= NDP_REPEAT_DEPTH) return;
    t->repeat[t->repeat_depth].start = t->pc;
    t->repeat[t->repeat_depth].escape = NDP_NONE;
    t->repeat[t->repeat_depth].remaining = 0;
    ++t->repeat_depth;
}

static unsigned int ndp_track_argument_count(uint8_t command)
{
    if (command < 0x80 || (command >= 0xb0 && command < 0xd0)) return 0;
    if (command >= 0x90 && command < 0xa0) return 1;
    switch (command) {
    case 0x80: case 0x81: case 0x82: case 0x83:
    case 0x86: case 0x87: case 0x88: case 0x8c: case 0x8d:
    case 0x8f: case 0xa2: case 0xa3: case 0xa4: case 0xa5:
    case 0xa7: case 0xa8: case 0xaa: case 0xab:
    case 0xf3: case 0xf4: case 0xfd: case 0xfe:
        return 1;
    case 0x89: case 0x8e:
        return ndp_s.legacy_commands ? 0 : 2;
    case 0xa0: case 0xac: case 0xf0: case 0xfc: case 0xff:
        return 2;
    case 0xa1: case 0xa6:
        return 3;
    default:
        return 0;
    }
}

static int ndp_track_can_read(const NdpTrack *t, uint32_t count)
{
    return t->sequence_data != 0 && t->pc <= t->sequence_size &&
           t->pc <= t->sequence_end && count <= t->sequence_size - t->pc &&
           count <= t->sequence_end - t->pc;
}

static uint8_t ndp_track_read8(NdpTrack *t)
{
    if (!ndp_track_can_read(t, 1)) return 0xff;
    return t->sequence_data[t->pc++];
}

static uint16_t ndp_track_read16(NdpTrack *t)
{
    uint16_t value;
    if (!ndp_track_can_read(t, 2)) return 0;
    value = ndp_le16(t->sequence_data + t->pc);
    t->pc += 2;
    return value;
}

static int16_t ndp_track_read_s16(NdpTrack *t)
{
    return (int16_t)ndp_track_read16(t);
}

static void ndp_repeat_escape(NdpTrack *t)
{
    NdpRepeat *r;
    if (t->repeat_depth == 0) return;
    r = &t->repeat[t->repeat_depth - 1];
    r->escape = t->pc;
    if (r->remaining == 1) {
        unsigned int depth = 1;
        int found = 0;
        while (ndp_track_can_read(t, 1)) {
            uint8_t c = ndp_track_read8(t);
            if (c < 0x60) {
                uint8_t length;
                do {
                    if (!ndp_track_can_read(t, 1)) break;
                    length = ndp_track_read8(t);
                } while (length == 255);
                continue;
            }
            if (c == 0xf1 || (ndp_s.legacy_commands && c == 0x8e)) {
                ++depth;
                continue;
            }
            if (c == 0xf3 || (ndp_s.legacy_commands && c == 0x8f)) {
                if (!ndp_track_can_read(t, 1)) break;
                (void)ndp_track_read8(t);
                if (--depth == 0) { found = 1; break; }
                continue;
            }
            if (c == 0xff) break;
            {
                unsigned int arguments = ndp_track_argument_count(c);
                if (!ndp_track_can_read(t, arguments)) break;
                t->pc += arguments;
            }
        }
        if (found) --t->repeat_depth;
    }
}

static void ndp_repeat_end(NdpTrack *t, uint8_t count)
{
    NdpRepeat *r;
    if (t->repeat_depth == 0) return;
    r = &t->repeat[t->repeat_depth - 1];
    if (r->remaining == 0) r->remaining = count == 0 ? 255 : count;
    if (r->remaining > 1) {
        --r->remaining;
        t->pc = r->start;
    } else {
        --t->repeat_depth;
    }
}

static void ndp_rhythm_repeat_start(void)
{
    NdpRepeat *r;
    if (ndp_s.rhythm_repeat_depth >= NDP_REPEAT_DEPTH) return;
    r = &ndp_s.rhythm_repeat[ndp_s.rhythm_repeat_depth++];
    r->start = ndp_s.rhythm_pc;
    r->escape = NDP_NONE;
    r->remaining = 0;
}

static void ndp_rhythm_repeat_end(uint8_t count)
{
    NdpRepeat *r;
    if (ndp_s.rhythm_repeat_depth == 0) return;
    r = &ndp_s.rhythm_repeat[ndp_s.rhythm_repeat_depth - 1];
    if (r->remaining == 0) r->remaining = count == 0 ? 255 : count;
    if (r->remaining > 1) {
        --r->remaining;
        ndp_s.rhythm_pc = r->start;
    } else {
        --ndp_s.rhythm_repeat_depth;
    }
}

static void ndp_rhythm_repeat_escape(void)
{
    NdpRepeat *r;
    uint32_t pc;
    unsigned int depth = 1;
    unsigned int guard = 0;
    if (ndp_s.rhythm_repeat_depth == 0) return;
    r = &ndp_s.rhythm_repeat[ndp_s.rhythm_repeat_depth - 1];
    r->escape = ndp_s.rhythm_pc;
    if (r->remaining != 1) return;
    pc = ndp_s.rhythm_pc;
    while (ndp_can_read(pc, 1) && guard++ < 65535) {
        uint8_t c = ndp_read8(&pc);
        if (c < 0x40) {
            uint8_t length;
            do { length = ndp_read8(&pc); } while (length == 255 && ndp_can_read(pc, 1));
            continue;
        }
        if (ndp_s.legacy_commands && c == 0x8e) { ++depth; continue; }
        if (ndp_s.legacy_commands && c == 0x8f) {
            (void)ndp_read8(&pc);
            if (--depth == 0) break;
            continue;
        }
        if (c == 0xf1) { ++depth; continue; }
        if (c == 0xf3) {
            (void)ndp_read8(&pc);
            if (--depth == 0) break;
            continue;
        }
        if (c < 0xc0) { (void)ndp_read8(&pc); continue; }
        if (c == 0xf0 || c == 0xfc) { (void)ndp_read16(&pc); continue; }
        if (c == 0xf4 || c == 0xfd || c == 0xfe) { (void)ndp_read8(&pc); continue; }
        if (c == 0xff) { (void)ndp_read16(&pc); break; }
    }
    ndp_s.rhythm_pc = pc;
    --ndp_s.rhythm_repeat_depth;
}

static void ndp_set_note(NdpTrack *t, uint8_t note, uint16_t length)
{
    int index;
    int target;
    t->old_note = t->note;
    t->note = note;
    t->duration = length;
    if (note == 0 || length == 0) {
        t->gate = 0;
        return;
    }
    if (t->fixed_gate != 0) t->gate = t->fixed_gate < length ? t->fixed_gate : length;
    else {
        unsigned int gate = ((unsigned int)length * t->q) >> 3;
        if (gate > t->q_sub) gate -= t->q_sub;
        else gate = 1;
        t->gate = (uint16_t)(gate == 0 ? 1 : gate);
    }
    index = (int)note - 1;
    if (index < 0) index = 0;
    if (index > 95) index = 95;
    target = (int)ndp_s.periods[index];
    if (target < 1) target = 1;
    t->target_period = target;
    if (t->release_pitch_skip || t->old_note == 0) {
        t->release_pitch_period = target;
        t->release_pitch_skip = 0;
    } else {
        int old_index = (int)t->old_note - 1;
        if (old_index < 0) old_index = 0;
        if (old_index > 95) old_index = 95;
        t->release_pitch_period = ndp_s.periods[old_index];
    }
    t->release_pitch_count = t->release_pitch_delay;
    if (t->portamento_step == 0 || t->period == 0 || t->old_note == 0) t->period = target;
    ndp_reset_voice(t);
    ndp_reset_pitch_env(t);
    ndp_reset_note_env(t);
    t->volume_interval_amount = 0;
    t->volume_interval_count = (uint8_t)(t->volume_interval & 0x7f);
    ndp_s.volume_interval_span = 0;
    t->release_count = 1;
    t->release_active = 0;
    t->hard_phase = 0;
    t->hard_count = t->hard_interval;
}

static void ndp_track_end(NdpTrack *t, unsigned int channel)
{
    uint16_t loop;
    if (t->is_effect) {
        t->ended = 1;
        t->enabled = 0;
        t->note = 0;
        t->gate = 0;
        ndp_track_key(t, channel, 0);
        return;
    }
    loop = ndp_track_read16(t);
    ndp_s.ended_mask |= (uint8_t)(1U << channel);
    if (!ndp_s.no_infinite_loop && loop != 0 && loop < ndp_s.size) {
        t->pc = loop;
        ++ndp_s.loop_count;
    } else {
        t->ended = 1;
        t->enabled = 0;
        t->note = 0;
    }
}

static void ndp_stream_prepare_se(void)
{
#if NDP_PROFILE == NDP_PROFILE_STREAM
    unsigned int channel;
    if (ndp_s.se_hardware_ready) return;
    for (channel = 0; channel < NDP_TRACKS; ++channel)
        ndp_setup_channel(NDP_STREAM_SE_CHANNEL_BASE + channel);
    ndp_setup_channel(NDP_STREAM_SE_NOISE_CHANNEL);
    for (channel = 0; channel < NDP_TRACKS; ++channel)
        ndp_s.se_tone_on[channel] = 0;
    ndp_s.se_noise_on = 0;
    ndp_s.se_hardware_ready = 1;
#endif
}

static int ndp_se_begin(const uint8_t *data, uint32_t size,
                         uint32_t offset, uint32_t end)
{
    uint8_t priority;
    unsigned int channel;
    if (data == 0 || end > size || offset > end || end - offset < 8U)
        return 0;
    for (channel = 0; channel < NDP_TRACKS; ++channel) {
        uint16_t track = ndp_le16(data + offset + channel * 2U);
        if (track != 0 && (track < 8U || offset + track >= end)) return 0;
    }
    priority = data[offset + 7U];
    if (priority > 7U) return 0;
    if (ndp_s.se_playing && priority > ndp_s.se_priority) return 0;
    ndp_se_stop();
    ndp_stream_prepare_se();
    ndp_s.se_data = data;
    ndp_s.se_size = size;
    ndp_s.se_effect_end = end;
    ndp_s.se_priority = priority;
    ndp_s.se_playing = 1;
    for (channel = 0; channel < NDP_TRACKS; ++channel) {
        NdpTrack *t = &ndp_s.se_track[channel];
        uint16_t track = ndp_le16(data + offset + channel * 2U);
        ndp_zero(t, sizeof(*t));
        t->env_pc = NDP_NONE;
        t->pitch_pc = NDP_NONE;
        t->note_pc = NDP_NONE;
        t->sequence_data = data;
        t->sequence_size = size;
        t->sequence_end = end;
        t->pc = offset + track;
        t->enabled = (uint8_t)(track != 0);
        t->ended = (uint8_t)(track == 0);
        t->volume = 15;
        t->env_level = 15;
        t->env_done = 1;
        t->mix = 1;
        t->q = 8;
        t->noise = 16;
        t->is_effect = 1;
    }
    return 1;
}

static int ndp_se_play_embedded(uint16_t address)
{
#if NDP_FEATURE_EMBEDDED_SE
    uint32_t offset;
    if (address < 0x4000U) offset = address;
    else {
        if (address < ndp_s.data_address) return 0;
        offset = (uint32_t)(address - ndp_s.data_address);
    }
    return ndp_se_begin(ndp_s.data, ndp_s.size, offset, ndp_s.size);
#else
    (void)address;
    return 0;
#endif
}

static void ndp_track_event(NdpTrack *t, unsigned int channel)
{
    unsigned int guard = 0;
    if (t->more_length) {
        uint8_t length = ndp_track_read8(t);
        t->duration = length;
        t->gate = length;
        t->more_length = (uint8_t)(length == 255);
        return;
    }
    while (t->enabled && t->duration == 0 && guard++ < 128) {
        uint8_t c = ndp_track_read8(t);
        if (c < 0x60) {
            uint8_t length = ndp_track_read8(t);
            ndp_set_note(t, c, length);
            t->more_length = (uint8_t)(length == 255);
            continue;
        }
        if (c < 0x70) { t->volume = (uint8_t)(15 - (c & 15)); continue; }
        if (c < 0x80) {
            t->voice = (uint8_t)(c & 15);
            t->hard_enabled = 0;
            continue;
        }
        if (c >= 0xb0 && c < 0xc0) {
            unsigned int v = t->volume + (c & 15);
            t->volume = (uint8_t)(v > 15 ? 15 : v);
            continue;
        }
        if (c >= 0xc0 && c < 0xd0) {
            unsigned int n = c & 15;
            t->volume = (uint8_t)(t->volume > n ? t->volume - n : 0);
            continue;
        }
        switch (c) {
        case 0x80: {
            uint8_t value = ndp_track_read8(t);
            uint8_t selector = (uint8_t)(t->pitch_env & 31);
            t->pitch_delay = value;
            if (selector != 0 && selector <= 16) {
                ndp_s.pitch_env_delay[selector - 1] = value;
                ndp_s.pitch_env_delay_valid |= (uint16_t)(1U << (selector - 1));
            }
            break;
        }
        case 0x81: t->mix = (uint8_t)(ndp_track_read8(t) & 3); break;
        case 0x82: t->pitch_env = ndp_track_read8(t); ndp_reset_pitch_env(t); break;
        case 0x83: t->noise = (uint8_t)(ndp_track_read8(t) & 31); break;
        case 0x84: t->next_legato = 0; break;
        case 0x85: t->next_legato = 1; break;
        case 0x86: t->q = (uint8_t)(ndp_track_read8(t) & 15); break;
        case 0x87: t->detune = (int8_t)ndp_track_read8(t); break;
        case 0x88: t->portamento_step = (int16_t)ndp_track_read8(t); break;
        case 0x89:
            if (ndp_s.legacy_commands) ndp_repeat_escape(t);
            else t->detune = (int16_t)(t->detune + ndp_track_read_s16(t));
            break;
        case 0x8a: t->release_pitch_delay = 0; break;
        case 0x8b: t->release_pitch_delay = 255; break;
        case 0x8c: t->release_pitch_delay = ndp_track_read8(t); break;
        case 0x8d: {
            uint8_t value = ndp_track_read8(t);
            if ((value & 0x80) != 0) {
                t->release_volume = (uint8_t)(value & 15);
                t->release_count = 1;
                t->release_active = 1;
            } else {
                t->release_level = value;
                t->release_active = 0;
            }
            break;
        }
        case 0x8e:
            if (ndp_s.legacy_commands) ndp_repeat_start(t);
            else t->detune = (int16_t)ndp_track_read16(t);
            break;
        case 0x8f:
            if (ndp_s.legacy_commands) ndp_repeat_end(t, ndp_track_read8(t));
            else if (ndp_track_read8(t) != 0) t->pitch_env |= 0x40;
            else t->pitch_env &= (uint8_t)~0x40;
            break;
        case 0xa0: {
            ndp_set_hard_period(ndp_track_read16(t));
            break;
        }
        case 0xa1:
            (void)ndp_track_read8(t); /* Fractional step is unused by v1.07. */
            t->portamento_step = (int16_t)ndp_track_read8(t);
            {
                uint8_t note = ndp_track_read8(t);
                if (note > 0 && note <= 96) t->period = ndp_s.periods[note - 1];
            }
            break;
        case 0xa2: t->q_sub = ndp_track_read8(t); break;
        case 0xa3:
            t->release_delay = ndp_track_read8(t);
            t->release_count = 1;
            break;
        case 0xa4: t->note_env = ndp_track_read8(t); ndp_reset_note_env(t); break;
        case 0xa5:
            t->volume_interval = ndp_track_read8(t);
            t->volume_interval_count = (uint8_t)(t->volume_interval & 0x7f);
            if (t->volume_interval == 0) t->volume_interval_target = 0;
            break;
        case 0xa6: {
            uint8_t note = ndp_track_read8(t);
            uint16_t period = ndp_track_read16(t);
            if (note > 0 && note <= 96) ndp_s.periods[note - 1] = period;
            break;
        }
        case 0xa7: t->volume_interval_target = ndp_track_read8(t); break;
        case 0xa8: {
            uint8_t note = ndp_track_read8(t);
            if (note > 0 && note <= 96) t->period = ndp_s.periods[note - 1];
            break;
        }
        case 0xa9: t->release_pitch_skip = 1; break;
        case 0xaa: t->fixed_gate = ndp_track_read8(t); break;
        case 0xab: {
            uint8_t value = ndp_track_read8(t);
            if ((value & 0x80) != 0) t->volume = t->saved_volume;
            else t->saved_volume = t->volume;
            break;
        }
        case 0xac: (void)ndp_se_play_embedded(ndp_track_read16(t)); break;
        case 0xf0: {
            uint8_t interval = ndp_track_read8(t);
            uint8_t loops = ndp_track_read8(t);
            if (ndp_s.fade_loop_count == 0) {
                if (ndp_s.fade_direction != 0) break;
                ndp_s.fade_loop_count = loops;
            }
            --ndp_s.fade_loop_count;
            if (ndp_s.fade_loop_count == 0) {
                ndp_s.fade_interval = interval == 0 ? 1 : interval;
                ndp_s.fade_count = (uint8_t)(ndp_s.fade_interval - 1U);
                ndp_s.fade_direction = 1;
            }
            break;
        }
        case 0xf1: ndp_repeat_start(t); break;
        case 0xf2: ndp_repeat_escape(t); break;
        case 0xf3: ndp_repeat_end(t, ndp_track_read8(t)); break;
        case 0xf4: {
            uint8_t interrupt_track = ndp_track_read8(t);
            if (interrupt_track >= 1 && interrupt_track <= 3)
                ndp_s.rhythm_channel = (uint8_t)(3 - interrupt_track);
            break;
        }
        case 0xfc: {
            uint8_t reg = ndp_track_read8(t);
            uint8_t value = ndp_track_read8(t);
            ndp_psg_direct_write(reg, value);
            break;
        }
        case 0xfd: ndp_s.slow_mask = ndp_track_read8(t); break;
        case 0xfe:
#if NDP_FEATURE_FAST_FORWARD
            ndp_s.fast_forward = ndp_track_read8(t);
#else
            (void)ndp_track_read8(t);
#endif
            break;
        case 0xff: ndp_track_end(t, channel); break;
        default:
            if (c >= 0x90 && c < 0xa0) {
                t->hard_enabled = 1;
                t->hard_shape = (uint8_t)(c & 15);
                t->hard_interval = ndp_track_read8(t);
                t->hard_count = t->hard_interval;
            } else {
                t->enabled = 0;
                t->ended = 1;
            }
            break;
        }
    }
}

static void ndp_volume_interval_tick(NdpTrack *t)
{
    uint8_t interval = (uint8_t)(t->volume_interval & 0x7f);
    if (interval == 0) return;
    if (interval >= 64) {
        ++ndp_s.volume_interval_span;
        if (ndp_s.volume_interval_span < 2) return;
        ndp_s.volume_interval_span = 0;
    }
    if (t->volume_interval_count > 1) {
        --t->volume_interval_count;
        return;
    }
    t->volume_interval_count = interval;
    if (t->volume_interval_amount < 15) ++t->volume_interval_amount;
}

static int ndp_current_volume(NdpTrack *t)
{
    int volume;
    int limit;
    unsigned int env = t->hard_enabled ? ndp_hard_level(t) : t->env_level;
    if (t->hard_enabled) volume = (int)env;
    else volume = (int)env - (15 - (int)t->volume);
    if ((t->volume_interval & 0x80) != 0) {
        volume -= t->volume_interval_amount;
    } else {
        limit = t->volume_interval_target != 0 ?
                (int)t->volume_interval_target : (int)t->volume;
        volume += t->volume_interval_amount;
        if (volume > limit) volume = limit;
    }
    if (volume < 0) volume = 0;
    if (volume > 15) volume = 15;
    return volume;
}

static void ndp_release_tick(NdpTrack *t)
{
    if (t->gate != 0) return;
    if (!t->release_active) {
        int volume = ndp_current_volume(t) - t->release_level;
        t->release_volume = (uint8_t)(volume > 0 ? volume : 0);
        t->release_count = 1;
        t->release_active = 1;
    }
    if (t->release_delay == 0) {
        t->release_volume = 0;
        return;
    }
    --t->release_count;
    if (t->release_count == 0) {
        t->release_count = (uint8_t)(t->release_delay + 1U);
        if (t->release_volume != 0) --t->release_volume;
    }
}

static void ndp_release_pitch_tick(NdpTrack *t)
{
    if (t->release_pitch_count == 0 || t->release_pitch_count == 255) return;
    --t->release_pitch_count;
    if (t->release_pitch_count == 0) {
        t->release_pitch_count = t->release_pitch_delay;
        t->release_pitch_period = t->target_period;
    }
}

static unsigned int ndp_effective_volume(NdpTrack *t)
{
    int volume = t->release_active ? (int)t->release_volume
                                   : ndp_current_volume(t);
    volume -= ndp_s.master_volume;
    if (volume < 0 || t->mute_frames > 1 ||
        (t->note == 0 && !t->release_active)) return 0;
    if (volume > 15) return 15;
    return (unsigned int)volume;
}

static void ndp_render_track(NdpTrack *t, unsigned int channel, unsigned int *noise_volume, unsigned int *noise_frequency)
{
    unsigned int volume;
    unsigned int opm_channel = ndp_track_opm_channel(t, channel);
    uint8_t render_mix;
    int index;
    int period;
    uint8_t kc, kf;
    if (!t->enabled && t->release_count == 0) {
        ndp_psg_volume_state[channel] = 0;
        ndp_psg_mix_state[channel] = 0;
        ndp_psg_hard_state[channel] = 0;
        ndp_track_key(t, channel, 0);
        ndp_opm((uint8_t)(0x70 + opm_channel), 127);
        return;
    }
    ndp_volume_interval_tick(t);
    ndp_voice_tick(t);
    if (t->hard_enabled && t->hard_count != 0) --t->hard_count;
    ndp_release_tick(t);
    ndp_release_pitch_tick(t);
    ndp_pitch_tick(t);
    ndp_note_tick(t);
    if (t->portamento_step != 0 && t->period != t->target_period) {
        int step = t->portamento_step;
        if (step < 0) step = -step;
        if (step == 0) step = 1;
        if (t->period < t->target_period) {
            t->period += step;
            if (t->period > t->target_period) t->period = t->target_period;
        } else {
            t->period -= step;
            if (t->period < t->target_period) t->period = t->target_period;
        }
    }
    index = (int)t->note - 1 + t->note_delta;
    if (index < 0) index = 0;
    if (index > 95) index = 95;
    period = t->period;
    if (t->note_delta != 0) period = ndp_s.periods[index];
    if (t->note == 0 && t->release_pitch_delay != 0 &&
        t->release_pitch_period > 0) period = t->release_pitch_period;
    period += t->pitch_delta + t->detune;
    if (t->voice_pitch_zero ||
        (t->hard_enabled && t->hard_interval != 0 && t->hard_count == 0)) period = 0;
    else if (period < 1) period = 1;
    if (period > 4095) period = 4095;
    ndp_period_to_opm(period, channel, &kc, &kf);
    ndp_opm((uint8_t)(0x28 + opm_channel), kc);
    ndp_opm((uint8_t)(0x30 + opm_channel), kf);
    volume = ndp_effective_volume(t);
    ndp_psg_period_state[channel] = (uint16_t)period;
    ndp_psg_volume_state[channel] = (uint8_t)volume;
    if (t->hard_enabled && t->hard_interval != 0)
        render_mix = (uint8_t)(t->hard_count == 0 ? 1 : 0);
    else
        render_mix = t->mix;
    ndp_psg_mix_state[channel] = render_mix;
    ndp_psg_hard_state[channel] = (uint8_t)(t->hard_enabled &&
        t->note != 0 && t->gate != 0 && ndp_s.master_volume == 0);
    if (ndp_psg_hard_state[channel]) ndp_s.hard_last_shape = t->hard_shape;
    ndp_apply_timbre(opm_channel, (unsigned int)period, render_mix);
    ndp_opm((uint8_t)(0x70 + opm_channel),
            ndp_tone_total_level(volume, channel));
    ndp_track_key(t, channel, volume != 0 && (render_mix & 1) != 0);
    if (volume != 0 && (render_mix & 2) != 0) {
        ndp_noise_merge(noise_volume, noise_frequency, volume, t->noise);
    }
}

static void ndp_rhythm_set_attenuation(unsigned int instrument, unsigned int value)
{
    unsigned int i;
    value &= 15;
    if (instrument == 31) {
        for (i = 0; i < 32; ++i) ndp_s.rhythm_attenuation[i] = (uint8_t)value;
    } else if (instrument < 32) {
        ndp_s.rhythm_attenuation[instrument] = (uint8_t)value;
    }
}

static void ndp_rhythm_adjust_attenuation(unsigned int instrument, int change)
{
    unsigned int first = instrument == 31 ? 0 : instrument;
    unsigned int last = instrument == 31 ? 31 : instrument;
    unsigned int i;
    if (first >= 32) return;
    for (i = first; i <= last; ++i) {
        int value = (int)ndp_s.rhythm_attenuation[i] + change;
        if (value < 0) value = 0;
        if (value > 15) value = 15;
        ndp_s.rhythm_attenuation[i] = (uint8_t)value;
    }
}

static void ndp_rhythm_write_register(uint8_t reg, uint8_t value)
{
    unsigned int channel = ndp_s.rhythm_channel;
    if (reg <= 5) {
        if ((reg & 1) == 0)
            ndp_s.rhythm_period = (uint16_t)((ndp_s.rhythm_period & 0x0f00) | value);
        else
            ndp_s.rhythm_period = (uint16_t)((ndp_s.rhythm_period & 0x00ff) |
                                             ((uint16_t)(value & 15) << 8));
    } else if (reg == 6) {
        ndp_s.rhythm_noise = (uint8_t)(value & 31);
    } else if (reg == 7) {
        ndp_s.rhythm_mix = (uint8_t)(((value >> channel) & 1 ? 0 : 1) |
                                     ((value >> (channel + 3)) & 1 ? 0 : 2));
    } else if (reg >= 8 && reg <= 10) {
        ndp_s.rhythm_level = (uint8_t)(value & 15);
    }
}

static void ndp_rhythm_start(uint8_t note)
{
    uint32_t voice;
    if (note >= 32) return;
    ndp_s.rhythm_note = note;
    ndp_s.rhythm_mix = 0;
    ndp_s.rhythm_level = 0;
    ndp_s.rhythm_noise = 16;
    ndp_s.rhythm_period = 0;
    ndp_s.rhythm_end_pending = 0;
    voice = ndp_s.rhythm_voice[note];
    if (voice == NDP_NONE) {
        /* A short fallback keeps hand-authored data useful without presets. */
        ndp_s.rhythm_mix = 2;
        ndp_s.rhythm_level = 12;
        ndp_s.rhythm_noise = note;
        ndp_s.rhythm_active = 1;
        ndp_s.rhythm_end_pending = 1;
        return;
    }
    ndp_s.rhythm_env_pc = voice;
    ndp_s.rhythm_active = 1;
}

static void ndp_rhythm_envelope(void)
{
    unsigned int guard = 0;
    while (ndp_s.rhythm_active && guard++ < 64 && ndp_can_read(ndp_s.rhythm_env_pc, 1)) {
        uint8_t command = ndp_read8(&ndp_s.rhythm_env_pc);
        if (command == 0xff) {
            ndp_s.rhythm_end_pending = 1;
            return;
        }
        if (command == 0x10) return;
        if (command >= 0x20 && command <= 0x23) {
            ndp_s.rhythm_mix = (uint8_t)(command & 3);
            continue;
        }
        if (!ndp_s.rhythm_voice_modern[ndp_s.rhythm_note]) {
            if (command <= 0x0f && ndp_can_read(ndp_s.rhythm_env_pc, 1)) {
                ndp_rhythm_write_register(command, ndp_read8(&ndp_s.rhythm_env_pc));
                continue;
            }
        } else {
            if (command == 1 && ndp_can_read(ndp_s.rhythm_env_pc, 1)) {
                ndp_s.rhythm_level = ndp_read8(&ndp_s.rhythm_env_pc);
                continue;
            }
            if (command < 6 && ndp_can_read(ndp_s.rhythm_env_pc, 2)) {
                uint8_t high = ndp_read8(&ndp_s.rhythm_env_pc);
                uint8_t low = ndp_read8(&ndp_s.rhythm_env_pc);
                ndp_s.rhythm_period = (uint16_t)(((uint16_t)(high & 15) << 8) | low);
                continue;
            }
            if (command >= 6 && command <= 15 && ndp_can_read(ndp_s.rhythm_env_pc, 1)) {
                ndp_rhythm_write_register(command, ndp_read8(&ndp_s.rhythm_env_pc));
                continue;
            }
        }
        ndp_s.rhythm_end_pending = 1;
        return;
    }
}

static unsigned int ndp_rhythm_effective_volume(void)
{
    unsigned int attenuation;
    unsigned int level = ndp_s.rhythm_level > 15 ? 15 : ndp_s.rhythm_level;
    if (ndp_s.rhythm_note >= 32) return 0;
    if (ndp_s.rhythm_channel < NDP_TRACKS &&
        ndp_s.track[ndp_s.rhythm_channel].mute_frames != 0) return 0;
    attenuation = ndp_s.rhythm_attenuation[ndp_s.rhythm_note] + ndp_s.master_volume;
    return level > attenuation ? level - attenuation : 0;
}

static void ndp_render_rhythm(unsigned int *noise_volume, unsigned int *noise_frequency)
{
    unsigned int volume;
    uint8_t kc, kf;
    if (!ndp_s.rhythm_active) return;
    volume = ndp_rhythm_effective_volume();
    if (volume != 0 && (ndp_s.rhythm_mix & 1) != 0 && ndp_s.rhythm_period != 0) {
        ndp_period_to_opm(ndp_s.rhythm_period, 3, &kc, &kf);
        ndp_opm((uint8_t)(0x28 + ndp_s.rhythm_channel), kc);
        ndp_opm((uint8_t)(0x30 + ndp_s.rhythm_channel), kf);
        ndp_apply_timbre(ndp_s.rhythm_channel, ndp_s.rhythm_period,
                         ndp_s.rhythm_mix);
        ndp_opm((uint8_t)(0x70 + ndp_s.rhythm_channel),
                ndp_tone_total_level(volume, 3));
        ndp_key(ndp_s.rhythm_channel, 1);
    } else {
        ndp_key(ndp_s.rhythm_channel, 0);
    }
    if (volume != 0 && (ndp_s.rhythm_mix & 2) != 0) {
        ndp_noise_merge(noise_volume, noise_frequency, volume,
                        ndp_s.rhythm_noise);
    }
    if (ndp_s.rhythm_channel < NDP_TRACKS) {
        ndp_psg_period_state[ndp_s.rhythm_channel] = ndp_s.rhythm_period;
        ndp_psg_volume_state[ndp_s.rhythm_channel] = (uint8_t)volume;
        ndp_psg_mix_state[ndp_s.rhythm_channel] = ndp_s.rhythm_mix;
    }
}

static void ndp_rhythm_event(void)
{
    unsigned int guard = 0;
    if (!ndp_s.rhythm_enabled) return;
    if (ndp_s.rhythm_more_length) {
        uint8_t length = ndp_read8(&ndp_s.rhythm_pc);
        ndp_s.rhythm_duration = length;
        ndp_s.rhythm_more_length = (uint8_t)(length == 255);
        return;
    }
    while (ndp_s.rhythm_duration == 0 && guard++ < 128) {
        uint8_t c = ndp_read8(&ndp_s.rhythm_pc);
        if (c < 0x40) {
            uint8_t length = ndp_read8(&ndp_s.rhythm_pc);
            if (c >= 0x20) ndp_rhythm_start((uint8_t)(c - 0x20));
            else {
                ndp_s.rhythm_active = 0;
                ndp_s.rhythm_end_pending = 0;
            }
            ndp_s.rhythm_duration = length;
            ndp_s.rhythm_more_length = (uint8_t)(length == 255);
        } else if (c < 0x80) {
            uint8_t value = ndp_read8(&ndp_s.rhythm_pc);
            if (c < 0x60)
                ndp_rhythm_adjust_attenuation(c & 31, value);
            else
                ndp_rhythm_adjust_attenuation(c & 31, -(int)value);
        } else if (ndp_s.legacy_commands && c == 0x89) {
            ndp_rhythm_repeat_escape();
        } else if (ndp_s.legacy_commands && c == 0x8e) {
            ndp_rhythm_repeat_start();
        } else if (ndp_s.legacy_commands && c == 0x8f) {
            ndp_rhythm_repeat_end(ndp_read8(&ndp_s.rhythm_pc));
        } else if (c < 0xc0) {
            uint8_t value = ndp_read8(&ndp_s.rhythm_pc);
            /* NDP 0.9 also emits 80-9F for rhythm-volume commands. */
            ndp_rhythm_set_attenuation(c & 31, value);
        } else if (c == 0xfc) {
            uint8_t reg = ndp_read8(&ndp_s.rhythm_pc);
            uint8_t value = ndp_read8(&ndp_s.rhythm_pc);
            ndp_rhythm_write_register(reg, value);
        } else if (c == 0xf1) {
            ndp_rhythm_repeat_start();
        } else if (c == 0xf2) {
            ndp_rhythm_repeat_escape();
        } else if (c == 0xf3) {
            ndp_rhythm_repeat_end(ndp_read8(&ndp_s.rhythm_pc));
        } else if (c == 0xf4) {
            uint8_t track = ndp_read8(&ndp_s.rhythm_pc);
            if (track >= 1 && track <= 3) ndp_s.rhythm_channel = (uint8_t)(3 - track);
        } else if (c == 0xfd) {
            ndp_s.slow_mask = ndp_read8(&ndp_s.rhythm_pc);
        } else if (c == 0xfe) {
#if NDP_FEATURE_FAST_FORWARD
            ndp_s.fast_forward = ndp_read8(&ndp_s.rhythm_pc);
#else
            (void)ndp_read8(&ndp_s.rhythm_pc);
#endif
        } else if (c == 0xff) {
            uint16_t loop = ndp_read16(&ndp_s.rhythm_pc);
            if (!ndp_s.no_infinite_loop && loop != 0 && loop < ndp_s.size) ndp_s.rhythm_pc = loop;
            else ndp_s.rhythm_enabled = 0;
        } else if (c == 0xf0) {
            (void)ndp_read16(&ndp_s.rhythm_pc);
        }
    }
}

static void ndp_update_se(unsigned int *noise_volume,
                          unsigned int *noise_frequency)
{
    unsigned int channel;
    unsigned int active = 0;
    if (!ndp_s.se_playing) return;
    for (channel = 0; channel < NDP_TRACKS; ++channel) {
        NdpTrack *t = &ndp_s.se_track[channel];
        unsigned int track_noise_volume = 0;
        unsigned int track_noise_frequency = 0;
        if (!t->enabled) continue;
        if (t->duration != 0) --t->duration;
        if (t->gate != 0) --t->gate;
        if (t->duration == 0) ndp_track_event(t, channel);
        if (!t->enabled) continue;
        ++active;
        ndp_render_track(t, channel, &track_noise_volume,
                         &track_noise_frequency);
        ndp_noise_merge(noise_volume, noise_frequency,
                        track_noise_volume, track_noise_frequency);
    }
    if (active == 0) ndp_s.se_playing = 0;
}

static void ndp_stream_se_noise_key(int on)
{
#if NDP_PROFILE == NDP_PROFILE_STREAM
    uint8_t state = (uint8_t)(on != 0);
    unsigned int channel = NDP_STREAM_SE_NOISE_CHANNEL;
    if (!ndp_s.se_hardware_ready || ndp_s.se_noise_on == state) return;
    ndp_s.se_noise_on = state;
    ndp_opm((uint8_t)(0x20 + channel),
            (uint8_t)(NDP_TONE_CONTROL | (state ? 0xc0 : 0)));
#else
    (void)on;
#endif
}

static void ndp_stream_render_se_noise(unsigned int volume,
                                       unsigned int frequency)
{
#if NDP_PROFILE == NDP_PROFILE_STREAM
    unsigned int channel = NDP_STREAM_SE_NOISE_CHANNEL;
    int period;
    uint8_t kc, kf;
    if (!ndp_s.se_hardware_ready) return;
    if (volume == 0) {
        ndp_opm((uint8_t)(0x70 + channel), 127);
        ndp_stream_se_noise_key(0);
        return;
    }
    period = (int)(((frequency & 31U) + 1U) * 96U);
    if (period > 4095) period = 4095;
    ndp_period_to_opm(period, 3, &kc, &kf);
    ndp_opm((uint8_t)(0x28 + channel), kc);
    ndp_opm((uint8_t)(0x30 + channel), kf);
    ndp_opm((uint8_t)(0x70 + channel),
            ndp_tone_total_level(volume, 3));
    ndp_stream_se_noise_key(1);
#else
    (void)volume;
    (void)frequency;
#endif
}

static NdpStreamState ndp_stream_s;

static uint16_t ndp_stream_be16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t ndp_stream_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void ndp_stream_reset(void)
{
    ndp_stream_s.active = 0;
    ndp_zero(&ndp_stream_s, sizeof(ndp_stream_s));
}

static uint8_t ndp_stream_attenuate(uint8_t value)
{
    unsigned int adjusted = value + ndp_s.master_volume * 4U;
    return (uint8_t)(adjusted > 127U ? 127U : adjusted);
}

static void ndp_stream_apply_master_volume(void)
{
    unsigned int slot;
    for (slot = 0; slot < 16; ++slot) {
        if ((ndp_stream_s.carrier_valid & (1U << slot)) != 0)
            ndp_opm_tracked((uint8_t)(0x70 + slot),
                            ndp_stream_attenuate(
                                ndp_stream_s.carrier_tl[slot]));
    }
}

static int ndp_stream_start_data(const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t frames;
    uint32_t events;
    uint32_t loop_event = 0;
    uint32_t loop_event_frame = 0;
    uint16_t loop_frame;
    uint16_t loop_wait = 0;
    uint8_t flags;
    if (bytes == 0 || size < 16 || size > 0xffffffffUL) return 0;
    if (bytes[0] != 'N' || bytes[1] != 'D' ||
        bytes[2] != 'S' || bytes[3] != 'R' || bytes[4] != 1) return 0;
    frames = ndp_stream_be32(bytes + 8);
    events = ndp_stream_be32(bytes + 12);
    if (frames == 0 || events > ((uint32_t)size - 16U) / 4U) return 0;
    flags = bytes[5];
    loop_frame = ndp_stream_be16(bytes + 6);
    if (loop_frame >= frames) return 0;
    if ((flags & 1U) != 0) {
        while (loop_event < events) {
            uint32_t offset = 16U + loop_event * 4U;
            loop_event_frame += ndp_stream_be16(bytes + offset);
            if (loop_event_frame >= loop_frame) break;
            ++loop_event;
        }
        if (loop_event < events)
            loop_wait = (uint16_t)(loop_event_frame - loop_frame);
    }
    ndp_stop();
    ndp_stream_s.data = bytes;
    ndp_stream_s.size = (uint32_t)size;
    ndp_stream_s.frame_count = frames;
    ndp_stream_s.frames_left = frames;
    ndp_stream_s.event_count = events;
    ndp_stream_s.event_index = 0;
    ndp_stream_s.loop_event_index = loop_event;
    ndp_stream_s.loop_wait_frames = loop_wait;
    ndp_stream_s.flags = flags;
    ndp_stream_s.loop_frame = loop_frame;
    ndp_stream_s.wait_frames = events ? ndp_stream_be16(bytes + 16) : 0;
    ndp_s.playing = 1;
    ndp_stream_s.active = 1;
    return 1;
}

static void ndp_stream_update_core(void)
{
    unsigned int processed = 0;
    while (ndp_stream_s.active && ndp_stream_s.event_index <
           ndp_stream_s.event_count && ndp_stream_s.wait_frames == 0 &&
           processed < NDP_STREAM_MAX_EVENTS_PER_UPDATE) {
        uint32_t offset = 16U + ndp_stream_s.event_index * 4U;
        uint8_t reg = ndp_stream_s.data[offset + 2];
        uint8_t value = ndp_stream_s.data[offset + 3];
        if (reg >= 0x70 && reg <= 0x7f && value < 0x80) {
            unsigned int slot = reg - 0x70;
            ndp_stream_s.carrier_tl[slot] = value;
            ndp_stream_s.carrier_valid |= (uint16_t)(1U << slot);
            value = ndp_stream_attenuate(value);
        }
        ndp_opm_tracked(reg, value);
        ++ndp_stream_s.event_index;
        ++processed;
        if (ndp_stream_s.event_index < ndp_stream_s.event_count) {
            offset += 4U;
            ndp_stream_s.wait_frames =
                ndp_stream_be16(ndp_stream_s.data + offset);
        }
    }
    if (ndp_stream_s.wait_frames == 0 &&
        ndp_stream_s.event_index < ndp_stream_s.event_count) return;
    if (!ndp_stream_s.active) return;
    if (ndp_stream_s.wait_frames != 0) --ndp_stream_s.wait_frames;
    if (ndp_stream_s.frames_left != 0) --ndp_stream_s.frames_left;
    if (ndp_stream_s.frames_left != 0) return;
    if (ndp_stream_s.flags & 1U) {
        ndp_stream_s.frames_left =
            ndp_stream_s.frame_count - ndp_stream_s.loop_frame;
        ndp_stream_s.event_index = ndp_stream_s.loop_event_index;
        ndp_stream_s.wait_frames = ndp_stream_s.loop_wait_frames;
    } else {
        ndp_stream_s.active = 0;
        ndp_s.playing = 0;
    }
}

int ndp_initialize(void)
{
    unsigned int ch;
    ndp_zero(&ndp_s, sizeof(ndp_s));
    ndp_stream_reset();
    ndp_set_hard_period(0x0400U);
    ndp_zero(ndp_pitch_error, sizeof(ndp_pitch_error));
    ndp_zero(ndp_volume_error, sizeof(ndp_volume_error));
    ndp_zero(ndp_psg_period_state, sizeof(ndp_psg_period_state));
    ndp_zero(ndp_psg_volume_state, sizeof(ndp_psg_volume_state));
    ndp_zero(ndp_psg_mix_state, sizeof(ndp_psg_mix_state));
    ndp_zero(ndp_psg_hard_state, sizeof(ndp_psg_hard_state));
    for (ch = 0; ch < 256; ++ch) ndp_s.opm_valid[ch] = 0;
    ndp_opm_raw(0x01, 0x02);
    for (ch = 0; ch < 8; ++ch) ndp_opm_raw(0x08, (uint8_t)ch);
    ndp_opm(0x1b, 0);
    ndp_opm(0x18, 0);
    ndp_opm(0x19, 0);
    ndp_opm(0x19, 0x80);
    for (ch = 0; ch < NDP_TRACKS; ++ch) ndp_setup_channel(ch);
    ndp_setup_noise();
    return 1;
}

void ndp_finalize(void)
{
    unsigned int ch;
    ndp_stop();
    for (ch = 0; ch < 8; ++ch) ndp_opm_raw(0x08, (uint8_t)ch);
    ndp_opm_raw(0x0f, 0);
}

int ndp_start(const void *data, size_t size)
{
#if NDP_PROFILE == NDP_PROFILE_STREAM
    return ndp_stream_start_data(data, size);
#else
    return ndp_start_at(data, size, NDP_DATA_ADDRESS);
#endif
}

int ndp_start_at(const void *data, size_t size, unsigned int data_address)
{
#if NDP_PROFILE == NDP_PROFILE_STREAM
    (void)data_address;
    return ndp_stream_start_data(data, size);
#else
    unsigned int i;
    const uint8_t *bytes = (const uint8_t *)data;
    if (bytes == 0 || size < 14 || size > 0xffffffffUL) return 0;
    ndp_stop();
    ndp_s.data = bytes;
    ndp_s.size = (uint32_t)size;
    ndp_s.data_address = (uint16_t)(data_address & 0xffffU);
    ndp_s.no_infinite_loop = (uint8_t)(bytes[11] & 1);
    ndp_s.legacy_commands = (uint8_t)(bytes[13] == 0);
    ndp_s.master_volume = 0;
    ndp_s.fade_interval = 0;
    ndp_s.fade_count = 0;
    ndp_s.fade_loop_count = 0;
    ndp_s.fade_direction = 0;
    ndp_s.ended_mask = 0;
    ndp_s.loop_count = 0;
    ndp_set_hard_period(0x0400U);
    ndp_s.hard_last_shape = 0;
    ndp_s.slow_mask = 0;
    ndp_s.slow_count = 0;
    ndp_s.fast_forward = 0;
    ndp_s.psg_noise = 0;
    for (i = 0; i < 96; ++i) ndp_s.periods[i] = ndp_default_periods[i];
    for (i = 0; i < 16; ++i) {
        ndp_s.voice[i] = NDP_NONE;
        ndp_s.pitch_env[i] = NDP_NONE;
        ndp_s.note_env[i] = NDP_NONE;
        ndp_s.pitch_env_delay[i] = 0;
    }
    ndp_s.pitch_env_delay_valid = 0;
    for (i = 0; i < 32; ++i) {
        ndp_s.rhythm_voice[i] = NDP_NONE;
        ndp_s.rhythm_voice_modern[i] = 0;
    }
    if (ndp_le16(bytes + 8) != 0 && ndp_le16(bytes + 8) < size)
        ndp_parse_definitions(ndp_le16(bytes + 8));
    ndp_s.rhythm_pc = ndp_le16(bytes + 0);
    ndp_s.rhythm_enabled = (uint8_t)(ndp_s.rhythm_pc != 0 && ndp_s.rhythm_pc < size);
    ndp_s.rhythm_duration = 0;
    ndp_s.rhythm_repeat_depth = 0;
    ndp_s.rhythm_note = 0;
    ndp_s.rhythm_active = 0;
    ndp_s.rhythm_end_pending = 0;
    ndp_s.rhythm_mix = 0;
    ndp_s.rhythm_level = 0;
    ndp_s.rhythm_noise = 16;
    ndp_s.rhythm_channel = 2;
    for (i = 0; i < 32; ++i) ndp_s.rhythm_attenuation[i] = 0;
    for (i = 0; i < NDP_TRACKS; ++i) {
        NdpTrack *t = &ndp_s.track[i];
        uint16_t offset = ndp_le16(bytes + 2 + i * 2);
        ndp_zero(t, sizeof(*t));
        t->sequence_data = bytes;
        t->sequence_size = (uint32_t)size;
        t->sequence_end = (uint32_t)size;
        t->pc = offset;
        t->enabled = (uint8_t)(offset != 0 && offset < size);
        t->ended = (uint8_t)!t->enabled;
        t->volume = 15;
        t->env_level = 15;
        t->mix = 1;
        t->q = 8;
        t->noise = 16;
        t->env_pc = NDP_NONE;
        t->pitch_pc = NDP_NONE;
        t->note_pc = NDP_NONE;
        if (!t->enabled) ndp_s.ended_mask |= (uint8_t)(1U << i);
    }
    ndp_s.playing = 1;
    return 1;
#endif
}

void ndp_stop(void)
{
    unsigned int ch;
    ndp_stream_reset();
    ndp_s.playing = 0;
    ndp_se_stop();
#if NDP_PROFILE == NDP_PROFILE_STREAM
    for (ch = 0; ch < NDP_TRACKS; ++ch) {
        uint8_t reg = (uint8_t)(0x20 + ch);
        uint8_t control = ndp_s.opm_valid[reg]
            ? (uint8_t)(ndp_s.opm_shadow[reg] & 0x3fU)
            : (uint8_t)NDP_TONE_CONTROL;
        ndp_opm_tracked(reg, control);
    }
    {
        uint8_t control = ndp_s.opm_valid[0x27]
            ? (uint8_t)(ndp_s.opm_shadow[0x27] & 0x3fU)
            : (uint8_t)NDP_TONE_CONTROL;
        ndp_opm_tracked(0x27, control);
    }
#endif
    for (ch = 0; ch < NDP_TRACKS; ++ch) {
        ndp_s.track[ch].enabled = 0;
        ndp_s.track[ch].note = 0;
        ndp_key(ch, 0);
        ndp_opm((uint8_t)(0x70 + ch), 127);
    }
    ndp_s.rhythm_active = 0;
    ndp_s.rhythm_end_pending = 0;
    ndp_noise_key(0);
    ndp_opm(0x7f, 127);
#ifdef NDP_PSG_STOP
    NDP_PSG_STOP();
#endif
}

static int ndp_update_fade_state(void)
{
    if (ndp_s.fade_direction == 0 || ndp_s.fade_interval == 0 ||
        ++ndp_s.fade_count < ndp_s.fade_interval) return 1;
    ndp_s.fade_count = 0;
    if (ndp_s.fade_direction > 0) {
        if (ndp_s.master_volume < 15) ++ndp_s.master_volume;
#if NDP_PROFILE == NDP_PROFILE_STREAM
        ndp_stream_apply_master_volume();
#endif
        if (ndp_s.master_volume >= 15) {
            ndp_stop();
            return 0;
        }
    } else {
        if (ndp_s.master_volume != 0) --ndp_s.master_volume;
#if NDP_PROFILE == NDP_PROFILE_STREAM
        ndp_stream_apply_master_volume();
#endif
        if (ndp_s.master_volume == 0) {
            ndp_s.fade_interval = 0;
            ndp_s.fade_direction = 0;
        }
    }
    return 1;
}

static void ndp_update_core(void)
{
    unsigned int ch;
    unsigned int noise_volume = 0;
    unsigned int noise_frequency = 0;
    unsigned int noise_tl;
    unsigned int track_noise_volume, track_noise_frequency;
    ndp_s.frame_noise = 0;
    if (ndp_s.rhythm_end_pending) {
        ndp_s.rhythm_active = 0;
        ndp_s.rhythm_end_pending = 0;
    }
    if (ndp_s.rhythm_duration != 0) --ndp_s.rhythm_duration;
    ndp_rhythm_event();
    ndp_rhythm_envelope();
    for (ch = 0; ch < NDP_TRACKS; ++ch) {
        NdpTrack *t = &ndp_s.track[ch];
        if (t->duration != 0) --t->duration;
        if (t->gate != 0) --t->gate;
        if (t->duration == 0 && t->enabled) ndp_track_event(t, ch);
        track_noise_volume = 0;
        track_noise_frequency = 0;
        ndp_render_track(t, ch, &track_noise_volume, &track_noise_frequency);
        if (!(ndp_s.rhythm_active && ch == ndp_s.rhythm_channel))
            ndp_noise_merge(&noise_volume, &noise_frequency,
                            track_noise_volume, track_noise_frequency);
        t->legato = t->next_legato;
        if (t->mute_frames > 1 && t->mute_frames < 255)
            --t->mute_frames;
    }
    ndp_update_se(&noise_volume, &noise_frequency);
    ndp_render_rhythm(&noise_volume, &noise_frequency);
    if (noise_volume != 0) {
        ndp_s.frame_noise = (uint8_t)(noise_frequency & 31U);
        noise_tl = ndp_noise_tl[noise_volume > 15 ? 15 : noise_volume]
                   + NDP_NOISE_TL_BIAS;
        if (noise_tl > 127) noise_tl = 127;
        ndp_opm(0x0f, (uint8_t)(0x80 |
                ndp_noise_period_register(noise_frequency)));
        ndp_opm(0x7f, (uint8_t)noise_tl);
        ndp_noise_key(1);
    } else {
        ndp_opm(0x7f, 127);
        ndp_noise_key(0);
    }
    if ((ndp_s.ended_mask & 7) == 7 && !ndp_s.rhythm_enabled && !ndp_s.rhythm_active)
        ndp_s.playing = 0;
}

static void ndp_emit_psg_frame(void)
{
#ifdef NDP_PSG_FRAME
    NDP_PSG_FRAME(ndp_psg_period_state, ndp_psg_volume_state,
                  ndp_psg_mix_state, ndp_s.frame_noise);
#endif
#ifdef NDP_PSG_RAW_FRAME
    NDP_PSG_RAW_FRAME(ndp_psg_period_state, ndp_psg_volume_state,
                      ndp_psg_mix_state, ndp_s.frame_noise,
                      ndp_psg_hard_state, ndp_s.hard_period,
                      ndp_s.hard_last_shape);
#endif
}

void ndp_update(void)
{
#if NDP_PROFILE == NDP_PROFILE_STREAM
    unsigned int noise_volume = 0;
    unsigned int noise_frequency = 0;
    if (!ndp_stream_s.active && !ndp_s.se_playing) return;
    if (!ndp_update_fade_state()) return;
    if (ndp_stream_s.active) ndp_stream_update_core();
    if (ndp_s.se_playing)
        ndp_update_se(&noise_volume, &noise_frequency);
    ndp_stream_render_se_noise(noise_volume, noise_frequency);
    ndp_emit_psg_frame();
    return;
#else
    unsigned int fast_ticks = 0;
    if (!ndp_s.playing && !ndp_s.se_playing) return;
    if (ndp_s.slow_mask != 0) {
        ndp_s.slow_count = (uint8_t)((ndp_s.slow_count + 1) & 7);
        if ((ndp_s.slow_mask & (1U << ndp_s.slow_count)) != 0) return;
    }
    if (!ndp_update_fade_state()) return;
    do {
        ndp_update_core();
        if (!ndp_s.fast_forward) break;
        ++fast_ticks;
    } while (fast_ticks < NDP_FAST_FORWARD_MAX_TICKS &&
             (ndp_s.playing || ndp_s.se_playing));
    if (fast_ticks >= NDP_FAST_FORWARD_MAX_TICKS ||
        (!ndp_s.playing && !ndp_s.se_playing))
        ndp_s.fast_forward = 0;
    ndp_emit_psg_frame();
#endif
}

void ndp_update_ticks(unsigned int ticks)
{
    while (ticks-- != 0) ndp_update();
}

void ndp_set_master_volume(unsigned int attenuation)
{
    ndp_s.master_volume = (uint8_t)(attenuation > 15 ? 15 : attenuation);
    ndp_s.fade_interval = 0;
    ndp_s.fade_count = 0;
    ndp_s.fade_direction = 0;
#if NDP_PROFILE == NDP_PROFILE_STREAM
    ndp_stream_apply_master_volume();
#endif
}

void ndp_fade_out(unsigned int frames_per_step)
{
    ndp_s.fade_interval = (uint8_t)(frames_per_step == 0 ? 1 :
                                    frames_per_step > 255 ? 255 : frames_per_step);
    ndp_s.fade_count = (uint8_t)(ndp_s.fade_interval - 1U);
    ndp_s.fade_direction = 1;
}

void ndp_fade_in(unsigned int frames_per_step)
{
    ndp_s.master_volume = 15;
    ndp_s.fade_interval = (uint8_t)(frames_per_step == 0 ? 1 :
                                    frames_per_step > 255 ? 255 : frames_per_step);
    ndp_s.fade_count = (uint8_t)(ndp_s.fade_interval - 1U);
    ndp_s.fade_direction = -1;
#if NDP_PROFILE == NDP_PROFILE_STREAM
    ndp_stream_apply_master_volume();
#endif
}

void ndp_mute_channel(unsigned int channel, unsigned int frames)
{
    if (channel >= NDP_TRACKS) return;
    if (frames <= 1) ndp_s.track[channel].mute_frames = 0;
    else ndp_s.track[channel].mute_frames =
        (uint8_t)(frames > 255 ? 255 : frames);
}

unsigned int ndp_master_volume(void)
{
    return ndp_s.master_volume;
}

unsigned int ndp_channel_mute_frames(unsigned int channel)
{
    return channel < NDP_TRACKS ? ndp_s.track[channel].mute_frames : 0;
}

int ndp_se_open(NdpSeBank *bank, const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    unsigned int count;
    unsigned int i;
    uint32_t previous = 0;
    if (bank == 0 || bytes == 0 || size < 3 || size > 0xffffffffUL)
        return 0;
    count = bytes[0];
    if (count == 0 || count > 99 || size < 1U + count * 2U) return 0;
    for (i = 0; i < count; ++i) {
        uint32_t offset = ndp_le16(bytes + 1U + i * 2U);
        uint32_t end = i + 1U < count
            ? ndp_le16(bytes + 1U + (i + 1U) * 2U) : (uint32_t)size;
        unsigned int channel;
        if (offset < 1U + count * 2U || offset < previous ||
            end < offset + 8U || end > size) return 0;
        for (channel = 0; channel < NDP_TRACKS; ++channel) {
            uint16_t track = ndp_le16(bytes + offset + channel * 2U);
            if (track != 0 && (track < 8 || offset + track >= end)) return 0;
        }
        if (bytes[offset + 7U] > 7) return 0;
        previous = offset;
    }
    bank->data = bytes;
    bank->size = size;
    bank->count = count;
    return 1;
}

int ndp_se_play(const NdpSeBank *bank, unsigned int effect)
{
    uint32_t offset;
    uint32_t end;
    if (bank == 0 || bank->data == 0 || effect >= bank->count) return 0;
    offset = ndp_le16(bank->data + 1U + effect * 2U);
    end = effect + 1U < bank->count
        ? ndp_le16(bank->data + 1U + (effect + 1U) * 2U)
        : (uint32_t)bank->size;
    return ndp_se_begin(bank->data, (uint32_t)bank->size, offset, end);
}

void ndp_se_stop(void)
{
    unsigned int channel;
    if (!ndp_s.se_playing) return;
    for (channel = 0; channel < NDP_TRACKS; ++channel) {
        ndp_s.se_track[channel].enabled = 0;
        ndp_s.se_track[channel].note = 0;
        ndp_track_key(&ndp_s.se_track[channel], channel, 0);
    }
#if NDP_PROFILE == NDP_PROFILE_STREAM
    ndp_stream_se_noise_key(0);
#endif
    ndp_s.se_playing = 0;
}

int ndp_se_is_playing(void)
{
    return ndp_s.se_playing != 0;
}

int ndp_is_playing(void)
{
    return ndp_s.playing != 0;
}

unsigned int ndp_loop_count(void)
{
    return ndp_s.loop_count;
}

unsigned int ndp_end_tracks(void)
{
    return ndp_s.ended_mask;
}

#endif /* NDP_IMPLEMENTATION */
#endif /* NDP_H_INCLUDED */
