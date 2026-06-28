#ifndef FLANGER_HPP_INCLUDED
#define FLANGER_HPP_INCLUDED

#include "../Defines.hpp"
#include "../Structures.hpp"
#include "LFO.hpp"
#include "NoiseGenerator.hpp"

/**
 * @brief Flanger effect DSP instance & processing state.
 *
 * The Flanger class implements a stereo flanger effect with oversampled
 * delay modulation, feedback, and internal low-pass filtering.
 */
class Flanger
{
private:
    /** @brief Current parameter values for the effect. */
    float params[NUM_PARAMS];
    
    /** @brief Active program or preset index. */
    int current_program;
    
    /** @brief Current sample rate in Hz. */
    double sample_rate;
    
    /** @brief Internal state flags. */
    char flag1;
    char flag2;  /**< should re-calculate all coefficients */
    
    /** @brief Current write position inside the delay buffer. */
    unsigned short write_pos;
    /** @brief Wrap mask used for delay buffer indexing. */
    unsigned short delay_mask;
    
    /** @brief Stereo delay buffers used by the flanger. */
    float delay_line_l[DELAY_LINE_SIZE] { 0.0f };
    float delay_line_r[DELAY_LINE_SIZE] { 0.0f };
    
    /** @brief Precomputed filter coefficients for input and feedback filters. */
    float coef_a1;
    float coef_a2;
    float coef_b0;
    float coef_b1;
    float coef_b2;
    float coef_c0;
    float coef_c1;
    float coef_c2;
    float coef_d0;
    float coef_d1;
    
    /** @brief State memory for the stereo input low-pass filter. */
    float f1_z1;
    float f1_z2;
    float f2_z1;
    float f2_z2;
    float f3_z1;
    
    /** @brief Output mix gains for wet and dry signals. */
    float wet_gain;
    float dry_gain;
    
    /** @brief Temporary working values used during processing. */
    float temp_l;
    float temp_r;
    float temp2_l;
    float temp2_r;
    float dither;
    float delay_samples;
    float max_delay;
    float feedback_l;
    float feedback_r;

    /** @brief Internal noise generator used for dither. */
    NoiseGenerator noise_generator;

    /** @brief LFO generator. */
    LFO lfo;    //< For left channel & non-spreaded stereo sound
    LFO lfo2;   //< For right channel

    /**
     * @brief Linear interpolation helper.
     * @param a Value at t = 0.
     * @param b Value at t = 1.
     * @param t Interpolation factor in [0, 1].
     * @return Interpolated result.
     */
    inline float lerp(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

public:
    /**
     * @brief Set the current processing sample rate.
     * @param new_sample_rate Sample rate in Hz.
     */
    void set_sample_rate(float new_sample_rate)
    {
        this->sample_rate = new_sample_rate;
        request_recalculate_params();  // Mark that sample rate has changed, should re-calculate coefficients
    }

    /**
     * @brief Set a parameter value.
     * @param index Parameter index defined by the Parameters enum.
     * @param value New parameter value.
     */
    void set_parameter(Parameters index, float value)
    {
        this->params[index] = value;
        request_recalculate_params();   // Mark that parameter has changed, should re-calculate coefficients
    }

    /**
     * @brief Get a parameter value.
     * @param index Parameter index defined by the Parameters enum.
     * @return Current parameter value.
     */
    float get_parameter(Parameters index) const
    {
        return this->params[index];
    }

    /**
     * @brief Recompute internal coefficients and derived values.
     *
     * This should be called after parameter or sample-rate changes.
     */
    void recalculate_params();

    /**
     * @brief Request a parameter recalculation on the next processing pass.
     */
    void request_recalculate_params() { flag2 = 1; }

    /**
     * @brief Process a single stereo sample pair through the flanger.
     * @param in_l Left input sample.
     * @param in_r Right input sample.
     * @param out_l Pointer to left output sample.
     * @param out_r Pointer to right output sample.
     */
    void process_sample(float in_l, float in_r, float* out_l, float* out_r);

    /**
     * @brief Reset all DSP state and clear delay buffers.
     */
    void reset_dsp();
};

#endif
