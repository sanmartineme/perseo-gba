"""
prototype.py - lectura del prototipo web como fuente de verdad de assets.

El prototipo (docs/prototipo_referencia.html) tiene el arte embebido como
literales JS: la paleta logica en COL{} y cada sprite en SPR.<nombre> como
una matriz de strings donde cada caracter es una clave de la paleta
('.' y ' ' = transparente).

Este modulo es la unica pieza que sabe leer ese formato; el resto del
pipeline (spritegen, levelgen) trabaja ya con datos limpios.

Ver docs/PLAN_MIGRACION_GBA_C.md, seccion 5 (pipeline de datos y assets).
"""
import json
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
PROTOTYPE = REPO / "docs" / "prototipo_referencia.html"

TRANSPARENT_CHARS = ".", " "


def _read_prototype(path=None):
    return (path or PROTOTYPE).read_text(encoding="utf-8")


def load_palette(path=None):
    """Devuelve {clave: (r5,g5,b5)} con la variante GBA de COL{}.

    Los colores se cuantizan a 5 bits por canal, que es lo que la GBA
    puede representar de verdad: asi lo que se ve en el PNG generado es
    exactamente lo que se vera en pantalla, sin sorpresas de redondeo.
    """
    src = _read_prototype(path)
    block = re.search(r"const COL\s*=\s*\{(.*?)\n\};", src, re.S)
    if not block:
        raise RuntimeError("No se encontro el bloque COL{} en el prototipo")
    pal = {}
    for key, gba_hex in re.findall(r"(\w):\['(#[0-9a-fA-F]{6})','#[0-9a-fA-F]{6}'\]", block.group(1)):
        r = int(gba_hex[1:3], 16) >> 3
        g = int(gba_hex[3:5], 16) >> 3
        b = int(gba_hex[5:7], 16) >> 3
        pal[key] = (r, g, b)
    return pal


def rgb5_to_rgb8(c):
    """Expande un color de 5 bits/canal a 8 bits/canal para el PNG."""
    return tuple((v << 3) | (v >> 2) for v in c)


def load_sprites(path=None):
    """Devuelve {nombre: [fila, ...]} con todos los SPR.* del prototipo."""
    src = _read_prototype(path)
    sprites = {}
    for name, body in re.findall(r"SPR\.(\w+)\s*=\s*\[(.*?)\];", src, re.S):
        rows = re.findall(r'"([^"]*)"', body)
        if rows:
            sprites[name] = rows
    return sprites


def sprite_size(rows):
    return max(len(r) for r in rows), len(rows)


def main():
    pal = load_palette()
    sprites = load_sprites()
    out = REPO / "tools" / "common" / "palette_gba.json"
    out.write_text(json.dumps({k: list(v) for k, v in sorted(pal.items())}, indent=2), encoding="utf-8")
    print(f"paleta: {len(pal)} colores -> {out.relative_to(REPO)}")
    print(f"sprites: {len(sprites)} encontrados")
    for name in sorted(sprites):
        w, h = sprite_size(sprites[name])
        print(f"  {name:20s} {w}x{h}")


if __name__ == "__main__":
    main()
