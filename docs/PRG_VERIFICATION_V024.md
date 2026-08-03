# Verificación PRG v0.24 sin cambios de runtime

## Alcance

Esta fase amplía únicamente la evidencia de decompilación del PRG 6502 y la
correspondencia reproducible con la ROM compatible. No cambia el juego, el
audio, el renderizador, los controles, la ventana de PC ni la interfaz Android.

La aplicación jugable continúa siendo v0.23. La etiqueta v0.24 identifica el
conjunto suplementario de análisis.

## ROM compatible

El verificador exige exactamente la revisión legal ya aceptada por el port:

- ROM CRC32: `D16EA396`
- ROM SHA-256: `ddb876c302cfd4ee19fabff8e3ede0b6801ded70c49980d1682a32d352953082`
- PRG CRC32: `943DFBBE`
- PRG SHA-256: `a2de35dfa7333b0458762a1485fb612b6bdc9b5a15269f0fcf5a6be67c0d89de`

No se almacena ningún byte de la ROM en el repositorio. Los manifiestos solo
contienen direcciones, tamaños, hashes y valores semánticos pequeños.

## Evidencia acumulada

La combinación de `tetris_prg_manifest.json` y
`tetris_prg_manifest_v024.json` comprueba:

| Evidencia | Total |
|---|---:|
| Rangos de rutinas 6502 | 76 |
| Tablas | 38 |
| Aristas JSR/JMP directas | 14 |
| Comprobaciones semánticas | 32 |
| Bytes PRG únicos cubiertos | 3,749 |
| Archivos del runtime modificados | 0 |

El cálculo de bytes usa la unión de intervalos para no contar dos veces regiones
que aparecen tanto como firma corta como rutina completa.

## Rutinas completas añadidas

La fase suplementaria verifica rangos completos para:

- rotación, caída y desplazamiento/DAS;
- staging del tetrimino actual;
- aparición y selección de la siguiente pieza;
- selección pseudoaleatoria y estadísticas por pieza;
- bloqueo, cortina de derrota y detección de filas;
- recepción de basura y actualización de líneas/puntuación BCD;
- estado de derrota y cambio normal/allegro;
- lectura del mando y reproducción de demo;
- LFSR, DMA de OAM y lectura física de puertos.

## Tablas añadidas

Se verifican además:

- coordenadas de cursores de nivel, altura y música;
- tabla de rotaciones y conversión orientación→sprite;
- direcciones PPU de estadísticas, tablero y récords;
- BCD 0–49, niveles y multiplicación por diez;
- dispatchers de efectos y tablas de ruido;
- envolventes de volumen y periodos de notas;
- duraciones NTSC;
- índices y cabeceras de las diez pistas musicales.

## Uso

```powershell
python tools\prg_verify_v024.py "C:\ROMs\Tetris (USA).nes" `
  --report prg-v024-report.json
```

Resultado esperado:

```text
PRG v0.24 verification: OK routines=76 tables=38 edges=14 semantic=32
unique verified PRG bytes: 3749
runtime files changed: 0
```

Para producir un listado con las etiquetas suplementarias, se puede combinar
`tools/tetris_symbols.json` con `tools/tetris_symbols_v024.json` en una copia
local de análisis. Ninguno de estos archivos contiene datos propietarios.

## Interpretación de porcentajes

- **Decompilación etiquetada/verificada del PRG 6502: 60%**: más subsistemas
  tienen nombres semánticos y evidencia reproducible, pero todavía no existe
  una fuente 6502 completa que reensamble byte por byte.
- **Correspondencia reproducible con la ROM: 56%**: más reglas, tablas y
  relaciones internas se pueden volver a comprobar automáticamente, pero falta
  comparar estados completos de RAM, PPU y APU durante partidas reales.

Los hashes del PRG completo validan la revisión de entrada, pero no se cuentan
como decompilación completa.
