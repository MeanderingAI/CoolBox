package io.coolbox.misc;
import static org.junit.jupiter.api.Assertions.*; import org.junit.jupiter.api.Test;
class MiscTest { @Test void placeholderNamesAreExposed(){assertEquals("Fuzzer",Fuzzer.functionName());assertEquals("Http",Http.functionName());} }
