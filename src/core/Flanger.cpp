#include "Flanger.hpp"
#include <cmath>
#include <cstring>

void Flanger::recalculate_params()
{
    auto st = this; // "st" means "State" (current Flanger DSP state)

    float sr = (float)st->sample_rate;
    float half_sr = sr * CONST_0_5;
    
    if (st->flag2 == 0) return;
    st->flag2 = 0;
    
    /* Clamp sample rate to reasonable range */
    if (st->sample_rate >= CONST_44100)
        half_sr = CONST_44100 * CONST_0_5;

    /* Calculate max delay in samples */
    st->max_delay = st->params[pParamDelay] * 0.001f * sr;
    
    // Reserve sufficient safety margin for 2x oversampled bidirectional LFO modulation to prevent buffer overrun.
    if (st->max_delay > (float)(static_cast<float>(DELAY_LINE_SIZE) / 4 - 2))
        st->max_delay = (float)(static_cast<float>(DELAY_LINE_SIZE) / 4 - 2);
    st->delay_mask = DELAY_LINE_SIZE - 1;
    
    /* Calculate LFO step */
    lfo.calculate_lfo_step(st->params[pParamRate], sr, OVERSAMPLE);

    /* Set LFO phase offset */
    lfo.set_phase(0.0f);                            // Left channel has no offset
    lfo2.set_phase(st->params[pParamStereoPhase]);  // Right channel has offset configured by user

    /* Configure anti-aliasing low-pass filter cutoff at ~18 kHz, correcting prior
       implementations that erroneously tied the cutoff to the LFO frequency. */
    float fc = INITIAL_LOW_PASS_FILTER_CUTOFF;
    if (fc > half_sr)
        fc = half_sr;

    double omega = M_PI * fc / sr;
    double sin_omega = sin(omega);
    double cos_omega = cos(omega);
    double alpha = sin_omega / CONST_2_0;  /* Q = 0.5 */
    
    /* Compute standard RBJ biquad coefficients for the input filter. */
    float a0 = 1.0f + alpha;
    st->coef_a1 = (1.0f - cos_omega) * CONST_0_5 / a0;  /* b0 */
    st->coef_a2 = (1.0f - cos_omega) / a0;              /* b1 */
    st->coef_b0 = (1.0f - cos_omega) * CONST_0_5 / a0;  /* b2 */
    st->coef_b1 = (-2.0f * cos_omega) / a0;             /* a1 */
    st->coef_b2 = (1.0f - alpha) / a0;                  /* a2 */
    
    /* Compute feedback path damping filter coefficients. This low-pass filter
       at ~4.5 kHz (¼ of input cutoff) prevents feedback instability at high gain. */
    float fb_fc = fc * CONST_0_25;
    if (fb_fc > half_sr)
        fb_fc = half_sr;
    omega = (float)(M_PI * fb_fc / sr);
    sin_omega = (float)sin(omega);
    cos_omega = (float)cos(omega);
    alpha = sin_omega / CONST_2_0; // Q = 0.5
    
    a0 = 1.0f + alpha;
    
    // Carefully apply standard RBJ low-pass coefficients to ensure unity DC gain (b0+b1+b2 = a0+a1+a2).
    st->coef_c0 = (1.0f - cos_omega) * CONST_0_5 / a0;  // b0: numerator zero coefficient
    st->coef_c1 = (1.0f - cos_omega) / a0;              // b1 = 2*b0 (corrected; was incorrectly 0.0)
    st->coef_c2 = (1.0f - cos_omega) * CONST_0_5 / a0;  // b2 = b0    (corrected; was incorrectly -b0)
    
    st->coef_d0 = (-2.0f * cos_omega) / a0;             // a1: first denominator coefficient
    st->coef_d1 = (1.0f - alpha) / a0;                  // a2: second denominator coefficient
    
    /* Depth parameter controls LFO modulation range only, not filter coefficients.
       These erroneous lines have been removed. */
    /* st->coef_a1 *= depth; */
    /* st->coef_a2 *= depth; */
    
    /* Calculate wet/dry mix */
    st->wet_gain = st->params[pParamMix];
    st->dry_gain = CONST_1_0 - st->params[pParamMix];
    
    /* Apply level gain */
    float level_lin = (float)pow(2.0, st->params[pParamLevel] / 6.0);
    st->wet_gain *= level_lin;
    st->dry_gain *= level_lin;
}

void Flanger::process_sample(float in_l, float in_r, float* out_l, float* out_r)
{
    auto st = this;

    float wet_l_accum = 0.0f;
    float wet_r_accum = 0.0f;
    
    /* Generate noise sample for dithering and small random modulation. */
    st->dither = noise_generator.nextNoise();
    
    /* Begin 2× oversampled processing loop. */
    for (uint8_t i = 0; i < OVERSAMPLE; i++)
    {
        float x_l, x_r;
        float f2_out_l, f2_out_r;
        unsigned short wp;
        
        /* 1. Advance the circular write pointer. */
        st->write_pos = (st->write_pos + 1) & st->delay_mask;
        wp = st->write_pos;
        
        /* 2. Sum input, feedback path, and dither. */    
        x_l = in_l + st->feedback_l + st->dither;
        x_r = in_r + st->feedback_r + st->dither;

        /* 3. Apply standard, stable Direct Form II stereo biquad low-pass filter. */   
        // Left channel using f1_z1, f1_z2 state variables.
        const float w_l = x_l - st->coef_b1 * st->f1_z1 - st->coef_b2 * st->f1_z2;
        f2_out_l = st->coef_a1 * w_l + st->coef_a2 * st->f1_z1 + st->coef_b0 * st->f1_z2;
        st->f1_z2 = st->f1_z1;
        st->f1_z1 = w_l;
        
        // Right channel using f2_z1, f2_z2 state variables.
        const float w_r = x_r - st->coef_b1 * st->f2_z1 - st->coef_b2 * st->f2_z2;
        f2_out_r = st->coef_a1 * w_r + st->coef_a2 * st->f2_z1 + st->coef_b0 * st->f2_z2;
        st->f2_z2 = st->f2_z1;
        st->f2_z1 = w_r;
        
        /* 4. Write filtered samples to the 2× delay lines. */
        st->delay_line_l[wp] = f2_out_l;
        st->delay_line_r[wp] = f2_out_r;

        /* 5. Advance LFO phase counter during oversampled processing. */
        const float lfo_value = lfo.generate_lfo();         // Left channel & non-spreaded stereo
        const float lfo_value_ch2 = lfo2.generate_lfo();    // Right channel ("ch2" means "2nd channel")

        /* 6. Apply depth parameter to scale LFO modulation around a center point (1.0 + depth×LFO). */
        float lfo_mod = 1.0f + st->params[pParamDepth] * lfo_value;
        if (lfo_mod < 0.05f) // Clamp minimum to prevent negative delay.
            lfo_mod = 0.05f;

        float lfo_mod_ch2 = 1.0f + st->params[pParamDepth] * lfo_value_ch2;
        if (lfo_mod_ch2 < 0.05f) // Clamp minimum to prevent negative delay.
            lfo_mod_ch2 = 0.05f;
        
        /* Compute delay in 2× oversampled domain. */
        const float delay_samples_2x = lfo_mod * st->max_delay * (float)OVERSAMPLE;
        const float delay_samples_2x_ch2 = lfo_mod_ch2 * st->max_delay * (float)OVERSAMPLE;
        
        /* Split delay into integer and fractional parts for linear interpolation. */
        const int delay_int = (int)delay_samples_2x;
        float frac = delay_samples_2x - (float)delay_int;

        const int delay_int_ch2 = (int)delay_samples_2x_ch2;
        float frac_ch2 = delay_samples_2x_ch2 - (float)delay_int_ch2;       
        
        /* 7. Read from delay line with linear interpolation. rp1 points to older sample (rp-1),
           ensuring correct interpolation direction to eliminate clicking artifacts. */
        const uint16_t rp = (st->write_pos - delay_int) & st->delay_mask;
        const uint16_t rp1 = (rp - 1) & st->delay_mask;

        const uint16_t rp_ch2 = (st->write_pos - delay_int_ch2) & st->delay_mask;
        const uint16_t rp1_ch2 = (rp_ch2 - 1) & st->delay_mask;

        const bool is_spread_mode = (st->params[pParamSpread] >= 0.5f);

        const float wet_l = st->delay_line_l[rp] * (CONST_1_0 - frac) + st->delay_line_l[rp1] * frac;
        const float wet_r = st->delay_line_r[is_spread_mode ? rp_ch2 : rp] *
            (CONST_1_0 - (is_spread_mode ? frac_ch2 : frac)) +
            st->delay_line_r[is_spread_mode ? rp1_ch2 : rp1] *
            (is_spread_mode ? frac_ch2 : frac);

        // Wet signal without time offset (spreading). See Step 8 below.
        const float wet_r_nonspreaded = st->delay_line_r[rp] * (CONST_1_0 - frac) + st->delay_line_r[rp1] * frac;

        /* 8. Compute feedback path with tanh saturation for soft clipping. */
        // Apply independent stereo Direct Form II low-pass filters to the feedback path,
        // preventing instability and howling at high feedback gains.
        //
        // NOTICE: Use non-spreaded (without time offset) wet signal for feedback loop rather than spreaded one,
        //         otherwise you may hear feedback resonance on only one channel.
        const float fb_in_l = wet_l;
        const float fb_in_r = wet_r_nonspreaded;

        // Left channel: Direct Form II state update.
        const float w_fb_l = fb_in_l - st->coef_d0 * st->temp_l - st->coef_d1 * st->temp2_l;
        float filtered_fb_l = st->coef_c0 * w_fb_l + st->coef_c1 * st->temp_l + st->coef_c2 * st->temp2_l;
        st->temp2_l = st->temp_l;
        st->temp_l = w_fb_l;
        
        // Right channel: Direct Form II state update.
        const float w_fb_r = fb_in_r - st->coef_d0 * st->temp_r - st->coef_d1 * st->temp2_r;
        float filtered_fb_r = st->coef_c0 * w_fb_r + st->coef_c1 * st->temp_r + st->coef_c2 * st->temp2_r;
        st->temp2_r = st->temp_r;
        st->temp_r = w_fb_r;
        
        // Retrieve feedback amount as a normalized value in [-1.0, 1.0] from the DPF framework.
        const float fb_amount = st->params[pParamFeedback];   
        
        // Apply tanh saturation with gain staging (0.5× internal, 1.5× external)
        // for smooth, tube-like soft clipping behavior.
        // NOTE: Too large external gain (e.g. 2.0f) produces too noisy feedback sound.
        //       1.5f is a gentle gain value which resembles the original behavior.
        float fb_sat_l = tanhf(filtered_fb_l * fb_amount * 0.5f) * 1.5f;
        float fb_sat_r = tanhf(filtered_fb_r * fb_amount * 0.5f) * 1.5f;
        
        st->feedback_l = fb_sat_l;
        st->feedback_r = fb_sat_r;
        
        /* 9. Accumulate wet output for averaging after the loop. */
        wet_l_accum += wet_l;
        wet_r_accum += wet_r;
    }
    /* End 2× oversampled loop. */
    
    /* 10. Downsample by averaging the accumulated oversampled output. */
    float final_wet_l = wet_l_accum / (float)OVERSAMPLE;
    float final_wet_r = wet_r_accum / (float)OVERSAMPLE;

    /* 11. Mix wet and dry signals according to the mix parameter. */
    *out_l = final_wet_l * st->wet_gain + in_l * st->dry_gain;
    *out_r = final_wet_r * st->wet_gain + in_r * st->dry_gain;
}

void Flanger::reset_dsp()
{
    auto st = this;

    /* When suspend DSP: clear all delay lines and state variables. */

    memset(st->delay_line_l, 0, sizeof(st->delay_line_l));
    memset(st->delay_line_r, 0, sizeof(st->delay_line_r));

    st->f1_z1 = st->f1_z2 = 0.0f;
    st->f2_z1 = st->f2_z2 = 0.0f;
    st->f3_z1 = 0.0f;
    st->feedback_l = st->feedback_r = 0.0f;
    st->temp_l = st->temp_r = 0.0f;
    st->temp2_l = st->temp2_r = 0.0f;
    st->write_pos = 0;

    lfo.reset();    
}
