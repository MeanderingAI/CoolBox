# Download and build quiche (QUIC/HTTP3 library)
# This script is intended for macOS/Linux

set -e

QUICHE_VERSION=0.21.0
QUICHE_DIR="external/quiche"

if [ ! -d "$QUICHE_DIR" ]; then
  echo "Cloning quiche..."
  git clone --recursive https://github.com/cloudflare/quiche.git "$QUICHE_DIR"
  cd "$QUICHE_DIR"
  git checkout $QUICHE_VERSION
  cargo build --release --features pkg-config-meta,qlog
  cd -
else
  echo "quiche already cloned."
fi

echo "quiche build complete."
