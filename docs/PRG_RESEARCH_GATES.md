# Puertas de investigación antes de volver a tocar el runtime

El runtime jugable queda congelado a partir de v0.24. No se modificarán `src/`,
el audio, el render, los controles, PC ni Android hasta completar las puertas de
investigación siguientes.

## Qué significa 100%

El 100% de investigación no significa que el port ya sea idéntico. Significa
que existe evidencia suficiente para empezar la sustitución del runtime sin
adivinar el comportamiento de la ROM.

| Puerta | Requisito para 100% | Estado inicial |
|---|---|---:|
| G1. Reconstrucción binaria del PRG | Una fuente 6502 recompila un PRG SHA-256 idéntico de 32 KiB | pendiente de CI v0.25 |
| G2. Partición completa | Cada byte del PRG pertenece a código, tabla, datos o relleno identificado | pendiente de informe ld65 |
| G3. Símbolos semánticos | Cada rutina y tabla tiene un nombre funcional, no solo `Lxxxx` | 60% aproximado |
| G4. Grafo de control | Entradas, llamadas, saltos indirectos y tablas de despacho verificadas | parcial |
| G5. Contratos de estado | Entradas/salidas de RAM, PPU y APU documentadas por subsistema | parcial |
| G6. Trazas dinámicas | Las 18 familias de trazas tienen captura de referencia y primer punto de divergencia reproducible | pendiente |
| G7. Reensamblado repetible | El proceso está fijado a revisiones, hashes y herramientas reproducibles | en progreso |

El runtime solo se descongelará cuando **G1–G7 estén al 100%**.

## v0.25: reconstrucción de fuente externa

La primera puerta usa la fuente pública 6502 fijada exactamente al commit:

```text
CelestialAmber/TetrisNESDisasm
94581c0306fa0009fe11fde48222875549d15b8b
```

Actions la clona con submódulos, la recompila y compara únicamente hashes con la
revisión legal aceptada por el port:

```text
PRG SHA-256: a2de35dfa7333b0458762a1485fb612b6bdc9b5a15269f0fcf5a6be67c0d89de
```

El ROM generado se elimina antes de publicar el artefacto. Solo se conservan:

- hash y resultado de equivalencia;
- revisión exacta de la fuente;
- estadísticas del mapa ld65;
- hashes de los archivos fuente externos.

## Diferencia entre métricas

- **Reconstrucción binaria del PRG** puede llegar a 100% cuando una fuente
  recompila exactamente los 32 KiB.
- **Decompilación semánticamente etiquetada** solo llega a 100% cuando todas las
  rutinas y tablas tienen nombres y contratos comprensibles.
- **Correspondencia funcional del ejecutable** permanece congelada mientras el
  runtime no se modifique.

Por tanto, completar G1 no autoriza por sí solo a subir la correspondencia del
port ni a afirmar que el código está totalmente entendido.

## Regla de seguridad

Cada PR de investigación debe cumplir:

1. cero cambios bajo `src/`;
2. cero cambios funcionales bajo `android/app/src`;
3. no publicar ROM, PRG, CHR, audio o trazas propietarias;
4. compilar el mismo runtime existente en Windows, Linux y Android;
5. publicar resultados reproducibles y revisiones fijadas;
6. no aumentar porcentajes hasta que la puerta correspondiente pase realmente.
