"""
prototype_to_json.py - extrae los niveles del prototipo a .json.

Los siete niveles del prototipo son CODIGO: buildLevel1()..buildLevel7(),
una serie de llamadas carve() y E(). Transcribir cinco de ellos a mano
son ~250 rectangulos y ~200 entidades, con la coordenada equivocada
esperando en cada linea; y un nivel mal copiado no falla al compilar,
falla cuando alguien no puede pasar de un salto.

Asi que se extraen. Los constructores son deliberadamente regulares
(sin condicionales ni variables intermedias), asi que alcanza con
reconocer cinco formas:

    const L = newLevel(w, h, 'nombre', {brick:'X', brickD:'Y'})
    L.song = '...';  L.spawn = [tx, ty];  L.story = [ "...", ... ]
    carve(L, x0, y0, x1, y1 [, id])
    E(L, 'tipo', tx, ty [, {props}])
    [[x,y], ...].forEach(c => E(L, 'tipo', c[0], c[1]))

Cualquier otra cosa dentro de un constructor es un error explicito: si
el prototipo cambia y aparece un bucle, es mejor enterarse aca que
generar un nivel al que le falta medio pasillo.

Uso:
    python3 tools/levelgen/prototype_to_json.py            # los 7
    python3 tools/levelgen/prototype_to_json.py 3 4 5 6 7  # solo esos

Comprobacion: los niveles 1 y 2 ya estaban escritos a mano, asi que el
extractor se valida contra ellos con --check antes de confiar en el
resto.
"""
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from common.prototype import REPO, PROTOTYPE

OUT_DIR = REPO / "assets" / "src" / "levels"

# Nombre de archivo de cada nivel, en orden.
LEVEL_FILES = [
    "level01.json", "level02.json", "level03.json", "level04.json",
    "level05.json", "level06.json", "level07.json",
]
# Identificador C de cada nivel (lo consume levelgen.py).
LEVEL_IDS = [
    "level01_tuneles", "level02_vertedero", "level03_estacion",
    "level04_residuos", "level05_madriguera", "level06_mercado",
    "level07_trono",
]

# Propiedades de E() renombradas al esquema del .json.
PROP_RENAME = {
    "to": "param",      # puerta: nivel destino
    "key": "boss",      # jefe
    "ab": "ability",    # santuario
    "rid": "relic",     # reliquia
    "dir0": "param",    # enemigo: direccion inicial
}
# Propiedades del prototipo que no se portan.
PROP_DROP = {"icon"}    # el sprite lo decide la capa de video por el tipo


def strip_comments(src):
    """Quita los comentarios // sin tocar los que van dentro de cadenas."""
    out = []
    for line in src.splitlines():
        res, i, quote = [], 0, None
        while i < len(line):
            ch = line[i]
            if quote:
                if ch == "\\":
                    res.append(line[i:i + 2]); i += 2; continue
                if ch == quote:
                    quote = None
            elif ch in "'\"":
                quote = ch
            elif ch == "/" and i + 1 < len(line) and line[i + 1] == "/":
                break
            res.append(ch); i += 1
        out.append("".join(res))
    return "\n".join(out)


def js_value(text):
    """Convierte un literal JS simple (numero, cadena, array) a Python."""
    text = text.strip()
    if re.fullmatch(r"-?\d+", text):
        return int(text)
    if text.startswith("'") or text.startswith('"'):
        return unquote(text)
    if text.startswith("["):
        return [js_value(p) for p in split_top(text[1:-1])]
    raise RuntimeError(f"No se sabe leer el valor JS: {text!r}")


def unescape(text):
    """Quita las barras de escape de una cadena ya SIN comillas."""
    return re.sub(r"\\(.)", r"\1", text)


def unquote(text):
    q = text[0]
    body = text[1:-1]
    return re.sub(r"\\(.)", r"\1", body) if q in "'\"" else body


def split_top(text):
    """Parte por comas del nivel superior (respeta [], {} y cadenas)."""
    parts, depth, quote, cur = [], 0, None, []
    for i, ch in enumerate(text):
        if quote:
            cur.append(ch)
            if ch == "\\":
                continue
            if ch == quote:
                quote = None
            continue
        if ch in "'\"":
            quote = ch; cur.append(ch); continue
        if ch in "[{(":
            depth += 1
        elif ch in "]})":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append("".join(cur)); cur = []
            continue
        cur.append(ch)
    if "".join(cur).strip():
        parts.append("".join(cur))
    return [p.strip() for p in parts if p.strip()]


def parse_props(text):
    """Lee {a:1, b:'x', c:[1,2]} a un dict."""
    out = {}
    for part in split_top(text.strip()[1:-1]):
        key, _, val = part.partition(":")
        out[key.strip()] = js_value(val)
    return out


def find_builder(src, n):
    m = re.search(r"function buildLevel%d\(\)\s*\{(.*?)\n\}" % n, src, re.S)
    if not m:
        raise RuntimeError(f"No se encontro buildLevel{n}()")
    return m.group(1)


def parse_builder(body):
    body = strip_comments(body)
    level = {"entities": [], "carve": []}

    m = re.search(r"newLevel\((\d+),(\d+),\s*'((?:[^'\\]|\\.)*)'\s*,\s*(\{[^}]*\})\)", body)
    if not m:
        raise RuntimeError("newLevel() no reconocido")
    level["name"] = unquote("'" + m.group(3) + "'")
    level["size"] = [int(m.group(1)), int(m.group(2))]
    level["fill"] = 1
    theme = parse_props(m.group(4))
    level["theme"] = {"brick": theme["brick"], "brickD": theme["brickD"]}

    m = re.search(r"L\.song\s*=\s*'(\w+)'", body)
    level["song"] = m.group(1) if m else None
    m = re.search(r"L\.spawn\s*=\s*\[(\d+),(\d+)\]", body)
    level["spawn"] = [int(m.group(1)), int(m.group(2))] if m else [4, 26]
    m = re.search(r"L\.story\s*=\s*\[(.*?)\];", body, re.S)
    if m:
        level["story"] = [unescape(x) for x in
                          re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(1))]

    # Rectangulos y entidades, EN ORDEN: carve() se pisa a si mismo, asi
    # que reordenarlos cambiaria el mapa.
    consumed = []
    for m in re.finditer(r"carve\(L,([^)]*)\)", body):
        args = [int(a) for a in split_top(m.group(1))]
        if len(args) == 4:
            args.append(0)      # el id por defecto de carve() es 0
        if len(args) != 5:
            raise RuntimeError(f"carve() con {len(args)} argumentos: {m.group(0)}")
        level["carve"].append(args)
        consumed.append(m.span())

    # Las entidades se recogen con su posicion en el archivo y se ordenan
    # por ella: las tandas de chapas ([[x,y],...].forEach) estan
    # intercaladas entre las E() sueltas, y respetar el orden del
    # original es lo que hace que el resultado sea comparable con los dos
    # niveles que ya estaban escritos a mano.
    found = []
    for m in re.finditer(r"E\(L,'(\w+)',\s*(\d+),\s*(\d+)\s*(?:,\s*(\{.*?\}))?\)", body):
        ent = {"type": m.group(1), "at": [int(m.group(2)), int(m.group(3))]}
        if m.group(4):
            for k, v in parse_props(m.group(4)).items():
                if k in PROP_DROP:
                    continue
                ent[PROP_RENAME.get(k, k)] = v
        found.append((m.start(), ent))
        consumed.append(m.span())

    for m in re.finditer(r"\[(\[[^\]]*\](?:\s*,\s*\[[^\]]*\])*)\]\s*\n?\s*\.forEach\("
                         r"\w+\s*=>\s*E\(L,'(\w+)',\s*\w+\[0\],\s*\w+\[1\]\)\)", body, re.S):
        kind = m.group(2)
        for i, pair in enumerate(re.findall(r"\[(\d+),(\d+)\]", m.group(1))):
            found.append((m.start() + i,
                          {"type": kind, "at": [int(pair[0]), int(pair[1])]}))
        consumed.append(m.span())

    found.sort(key=lambda t: t[0])
    level["entities"] = [e for _, e in found]

    check_nothing_left(body, consumed)
    return level


def check_nothing_left(body, consumed):
    """Se asegura de que no quedo codigo del constructor sin interpretar."""
    mask = bytearray(len(body))
    for a, b in consumed:
        for i in range(a, b):
            mask[i] = 1
    rest = "".join(c for i, c in enumerate(body) if not mask[i])
    # Lo que puede quedar: las asignaciones ya leidas, el return y espacios.
    rest = re.sub(r"const L\s*=\s*newLevel\([^;]*;", "", rest, flags=re.S)
    rest = re.sub(r"L\.(song|spawn)\s*=\s*[^;]*;", "", rest)
    rest = re.sub(r"L\.story\s*=\s*\[.*?\];", "", rest, flags=re.S)
    rest = re.sub(r"return L;", "", rest)
    rest = re.sub(r"\.forEach\(\w+\s*=>\s*\)", "", rest)
    leftover = re.sub(r"[\s;]+", "", rest)
    if leftover:
        raise RuntimeError("Codigo del constructor sin interpretar:\n" + leftover[:400])


def emit(level, index, out_path):
    doc = {"id": LEVEL_IDS[index], "name": level["name"], "size": level["size"],
           "fill": level["fill"], "spawn": level["spawn"], "theme": level["theme"]}
    if level["song"]:
        doc["song"] = level["song"]
    if "story" in level:
        doc["story"] = level["story"]
    doc["carve"] = level["carve"]
    doc["entities"] = level["entities"]
    out_path.write_text(json.dumps(doc, ensure_ascii=False, indent=2) + "\n",
                        encoding="utf-8")
    return doc


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    check = "--check" in sys.argv
    wanted = [int(a) for a in args] if args else list(range(1, 8))

    src = PROTOTYPE.read_text(encoding="utf-8")
    for n in wanted:
        level = parse_builder(find_builder(src, n))
        out = OUT_DIR / LEVEL_FILES[n - 1]
        if check:
            old = json.loads(out.read_text(encoding="utf-8")) if out.exists() else None
            new = {"id": LEVEL_IDS[n - 1], "name": level["name"],
                   "size": level["size"], "fill": level["fill"],
                   "spawn": level["spawn"], "theme": level["theme"]}
            if level["song"]:
                new["song"] = level["song"]
            if "story" in level:
                new["story"] = level["story"]
            new["carve"] = level["carve"]
            new["entities"] = level["entities"]
            same = old == new
            print(f"nivel {n}: {'IGUAL' if same else 'DISTINTO'} a {out.name}")
            if not same and old:
                for key in new:
                    if old.get(key) != new[key]:
                        print(f"    difiere en '{key}'")
                        if key == "entities":
                            oe = old.get(key, [])
                            for i in range(max(len(oe), len(new[key]))):
                                a = oe[i] if i < len(oe) else None
                                b = new[key][i] if i < len(new[key]) else None
                                if a != b:
                                    print(f"      [{i}] json={a}")
                                    print(f"           proto={b}")
            continue
        emit(level, n - 1, out)
        print(f"nivel {n}: {level['name']} — {len(level['carve'])} rectangulos, "
              f"{len(level['entities'])} entidades -> {out.name}")


if __name__ == "__main__":
    main()
