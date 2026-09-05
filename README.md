# Perseo: Sombras de Silencio — Port a C / Game Boy Advance

Port nativo a GBA (ROM `.gba`, en C sobre devkitARM + libtonc) del prototipo web jugable
del juego. El prototipo original (HTML5 Canvas) queda documentado como referencia de diseño
y comportamiento en [docs/prototipo_referencia.html](docs/prototipo_referencia.html).

## Punto de partida

- **Plan de migración:** [docs/PLAN_MIGRACION_GBA_C.md](docs/PLAN_MIGRACION_GBA_C.md) — arquitectura,
  restricciones de la plataforma, pipeline de assets y trazabilidad JS → C.
- **Checklist de ejecución:** [docs/TAREAS_MIGRACION_GBA.md](docs/TAREAS_MIGRACION_GBA.md) — tareas
  atómicas por fase, para ir marcándolas una a una.
- **Diseño del juego:** [docs/DOCUMENTACION_PERSEO.md](docs/DOCUMENTACION_PERSEO.md),
  [docs/PRD_Perseo_Metroidvania.md](docs/PRD_Perseo_Metroidvania.md),
  [docs/Estilo_Grafico_Perseo.md](docs/Estilo_Grafico_Perseo.md).

## Arquitectura

El código en `source/` está dividido en dos capas (ver sección 4 del plan de migración):

- `source/core/` — lógica de juego 100% portable (física, entidades, IA, niveles, jefes,
  diálogo, guardado). No incluye ningún header de hardware.
- `source/platform/gba/` — implementación real sobre libtonc/hardware de GBA (vídeo, input,
  audio, guardado en SRAM).
- `source/platform/sdl/` — (opcional) build de escritorio del mismo `core/` para iterar rápido.

## Requisitos de build (pendiente de instalar — ver Fase 0 del checklist)

- [devkitPro](https://devkitpro.org/wiki/Getting_Started) con el grupo `gba-dev` (incluye
  devkitARM + libgba).
- [libtonc](https://github.com/devkitPro/libtonc).
- Python 3 (para los scripts de `tools/`).
- `mGBA` y/o `no$gba` para pruebas.

## Compilar

```sh
make            # genera build/perseo.gba (requiere devkitARM instalado y $DEVKITPRO configurado)
```
