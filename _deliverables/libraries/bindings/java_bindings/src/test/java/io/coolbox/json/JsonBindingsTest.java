package io.coolbox.json;
import org.junit.jupiter.api.Test;
import java.util.List;
import java.util.Map;
import static org.junit.jupiter.api.Assertions.*;
class JsonBindingsTest {
 @Test void encodesAndDecodesMapsAndArrays(){
  var map=Map.of("name","Ada Lovelace","city","London");
  assertEquals(map,Json.jsonDecode(Json.jsonEncode(map)));
  var values=List.of("one","two words","\"quoted\"");
  assertEquals(values,Json.jsonDecodeArray(Json.jsonEncodeArray(values)));
  assertTrue(Json.jsonDecode("[]").isEmpty());
 }
}
