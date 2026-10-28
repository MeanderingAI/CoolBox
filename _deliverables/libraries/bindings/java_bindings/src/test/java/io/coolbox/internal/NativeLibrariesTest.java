package io.coolbox.internal;

import com.sun.jna.Library;
import com.sun.jna.Pointer;
import io.coolbox.json.Json;
import org.junit.jupiter.api.Test;

import java.nio.charset.StandardCharsets;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;

class NativeLibrariesTest {
    private interface JsonEncodingNative extends Library {
        Pointer coolbox_json_encode_array(String[] values, long count, Pointer error, long capacity);
    }

    private interface StringEncodingNative extends Library {
        Pointer coolbox_c_duplicate_string(String value);
        void coolbox_c_free_string(Pointer value);
    }

    @Test
    void bothLibrariesUseUtf8RegardlessOfJnaDefaultEncoding() {
        String previous = System.getProperty("jna.encoding");
        System.setProperty("jna.encoding", StandardCharsets.US_ASCII.name());
        try {
            String text = "caf\u00e9 \u6771\u4eac \ud83d\ude80";
            JsonEncodingNative bridge = NativeLibraries.bridge(JsonEncodingNative.class);
            ErrorBuffer error = new ErrorBuffer();
            String encoded = Matrices.takeString(error.check(bridge.coolbox_json_encode_array(
                    new String[] {text}, 1, error.pointer(), error.capacity())));
            assertEquals(List.of(text), Json.jsonDecodeArray(encoded));

            StringEncodingNative cBindings = NativeLibraries.cBindings(StringEncodingNative.class);
            Pointer copy = cBindings.coolbox_c_duplicate_string(text);
            try {
                assertEquals(text, copy.getString(0, StandardCharsets.UTF_8.name()));
            } finally {
                cBindings.coolbox_c_free_string(copy);
            }
        } finally {
            if (previous == null) {
                System.clearProperty("jna.encoding");
            } else {
                System.setProperty("jna.encoding", previous);
            }
        }
    }
}
