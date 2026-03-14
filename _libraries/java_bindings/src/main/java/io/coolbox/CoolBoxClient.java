package io.coolbox;

import java.time.Instant;
import java.util.List;

/** Plain Java entry point for future CoolBox integrations. */
public final class CoolBoxClient {
    private static final String VERSION = "0.1.0";

    private final String endpoint;

    private CoolBoxClient(String endpoint) {
        this.endpoint = endpoint;
    }

    public static CoolBoxClient createDefault() {
        return new CoolBoxClient("local://coolbox");
    }

    public static CoolBoxClient forEndpoint(String endpoint) {
        return new CoolBoxClient(endpoint);
    }

    public String getEndpoint() {
        return endpoint;
    }

    public String getVersion() {
        return VERSION;
    }

    public String describe() {
        return "CoolBox Java bindings ready for plain Java integrations.";
    }

    public List<String> getCapabilities() {
        return List.of(
                "metadata",
                "version",
                "plain-java-sdk-scaffold"
        );
    }

    public Instant generatedAt() {
        return Instant.now();
    }
}
