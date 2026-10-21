#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
vendor="$root/ps5/vendor"
mkdir -p "$vendor"
checkout() {
  local name=$1 url=$2 revision=$3
  if [[ ! -d "$vendor/$name" ]]; then
    git clone --no-checkout --depth 1 "$url" "$vendor/$name"
    git -C "$vendor/$name" fetch --depth 1 origin "$revision"
    git -C "$vendor/$name" checkout --detach "$revision"
  fi
  git -C "$vendor/$name" cat-file -e "$revision^{commit}"
}
checkout ps5-opengl https://github.com/darkxex/ps5-opengl.git 6b6bb2039f741287ed7f1bc6bc4b2c589709a0ba
checkout raylib https://github.com/raysan5/raylib.git 02566453880e06bec97f5b8160fa6fe1e98397ca
checkout SDL https://github.com/ps5-payload-dev/SDL.git 8c56053f13ca13a0c050de613706ff69eb615836
# The SDL builder exports the pinned revision, regardless of checkout HEAD.
for name in ps5-opengl raylib; do
  case "$name" in
    ps5-opengl) expected=6b6bb2039f741287ed7f1bc6bc4b2c589709a0ba ;;
    raylib) expected=02566453880e06bec97f5b8160fa6fe1e98397ca ;;
  esac
  [[ $(git -C "$vendor/$name" rev-parse HEAD) == "$expected" ]] || {
    echo "Unexpected $name revision; preserve your checkout and use the pinned revision." >&2; exit 1;
  }
  [[ -z $(git -C "$vendor/$name" status --porcelain --untracked-files=no) ]] || {
    echo "Tracked edits in $name; refusing to build modified dependency sources." >&2; exit 1;
  }
done
archive="$vendor/ps5-opengl-sdk-1.0.2.tar.gz"
if [[ ! -f "$archive" ]]; then
  curl --fail --location --retry 3 \
    https://github.com/darkxex/ps5-opengl/releases/download/v1.0.2/ps5-opengl-sdk-1.0.2.tar.gz \
    --output "$archive.part"
  mv "$archive.part" "$archive"
fi
printf '%s  %s\n' 30d8e51dd062739b629346102be5932728ba714bc50b1bdcdf08306c8843d9a4 "$archive" | sha256sum --check
if [[ ! -d "$vendor/opengl-sdk/ps5-opengl-sdk-1.0.2" ]]; then
  mkdir -p "$vendor/opengl-sdk"
  tar -xzf "$archive" -C "$vendor/opengl-sdk"
fi
(cd "$vendor/opengl-sdk/ps5-opengl-sdk-1.0.2" && sha256sum --check SHA256SUMS)
