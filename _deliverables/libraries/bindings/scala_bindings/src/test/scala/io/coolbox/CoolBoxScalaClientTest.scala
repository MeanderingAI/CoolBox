package io.coolbox

import org.junit.jupiter.api.Assertions.{assertEquals, assertFalse, assertTrue}
import org.junit.jupiter.api.Test

class CoolBoxScalaClientTest {
  @Test
  def defaultClientHasExpectedMetadata(): Unit = {
    val client = CoolBoxScalaClient.createDefault()

    assertEquals("local://coolbox", client.endpoint)
    assertTrue(client.isReady)
    assertEquals("1.0.0", client.getVersion)
    assertEquals(3, client.getCapabilityCount)
    assertEquals("metadata", client.getCapabilityAt(0))
    assertTrue(client.getCapabilities.contains("metadata_management"))
    assertFalse(client.describe.isBlank)
  }
}
