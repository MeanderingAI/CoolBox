package io.coolbox.timer;
import io.coolbox.internal.NativeHandle;
/** Timing-sample aggregator (Python: {@code timer.TimingStats}). */
public class TimingStats extends NativeHandle {
 protected TimingStats(boolean concurrent){super(TimerNative.INSTANCE.coolbox_misc_stats_new(concurrent?1:0),TimerNative.INSTANCE::coolbox_misc_stats_free);}
 public TimingStats(){this(false);} public void recordNs(long durationNs){TimerNative.INSTANCE.coolbox_misc_stats_record_ns(handle(),durationNs);} void recordMilliseconds(double elapsedMs){recordNs((long)(elapsedMs*1_000_000.0));} public TimingSnapshot snapshot(){return new TimingSnapshot(TimerNative.INSTANCE.coolbox_misc_stats_count(handle()),TimerNative.INSTANCE.coolbox_misc_stats_total_ns(handle()),TimerNative.INSTANCE.coolbox_misc_stats_min_ns(handle()),TimerNative.INSTANCE.coolbox_misc_stats_max_ns(handle()));} public void reset(){TimerNative.INSTANCE.coolbox_misc_stats_reset(handle());}
}
