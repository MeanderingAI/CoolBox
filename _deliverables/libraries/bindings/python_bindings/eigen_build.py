import hashlib
import os
from pathlib import Path
import shutil
import tarfile
import tempfile
from urllib.error import URLError
from urllib.request import urlopen


EIGEN_VERSION = "3.4.0"
EIGEN_ARCHIVE_URL = (
    "https://gitlab.com/libeigen/eigen/-/archive/"
    f"{EIGEN_VERSION}/eigen-{EIGEN_VERSION}.tar.gz"
)
EIGEN_ARCHIVE_SHA256 = (
    "8586084f71f9bde545ee7fa6d00288b264a2b7ac3607b974e54d13e7162c1c72"
)


def resolve_eigen_include_dirs(paths):
    seen = set()
    resolved = []
    for raw_path in paths:
        if not raw_path:
            continue

        candidate = Path(raw_path)
        variants = [candidate]
        if candidate.name.lower() != "eigen3":
            variants.append(candidate / "eigen3")

        for variant in variants:
            if not (variant / "Eigen" / "Core").is_file():
                continue

            normalized = str(variant)
            if normalized in seen:
                continue
            seen.add(normalized)
            resolved.append(normalized)

    return resolved


def _validate_archive_members(archive, extraction_root):
    root = extraction_root.resolve()
    for member in archive.getmembers():
        if member.issym() or member.islnk():
            raise RuntimeError(
                f"Refusing to extract link from Eigen archive: {member.name}"
            )
        destination = (root / member.name).resolve()
        try:
            destination.relative_to(root)
        except ValueError as error:
            raise RuntimeError(
                f"Refusing to extract unsafe Eigen archive path: {member.name}"
            ) from error


def provision_eigen(
    destination,
    archive_url=EIGEN_ARCHIVE_URL,
    archive_sha256=EIGEN_ARCHIVE_SHA256,
):
    destination = Path(destination)
    existing = resolve_eigen_include_dirs([destination])
    if existing:
        return existing[0]

    if os.environ.get("COOLBOX_DISABLE_EIGEN_DOWNLOAD", "").lower() in {
        "1",
        "true",
        "yes",
        "on",
    }:
        raise RuntimeError(
            "Eigen headers were not found and automatic download is disabled. "
            "Install Eigen or set EIGEN3_INCLUDE_DIR/EIGEN_INCLUDE_DIR."
        )

    destination.parent.mkdir(parents=True, exist_ok=True)
    archive_path = None
    try:
        with tempfile.NamedTemporaryFile(
            prefix="eigen-", suffix=".tar.gz", dir=str(destination.parent),
            delete=False
        ) as archive_file:
            archive_path = Path(archive_file.name)
            digest = hashlib.sha256()
            print(f"Downloading Eigen {EIGEN_VERSION} from {archive_url}")
            with urlopen(archive_url, timeout=60) as response:
                while True:
                    chunk = response.read(1024 * 1024)
                    if not chunk:
                        break
                    archive_file.write(chunk)
                    digest.update(chunk)

        actual_sha256 = digest.hexdigest()
        if actual_sha256 != archive_sha256:
            raise RuntimeError(
                "Eigen archive checksum mismatch: "
                f"expected {archive_sha256}, got {actual_sha256}"
            )

        with tempfile.TemporaryDirectory(
            prefix="eigen-extract-", dir=str(destination.parent)
        ) as extraction_dir:
            extraction_root = Path(extraction_dir)
            with tarfile.open(str(archive_path), mode="r:gz") as archive:
                _validate_archive_members(archive, extraction_root)
                archive.extractall(str(extraction_root))

            extracted = extraction_root / f"eigen-{EIGEN_VERSION}"
            if not resolve_eigen_include_dirs([extracted]):
                raise RuntimeError(
                    "Downloaded Eigen archive does not contain Eigen/Core"
                )

            if destination.exists():
                shutil.rmtree(str(destination))
            shutil.move(str(extracted), str(destination))
    except (OSError, URLError) as error:
        raise RuntimeError(
            "Unable to download Eigen headers. Install Eigen or set "
            "EIGEN3_INCLUDE_DIR/EIGEN_INCLUDE_DIR to an existing Eigen "
            f"include directory. Download error: {error}"
        ) from error
    finally:
        if archive_path is not None:
            try:
                archive_path.unlink()
            except FileNotFoundError:
                pass

    resolved = resolve_eigen_include_dirs([destination])
    if not resolved:
        raise RuntimeError(
            f"Eigen provisioning completed without usable headers at {destination}"
        )
    return resolved[0]
