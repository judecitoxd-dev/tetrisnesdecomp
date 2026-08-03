# Investigación PRG v0.25

Esta fase mantiene congelado el runtime v0.23 y trabaja únicamente sobre la
reconstrucción y comprensión del PRG 6502.

## Fuente externa fijada

- Repositorio: `CelestialAmber/TetrisNESDisasm`
- Commit: `94581c0306fa0009fe11fde48222875549d15b8b`
- Salida: PRG de 32 KiB
- PRG objetivo SHA-256:
  `a2de35dfa7333b0458762a1485fb612b6bdc9b5a15269f0fcf5a6be67c0d89de`

El workflow recompila la fuente en Linux y compara hashes. La imagen generada se
elimina antes de publicar artefactos.

## Resultado verificado

La reconstrucción produjo:

```text
PRG SHA-256: a2de35dfa7333b0458762a1485fb612b6bdc9b5a15269f0fcf5a6be67c0d89de
CHR SHA-256: 23789834e66947081aa5d34efe092beb69a91273abc62089a8bb7c00f67ff751
```

Ambos coinciden exactamente con la revisión legal usada por el port. La imagen
completa tiene un hash distinto únicamente porque su cabecera iNES de 16 bytes
no es igual; los 32 KiB de PRG y los 16 KiB de CHR sí son idénticos.

Por ello:

- **G1, reconstrucción binaria del PRG: 100%**.
- **G2, partición completa del PRG: 100%**.

## Mapa ld65

El informe de depuración demuestra:

- 32,768 bytes de segmentos PRG;
- 32,768 bytes asociados a spans de fuente;
- 30 archivos fuente;
- 8,304 registros de línea;
- 1,464 registros de símbolos;
- 1,169 etiquetas únicas;
- 1,071 etiquetas consideradas semánticas;
- 98 etiquetas todavía genéricas;
- proporción de nombres semánticos: 91.6168%.

La partición completa no significa que cada nombre haya sido auditado. G3 sigue
abierto hasta reemplazar o clasificar las 98 etiquetas genéricas y revisar el
significado funcional de las demás.

## Informes producidos

- `external-prg-equivalence.json`: equivalencia completa, PRG y CHR.
- `ld65-source-map.json`: segmentos, spans, etiquetas y cobertura de fuente.
- `external-source-revision.txt`: commit exacto usado.
- `external-source-files.sha256`: hashes de los archivos fuente principales.
- `research/prg_research_v025_result.json`: resumen permanente sin datos de ROM.

## Estado después de v0.25

| Puerta | Estado |
|---|---:|
| G1. Reconstrucción binaria | 100% |
| G2. Partición completa | 100% |
| G3. Símbolos semánticos | 91.6168% |
| G4. Grafo de control | 35% |
| G5. Contratos de estado | 25% |
| G6. Familias de trazas | 0% |
| G7. Toolchain reproducible | 100% |
| Preparación total, promedio de puertas | 64.5167% |

La correspondencia funcional del ejecutable continúa congelada en 50%. No se
volverá a tocar el runtime hasta que G1–G7 estén al 100%.
