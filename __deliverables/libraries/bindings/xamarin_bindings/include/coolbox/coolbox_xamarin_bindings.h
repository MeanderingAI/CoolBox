#ifndef COOLBOX_XAMARIN_BINDINGS_H
#define COOLBOX_XAMARIN_BINDINGS_H

#include <stddef.h>

#if defined(_WIN32) && defined(COOLBOX_XAMARIN_BINDINGS_SHARED)
#if defined(COOLBOX_XAMARIN_BINDINGS_EXPORTS)
#define COOLBOX_XAMARIN_API __declspec(dllexport)
#else
#define COOLBOX_XAMARIN_API __declspec(dllimport)
#endif
#else
#define COOLBOX_XAMARIN_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CoolBoxXamarinLiquidElementSpec {
    int left;
    int top;
    int right;
    int bottom;
    float drift_amplitude_px;
    float bob_amplitude_px;
    float drift_speed_hz;
    float bob_speed_hz;
    float phase_offset;
} CoolBoxXamarinLiquidElementSpec;

COOLBOX_XAMARIN_API const char *coolbox_xamarin_version(void);
COOLBOX_XAMARIN_API const char *coolbox_xamarin_default_endpoint(void);
COOLBOX_XAMARIN_API size_t coolbox_xamarin_capability_count(void);
COOLBOX_XAMARIN_API const char *coolbox_xamarin_capability_at(size_t index);

COOLBOX_XAMARIN_API void coolbox_xamarin_liquid_animated_bounds(
    const CoolBoxXamarinLiquidElementSpec *spec,
    float elapsed_seconds,
    int *out_left,
    int *out_top,
    int *out_right,
    int *out_bottom);

#ifdef __cplusplus
}
#endif

#endif
