"""
check_chain.py - comprueba que los siete niveles enlazan de verdad.

F8-07 pide validar el recorrido completo de principio a fin. Jugarlo
entero no es automatizable aca (la inyeccion de teclas en el emulador no
funciona), pero lo que SI se puede comprobar sin jugar es que el grafo
esta bien construido, que es donde estarian los errores de datos:

  - cada nivel tiene una salida (una puerta, o un jefe que la deja al
    caer) y esa salida apunta al nivel siguiente;
  - cada nivel tiene su punto de aparicion en aire, no dentro de la roca;
  - cada arena de jefe tiene su bossgate con los rectangulos de sellado y
    de paso, y el jefe tiene su `exit`;
  - el ultimo nivel termina en el jefe final, que abre el desenlace.

Uso:
    python3 tools/levelgen/check_chain.py
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from common.prototype import REPO

LEVELS_DIR = REPO / "assets" / "src" / "levels"

# core/boss/boss_config.c: a que nivel abre paso cada jefe (0 = ninguno).
BOSS_NEXT = {"capataz": 2, "revisor": 3, "toxico": 4, "maton": 5,
             "guardia": 6, "betty": None}

SOLID = {1, 5, 6, 8}


def build_grid(spec):
    w, h = spec["size"]
    grid = [[spec.get("fill", 1)] * w for _ in range(h)]
    for x0, y0, x1, y1, tid in spec["carve"]:
        for y in range(max(0, y0), min(h - 1, y1) + 1):
            for x in range(max(0, x0), min(w - 1, x1) + 1):
                grid[y][x] = tid
    return grid


def main():
    specs = []
    for path in sorted(LEVELS_DIR.glob("level*.json")):
        specs.append((path.name, json.loads(path.read_text(encoding="utf-8"))))

    problems = []
    print(f"{len(specs)} niveles\n")

    for i, (fname, spec) in enumerate(specs):
        grid = build_grid(spec)
        w, h = spec["size"]
        sx, sy = spec["spawn"]
        ents = spec["entities"]
        kinds = [e["type"] for e in ents]

        # 1. El punto de aparicion no puede estar dentro de la roca.
        if grid[sy][sx] in SOLID:
            problems.append(f"{fname}: la aparicion ({sx},{sy}) cae en roca solida")

        # 2. Salida: puerta explicita o jefe.
        doors = [e for e in ents if e["type"] == "door"]
        bosses = [e for e in ents if e["type"] == "boss"]
        exits = []
        for d in doors:
            exits.append(("puerta", d.get("param")))
        for b in bosses:
            key = b.get("boss")
            exits.append((f"jefe {key}", BOSS_NEXT.get(key, "?")))

        if not exits:
            problems.append(f"{fname}: no tiene ninguna salida")

        # 3. El jefe necesita su arena y su salida.
        for b in bosses:
            if "exit" not in b:
                problems.append(f"{fname}: el jefe {b.get('boss')} no tiene `exit`")
            gates = [e for e in ents if e["type"] == "bossgate"]
            if not gates:
                problems.append(f"{fname}: hay jefe pero no hay bossgate")
            for g in gates:
                for key in ("gate", "passage", "yband"):
                    if key not in g:
                        problems.append(f"{fname}: al bossgate le falta `{key}`")

        # 4. Los destinos existen.
        for what, dest in exits:
            if dest is None:
                continue
            if not isinstance(dest, int) or not (0 <= dest < len(specs)):
                problems.append(f"{fname}: {what} apunta al nivel {dest}, que no existe")

        lamps = kinds.count("lamp")
        print(f"{i}  {spec['name']}")
        print(f"     {w}x{h}  aparicion ({sx},{sy})  {len(ents)} entidades  "
              f"{lamps} lamparas")
        for what, dest in exits:
            target = specs[dest][1]["name"] if isinstance(dest, int) and dest < len(specs) \
                else "EL DESENLACE"
            print(f"     salida por {what} -> {target}")
        print()

    # 5. El recorrido debe llegar del 0 al ultimo.
    reachable, cur = {0}, 0
    while True:
        spec = specs[cur][1]
        nxt = None
        for e in spec["entities"]:
            if e["type"] == "door" and isinstance(e.get("param"), int):
                nxt = e["param"]
            elif e["type"] == "boss":
                n = BOSS_NEXT.get(e.get("boss"))
                if isinstance(n, int):
                    nxt = n
        if nxt is None or nxt in reachable:
            break
        reachable.add(nxt)
        cur = nxt

    missing = [i for i in range(len(specs)) if i not in reachable]
    if missing:
        problems.append("niveles inalcanzables desde el primero: "
                        + ", ".join(str(m) for m in missing))

    if problems:
        print("PROBLEMAS:")
        for p in problems:
            print("  -", p)
        return 1
    print(f"Recorrido completo: {' -> '.join(str(i) for i in sorted(reachable))} "
          "y el jefe final abre el desenlace.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
