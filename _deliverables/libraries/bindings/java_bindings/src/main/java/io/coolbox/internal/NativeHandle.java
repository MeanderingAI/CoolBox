package io.coolbox.internal;

import com.sun.jna.Pointer;
import io.coolbox.CoolBoxException;

import java.lang.ref.Cleaner;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Consumer;

/**
 * Base class for Java objects that own a native handle.
 *
 * <p>The handle is released exactly once: either explicitly via {@link #close()} (preferred, e.g. with
 * try-with-resources) or by a {@link Cleaner} when the object becomes unreachable. Subclasses must not capture
 * {@code this} in the free function.
 */
public abstract class NativeHandle implements AutoCloseable {
    private static final Cleaner CLEANER = Cleaner.create();

    private final State state;
    private final Cleaner.Cleanable cleanable;

    protected NativeHandle(Pointer handle, Consumer<Pointer> free) {
        if (handle == null) {
            throw new CoolBoxException("native constructor returned a null handle");
        }
        this.state = new State(handle, free);
        this.cleanable = CLEANER.register(this, state);
    }

    /** Returns the live native handle; throws if the object has been closed. */
    public final Pointer handle() {
        Pointer pointer = state.pointer.get();
        if (pointer == null) {
            throw new IllegalStateException(getClass().getSimpleName() + " has been closed");
        }
        return pointer;
    }

    public final boolean isClosed() {
        return state.pointer.get() == null;
    }

    @Override
    public void close() {
        cleanable.clean();
    }

    private static final class State implements Runnable {
        private final AtomicReference<Pointer> pointer;
        private final Consumer<Pointer> free;

        private State(Pointer pointer, Consumer<Pointer> free) {
            this.pointer = new AtomicReference<>(pointer);
            this.free = free;
        }

        @Override
        public void run() {
            Pointer value = pointer.getAndSet(null);
            if (value != null && free != null) {
                free.accept(value);
            }
        }
    }
}
