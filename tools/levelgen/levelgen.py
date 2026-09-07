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

BANNER = ("/* ARCHIVO GENERADO por tools/levelgen/levelgen.py — no editar a mano.\n"
          "   Fuente: {src}\n"
          "   Ver tools/levelgen/schema_nivel.md */\n")


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
        ents.append((ENTITY_TYPES[kind], tx, ty, int(e.get("param", 0))))

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

    lines = [banner, '#include "level.h"', "", f"static const uint8_t {ident}_tiles[{w * h}] = {{"]
    flat = [v for row in grid for v in row]
    for i in range(0, len(flat), 32):
        lines.append("    " + ",".join(str(v) for v in flat[i:i + 32]) + ",")
    lines.append("};")
    lines.append("")

    if ents:
        lines.append(f"static const LevelEntitySpawn {ident}_entities[{len(ents)}] = {{")
        for kind, tx, ty, param in ents:
            lines.append(f"    {{ {kind}, {tx}, {ty}, {param} }},")
        lines.append("};")
        ent_ref, ent_count = f"{ident}_entities", len(ents)
    else:
        ent_ref, ent_count = "0", 0
    lines.append("")

    sx, sy = spec["spawn"]
    lines += [
        f"const Level {ident} = {{",
        f"    .w = {w}, .h = {h},",
        f"    .tiles = {ident}_tiles,",
        f"    .spawn_tx = {sx}, .spawn_ty = {sy},",
        f"    .entities = {ent_ref}, .entity_count = {ent_count},",
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
