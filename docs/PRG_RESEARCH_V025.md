# Investigación PRG v0.25

Esta fase mantiene congelado el runtime v0.23 y trabaja únicamente sobre la
reconstrucción y comprensión del PRG 6502.

## Fuente externa fijada

- Repositorio: `CelestialAmber/TetrisNESDisasm`
- Commit: `94581c0306fa0009fe11fde48222875549d15b8b`
- Salida esperada: PRG de 32 KiB
- PRG objetivo SHA-256:
  `a2de35dfa7333b0458762a1485fb612b6bdc9b5a15269f0fcf5a6be67c0d89de`

El workflow recompila la fuente en Linux y compara hashes. La imagen generada se
elimina antes de publicar artefactos.

## Informes producidos

- `external-prg-equivalence.json`: equivalencia completa, PRG y CHR.
- `ld65-source-map.json`: segmentos, spans, etiquetas y cobertura de fuente.
- `external-source-revision.txt`: commit exacto usado.
- `external-source-files.sha256`: hashes de los archivos fuente principales.

## Regla de interpretación

- PRG idéntico: la reconstrucción binaria puede marcarse 100%.
- Mapa de 32 KiB completo: la partición de bytes puede marcarse 100%.
- Ninguno de los dos resultados eleva automáticamente la comprensión semántica
  ni la correspondencia funcional del runtime.

El runtime seguirá congelado aunque G1 y G2 pasen. Aún deberán completarse
símbolos semánticos, control de flujo, contratos de estado y 18 familias de
trazas dinámicas.
