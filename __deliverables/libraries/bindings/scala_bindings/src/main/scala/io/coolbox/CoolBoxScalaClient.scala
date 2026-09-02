package io.coolbox

import com.sun.jna.{Library, Native, NativeLibrary}

import java.nio.file.{Files, Path, Paths}
import java.time.Instant
import scala.collection.mutable.ListBuffer

final class CoolBoxScalaClient private (val endpoint: String) {
  def getVersion: String = CoolBoxScalaClient.nativeBindings.coolbox_c_version()

  def describe: String = CoolBoxScalaClient.nativeBindings.coolbox_c_describe()

  def getCapabilityCount: Int = CoolBoxScalaClient.nativeBindings.coolbox_c_capability_count()

  def getCapabilityAt(index: Int): String = {
    if (index < 0 || index >= getCapabilityCount) ""
    else CoolBoxScalaClient.nativeBindings.coolbox_c_capability_at(index)
  }

  def getCapabilities: Vector[String] = {
    val values = ListBuffer.empty[String]
    var index = 0
    while (index < getCapabilityCount) {
      values += getCapabilityAt(index)
      index += 1
    }
    values.toVector
  }

  def generatedAt: Instant = Instant.now()

  def isReady: Boolean = CoolBoxScalaClient.nativeBindings.coolbox_c_is_ready() != 0

  def uuidV1: String = CoolBoxScalaClient.nativeBindings.coolbox_c_uuid_v1()

  def uuidV2(localIdentifier: Long, localDomain: Int): String =
    CoolBoxScalaClient.nativeBindings.coolbox_c_uuid_v2(localIdentifier.toInt, localDomain)

  def uuidV3(namespaceUuid: String, name: String): String =
    CoolBoxScalaClient.nativeBindings.coolbox_c_uuid_v3(namespaceUuid, name)

  def uuidV4: String = CoolBoxScalaClient.nativeBindings.coolbox_c_uuid_v4()

  def uuidV5(namespaceUuid: String, name: String): String =
    CoolBoxScalaClient.nativeBindings.coolbox_c_uuid_v5(namespaceUuid, name)

  def uuidV6: String = CoolBoxScalaClient.nativeBindings.coolbox_c_uuid_v6()

  def uuidV8(customEntropyHex: String): String =
    CoolBoxScalaClient.nativeBindings.coolbox_c_uuid_v8(customEntropyHex)

  def guid: String = CoolBoxScalaClient.nativeBindings.coolbox_c_guid()
}

object CoolBoxScalaClient {
  private val nativeBindings: CoolBoxNative = loadNativeBindings()

  def createDefault(): CoolBoxScalaClient = new CoolBoxScalaClient("local://coolbox")

  def forEndpoint(endpoint: String): CoolBoxScalaClient = new CoolBoxScalaClient(endpoint)

  private def loadNativeBindings(): CoolBoxNative = {
    nativeSearchDirectories().foreach(directory =>
      NativeLibrary.addSearchPath("coolbox_c_bindings", directory.toString)
    )

    try {
      Native.load("coolbox_c_bindings", classOf[CoolBoxNative])
    } catch {
      case error: UnsatisfiedLinkError =>
        throw new IllegalStateException(
          "Unable to load native CoolBox C bindings. Set coolbox.c.bindings.dir or COOLBOX_C_BINDINGS_DIR to the build directory.",
          error
        )
    }
  }

  private def nativeSearchDirectories(): Vector[Path] = {
    val directories = ListBuffer.empty[Path]
    addIfDirectory(directories, Option(System.getProperty("coolbox.c.bindings.dir")))
    addIfDirectory(directories, Option(System.getenv("COOLBOX_C_BINDINGS_DIR")))

    val userDir = Paths.get("").toAbsolutePath.normalize
    addIfDirectory(directories, userDir.resolve("_deliverables/libraries/bindings/c_bindings/build"))
    addIfDirectory(directories, userDir.resolve("../c_bindings/build"))

    directories.toVector
  }

  private def addIfDirectory(directories: ListBuffer[Path], candidate: Option[String]): Unit = {
    candidate.filter(_.nonEmpty).foreach(path => addIfDirectory(directories, Paths.get(path)))
  }

  private def addIfDirectory(directories: ListBuffer[Path], candidate: Path): Unit = {
    val normalized = candidate.toAbsolutePath.normalize
    if (Files.isDirectory(normalized) && !directories.contains(normalized)) {
      directories += normalized
    }
  }

  trait CoolBoxNative extends Library {
    def coolbox_c_version(): String
    def coolbox_c_describe(): String
    def coolbox_c_capability_count(): Int
    def coolbox_c_capability_at(index: Int): String
    def coolbox_c_is_ready(): Int
    def coolbox_c_uuid_v1(): String
    def coolbox_c_uuid_v2(localIdentifier: Int, localDomain: Int): String
    def coolbox_c_uuid_v3(namespaceUuid: String, name: String): String
    def coolbox_c_uuid_v4(): String
    def coolbox_c_uuid_v5(namespaceUuid: String, name: String): String
    def coolbox_c_uuid_v6(): String
    def coolbox_c_uuid_v8(customEntropyHex: String): String
    def coolbox_c_guid(): String
  }
}
