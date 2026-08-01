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
| Columnas de borrado | `0x17FE` | `$97FE` | 10 bytes |
| Paletas por nivel | `0x184C` | `$984C` | 40 bytes |
| Orientaciones/aparición | `0x194E` | `$994E` | 27 bytes confirmados |
| Valores BCD de puntuación | `0x1CA5` | `$9CA5` | 10 bytes |

`src/rom.c` lee la tabla de paletas desde la ROM legal. Las demás tablas permiten contrastar
constantes traducidas al núcleo y pueden inspeccionarse con `tools/rom_tables.py`.

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
- La correspondencia binaria requiere un ensamblado enlazable y control exacto del layout, aún no
  implementados.
