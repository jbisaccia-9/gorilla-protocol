#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUTPUT="$ROOT/RawContent/Audio/Italian"
mkdir -p "$OUTPUT"

render_line() {
  local name="$1"
  local line="$2"
  local raw="$OUTPUT/$name.raw.wav"
  espeak-ng -v it+m3 -s 145 -p 30 -a 175 -w "$raw" "$line"
  ffmpeg -hide_banner -loglevel error -y -i "$raw" \
    -af "asetrate=39000,aresample=44100,acompressor=threshold=-18dB:ratio=3" \
    -ac 1 -ar 44100 "$OUTPUT/$name.wav"
  rm -f "$raw"
}

render_line mission_start "Operazione Scimmia di Mare. Entriamo piano... più o meno."
render_line spotted "Mi hanno visto. Che maleducati!"
render_line banana "Una banana tattica. Tecnologia italiana."
render_line hurt "Ah! La pelliccia era nuova!"
render_line objective "Documento preso. Adesso, fuga elegante."
render_line complete "Missione compiuta. Nessuno sospettava del gorilla."
render_line punch "Permesso!"

echo "Italian Bruno voice lines created in $OUTPUT"
