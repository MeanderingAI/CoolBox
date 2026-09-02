using System;
using System.Runtime.InteropServices;

namespace CoolBox.XamarinBindings
{
    internal static class CoolBoxXamarinNative
    {
        internal const string NativeLibrary = "coolbox_xamarin_bindings";

        [StructLayout(LayoutKind.Sequential)]
        internal struct LiquidElementSpec
        {
            public int Left;
            public int Top;
            public int Right;
            public int Bottom;
            public float DriftAmplitudePx;
            public float BobAmplitudePx;
            public float DriftSpeedHz;
            public float BobSpeedHz;
            public float PhaseOffset;
        }

        [DllImport(NativeLibrary, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr coolbox_xamarin_version();

        [DllImport(NativeLibrary, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr coolbox_xamarin_default_endpoint();

        [DllImport(NativeLibrary, CallingConvention = CallingConvention.Cdecl)]
        internal static extern UIntPtr coolbox_xamarin_capability_count();

        [DllImport(NativeLibrary, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr coolbox_xamarin_capability_at(UIntPtr index);

        [DllImport(NativeLibrary, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void coolbox_xamarin_liquid_animated_bounds(
            ref LiquidElementSpec spec,
            float elapsedSeconds,
            out int outLeft,
            out int outTop,
            out int outRight,
            out int outBottom);

        internal static string PtrToString(IntPtr ptr)
        {
            return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringAnsi(ptr) ?? string.Empty;
        }
    }
}
