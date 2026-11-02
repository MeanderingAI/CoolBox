package io.coolbox.uuid;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class UuidBindingsTest {
 @Test void generatesUuidVersionsAndGuid(){
  assertFalse(Uuids.uuidV1().isBlank());
  assertFalse(Uuids.uuidV2(17,1).isBlank());
  assertFalse(Uuids.uuidV3("6ba7b810-9dad-11d1-80b4-00c04fd430c8","example").isBlank());
  assertFalse(Uuids.uuidV4().isBlank());
  var v5=Uuids.uuidV5("6ba7b810-9dad-11d1-80b4-00c04fd430c8","example");
  assertFalse(v5.isBlank());assertEquals(v5,Uuids.uuidV5("6ba7b810-9dad-11d1-80b4-00c04fd430c8","example"));
  assertFalse(Uuids.uuidV6().isBlank());assertEquals(7,java.util.UUID.fromString(Uuids.uuidV7()).version());assertFalse(Uuids.uuidV8("0102030405060708").isBlank());assertFalse(Uuids.guid().isBlank());
 }
}
