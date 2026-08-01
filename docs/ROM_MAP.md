# Mapa inicial de la ROM comprobada

Este documento usa offsets relativos al comienzo del PRG, después de la cabecera iNES/NES 2.0.
Solo describe ubicaciones; el repositorio no incluye los bytes originales.

## Identificación

| Campo | Valor |
|---|---|
| CRC32 del archivo | `D16EA396` |
| SHA-1 | `3026d28b63d94c921fe58364f8b0659d10b5a0ac` |
| Mapper | MMC1 / 1 |
| PRG | 32 KiB |
| CHR | 16 KiB |

## Vectores

| Vector | Dirección CPU |
|---|---:|
| NMI | `$8005` |
| RESET | `$FF00` |
| IRQ/BRK | `$804A` |

## Tablas pequeñas identificadas

| Contenido | Offset PRG | Dirección CPU mostrada por la herramienta | Tamaño |
|---|---:|---:|---:|
| Gravedad NTSC | `0x098E` | `$898E` | 30 bytes |
| Columnas de borrado | `0x17FE` | `$97FE` | 10 bytes |
| Paletas por nivel | `0x184C` | `$984C` | 40 bytes |
| Orientaciones de aparición | `0x194E` | `$994E` | 8 bytes |
| Valores BCD de puntuación | `0x1CA5` | `$9CA5` | 10 bytes |

`src/rom.c` lee la tabla de paletas por nivel directamente desde la ROM legal. El resto sirve para
contrastar constantes traducidas al núcleo, y puede inspeccionarse con `tools/rom_tables.py`.

## Modo B

La rutina de inicialización identificada genera doce filas candidatas en la zona inferior. Para
cada celda selecciona entre vacío y tres tipos de bloque mediante una tabla de ocho entradas, y
fuerza al menos una celda vacía por fila. Una segunda tabla determina cuánto del campo se limpia
para las alturas 0–5.

El port mantiene esas propiedades y usa el mismo LFSR interno de dos bytes, pero todavía falta
registrar y comparar el estado completo del RNG por fotograma contra un emulador de referencia.

## Limitaciones del direccionamiento

Las direcciones CPU dependen del banco MMC1 activo. La columna de dirección es útil para localizar
las tablas en la revisión comprobada, pero no sustituye un mapa completo de bancos. Una fase futura
separará cada banco y etiquetará las referencias cruzadas.
