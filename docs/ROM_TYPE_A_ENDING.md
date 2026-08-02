# Final A-Type reconstruido desde la ROM

La v0.9 reconstruye el final A-Type usando exclusivamente datos leídos durante la ejecución desde la ROM legal del usuario. El repositorio no contiene nametables, bancos CHR, metasprites ni audio extraídos.

## Dump soportado

Los offsets se activan únicamente para la ROM con CRC32 `D16EA396`.

| Recurso | Offset relativo al PRG | Límite comprobado |
|---|---:|---:|
| Paleta de finales | `0x2D43` | `0x2D67` |
| Parche para puntuación ≥120,000 | `0x28CC` | `0x2925` |
| Tabla `oamContentLookup` | `0x0C6C` | 90 punteros |
| Fondo A-Type | `0x5268` | `0x56C9` |
| Banco CHR del final | banco 2 | 4 KiB |

El stream del fondo realiza 1,024 escrituras PPU. El parche ≥120,000 realiza 59 escrituras directas mediante el formato `patchToPpu`.

## Selección del cohete

La traducción sigue los umbrales de `selectEndingScreen`:

| Puntuación | Clase | Metasprite principal | X inicial | Y inicial |
|---:|---:|---:|---:|---:|
| 30,000–49,999 | 0 | `0x3E` | `0x54` | `0xBF` |
| 50,000–69,999 | 1 | `0x41` | `0x54` | `0xBF` |
| 70,000–99,999 | 2 | `0x44` | `0x50` | `0xBF` |
| 100,000–119,999 | 3 | `0x47` | `0x48` | `0xBF` |
| ≥120,000 | 4 | `0x4A` | `0xA0` | `0xC7` |

Los chorros alternan entre los pares `0x3F/0x40`, `0x42/0x43`, `0x45/0x46`, `0x48/0x49` y `0x23/0x24`.

## Temporización

- El cohete permanece quieto durante 240 actualizaciones.
- Mientras Y es al menos `0xB0`, avanza un píxel cada dos fotogramas.
- Entre `0x80` y `0xAF`, avanza un píxel por fotograma.
- Por encima de `0x80`, avanza dos píxeles por fotograma.
- Al llegar a Y cero deja de enviarse a OAM.

La implementación usa el tiempo transcurrido desde la entrada a la pantalla para reconstruir esta secuencia a 60 actualizaciones por segundo. La paridad se conserva de forma determinista respecto al inicio del final; todavía no se sincroniza contra el contador global de una consola NES concreta.

## Orden de pantallas

Para A-Type con al menos 30,000 puntos:

1. Se muestra el final y el cohete.
2. Al confirmar, se abre la entrada de nombre cuando la puntuación clasificó.
3. Después se muestra la tabla de récords.

Para puntuaciones que no clasifican, confirmar vuelve al título. B-Type conserva su flujo de final tras completar las 25 líneas.

## Verificación

`tools/type_a_ending_verify.py` comprueba:

- límites y número de escrituras del fondo;
- límites del parche ≥120,000;
- los quince metasprites requeridos por los cinco cohetes;
- punteros OAM, terminadores y hashes de regresión.

La prueba local con SDL2 y la ROM comprobada cargó dos fondos, cinco clases de cohete y quince metasprites sin faltantes.

Esto mejora la fidelidad visual y de estados, pero no constituye todavía una emulación PPU/APU por ciclos ni una recompilación binariamente idéntica del PRG.
