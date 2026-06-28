#ifndef LFO_HPP_INCLUDED
#define LFO_HPP_INCLUDED

#include "Defines.hpp"

#include <cmath>

class LFO
{
private:
    /** @brief LFO phase accumulator (unused in current implementation). */
    unsigned short lfo_phase { 0 };
    /** @brief Integer phase counter for the LFO. */
    int lfo_counter { 0 };
    /** @brief Phase increment per sample for the LFO. */
    int lfo_step { 1 };
    /** @brief Target LFO phase value for internal control. */
    int lfo_target { 0 };
    /** @brief Current LFO output value in [-1.0, 1.0]. */
    float lfo_value { 0.0f };

public:
    float generate_lfo()
    {
        // Accumulate LFO step counter.
        this->lfo_counter += this->lfo_step;

        // The 32-bit signed counter naturally wraps around due to integer overflow,
        // providing a continuous phase signal. Map it to a floating-point phase angle
        // in the range [-π, π). Scaling factor ~1.462918e-9 derived from π / 2^31.
        float phase = (float)this->lfo_counter * (M_PI / 2147483648.0f);
        
        // Generate a pure sine wave oscillation. Output is strictly bounded to [-1.0, 1.0].
        return std::sinf(phase);
    }

    void calculate_lfo_step(float lfo_rate, float sample_rate, int oversample_count)
    {
        this->lfo_step = (int)(lfo_rate * CONST_2147483648 / (sample_rate * (float)oversample_count));
        if (this->lfo_step < 1)
            this->lfo_step = 1;
    }

    void reset()
    {
        this->lfo_counter = 0;
    }
};

#endif
