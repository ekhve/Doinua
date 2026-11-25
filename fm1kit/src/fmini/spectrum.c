/* SPDX-License-Identifier: GPL-3.0-only */
/* Included by engines.c. Fill one window on the audio side; calculate the spectrum on the app side. */
enum { SPECTRUM_SIZE = 256, SPECTRUM_DECIMATION = 4 };
static int spectrum_track = -1;
static unsigned spectrum_count;
static int16_t spectrum_samples[SPECTRUM_SIZE];
static int16_t spectrum_real[SPECTRUM_SIZE], spectrum_imag[SPECTRUM_SIZE];
static const uint8_t spectrum_edges[KIT_SPECTRUM_BANDS + 1] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 17, 20, 24, 29, 34, 41, 49, 59, 70, 83, 99, 118, 129
};

void kit_track_spectrum(int pt)
{
    uint32_t lock;
    int target = okp(pt) && fmx[pt] ? pt : -1;
    if (target == spectrum_track) return;
    lock = kit__lock();
    spectrum_track = target;
    spectrum_count = 0;
    kit__unlock(lock);
}

static void spectrum_capture(const int32_t *out)
{
    unsigned i;
    /* Average four samples before downsampling. CTL divides a window exactly. */
    for (i = 0; i < CTL; i += SPECTRUM_DECIMATION) {
        int32_t sum = (out[i] >> 2) + (out[i + 1] >> 2) + (out[i + 2] >> 2) + (out[i + 3] >> 2);
        spectrum_samples[spectrum_count++] = clamp(sum, -32767, 32767);
    }
}

static void spectrum_fft(void)
{
    unsigned i, step;
    for (step = 2; step <= SPECTRUM_SIZE; step <<= 1) {
        unsigned half = step / 2, phase_step = 1024 / step;
        for (i = 0; i < SPECTRUM_SIZE; i += step) {
            unsigned j;
            for (j = 0; j < half; j++) {
                unsigned a = i + j, b = a + half, phase = j * phase_step;
                int32_t cosine = KIT_SINE[phase + 256], sine = -KIT_SINE[phase];
                int32_t re = (cosine * spectrum_real[b] >> 15) - (sine * spectrum_imag[b] >> 15);
                int32_t im = (cosine * spectrum_imag[b] >> 15) + (sine * spectrum_real[b] >> 15);
                int32_t ar = spectrum_real[a], ai = spectrum_imag[a];
                spectrum_real[a] = (ar + re) >> 1; spectrum_real[b] = (ar - re) >> 1;
                spectrum_imag[a] = (ai + im) >> 1; spectrum_imag[b] = (ai - im) >> 1;
            }
        }
    }
}

int kit_track_spectrum_read(uint8_t bands[KIT_SPECTRUM_BANDS])
{
    uint32_t lock;
    unsigned i, reversed = 0;
    if (!bands) return 0;
    lock = kit__lock();
    if (spectrum_count < SPECTRUM_SIZE) {
        kit__unlock(lock);
        return 0;
    }
    memcpy(spectrum_real, spectrum_samples, sizeof spectrum_real);
    spectrum_count = 0;
    kit__unlock(lock);
    for (i = 0; i < SPECTRUM_SIZE; i++) {
        int32_t weight = (32767 - KIT_SINE[(256 + i * 4) & 1023]) >> 1;
        spectrum_real[i] = (spectrum_real[i] * weight) >> 15;
    }
    for (i = 0; i < SPECTRUM_SIZE; i++) {
        unsigned bit;
        if (i < reversed) {
            int16_t sample = spectrum_real[i];
            spectrum_real[i] = spectrum_real[reversed];
            spectrum_real[reversed] = sample;
        }
        for (bit = SPECTRUM_SIZE >> 1; bit && (reversed & bit); bit >>= 1) reversed ^= bit;
        reversed |= bit;
    }
    memset(spectrum_imag, 0, sizeof spectrum_imag);
    spectrum_fft();
    for (i = 0; i < KIT_SPECTRUM_BANDS; i++) {
        unsigned j, peak = 0, level = 0;
        for (j = spectrum_edges[i]; j < spectrum_edges[i + 1]; j++) {
            int re = spectrum_real[j], im = spectrum_imag[j];
            unsigned r = re < 0 ? -re : re, m = im < 0 ? -im : im;
            unsigned magnitude = r > m ? r + m / 2 : m + r / 2;
            if (magnitude > peak) peak = magnitude;
        }
        if (peak > 4) {
            while (peak >= 16) { peak >>= 1; level += 8; }
            level += peak - 4;
        }
        bands[i] = (uint8_t)(level > 100 ? 100 : level);
    }
    return 1;
}

int kit_track_spectrum_position(int pt, int cutoff)
{
    unsigned i, bin_q8;
    uint32_t inc;
    if (!okp(pt) || !fmx[pt]) return 0;
    inc = pitch_inc((uint32_t)clamp(FMINI_CUT[clamp(cutoff, 0, 127)], 0, 2047));
    /* With 256 points and 4:1 decimation, frequency in bins is phase increment * 1024 / 2^32. */
    bin_q8 = inc >> 14;
    for (i = 0; i < KIT_SPECTRUM_BANDS; i++) {
        unsigned lower = spectrum_edges[i] * 256u, upper = spectrum_edges[i + 1] * 256u;
        if (bin_q8 < lower) return 0;
        if (bin_q8 < upper)
            return (int)((i * 100u + (bin_q8 - lower) * 100u / (upper - lower)) / KIT_SPECTRUM_BANDS);
    }
    return 100;
}
