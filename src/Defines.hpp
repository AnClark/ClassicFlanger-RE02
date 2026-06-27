#ifndef DEFINES_HPP_INCLUDED
#define DEFINES_HPP_INCLUDED

/** @brief Size of the circular delay line buffer. */
static constexpr int DELAY_LINE_SIZE = 65536;

/** @brief Initial low-pass filter cutoff for flanger effect. */
static constexpr float INITIAL_LOW_PASS_FILTER_CUTOFF = 18000.0f;

/** @brief Oversampling factor used by the flanger processing. */
static constexpr int OVERSAMPLE = 2;

/** @brief Constant values derived from reverse analysis of the original plugin. */
static constexpr float CONST_0_5          = 0.5f;
static constexpr float CONST_1_0          = 1.0f;
static constexpr float CONST_2_0          = 2.0f;
static constexpr float CONST_LN2          = 0.69314718056f;
static constexpr float CONST_TWO_THIRD    = 0.66666666667f;
static constexpr float CONST_FIVE_THIRD   = 1.66666666667f;
static constexpr float CONST_MINUS3       = -3.0f;
static constexpr float CONST_384          = 384.0f;
static constexpr float CONST_32768        = 32768.0f;
static constexpr float CONST_2147483648   = 2147483648.0f;
static constexpr float CONST_44100        = 44100.0f;
static constexpr float CONST_7900         = 7900.0f;
static constexpr float CONST_ONE_THIRD    = 0.33333333333f;
static constexpr float CONST_0_25         = 0.25f;
static constexpr float CONST_8_0          = 8.0f;

#endif
