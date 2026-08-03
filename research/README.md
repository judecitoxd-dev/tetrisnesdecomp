# Investigación congelada del PRG

Este directorio registra el estado de investigación previo a cualquier nueva
modificación del runtime.

El archivo `prg_research_status.json` es la fuente legible por máquinas para las
siete puertas definidas en `docs/PRG_RESEARCH_GATES.md`.

Mientras `runtime_frozen` sea `true`, ningún PR de investigación puede modificar
`src/`, el audio, el render, los controles o el runtime Android.
