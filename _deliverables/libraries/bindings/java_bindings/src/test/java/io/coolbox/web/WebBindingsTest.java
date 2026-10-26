package io.coolbox.web;
import org.junit.jupiter.api.Test;
import java.net.ServerSocket;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.file.Files;
import java.util.Map;
import static org.junit.jupiter.api.Assertions.*;
class WebBindingsTest {
 @Test void validatesFormsRendersHtmlAndServesRoutesOnFreePort() throws Exception {
  var form=new FormSchema("{\"fields\":{\"name\":{\"type\":\"string\",\"required\":true}}}");
  assertTrue(form.validate("{\"name\":\"Ada\"}").valid());
  assertFalse(form.validate("{}").valid());
  assertThrows(RuntimeException.class,()->new FormSchema("{invalid"));
  var htmlRoot=Files.createTempDirectory("coolbox-html");Files.writeString(htmlRoot.resolve("index.html"),"<h1>{{name}}</h1>");
  assertEquals("<h1>&lt;Ada&gt;</h1>",Html.render(htmlRoot.toString(),"index.html",Map.of("name","<Ada>")).body());
  assertEquals("<h1>{{name}}</h1>",Html.file(htmlRoot.toString(),"index.html").body());
  int port;try(var socket=new ServerSocket(0)){port=socket.getLocalPort();}
  try(var server=new Server(port)){
   server.enableCors("http://localhost");
   server.get("/hello",request->new Response(201,"hello "+request.getQueryParam("name","world")).setHeader("X-CoolBox","ok"));
   server.start();assertTrue(server.isRunning());
   var response=HttpClient.newHttpClient().send(HttpRequest.newBuilder(URI.create("http://127.0.0.1:"+port+"/hello?name=Ada")).GET().build(),HttpResponse.BodyHandlers.ofString());
   assertEquals(201,response.statusCode());assertEquals("hello Ada",response.body());assertEquals("ok",response.headers().firstValue("X-CoolBox").orElseThrow());assertEquals("http://localhost",response.headers().firstValue("Access-Control-Allow-Origin").orElseThrow());
   server.stop();
  }
  assertThrows(RuntimeException.class,()->FormSchema.fromFile("missing-coolbox-form-schema.json"));
 }
}
