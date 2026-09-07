"""
ascii_to_png.py - convierte el arte ASCII del prototipo a PNG indexado.

Entradas:
  - docs/prototipo_referencia.html : sprites SPR.* y paleta COL{}
  - assets/src/tiles/*.txt         : tiles del mundo (transcripcion editable
                                     de drawTile(), que era procedural)

Salidas (assets/src/**.png + .json):
  PNG indexado de 4bpp-compatible (<=16 colores, indice 0 transparente),
  con la paleta ya cuantizada a 5 bits por canal — o sea, exactamente los
  colores que la GBA puede mostrar. A partir de aca el PNG es la fuente de
  verdad editable (Aseprite/GraphicsGale) y el prototipo deja de mandar:
  este script solo se vuelve a correr si se quiere re-sembrar desde cero.

Ver docs/PLAN_MIGRACION_GBA_C.md, seccion 5.
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from common.prototype import (REPO, load_palette, load_sprites, rgb5_to_rgb8,
                              sprite_size, TRANSPARENT_CHARS)

from PIL import Image

MAX_COLORS = 16  # 4bpp: 15 colores + el indice 0 transparente

# Grupos de sprites que comparten banco de paleta. Cada grupo se emite
# como una hoja vertical de frames del mismo tamano, que es lo que
# espera grit con meta-tiles (-Mw/-Mh) para el mapeo 1D de OBJ.
SPRITE_GROUPS = {
    "perseo/perseo": [
        "p_idle1", "p_idle2",
        "p_walk1", "p_walk2", "p_walk3", "p_walk4",
        "p_jump", "p_fall", "p_dash",
        "p_atk1", "p_atk2", "p_atk3",
    ],
}


def build_palette(char_sets, pal):
    """Asigna indices 1..15 a los caracteres usados; 0 queda transparente."""
    used = sorted(set().union(*char_sets) - set(TRANSPARENT_CHARS))
    missing = [c for c in used if c not in pal]
    if missing:
        raise RuntimeError(f"Caracteres sin color en COL{{}}: {missing}")
    if len(used) + 1 > MAX_COLORS:
        raise RuntimeError(f"{len(used) + 1} colores > {MAX_COLORS} (4bpp) en el grupo")
    return {c: i + 1 for i, c in enumerate(used)}


def write_indexed_png(path, rows_grid, char_index, pal, width, height):
    img = Image.new("P", (width, height), 0)
    flat = [0, 0, 0] * 256
    for ch, idx in char_index.items():
        r, g, b = rgb5_to_rgb8(pal[ch])
        flat[idx * 3:idx * 3 + 3] = [r, g, b]
    img.putpalette(flat)
    px = img.load()
    for y, row in enumerate(rows_grid):
        for x, ch in enumerate(row):
            if ch not in TRANSPARENT_CHARS:
                px[x, y] = char_index[ch]
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path)


def emit_sprite_group(name, frame_names, sprites, pal):
    frames = []
    for fn in frame_names:
        if fn not in sprites:
            raise RuntimeError(f"El prototipo no define SPR.{fn}")
        frames.append(sprites[fn])
    sizes = {sprite_size(f) for f in frames}
    if len(sizes) != 1:
        raise RuntimeError(f"Grupo {name}: frames de tamanos distintos {sizes}")
    fw, fh = sizes.pop()

    char_index = build_palette([{c for row in f for c in row} for f in frames], pal)

    grid = []
    for f in frames:
        for row in f:
            grid.append(row.ljust(fw, "."))
    out = REPO / "assets" / "src" / "sprites" / f"{name}.png"
    write_indexed_png(out, grid, char_index, pal, fw, fh * len(frames))

    meta = {
        "frame_size": [fw, fh],
        "frames": frame_names,
        "palette": {"0": "transparente", **{str(i): c for c, i in char_index.items()}},
    }
    out.with_suffix(".json").write_text(json.dumps(meta, indent=2), encoding="utf-8")
    print(f"  {out.relative_to(REPO)}  {fw}x{fh} x{len(frames)} frames, "
          f"{len(char_index) + 1} colores")
    return meta


def parse_tile_file(path):
    """Lee un .txt de tiles: bloques de 8 lineas separados por comentarios."""
    tiles, cur = [], []
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.rstrip()
        if not line or line.lstrip().startswith("#"):
            if cur:
                tiles.append(cur)
                cur = []
            continue
        cur.append(line)
    if cur:
        tiles.append(cur)
    for i, t in enumerate(tiles):
        if len(t) != 8 or any(len(r) != 8 for r in t):
            raise RuntimeError(f"{path.name}: el tile {i} no es de 8x8")
    return tiles


def emit_tileset(txt_path, pal):
    tiles = parse_tile_file(txt_path)
    char_index = build_palette([{c for row in t for c in row} for t in tiles], pal)
    grid = [row for t in tiles for row in t]
    out = txt_path.with_suffix(".png")
    write_indexed_png(out, grid, char_index, pal, 8, 8 * len(tiles))
    meta = {
        "tile_count": len(tiles),
        "palette": {"0": "transparente", **{str(i): c for c, i in char_index.items()}},
    }
    out.with_suffix(".json").write_text(json.dumps(meta, indent=2), encoding="utf-8")
    print(f"  {out.relative_to(REPO)}  {len(tiles)} tiles, {len(char_index) + 1} colores")
    return meta


def main():
    pal = load_palette()
    sprites = load_sprites()
    print("Sprites:")
    for name, frames in SPRITE_GROUPS.items():
        emit_sprite_group(name, frames, sprites, pal)
    print("Tilesets:")
    for txt in sorted((REPO / "assets" / "src" / "tiles").glob("*.txt")):
        emit_tileset(txt, pal)


if __name__ == "__main__":
    main()
