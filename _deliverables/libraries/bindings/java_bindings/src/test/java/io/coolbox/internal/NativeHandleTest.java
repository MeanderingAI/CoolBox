package io.coolbox.internal;

import com.sun.jna.Pointer;
import io.coolbox.CoolBoxException;
import org.junit.jupiter.api.Test;

import java.util.concurrent.atomic.AtomicInteger;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertSame;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

class NativeHandleTest {
    @Test
    void closingReleasesHandleExactlyOnceAndRejectsFurtherUse() {
        Pointer pointer = Pointer.createConstant(1);
        AtomicInteger releases = new AtomicInteger();
        NativeHandle handle = new NativeHandle(pointer, freed -> {
            assertSame(pointer, freed);
            releases.incrementAndGet();
        }) {};
        assertSame(pointer, handle.handle());
        assertFalse(handle.isClosed());
        handle.close();
        handle.close();
        assertEquals(1, releases.get());
        assertTrue(handle.isClosed());
        assertThrows(IllegalStateException.class, handle::handle);
    }

    @Test
    void failedNativeConstructionIsRejected() {
        assertThrows(CoolBoxException.class, () -> new NativeHandle(null, pointer -> {}) {});
    }
}
