package io.coolbox;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

class CoolBoxClientTest {
    @Test
    void defaultClientHasExpectedMetadata() {
        CoolBoxClient client = CoolBoxClient.createDefault();

        assertEquals("local://coolbox", client.getEndpoint());
        assertTrue(client.isReady());
        assertEquals("1.0.0", client.getVersion());
        assertEquals(4, client.getCapabilityCount());
        assertEquals("metadata", client.getCapabilityAt(0));
        assertTrue(client.getCapabilities().contains("metadata_management"));
        assertTrue(client.getCapabilities().contains("uuid_generation"));
        assertFalse(client.describe().isBlank());
    }
}
