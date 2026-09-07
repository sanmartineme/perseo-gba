"""
ascii_to_png.py - convierte el arte ASCII del prototipo a PNG indexado.

Entradas:
  - docs/prototipo_referencia.html : sprites SPR.* y paleta COL{}
  - assets/src/**/*.txt            : arte propio, para lo que el prototipo
                                     dibujaba de forma procedural (los tiles
                                     del mundo y las partículas)

Salidas (assets/src/**.png + .json):
  PNG indexado compatible con 4bpp (<=16 colores, índice 0 transparente),
  con la paleta ya cuantizada a 5 bits por canal — o sea, exactamente los
  colores que la GBA puede mostrar. A partir de acá el PNG es la fuente de
  verdad editable (Aseprite/GraphicsGale) y el prototipo deja de mandar:
  este script sólo se vuelve a correr para re-sembrar desde cero.

Ver docs/PLAN_MIGRACION_GBA_C.md, sección 5.
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from common.prototype import (REPO, load_palette, load_sprites, rgb5_to_rgb8,
                              TRANSPARENT_CHARS)

from PIL import Image

MAX_COLORS = 16  # 4bpp: 15 colores + el índice 0 transparente

# Hojas de sprites. Cada una se emite como una tira vertical de frames del
# mismo tamaño, que es lo que espera grit con meta-tiles (-Mw/-Mh) para el
# mapeo 1D de OBJ.
#
# "bank" agrupa hojas que comparten paleta: en 4bpp cada sprite elige un
# banco de 16 colores (ATTR2_PALBANK), así que todo lo que se dibuje con el
# mismo banco tiene que haberse cuantizado contra la MISMA paleta. Por eso
# la paleta se calcula por banco y no por hoja.
#
# "frames" toma los sprites del prototipo; "source" toma el arte de un .txt.
SPRITE_SHEETS = [
    {"name": "perseo/perseo", "bank": "perseo", "size": (16, 16), "frames": [
        "p_idle1", "p_idle2",
        "p_walk1", "p_walk2", "p_walk3", "p_walk4",
        "p_jump", "p_fall", "p_dash",
        "p_atk1", "p_atk2", "p_atk3",
    ]},
    {"name": "enemigos/enemies_16x8", "bank": "enemies", "size": (16, 8), "frames": [
        "rat1", "rat2", "gunner1", "gunner2",
    ]},
    {"name": "enemigos/enemies_8x8", "bank": "enemies", "size": (8, 8), "frames": [
        "roach1", "roach2", "mosq1", "mosq2", "bat1", "bat2",
    ]},
    {"name": "enemigos/enemies_16x16", "bank": "enemies", "size": (16, 16), "frames": [
        "thug1", "thug2", "brute1", "brute2",
    ]},
    # Los jefes van en su propio banco: entre los seis usan 15 colores, que
    # no entrarian junto a los del resto de enemigos en un mismo banco de 16.
    # Cada uno tiene dos frames de marcha, y el orden aca fija el campo
    # `sprite` de BOSS_CONFIGS en core/boss/boss_config.c.
    {"name": "jefes/bosses", "bank": "boss", "size": (32, 32), "frames": [
        "boss_capataz", "boss_capataz_2",
        "boss_revisor", "boss_revisor_2",
        "boss_toxico", "boss_toxico_2",
        "boss_maton", "boss_maton_2",
        "boss_guardia", "boss_guardia_2",
        "boss_betty", "boss_betty_2",
    ]},
    {"name": "fx/fx_8x8", "bank": "fx", "size": (8, 8), "frames": [
        "traza", "traza2", "junkproj", "shock", "heart", "chapa", "slash", "slash2",
    ]},
    {"name": "fx/fx_particles", "bank": "fx", "size": (8, 8),
     "source": "assets/src/sprites/fx/fx_particles.txt"},

    # Decorado interactivo (Fase 7): santuarios, reliquias, lamparas de
    # control, puertas y carteles. Van en su propio banco de paleta porque
    # el de efectos ya esta lleno. Una hoja por tamano de objeto: la GBA
    # solo admite ciertas formas de OBJ, y mezclarlas en una hoja obligaria
    # a calcular offsets a mano.
    {"name": "props/props_16x16", "bank": "props", "size": (16, 16), "frames": [
        "shrine",
    ]},
    # La puerta del prototipo es de 16x24, que no es un tamano valido de
    # OBJ: se declara 16x32 y emit_sheet rellena las 8 filas de abajo con
    # transparente, asi que la posicion en pantalla no cambia.
    {"name": "props/props_16x32", "bank": "props", "size": (16, 32), "frames": [
        "door",
    ]},
    {"name": "props/props_8x16", "bank": "props", "size": (8, 16), "frames": [
        "lamp_off", "lamp_on", "sign",
    ]},
    {"name": "props/props_8x8", "bank": "props", "size": (8, 8), "frames": [
        "relic1", "relic2", "relic3",
    ]},
]

# Arte que el prototipo NO tiene como sprite. El cartel lo dibujaba con
# tres rectangulos sueltos sobre el canvas ([index.html:2310]); aca hace
# falta un sprite de verdad, asi que se reproduce esa misma forma —
# tablero, marca amarilla y poste — con los mismos colores.
EXTRA_ART = {
    "sign": [
        "........",
        "........",
        "NNNNNNNN",
        "NNNNNNNN",
        "NNNYYNNN",
        "NNNYYNNN",
        "NNNYYNNN",
        "NNNNNNNN",
        "NNNNNNNN",
        "...nn...",
        "...nn...",
        "...nn...",
        "...nn...",
        "...nn...",
        "...nn...",
        "...nn...",
    ],
}


def build_palette(char_sets, pal):
    """Asigna índices 1..15 a los caracteres usados; el 0 queda transparente."""
    used = sorted(set().union(*char_sets) - set(TRANSPARENT_CHARS))
    missing = [c for c in used if c not in pal]
    if missing:
        raise RuntimeError(f"Caracteres sin color en COL: {missing}")
    if len(used) + 1 > MAX_COLORS:
        raise RuntimeError(f"{len(used) + 1} colores > {MAX_COLORS} (4bpp)")
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


def parse_block_file(path, size):
    """Lee un .txt de arte: bloques separados por líneas en blanco/comentarios."""
    bw, bh = size
    blocks, cur = [], []
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.rstrip()
        if not line or line.lstrip().startswith("#"):
            if cur:
                blocks.append(cur)
                cur = []
            continue
        cur.append(line)
    if cur:
        blocks.append(cur)
    for i, b in enumerate(blocks):
        if len(b) != bh or any(len(r) != bw for r in b):
            raise RuntimeError(f"{path.name}: el bloque {i} no es de {bw}x{bh}")
    return blocks


def sheet_frames(sheet, sprites):
    """Devuelve [(nombre, filas)] de una hoja, venga del prototipo o de un .txt."""
    if "source" in sheet:
        blocks = parse_block_file(REPO / sheet["source"], sheet["size"])
        base = sheet["name"].split("/")[-1]
        return [(f"{base}_{i}", b) for i, b in enumerate(blocks)]
    out = []
    for fn in sheet["frames"]:
        rows = sprites.get(fn) or EXTRA_ART.get(fn)
        if rows is None:
            raise RuntimeError(f"El prototipo no define SPR.{fn} y no hay arte propio")
        out.append((fn, rows))
    return out


def emit_sheet(sheet, frames, char_index, pal):
    fw, fh = sheet["size"]
    grid = []
    for _, rows in frames:
        # Los frames más chicos que la celda se rellenan con transparente
        # abajo y a la derecha: como ahí no se dibuja nada, la posición del
        # sprite sigue coincidiendo con la de su caja de colisión.
        padded = [r.ljust(fw, ".") for r in rows] + ["." * fw] * (fh - len(rows))
        grid.extend(padded[:fh])
    name = sheet["name"]
    out = REPO / "assets" / "src" / "sprites" / (name + ".png")
    write_indexed_png(out, grid, char_index, pal, fw, fh * len(frames))

    meta = {
        "frame_size": [fw, fh],
        "bank": sheet["bank"],
        "frames": [n for n, _ in frames],
        "palette": {"0": "transparente", **{str(i): c for c, i in char_index.items()}},
    }
    out.with_suffix(".json").write_text(json.dumps(meta, indent=2), encoding="utf-8")
    print(f"  {out.relative_to(REPO)}  {fw}x{fh} x{len(frames)} frames")
    return meta


def emit_tileset(txt_path, pal):
    tiles = parse_block_file(txt_path, (8, 8))
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
    return meta, char_index


# ---------------------------------------------------------------------
# Paletas de nivel
# ---------------------------------------------------------------------
# Cada nivel del prototipo tiene su `theme` — dos colores, el ladrillo y
# su sombra — y con eso cambia de aspecto entero: cloacas grises, chatarra
# marron, estacion verdosa, veneno, tierra, mercado, trono.
#
# Aca NO se genera un tileset por nivel. El dibujo de los once tiles es el
# mismo en los siete; lo unico que cambia son dos colores. Asi que se
# emite una tabla de siete paletas de 16 colores y la capa de video
# intercambia la paleta de fondo al cargar un nivel: 32 bytes copiados una
# vez por nivel, en vez de siete tilesets ocupando VRAM y ROM.
#
# El tileset esta dibujado con el tema del Nivel 1 ('T' ladrillo, 't'
# sombra), asi que esos dos indices son los que se sustituyen.
BRICK_CHAR, BRICK_DARK_CHAR = "T", "t"


def emit_level_palettes(pal, char_index):
    levels = sorted((REPO / "assets" / "src" / "levels").glob("*.json"))
    if not levels:
        return
    rows = []
    for path in levels:
        spec = json.loads(path.read_text(encoding="utf-8"))
        theme = spec.get("theme", {})
        colors = [0] * 16
        for ch, idx in char_index.items():
            colors[idx] = rgb15(pal[ch])
        for key, ch in (("brick", BRICK_CHAR), ("brickD", BRICK_DARK_CHAR)):
            if theme.get(key) and ch in char_index:
                if theme[key] not in pal:
                    raise RuntimeError(f"{path.name}: color de tema desconocido {theme[key]!r}")
                colors[char_index[ch]] = rgb15(pal[theme[key]])
        rows.append((spec.get("id", path.stem), spec.get("name", ""), theme, colors))

    out = [
        "/* ARCHIVO GENERADO por tools/spritegen/ascii_to_png.py - no editar a mano.",
        "   Fuente: el campo `theme` de cada assets/src/levels/*.json y la",
        "   paleta COL{} del prototipo.",
        "",
        "   Un tileset, siete paletas. Los once tiles se dibujan igual en",
        "   todos los niveles; lo unico que cambia entre zonas son el color",
        "   del ladrillo y el de su sombra, asi que en vez de siete tilesets",
        "   se intercambia la paleta de fondo al cargar el nivel. */",
        "#include <stdint.h>",
        "",
        f"#define LEVEL_PALETTE_COUNT {len(rows)}",
        "",
        "const uint16_t LEVEL_PALETTES[LEVEL_PALETTE_COUNT][16] = {",
    ]
    for ident, name, theme, colors in rows:
        out.append(f"    /* {name} — ladrillo '{theme.get('brick', '?')}',"
                   f" sombra '{theme.get('brickD', '?')}' */")
        out.append("    { " + ", ".join(f"0x{c:04X}" for c in colors) + " },")
    out.append("};")

    dst = REPO / "source" / "platform" / "gba" / "level_palettes.c"
    dst.write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"  {dst.relative_to(REPO)}  {len(rows)} paletas de nivel")


def rgb15(c):
    r, g, b = c
    return (r & 31) | ((g & 31) << 5) | ((b & 31) << 10)


def main():
    pal = load_palette()
    sprites = load_sprites()

    # La paleta se calcula sobre TODAS las hojas de un mismo banco a la vez,
    # para que compartan índices y puedan usar el mismo ATTR2_PALBANK.
    loaded = [(sh, sheet_frames(sh, sprites)) for sh in SPRITE_SHEETS]
    banks = {}
    for sh, frames in loaded:
        chars = set()
        for _, rows in frames:
            chars |= {c for row in rows for c in row}
        banks.setdefault(sh["bank"], set()).update(chars)
    bank_index = {b: build_palette([chars], pal) for b, chars in banks.items()}
    for b, idx in sorted(bank_index.items()):
        print(f"Banco de paleta '{b}': {len(idx) + 1} colores")

    print("Sprites:")
    for sh, frames in loaded:
        emit_sheet(sh, frames, bank_index[sh["bank"]], pal)

    print("Tilesets:")
    for txt in sorted((REPO / "assets" / "src" / "tiles").glob("*.txt")):
        _meta, tile_chars = emit_tileset(txt, pal)
        emit_level_palettes(pal, tile_chars)


if __name__ == "__main__":
    main()
