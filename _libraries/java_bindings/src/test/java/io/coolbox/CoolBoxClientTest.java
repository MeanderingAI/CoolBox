package io.coolbox;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

class CoolBoxClientTest {
    @Test
    void defaultClientHasExpectedMetadata() {
        CoolBoxClient client = CoolBoxClient.createDefault();

        assertEquals("local://coolbox", client.getEndpoint());
        assertEquals("0.1.0", client.getVersion());
        assertTrue(client.getCapabilities().contains("plain-java-sdk-scaffold"));
    }
}
