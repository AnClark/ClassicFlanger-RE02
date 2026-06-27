#ifndef CLASSIC_FLANGER_PLUGIN_H_INCLUDED
#define CLASSIC_FLANGER_PLUGIN_H_INCLUDED

#include "DistrhoPlugin.hpp"
#include "Structures.hpp"
#include "core/Flanger.hpp"

/**
 * @file ClassicFlangerPlugin.h
 * @brief Main plugin class for the Classic Flanger audio effect.
 *
 * Implements a stereo flanger using a modulated delay line per channel,
 * low-frequency oscillators (LFOs), feedback, mix and polarity controls.
 */

/**
 * @class ClassicFlangerPlugin
 * @brief DISTRHO audio plugin implementation of a classic flanger effect.
 *
 * The plugin processes stereo audio by applying a variable delay to each
 * channel. The delay time is modulated by an LFO, and feedback, dry/wet mix,
 * stereo phase offset and waveform polarity can be adjusted through parameters.
 */
class ClassicFlangerPlugin : public DISTRHO::Plugin
{
public:
    /**
     * @brief Constructs the plugin and initializes its internal state.
     */
    ClassicFlangerPlugin();

protected:
    // ── Plugin metadata ────────────────────────────────────────────────────

    /** @brief Returns the plugin's short label/name. */
    const char* getLabel()   const override { return DISTRHO_PLUGIN_NAME; }

    /** @brief Returns the plugin vendor/maker string. */
    const char* getMaker()   const override { return DISTRHO_PLUGIN_BRAND; }

    /** @brief Returns the plugin license string (GPLv3+). */
    const char* getLicense() const override { return "GPLv3+"; }

    /** @brief Returns the plugin version as a packed integer (1.0.0). */
    uint32_t    getVersion() const override { return d_version(1, 0, 0); }

    // ── Parameters ────────────────────────────────────────────────────────

    /**
     * @brief Initializes a plugin parameter descriptor.
     * @param index Zero-based parameter index.
     * @param param Reference to the DISTRHO parameter structure to fill.
     */
    void initParameter(uint32_t index, Parameter& param) override;

    /**
     * @brief Retrieves the current normalized value of a parameter.
     * @param index Zero-based parameter index.
     * @return Current parameter value in the range [0.0, 1.0] or the parameter's native range.
     */
    float getParameterValue(uint32_t index) const override;

    /**
     * @brief Sets the value of a parameter from the host.
     * @param index Zero-based parameter index.
     * @param value New parameter value.
     */
    void setParameterValue(uint32_t index, float value) override;

    // ── State ──────────────────────────────────────────────────────────────

    /** @brief Initializes a plugin state key. */
    void initState(uint32_t index, State& state) override;

    /** @brief Called when the host sets a state value. */
    void setState(const char* key, const char* value) override;

    // ── Audio processing ──────────────────────────────────────────────────

    /** @brief Called when the plugin is activated (prepares DSP state). */
    void activate() override;

    /** @brief Called when the plugin is deactivated (cleans up DSP state). */
    void deactivate() override;

    /**
     * @brief Called when the host sample rate changes.
     * @param newSampleRate New sample rate in Hz.
     */
    void sampleRateChanged(double newSampleRate) override;

    /**
     * @brief Processes one audio buffer.
     * @param inputs  Array of input channel pointers.
     * @param outputs Array of output channel pointers.
     * @param frames  Number of sample frames to process.
     */
    void run(const float** inputs, float** outputs, uint32_t frames) override;

private:
    /* DSP objects */
    Flanger fFlanger;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassicFlangerPlugin)
};

#endif // CLASSIC_FLANGER_PLUGIN_H_INCLUDED
