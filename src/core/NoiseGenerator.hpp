#ifndef NOISE_GENERATOR_HPP_INCLUDED
#define NOISE_GENERATOR_HPP_INCLUDED

#include <cstdint>

/** @brief Random noise amplitude used for delay modulation and dithering. */
static constexpr float kModAmp = 6e-8f;   // DAT_00485f5c – chorus/randomisation amplitude

/**
 * @brief Noise generator used to emulate the original Delphi RNG behavior.
 *
 * This generator is used by the Classic Flanger effect for small randomized
 * modulation and dither. The implementation mirrors the XOR-shift sequence
 * observed in the original plugin's assembly.
 */
class NoiseGenerator
{
private:
    /** @brief Internal 32-bit RNG state. */
    uint32_t fRngState { 0x12345678u };

public:
    /**
     * @brief Generate the next noise sample.
     *
     * @return Noise value mapped to approximately (-0.5, +0.5), scaled by
     *         the modulation amplitude constant.
     */
    float nextNoise()
    {
        fRngState ^= fRngState << 13;
        fRngState ^= fRngState >> 17;
        fRngState ^= fRngState << 5;

        // Map full uint32 range to (−0.5, +0.5) then scale by kModAmp.
        return (float)(int32_t)fRngState * (1.0f / 4294967296.0f) * kModAmp;
    }
};

#endif
