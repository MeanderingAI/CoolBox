package io.coolbox.timer;

import java.util.Objects;
import java.util.function.DoubleConsumer;
import io.coolbox.internal.NativeHandle;

/** Scope stopwatch invoking a callback once when closed (Python: {@code timer.ScopedTimer}). */
public final class ScopedTimer extends NativeHandle {
    private final TimerNative.ElapsedCallback callback;
    public ScopedTimer(TimingStats stats) { this(Objects.requireNonNull(stats,"stats")::recordMilliseconds); }
    public ScopedTimer(ConcurrentTimingStats stats) { this(Objects.requireNonNull(stats,"stats")::recordMilliseconds); }
    public ScopedTimer(DoubleConsumer onClose) {
        this(new CallbackState(Objects.requireNonNull(onClose,"onClose")));
    }
    private ScopedTimer(CallbackState state) {
        super(TimerNative.INSTANCE.coolbox_misc_scoped_timer_new(state.callback,null),pointer->{ state.callback.hashCode(); TimerNative.INSTANCE.coolbox_misc_scoped_timer_free(pointer); });
        callback=state.callback;
    }
    private static final class CallbackState {
        final TimerNative.ElapsedCallback callback;
        CallbackState(DoubleConsumer consumer) { callback=(elapsed,user)->consumer.accept(elapsed); }
    }
    public double elapsedMs() { return TimerNative.INSTANCE.coolbox_misc_scoped_timer_elapsed_ms(handle()); }
    @Override public void close() { if(!isClosed()) TimerNative.INSTANCE.coolbox_misc_scoped_timer_close(handle()); super.close(); }
}
