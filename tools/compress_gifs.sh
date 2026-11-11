#!/usr/bin/env bash
# Re-encode the animated GIFs written by MATLAB/Octave with a proper palette.
# Octave's GIF writer does not compress well; this keeps the repo small.
# Usage: tools/compress_gifs.sh [files...]   (default: matlab/figures/*.gif)
set -euo pipefail
files=("$@")
[ ${#files[@]} -eq 0 ] && files=(matlab/figures/*.gif)
for f in "${files[@]}"; do
  tmp="$(mktemp --suffix=.gif)"
  ffmpeg -v error -y -i "$f" \
    -vf "fps=12,scale=440:-1:flags=lanczos,split[a][b];[a]palettegen=max_colors=48[p];[b][p]paletteuse=dither=none" \
    -loop 0 "$tmp"
  mv "$tmp" "$f"
  echo "$f: $(du -h "$f" | cut -f1)"
done
