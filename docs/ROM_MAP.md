# Mapa de la ROM comprobada

Este documento usa offsets relativos al comienzo del PRG, después de la cabecera NES 2.0. Solo
registra ubicaciones y símbolos; el repositorio no incluye bytes originales.

## Identificación

| Campo | Valor |
|---|---|
| CRC32 del archivo | `D16EA396` |
| SHA-1 | `3026d28b63d94c921fe58364f8b0659d10b5a0ac` |
| Mapper | MMC1 / 1 |
| PRG | 32 KiB, dos bancos de 16 KiB |
| CHR | 16 KiB |

## Modelo de bancos usado por las herramientas

| Banco | Offset PRG | Ventana CPU |
|---|---:|---:|
| 0 | `0x0000-0x3FFF` | `$8000-$BFFF` |
| 1, fijo | `0x4000-0x7FFF` | `$C000-$FFFF` |

Las escrituras MMC1 pueden cambiar la ventana conmutada durante la ejecución. El modelo anterior es
suficiente para los puntos de entrada y símbolos actualmente confirmados, pero una reconstrucción
completa deberá seguir el estado real del mapper.

## Vectores

| Vector | Dirección CPU |
|---|---:|
| NMI | `$8005` |
| RESET | `$FF00` |
| IRQ/BRK | `$804A` |

## Tablas identificadas

| Contenido | Offset PRG | Dirección CPU | Tamaño |
|---|---:|---:|---:|
| Gravedad NTSC | `0x098E` | `$898E` | 30 bytes |
| Tabla de punteros OAM | `0x0C6C` | `$8C6C` | 90 punteros |
| Columnas de borrado | `0x17FE` | `$97FE` | 10 bytes |
| Paletas por nivel | `0x184C` | `$984C` | 40 bytes |
| Orientaciones/aparición | `0x194E` | `$994E` | 27 bytes confirmados |
| Valores BCD de puntuación | `0x1CA5` | `$9CA5` | 10 bytes |

`src/rom.c` lee la tabla de paletas desde la ROM legal. Las demás tablas permiten contrastar
constantes traducidas al núcleo y pueden inspeccionarse con `tools/rom_tables.py`.

## Streams PPU y parches de finales

| Recurso | Inicio PRG | Fin comprobado | Banco CHR |
|---|---:|---:|---:|
| Paleta de finales | `0x2D43` | `0x2D67` | — |
| Castillo B-Type | `0x49A6` | `0x4E07` | 1 |
| Final B-Type normal | `0x4E07` | `0x5268` | 2 |
| Final A-Type | `0x5268` | `0x56C9` | 2 |

Parches directos `patchToPpu`:

| Recurso | Inicio | Fin |
|---|---:|---:|
| Concierto altura 0 | `0x2834` | `0x284A` |
| Concierto altura 1 | `0x284A` | `0x2862` |
| Concierto altura 2 | `0x2862` | `0x287A` |
| Concierto altura 3 | `0x287A` | `0x2896` |
| Concierto altura 4 | `0x2896` | `0x28A8` |
| Final A-Type ≥120,000 | `0x28CC` | `0x2925` |

## Demo NTSC

| Recurso | Offset PRG | Tamaño usado |
|---|---:|---:|
| Botones y duración | `0x5D00` | 512 bytes |
| Secuencia de piezas | `0x5F00` | 40 bytes consumidos actualmente |

## Metasprites del final A-Type

| Umbral | Cuerpo | Chorros | X inicial | Y inicial |
|---:|---:|---:|---:|---:|
| 30k | `0x3E` | `0x3F`, `0x40` | `0x54` | `0xBF` |
| 50k | `0x41` | `0x42`, `0x43` | `0x54` | `0xBF` |
| 70k | `0x44` | `0x45`, `0x46` | `0x50` | `0xBF` |
| 100k | `0x47` | `0x48`, `0x49` | `0x48` | `0xBF` |
| 120k | `0x4A` | `0x23`, `0x24` | `0xA0` | `0xC7` |

## Rutinas etiquetadas con alta confianza

| Símbolo | Dirección CPU | Función resumida |
|---|---:|---|
| `nmi_entry` | `$8005` | entrada NMI |
| `irq_entry` | `$804A` | entrada IRQ/BRK |
| `reset_entry` | `$FF00` | reinicio |
| `check_piece_collision` | `$948B` | colisión de pieza |
| `animate_line_clear` | `$977F` | animación de borrado |
| `upload_level_palette` | `$9808` | carga paleta de nivel |
| `select_next_piece` | `$98EB` | selección de siguiente pieza |
| `select_random_piece` | `$9907` | elección aleatoria/repetición |
| `record_lock_height` | `$9CAF` | altura de bloqueo/retardo |
| `advance_rng_lfsr` | `$AB47` | avance del LFSR |
| `perform_oam_dma` | `$AB5E` | DMA de sprites |
| `read_controller_ports` | `$AB69` | lectura de mandos |
| `merge_controller_reads` | `$AB8B` | estabilización de entrada |

La fuente de verdad editable es `tools/tetris_symbols.json`.

## Modo B

La inicialización genera doce filas candidatas en la zona inferior. Cada celda selecciona entre
vacío y tres tipos de bloque mediante una tabla, y la rutina fuerza al menos un hueco por fila. Otra
tabla determina cuánto campo permanece para las alturas 0–5.

El port mantiene esas propiedades y usa un LFSR de dos bytes, pero todavía falta registrar y
comparar el estado completo por fotograma para todas las combinaciones de nivel, altura y semilla.

## Limitaciones

- Las direcciones CPU dependen del banco MMC1 activo.
- El recorrido agresivo puede confundir datos con instrucciones.
- Una etiqueta indica una hipótesis o identificación; no implica que la rutina ya esté traducida.
- Los offsets de streams y metasprites solo se activan para el CRC comprobado.
- La correspondencia binaria requiere un ensamblado enlazable y control exacto del layout, aún no
  implementados.

## Verificación ampliada v0.23

El manifiesto verifica 53 entradas de rutina. Entre las nuevas familias están:

- `$8161-$81FC`: dispatch de modo, estado y controles de ambos jugadores;
- `$86DC-$8875`: inicialización y generación completa del campo B-Type;
- `$88AB/$8914/$89AE`: rotación, caída y desplazamiento/DAS;
- `$988E-$9969`: aparición, selección aleatoria y estadísticas de piezas;
- `$99A2-$9B58`: bloqueo, cortina, filas completas, basura, líneas y score;
- `$9CBF-$9E37`: derrota, allegro, mando, demo y estados auxiliares.

La antigua región combinada de `$994E` se divide ahora en tres tablas:
`tetrimino_type_from_orientation` (`$993B`, 19 bytes), `spawn_table`
(`$994E`, 8 bytes) y `spawn_orientation_from_orientation` (`$9956`, 19 bytes).
También se verifica `orientation_table` (`$8A9C`, 228 bytes) y
`garbage_lines_by_clear_count` (`$9B53`, 5 bytes).
