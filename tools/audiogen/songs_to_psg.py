"""
songs_to_psg.py - traduce la música del prototipo a datos para los
canales PSG de la GBA.

Genera source/core/audio_data.c a partir de:
  - SONGS{} de docs/prototipo_referencia.html, que son datos puros
    (patrones de notas en Hz) y se parsean directamente.
  - Los efectos, que en el prototipo son CÓDIGO (llamadas a tone() y
    noise() encadenadas con setTimeout), y por lo tanto están
    transcritos a mano más abajo, uno a uno.

Ver docs/PLAN_MIGRACION_GBA_C.md, sección 5.
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from common.prototype import REPO, PROTOTYPE

WAVES = {"square": "WAVE_SQUARE", "triangle": "WAVE_TRIANGLE",
         "sine": "WAVE_SINE", "sawtooth": "WAVE_SAW"}

# Nombre en el prototipo -> enum SongId. El orden es el de core/audio.h.
SONG_IDS = [
    ("title", "SONG_TITLE"), ("lvl1", "SONG_LVL1"), ("lvl2", "SONG_LVL2"),
    ("lvl3", "SONG_LVL3"), ("lvl4", "SONG_LVL4"), ("lvl5", "SONG_LVL5"),
    ("lvl6", "SONG_LVL6"), ("lvl7", "SONG_LVL7"),
    ("boss", "SONG_BOSS"), ("bossFinal", "SONG_BOSS_FINAL"), ("end", "SONG_END"),
]


def gain_to_psg(gain):
    """Las ganancias de Web Audio (0.026..0.09) al rango 0..15 del PSG."""
    return max(1, min(15, round(gain * 160)))


def secs_to_frames(sec):
    return max(1, min(255, round(sec * 60)))


def parse_songs():
    src = PROTOTYPE.read_text(encoding="utf-8")
    block = re.search(r"const SONGS\s*=\s*\{(.*?)\n\};", src, re.S).group(1)
    songs = {}
    # Cada entrada es "nombre:{...}" y puede ocupar varias líneas.
    for m in re.finditer(r"(\w+)\s*:\s*\{(.*?)\}(?=\s*,\s*\n\s*(?:\w+\s*:|//)|\s*\n\};|\s*,?\s*$)",
                         block, re.S):
        name, body = m.group(1), m.group(2)
        song = {}
        song["bpm"] = int(re.search(r"bpm\s*:\s*(\d+)", body).group(1))
        for key, default in (("waveBass", "triangle"), ("waveLead", "square")):
            mm = re.search(key + r"\s*:\s*'(\w+)'", body)
            song[key] = mm.group(1) if mm else default
        for key, default in (("volBass", 0.05), ("volLead", 0.028)):
            mm = re.search(key + r"\s*:\s*([\d.]+)", body)
            song[key] = float(mm.group(1)) if mm else default
        song["echo"] = "echo:true" in body.replace(" ", "")
        for key in ("bass", "lead", "drum"):
            mm = re.search(key + r"\s*:\s*\[([^\]]*)\]", body)
            if mm:
                song[key] = [max(0, int(v)) for v in mm.group(1).split(",")]
        songs[name] = song
    return songs


# ---------------------------------------------------------------------
# Efectos, transcritos a mano de SFX{} del prototipo. Formato de paso:
#   (freq, slide_to, delay_seg, dur_seg, gain, wave)
# y por efecto, además, (dur_ruido_seg, gain_ruido) o None.
# ---------------------------------------------------------------------
SFX = {
    "SFX_JUMP":      ([(220, 440, 0, 0.10, 0.05, "square")], None),
    "SFX_DJUMP":     ([(330, 660, 0, 0.12, 0.05, "square")], None),
    "SFX_DASH":      ([(150, 80, 0, 0.10, 0.04, "sawtooth")], (0.12, 0.06)),
    "SFX_ATK":       ([(700, 300, 0, 0.06, 0.04, "square")], None),
    "SFX_TRAZA":     ([(300, 140, 0, 0.10, 0.05, "triangle")], (0.06, 0.035)),
    "SFX_SPLAT":     ([(120, 60, 0, 0.14, 0.05, "square")], (0.16, 0.075)),
    "SFX_HIT_ENEMY": ([(180, 90, 0, 0.08, 0.05, "square")], (0.08, 0.07)),
    "SFX_HURT":      ([(140, 60, 0, 0.25, 0.07, "sawtooth")], (0.15, 0.06)),
    "SFX_PICK":      ([(880, 0, 0, 0.07, 0.05, "square"),
                       (1320, 0, 0.07, 0.10, 0.05, "square")], None),
    # Arpegio de cuatro notas: es el sonido de desbloquear una habilidad,
    # el momento mas "premio" del juego, asi que se conservan las cuatro.
    "SFX_ABILITY":   ([(440, 0, 0.00, 0.18, 0.06, "square"),
                       (554, 0, 0.11, 0.18, 0.06, "square"),
                       (659, 0, 0.22, 0.18, 0.06, "square"),
                       (880, 0, 0.33, 0.18, 0.06, "square")], None),
    "SFX_BREAK":     ([(100, 50, 0, 0.15, 0.06, "square")], (0.20, 0.09)),
    "SFX_DOOR":      ([(110, 55, 0, 0.40, 0.05, "square")], (0.30, 0.04)),
    "SFX_CHECK":     ([(523, 0, 0.00, 0.10, 0.06, "triangle"),
                       (784, 0, 0.10, 0.15, 0.06, "triangle")], None),
    "SFX_BOSS":      ([(80, 40, 0, 0.50, 0.09, "sawtooth")], (0.40, 0.07)),
    "SFX_DENY":      ([(160, 120, 0, 0.08, 0.05, "square")], None),
}

SFX_ORDER = ["SFX_JUMP", "SFX_DJUMP", "SFX_DASH", "SFX_ATK", "SFX_TRAZA",
             "SFX_SPLAT", "SFX_HIT_ENEMY", "SFX_HURT", "SFX_PICK",
             "SFX_ABILITY", "SFX_BREAK", "SFX_DOOR", "SFX_CHECK",
             "SFX_BOSS", "SFX_DENY"]


def main():
    songs = parse_songs()
    missing = [n for n, _ in SONG_IDS if n not in songs]
    if missing:
        raise RuntimeError(f"El prototipo no define estas canciones: {missing}")

    out = ["/* ARCHIVO GENERADO por tools/audiogen/songs_to_psg.py — no editar a mano.",
           "   Fuente: docs/prototipo_referencia.html (SONGS y SFX).",
           "",
           "   Las notas van en Hz tal como estaban en el prototipo; la capa de",
           "   audio de la GBA las convierte al registro de frecuencia de cada",
           "   canal. Los volumenes ya vienen convertidos de la ganancia de Web",
           "   Audio (0.026..0.09) al rango 0..15 del PSG. */",
           '#include "audio.h"', ""]

    for name, _ in SONG_IDS:
        s = songs[name]
        for track in ("bass", "lead", "drum"):
            if track in s:
                vals = ", ".join(str(v) for v in s[track])
                ctype = "uint16_t" if track != "drum" else "uint8_t"
                out.append(f"static const {ctype} {name}_{track}[] = {{ {vals} }};")
    out.append("")

    out.append("static const Song SONGS[SONG_COUNT] = {")
    out.append("    [SONG_NONE] = { 0 },")
    for name, enum in SONG_IDS:
        s = songs[name]
        drum = (f".drum = {name}_drum, .drum_len = sizeof({name}_drum)"
                if "drum" in s else ".drum = 0, .drum_len = 0")
        out.append(f"    [{enum}] = {{")
        out.append(f"        .bpm = {s['bpm']},")
        out.append(f"        .wave_bass = {WAVES[s['waveBass']]}, .wave_lead = {WAVES[s['waveLead']]},")
        out.append(f"        .vol_bass = {gain_to_psg(s['volBass'])}, .vol_lead = {gain_to_psg(s['volLead'])},")
        out.append(f"        .echo = {'true' if s['echo'] else 'false'},")
        out.append(f"        .bass = {name}_bass, .bass_len = sizeof({name}_bass) / 2,")
        out.append(f"        .lead = {name}_lead, .lead_len = sizeof({name}_lead) / 2,")
        out.append(f"        {drum},")
        out.append("    },")
    out.append("};")
    out.append("")

    out.append("static const SfxDef SFXS[SFX_COUNT] = {")
    for enum in SFX_ORDER:
        steps, noise = SFX[enum]
        out.append(f"    [{enum}] = {{")
        out.append("        .steps = {")
        for freq, slide, delay, dur, gain, wave in steps:
            out.append(f"            {{ {freq}, {slide}, {secs_to_frames(delay) if delay else 0}, "
                       f"{secs_to_frames(dur)}, {gain_to_psg(gain)}, {WAVES[wave]} }},")
        out.append("        },")
        out.append(f"        .step_count = {len(steps)},")
        if noise:
            out.append(f"        .noise_dur = {secs_to_frames(noise[0])}, "
                       f".noise_vol = {gain_to_psg(noise[1])},")
        out.append("    },")
    out.append("};")
    out.append("")
    out.append("const Song *audio_get_song(SongId id) {")
    out.append("    if (id < 0 || id >= SONG_COUNT) id = SONG_NONE;")
    out.append("    return &SONGS[id];")
    out.append("}")
    out.append("")
    out.append("const SfxDef *audio_get_sfx(SfxId id) {")
    out.append("    if (id < 0 || id >= SFX_COUNT) id = SFX_JUMP;")
    out.append("    return &SFXS[id];")
    out.append("}")

    dst = REPO / "source" / "core" / "audio_data.c"
    dst.write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"{len(SONG_IDS)} canciones y {len(SFX_ORDER)} efectos -> {dst.relative_to(REPO)}")


if __name__ == "__main__":
    main()
