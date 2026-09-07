"""
levelgen.py - convierte los niveles declarativos (.json) en datos de C.

Uso:
    python3 tools/levelgen/levelgen.py assets/src/levels/ source/core/level/

Por cada nivel emite un par .h/.c con el tilemap ya "horneado" (1 byte por
tile, que el compilador coloca en ROM) y la lista de entidades iniciales.
El formato de entrada esta documentado en tools/levelgen/schema_nivel.md.

Los archivos generados NO se editan a mano: se regeneran desde el .json.
"""
import json
import sys
from pathlib import Path

# Nombres de E() en el prototipo -> enum EntityType de core/entity.h
ENTITY_TYPES = {
    "sign": "ENT_SIGN", "lamp": "ENT_LAMP", "door": "ENT_DOOR", "vdoor": "ENT_VDOOR",
    "shrine": "ENT_SHRINE", "bossgate": "ENT_BOSSGATE", "boss": "ENT_BOSS",
    "chapa": "ENT_CHAPA", "relic": "ENT_RELIC", "hp": "ENT_HP",
    "rat": "ENT_RAT", "roach": "ENT_ROACH", "mosq": "ENT_MOSQ", "gunner": "ENT_GUNNER",
    "bat": "ENT_BAT", "thug": "ENT_THUG", "brute": "ENT_BRUTE",
}

# Nombre de jefe -> enum BossId de core/boss/boss_config.h. El orden es el
# del recorrido del juego, igual que BOSS_CONFIG en el prototipo.
BOSS_IDS = {
    "capataz": "BOSS_CAPATAZ", "revisor": "BOSS_REVISOR", "toxico": "BOSS_TOXICO",
    "maton": "BOSS_MATON", "guardia": "BOSS_GUARDIA", "betty": "BOSS_BETTY",
}

BANNER = ("/* ARCHIVO GENERADO por tools/levelgen/levelgen.py — no editar a mano.\n"
          "   Fuente: {src}\n"
          "   Ver tools/levelgen/schema_nivel.md */\n")



# El JSON nombra la cancion como en SONGS{} del prototipo; el motor la
# quiere como SongId. La tabla vive aca y no en el JSON para que los
# niveles no tengan que conocer los identificadores de C.
SONG_ENUM = {
    "title": "SONG_TITLE", "lvl1": "SONG_LVL1", "lvl2": "SONG_LVL2",
    "lvl3": "SONG_LVL3", "lvl4": "SONG_LVL4", "lvl5": "SONG_LVL5",
    "lvl6": "SONG_LVL6", "lvl7": "SONG_LVL7", "boss": "SONG_BOSS",
    "bossFinal": "SONG_BOSS_FINAL", "end": "SONG_END",
}



def c_string(text):
    """Literal de C. El texto va en UTF-8 tal cual: la capa de UI decodifica
    y dibuja las vocales acentuadas sin tilde (ver pal_gba_text.c), asi que
    los .json se siguen escribiendo en espanol normal."""
    out = text.replace("\\", "\\\\").replace('"', '\\"')
    return '"' + out + '"'


def song_enum(name):
    if not name:
        return "SONG_NONE"
    if name not in SONG_ENUM:
        raise RuntimeError(f"Cancion desconocida en el nivel: {name}")
    return SONG_ENUM[name]


def build_tiles(spec):
    w, h = spec["size"]
    fill = spec.get("fill", 1)
    grid = [[fill] * w for _ in range(h)]
    for x0, y0, x1, y1, tid in spec["carve"]:
        for y in range(max(0, y0), min(h - 1, y1) + 1):
            row = grid[y]
            for x in range(max(0, x0), min(w - 1, x1) + 1):
                row[x] = tid
    return grid


def emit(spec, src_path, out_dir):
    ident = spec["id"]
    w, h = spec["size"]
    grid = build_tiles(spec)

    ents = []
    for e in spec.get("entities", []):
        kind = e["type"]
        if kind not in ENTITY_TYPES:
            raise RuntimeError(f"{src_path.name}: tipo de entidad desconocido '{kind}'")
        tx, ty = e["at"]

        param = e.get("param", 0)
        if kind == "boss":
            boss = e.get("boss")
            if boss not in BOSS_IDS:
                raise RuntimeError(f"{src_path.name}: jefe desconocido '{boss}'")
            param = BOSS_IDS[boss]
        else:
            param = str(int(param))

        gate = e.get("gate", [0, 0, 0, 0])
        passage = e.get("passage", [0, 0, 0, 0])
        yband = e.get("yband", [0, 0])
        exit_at = e.get("exit", [0, 0])
        ents.append((ENTITY_TYPES[kind], tx, ty, param, gate, passage, yband, exit_at))

    header = out_dir / f"{ident}.h"
    source = out_dir / f"{ident}.c"
    banner = BANNER.format(src=src_path.as_posix())

    guard = f"PERSEO_{ident.upper()}_H"
    header.write_text(
        f"{banner}#ifndef {guard}\n#define {guard}\n\n"
        f'#include "level.h"\n\n'
        f"/* {spec['name']} — {w}x{h} tiles */\n"
        f"extern const Level {ident};\n\n"
        f"#endif /* {guard} */\n",
        encoding="utf-8")

    lines = [banner, '#include "level.h"', '#include "../boss/boss_config.h"', '#include "../audio.h"', "",
             f"static const uint8_t {ident}_tiles[{w * h}] = {{"]
    flat = [v for row in grid for v in row]
    for i in range(0, len(flat), 32):
        lines.append("    " + ",".join(str(v) for v in flat[i:i + 32]) + ",")
    lines.append("};")
    lines.append("")

    if ents:
        lines.append(f"static const LevelEntitySpawn {ident}_entities[{len(ents)}] = {{")
        for kind, tx, ty, param, gate, passage, yband, exit_at in ents:
            g = ", ".join(str(v) for v in gate)
            pa = ", ".join(str(v) for v in passage)
            yb = ", ".join(str(v) for v in yband)
            lines.append(f"    {{ {kind}, {tx}, {ty}, {param}, "
                         f"{{ {g} }}, {{ {pa} }}, {{ {yb} }}, {exit_at[0]}, {exit_at[1]} }},")
        lines.append("};")
        ent_ref, ent_count = f"{ident}_entities", len(ents)
    else:
        ent_ref, ent_count = "0", 0
    lines.append("")

    story = spec.get("story", [])
    if story:
        lines.append(f"static const char *const {ident}_story[{len(story)}] = {{")
        for page in story:
            lines.append(f"    {c_string(page)},")
        lines.append("};")
        lines.append("")
        story_ref, story_count = f"{ident}_story", len(story)
    else:
        story_ref, story_count = "0", 0

    sx, sy = spec["spawn"]
    lines += [
        f"const Level {ident} = {{",
        f"    .w = {w}, .h = {h},",
        f"    .tiles = {ident}_tiles,",
        f"    .spawn_tx = {sx}, .spawn_ty = {sy},",
        f"    .entities = {ent_ref}, .entity_count = {ent_count},",
        f"    .song = {song_enum(spec.get('song'))},",
        f"    .name = {c_string(spec['name'])},",
        f"    .story = {story_ref}, .story_count = {story_count},",
        "};",
        "",
    ]
    source.write_text("\n".join(lines), encoding="utf-8")

    solid = sum(1 for v in flat if v in (1, 5, 6, 8))
    print(f"  {ident}: {w}x{h} = {w*h} tiles ({solid} solidos), "
          f"{len(ents)} entidades -> {header.name} + {source.name}")


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 1
    src_dir, out_dir = Path(sys.argv[1]), Path(sys.argv[2])
    out_dir.mkdir(parents=True, exist_ok=True)
    specs = sorted(src_dir.glob("*.json"))
    if not specs:
        print(f"No hay niveles en {src_dir}")
        return 1
    print(f"Niveles ({len(specs)}):")
    for p in specs:
        emit(json.loads(p.read_text(encoding="utf-8")), p, out_dir)
    return 0


if __name__ == "__main__":
    sys.exit(main())
