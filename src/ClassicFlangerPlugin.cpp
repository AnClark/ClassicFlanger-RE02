#include "ClassicFlangerPlugin.h"

// ─────────────────────────────────────────────────────────────────────────────
// Classic Flanger plugin class
// ─────────────────────────────────────────────────────────────────────────────

ClassicFlangerPlugin::ClassicFlangerPlugin()
    : DISTRHO::Plugin(NUM_PARAMS, 0, 3)  // 3 states: preset_name, preset_modified, preset_type
{
    // Default parameter values – physical units matching initParameter() ranges
    for (uint32_t i = 0; i < NUM_PARAMS; ++i)
        setParameterValue(Parameters(i), paramInfo[i].defaultVal);

    // Request setting initial sample rate for Flanger DSP
    sampleRateChanged(getSampleRate());
}

// ── Parameters ────────────────────────────────────────────────────────
void ClassicFlangerPlugin::initParameter(uint32_t index, Parameter& param)
{
    param.hints = kParameterIsAutomatable;
    param.name = paramInfo[index].name;
    param.symbol = String(paramInfo[index].name).toBasic().toLower();
    param.unit = paramInfo[index].label;
    param.ranges = DISTRHO::ParameterRanges(paramInfo[index].defaultVal, paramInfo[index].minVal, paramInfo[index].maxVal);

    if (paramInfo[index].isSwitch)
        param.hints |= kParameterIsBoolean;

    switch (index)
    {
        case pParamRate:
        case pParamDelay:
            param.hints |= kParameterIsLogarithmic;
            break;
    }
}

float ClassicFlangerPlugin::getParameterValue(uint32_t index) const
{
    DISTRHO_SAFE_ASSERT_RETURN(index < NUM_PARAMS, 0.0f);

    return fFlanger.get_parameter(Parameters(index));
}

void ClassicFlangerPlugin::setParameterValue(uint32_t index, float value)
{
    DISTRHO_SAFE_ASSERT_RETURN(index < NUM_PARAMS, );

    fFlanger.set_parameter(Parameters(index), std::clamp(value, paramInfo[index].minVal,
                                        paramInfo[index].maxVal));
}

// ── State ──────────────────────────────────────────────────────────────
void ClassicFlangerPlugin::initState(uint32_t index, State& state)
{
    state.hints = kStateIsHostWritable;

    switch (index)
    {
    case 0:
        state.key          = "preset_name";
        state.defaultValue = "";
        state.label        = "Current Preset Name";
        break;
    case 1:
        state.key          = "preset_modified";
        state.defaultValue = "false";
        state.label        = "Preset Modified";
        break;
    case 2:
        state.key          = "preset_type";
        state.defaultValue = "Factory";
        state.label        = "Preset Type";
        break;
    default:
        break;
    }
}

void ClassicFlangerPlugin::setState(const char* /*key*/, const char* /*value*/)
{
    // Preset state is managed by the UI; the DSP side does not need to act on it.
    // DPF will forward state changes to the UI via stateChanged() automatically.
}

// ── Audio processing ──────────────────────────────────────────────────
void ClassicFlangerPlugin::activate()
{
    fFlanger.reset_dsp();
}

void ClassicFlangerPlugin::deactivate()
{
    fFlanger.reset_dsp();
}

void ClassicFlangerPlugin::sampleRateChanged(double newSampleRate)
{
    fFlanger.set_sample_rate(static_cast<float>(newSampleRate));
}

void ClassicFlangerPlugin::run(const float** inputs, float** outputs, uint32_t frames)
{
    // Recalculate on demand
    fFlanger.recalculate_params();

    // Now process and play samples
    for (uint32_t i = 0; i < frames; i++) {
        const float in_l = inputs[0][i];
        const float in_r = inputs[1][i];
        float out_l, out_r;
        
        fFlanger.process_sample(in_l, in_r, &out_l, &out_r);
        
        outputs[0][i] = out_l;
        outputs[1][i] = out_r;
    }
}


// ─────────────────────────────────────────────────────────────────────────────
// Entry point
// ─────────────────────────────────────────────────────────────────────────────

START_NAMESPACE_DISTRHO

Plugin* createPlugin()
{
    return new ClassicFlangerPlugin();
}

END_NAMESPACE_DISTRHO
