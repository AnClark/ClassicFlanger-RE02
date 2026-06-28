#ifndef STRUCTURES_H_INCLUDED
#define STRUCTURES_H_INCLUDED

/**
 * @file Structures.h
 * @brief Parameter enumerations and metadata for the Classic Flanger plugin.
 */

/**
 * @enum Parameters
 * @brief Unique identifiers for each plugin parameter.
 *
 * The values are used as indices into the parameter value array and into
 * the @ref paramInfo metadata table.
 */
enum Parameters
{
    pParamDelay,
    pParamRate,
    pParamDepth,
    pParamFeedback,
    pParamMix,
    pParamLevel,
    pParamSpread,
    pParamStereoPhase,
    NUM_PARAMS  ///< Total number of parameters.
};

/**
 * @struct ParamInfo
 * @brief Static metadata describing one plugin parameter.
 *
 * Holds the display name, unit label, value range, default value and a
 * flag indicating whether the parameter behaves as a switch.
 */
struct ParamInfo
{
    const char *name;       ///< Parameter display name.
    const char *label;      ///< Parameter unit label (e.g. "Hz", "ms").
    float minVal;           ///< Minimum allowed value.
    float maxVal;           ///< Maximum allowed value.
    float defaultVal;       ///< Default value used on initialization.
    int isSwitch;           ///< 0 = continuous parameter, 1 = switch/enum parameter.
};

/**
 * @var paramInfo
 * @brief Static table of metadata for all plugin parameters.
 *
 * The table order matches the @ref Parameters enumeration. It is used by
 * the plugin class to initialize DISTRHO parameter descriptors.
 */
static const ParamInfo paramInfo[NUM_PARAMS] = {
    /* name,        label,    min,   max,   default, switch */
    { "Delay",       "ms",     0.05f,  10.0f,  4.0f,    0 },  ///< pParamDelay
    { "Rate",        "Hz",     0.05f, 20.0f,  0.5f,    0 },  ///< pParamRate
    { "Depth",       "",      0.0f,  1.0f, 0.5f,   0 },  ///< pParamDepth
    { "Feedback",    "",     -1.0f,1.0f, 0.0f,    0 },  ///< pParamFeedback
    { "Mix",         "",      0.0f,  1.0f, 0.5f,   0 },  ///< pParamMix
    { "Level",       "dB",    -24.0f, 6.0f,   0.0f,   0 },   ///< pParamLevel
    { "Spread",      "",       0.0f,   1.0f,   0.0f,   1},   ///< pParamSpread
    { "Stereo Phase", "",   0.0f,      180.0f, 90.0f,   0}  ///< pParamStereoPhase
};

#endif // STRUCTURES_H_INCLUDED
