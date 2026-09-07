"""
story_to_c.py - lleva el texto narrativo del prototipo a datos de C.

Genera source/core/story_data.c a partir de INTRO_STORY, ENDING_STORY,
CREDITS y BOSS_DIALOG de docs/prototipo_referencia.html. Son datos puros,
asi que se parsean directamente: nadie transcribe dialogos a mano, que es
justo donde se cuelan las erratas.

Ver docs/TAREAS_MIGRACION_GBA.md, F7-06 / F7-07 / F7-08.
"""
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from common.prototype import REPO, PROTOTYPE

# Nombre en el prototipo -> BossId de core/boss/boss_config.h.
BOSS_IDS = [
    ("capataz", "BOSS_CAPATAZ"), ("revisor", "BOSS_REVISOR"),
    ("toxico", "BOSS_TOXICO"), ("maton", "BOSS_MATON"),
    ("guardia", "BOSS_GUARDIA"), ("betty", "BOSS_BETTY"),
]


def c_string(text):
    """Literal de C. El texto va en UTF-8 tal cual: la capa de UI lo
    decodifica y dibuja las tildes sin acento (ver pal_gba_text.c)."""
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def find_array(src, name):
    """Devuelve el cuerpo de `const NOMBRE=[...];` del prototipo."""
    m = re.search(r"const %s\s*=\s*\[(.*?)\n\];" % name, src, re.S)
    if not m:
        raise RuntimeError(f"El prototipo no define {name}")
    return m.group(1)


# Nombre de escena del prototipo -> constante de StoryScene (core/dialogue.h).
# Si el prototipo estrena una escena y no esta aca, el generador falla en vez
# de emitir una vineta sin ilustracion: es mas facil enterarse ahora que
# descubrir una pantalla vacia jugando.
SCENES = {
    "street_calm": "SCENE_STREET_CALM",
    "ambush":      "SCENE_AMBUSH",
    "scared":      "SCENE_SCARED",
    "caged":       "SCENE_CAGED",
    "defiant":     "SCENE_DEFIANT",
    "sewer_drag":  "SCENE_SEWER_DRAG",
    "vow":         "SCENE_VOW",
    "betty_fall":  "SCENE_BETTY_FALL",
    "approach":    "SCENE_APPROACH",
    "open":        "SCENE_OPEN",
    "reunion":     "SCENE_REUNION",
    "epilogue":    "SCENE_EPILOGUE",
}


def parse_pages(body):
    """Lee entradas {scene:..., who:..., text:'...'} en orden."""
    pages = []
    for m in re.finditer(r"\{([^{}]*)\}", body, re.S):
        entry = m.group(1)
        who = re.search(r"who\s*:\s*'((?:[^'\\]|\\.)*)'", entry)
        text = re.search(r"text\s*:\s*'((?:[^'\\]|\\.)*)'", entry)
        scene = re.search(r"scene\s*:\s*'([^']*)'", entry)
        if not text:
            continue
        name = scene.group(1) if scene else None
        if name is not None and name not in SCENES:
            raise RuntimeError(
                f"escena '{name}' sin equivalente en SCENES; anadila aca y a "
                "StoryScene en source/core/dialogue.h")
        pages.append((unescape(who.group(1)) if who else None,
                      unescape(text.group(1)),
                      SCENES[name] if name else "SCENE_NONE"))
    return pages


def unescape(s):
    return s.replace("\\'", "'").replace('\\"', '"').replace("\\\\", "\\")


def parse_credits(body):
    """CREDITS son {title, lines:[...]}, con 0, 1 o 2 lineas."""
    out = []
    for m in re.finditer(r"\{\s*title\s*:\s*'((?:[^'\\]|\\.)*)'\s*,\s*lines\s*:\s*\[(.*?)\]\s*\}",
                         body, re.S):
        title = unescape(m.group(1))
        lines = [unescape(x) for x in re.findall(r"'((?:[^'\\]|\\.)*)'", m.group(2))]
        out.append((title, lines))
    return out


def parse_boss_dialog(src):
    m = re.search(r"const BOSS_DIALOG\s*=\s*\{(.*?)\n\};", src, re.S)
    if not m:
        raise RuntimeError("El prototipo no define BOSS_DIALOG")
    body = m.group(1)
    out = {}
    for bm in re.finditer(r"(\w+)\s*:\s*\[(.*?)\]\s*(?:,|\Z)", body, re.S):
        out[bm.group(1)] = parse_pages(bm.group(2))
    return out


def emit_pages(out, ident, pages):
    out.append(f"static const StoryPage {ident}[{len(pages)}] = {{")
    for who, text, scene in pages:
        w = c_string(who) if who else "0"
        out.append(f"    {{ {w}, {c_string(text)}, {scene} }},")
    out.append("};")
    out.append("")


def main():
    src = PROTOTYPE.read_text(encoding="utf-8")
    intro = parse_pages(find_array(src, "INTRO_STORY"))
    ending = parse_pages(find_array(src, "ENDING_STORY"))
    credits = parse_credits(find_array(src, "CREDITS"))
    dialogs = parse_boss_dialog(src)

    missing = [n for n, _ in BOSS_IDS if n not in dialogs]
    if missing:
        raise RuntimeError(f"Sin dialogo para: {missing}")

    out = ["/* ARCHIVO GENERADO por tools/storygen/story_to_c.py - no editar a mano.",
           "   Fuente: docs/prototipo_referencia.html",
           "   (INTRO_STORY, ENDING_STORY, CREDITS y BOSS_DIALOG). */",
           '#include "dialogue.h"', ""]

    emit_pages(out, "INTRO", intro)
    emit_pages(out, "ENDING", ending)

    out.append(f"static const CreditPage CREDITS[{len(credits)}] = {{")
    for title, lines in credits:
        l1 = c_string(lines[0]) if len(lines) > 0 else "0"
        l2 = c_string(lines[1]) if len(lines) > 1 else "0"
        out.append(f"    {{ {c_string(title)}, {l1}, {l2} }},")
    out.append("};")
    out.append("")

    for name, enum in BOSS_IDS:
        emit_pages(out, f"DLG_{name.upper()}", dialogs[name])

    out.append("/* Indexado por BossId: el orden es el del recorrido del juego. */")
    out.append("static const StoryPage *const BOSS_LINES[BOSS_COUNT] = {")
    for name, enum in BOSS_IDS:
        out.append(f"    [{enum}] = DLG_{name.upper()},")
    out.append("};")
    out.append("static const uint8_t BOSS_LINE_COUNT[BOSS_COUNT] = {")
    for name, enum in BOSS_IDS:
        out.append(f"    [{enum}] = {len(dialogs[name])},")
    out.append("};")
    out.append("")

    out += [
        "const StoryPage *story_intro(uint8_t *count) {",
        f"    *count = {len(intro)};",
        "    return INTRO;",
        "}",
        "",
        "const StoryPage *story_ending(uint8_t *count) {",
        f"    *count = {len(ending)};",
        "    return ENDING;",
        "}",
        "",
        "const CreditPage *story_credits(uint8_t *count) {",
        f"    *count = {len(credits)};",
        "    return CREDITS;",
        "}",
        "",
        "const StoryPage *boss_dialogue(BossId id, uint8_t *count) {",
        "    if (id < 0 || id >= BOSS_COUNT) id = BOSS_CAPATAZ;",
        "    *count = BOSS_LINE_COUNT[id];",
        "    return BOSS_LINES[id];",
        "}",
    ]

    dst = REPO / "source" / "core" / "story_data.c"
    dst.write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"intro {len(intro)}, final {len(ending)}, creditos {len(credits)}, "
          f"dialogos {sum(len(v) for v in dialogs.values())} lineas -> "
          f"{dst.relative_to(REPO)}")


if __name__ == "__main__":
    main()
