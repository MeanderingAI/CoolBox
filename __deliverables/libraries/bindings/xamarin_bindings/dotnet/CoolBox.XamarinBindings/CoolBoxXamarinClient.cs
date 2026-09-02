using System;
using System.Collections.Generic;

namespace CoolBox.XamarinBindings
{
    public static class CoolBoxXamarinClient
    {
        public static string Version => CoolBoxXamarinNative.PtrToString(CoolBoxXamarinNative.coolbox_xamarin_version());

        public static string DefaultEndpoint => CoolBoxXamarinNative.PtrToString(CoolBoxXamarinNative.coolbox_xamarin_default_endpoint());

        public static IReadOnlyList<string> Capabilities
        {
            get
            {
                var count = (int)CoolBoxXamarinNative.coolbox_xamarin_capability_count();
                var result = new List<string>(Math.Max(0, count));
                for (var i = 0; i < count; i++)
                {
                    var ptr = CoolBoxXamarinNative.coolbox_xamarin_capability_at((UIntPtr)i);
                    result.Add(CoolBoxXamarinNative.PtrToString(ptr));
                }
                return result;
            }
        }

        public static (int Left, int Top, int Right, int Bottom) LiquidAnimatedBounds(
            int left,
            int top,
            int right,
            int bottom,
            float elapsedSeconds,
            float driftAmplitudePx = 8.0f,
            float bobAmplitudePx = 4.0f,
            float driftSpeedHz = 0.28f,
            float bobSpeedHz = 0.44f,
            float phaseOffset = 0.0f)
        {
            var spec = new CoolBoxXamarinNative.LiquidElementSpec
            {
                Left = left,
                Top = top,
                Right = right,
                Bottom = bottom,
                DriftAmplitudePx = driftAmplitudePx,
                BobAmplitudePx = bobAmplitudePx,
                DriftSpeedHz = driftSpeedHz,
                BobSpeedHz = bobSpeedHz,
                PhaseOffset = phaseOffset,
            };

            CoolBoxXamarinNative.coolbox_xamarin_liquid_animated_bounds(
                ref spec,
                elapsedSeconds,
                out var outLeft,
                out var outTop,
                out var outRight,
                out var outBottom);

            return (outLeft, outTop, outRight, outBottom);
        }
    }
}
