#!/bin/bash
# Rebuild the embedded web UI from the opna monorepo → ui/webui.zip
set -e
cd "$(dirname "$0")/.."

OPNA=../opna
[ -d "$OPNA/apps/phaser" ] || { echo "opna monorepo not found at $OPNA" >&2; exit 1; }

(cd "$OPNA" && pnpm install --silent && pnpm -F @opna/phaser exec vite build --config vite/config.juce.mjs)

rm -f ui/webui.zip
(cd "$OPNA/apps/phaser/dist-juce" && zip -q -r - .) > ui/webui.zip
echo "ui/webui.zip: $(du -h ui/webui.zip | cut -f1)"
