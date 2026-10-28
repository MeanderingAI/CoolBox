package io.coolbox.timer;
import com.sun.jna.Callback;
import com.sun.jna.Library;
import com.sun.jna.Pointer;
import io.coolbox.internal.NativeLibraries;
interface TimerNative extends Library {
 TimerNative INSTANCE=NativeLibraries.bridge(TimerNative.class);
 interface ElapsedCallback extends Callback { void invoke(double elapsedMs, Pointer userData); }
 Pointer coolbox_misc_timer_new(); void coolbox_misc_timer_free(Pointer h); void coolbox_misc_timer_start(Pointer h); void coolbox_misc_timer_stop(Pointer h); void coolbox_misc_timer_reset(Pointer h); double coolbox_misc_timer_elapsed_ms(Pointer h); double coolbox_misc_timer_lap_ms(Pointer h); int coolbox_misc_timer_is_running(Pointer h);
 Pointer coolbox_misc_stats_new(int concurrent); void coolbox_misc_stats_free(Pointer h); void coolbox_misc_stats_record_ns(Pointer h,long ns); void coolbox_misc_stats_reset(Pointer h); long coolbox_misc_stats_count(Pointer h); long coolbox_misc_stats_total_ns(Pointer h); long coolbox_misc_stats_min_ns(Pointer h); long coolbox_misc_stats_max_ns(Pointer h);
 Pointer coolbox_misc_snapshot_new(long count,long total,long min,long max); void coolbox_misc_snapshot_free(Pointer h); long coolbox_misc_snapshot_count(Pointer h); long coolbox_misc_snapshot_total_ns(Pointer h); long coolbox_misc_snapshot_min_ns(Pointer h); long coolbox_misc_snapshot_max_ns(Pointer h); double coolbox_misc_snapshot_mean_ns(Pointer h);
 Pointer coolbox_misc_scoped_timer_new(ElapsedCallback cb,Pointer user); double coolbox_misc_scoped_timer_elapsed_ms(Pointer h); double coolbox_misc_scoped_timer_close(Pointer h); void coolbox_misc_scoped_timer_free(Pointer h);
}
