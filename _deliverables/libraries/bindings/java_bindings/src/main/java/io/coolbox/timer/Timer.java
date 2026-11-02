package io.coolbox.timer;

import io.coolbox.internal.NativeHandle;

/** Monotonic stopwatch (Python: {@code timer.Timer}). */
public final class Timer extends NativeHandle {
    public Timer() { super(TimerNative.INSTANCE.coolbox_misc_timer_new(),TimerNative.INSTANCE::coolbox_misc_timer_free); }
    public Timer start() { TimerNative.INSTANCE.coolbox_misc_timer_start(handle()); return this; }
    public Timer stop() { TimerNative.INSTANCE.coolbox_misc_timer_stop(handle()); return this; }
    public Timer reset() { TimerNative.INSTANCE.coolbox_misc_timer_reset(handle()); return this; }
    public double elapsedMs() { return TimerNative.INSTANCE.coolbox_misc_timer_elapsed_ms(handle()); }
    public double lapMs() { return TimerNative.INSTANCE.coolbox_misc_timer_lap_ms(handle()); }
    public boolean isRunning() { return TimerNative.INSTANCE.coolbox_misc_timer_is_running(handle())!=0; }
}
