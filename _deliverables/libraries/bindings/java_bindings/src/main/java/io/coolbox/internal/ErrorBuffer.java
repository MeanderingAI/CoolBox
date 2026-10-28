package io.coolbox.internal;

import com.sun.jna.Memory;
import com.sun.jna.Pointer;
import io.coolbox.CoolBoxException;

import java.nio.charset.StandardCharsets;

/**
 * Scratch buffer for the {@code (char* error, size_t error_capacity)} convention used by the native bridges.
 *
 * <pre>{@code
 * ErrorBuffer err = new ErrorBuffer();
 * err.check(api.coolbox_x(handle, err.pointer(), err.capacity()));
 * }</pre>
 */
public final class ErrorBuffer {
    public static final int DEFAULT_CAPACITY = 1024;

    private final Memory memory;

    public ErrorBuffer() {
        this(DEFAULT_CAPACITY);
    }

    public ErrorBuffer(int capacity) {
        memory = new Memory(capacity);
        memory.clear();
    }

    public Pointer pointer() {
        return memory;
    }

    public long capacity() {
        return memory.size();
    }

    public String message() {
        String text = memory.getString(0, StandardCharsets.UTF_8.name());
        return text == null ? "" : text;
    }

    /** Throws if {@code status} is zero (the bridges' failure convention for boolean-like status ints). */
    public void check(int status) {
        if (status == 0) {
            throw failure();
        }
    }

    /** Throws if the native call returned a null handle. */
    public Pointer check(Pointer handle) {
        if (handle == null) {
            throw failure();
        }
        return handle;
    }

    /** Throws if the error buffer holds a message. Useful for calls that return plain values. */
    public void throwIfSet() {
        if (memory.getByte(0) != 0) {
            throw failure();
        }
    }

    public CoolBoxException failure() {
        String message = message();
        return new CoolBoxException(message.isEmpty() ? "native CoolBox call failed" : message);
    }
}
