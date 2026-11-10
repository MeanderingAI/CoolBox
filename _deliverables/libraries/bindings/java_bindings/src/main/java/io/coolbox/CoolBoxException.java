package io.coolbox;

/** Raised when a native CoolBox call reports a failure. */
public class CoolBoxException extends RuntimeException {
    private static final long serialVersionUID = 1L;

    public CoolBoxException(String message) {
        super(message);
    }

    public CoolBoxException(String message, Throwable cause) {
        super(message, cause);
    }
}
