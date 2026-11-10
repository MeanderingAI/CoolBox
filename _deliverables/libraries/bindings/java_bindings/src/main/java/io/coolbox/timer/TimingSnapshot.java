package io.coolbox.timer;

/** Immutable timing aggregate (Python: {@code timer.TimingSnapshot}). */
public record TimingSnapshot(long count,long totalNs,long minNs,long maxNs) {
    public double meanNs() { return count==0?0.0:(double)totalNs/count; }
}
