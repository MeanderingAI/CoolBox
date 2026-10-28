package io.coolbox.uuid;

/** UUID and GUID facade over the existing {@code coolbox_c_bindings} functions. */
public final class Uuids {
 private static final java.security.SecureRandom RANDOM=new java.security.SecureRandom();
 private Uuids() {}
 public static String uuidV1(){return UuidNative.INSTANCE.coolbox_c_uuid_v1();}
 public static String uuidV2(int localIdentifier,int localDomain){return UuidNative.INSTANCE.coolbox_c_uuid_v2(localIdentifier,localDomain);}
 public static String uuidV3(String namespaceUuid,String name){return UuidNative.INSTANCE.coolbox_c_uuid_v3(namespaceUuid,name);}
 public static String uuidV4(){return UuidNative.INSTANCE.coolbox_c_uuid_v4();}
 public static String uuidV5(String namespaceUuid,String name){return UuidNative.INSTANCE.coolbox_c_uuid_v5(namespaceUuid,name);}
 public static String uuidV6(){return UuidNative.INSTANCE.coolbox_c_uuid_v6();}
 public static String uuidV7(){long timestamp=System.currentTimeMillis()&0x0000ffffffffffffL;long most=(timestamp<<16)|0x7000L|RANDOM.nextInt(0x1000);long least=(RANDOM.nextLong()&0x3fffffffffffffffL)|0x8000000000000000L;return new java.util.UUID(most,least).toString();}
 public static String uuidV8(String customEntropyHex){return UuidNative.INSTANCE.coolbox_c_uuid_v8(customEntropyHex);}
 public static String guid(){return UuidNative.INSTANCE.coolbox_c_guid();}
}
