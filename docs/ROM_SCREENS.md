# Pantallas reconstruidas desde la ROM

La versión v0.8 incorpora un intérprete del formato usado por la rutina `bulkCopyToPpu` del cartucho. El port lee los streams comprimidos directamente desde el PRG de la copia legal del usuario, aplica escrituras horizontales, verticales y repetidas a una nametable virtual, interpreta atributos y paletas y finalmente dibuja los tiles del banco CHR correspondiente.

No se guardan PNG, nametables descomprimidas, bancos CHR ni otros recursos derivados en el repositorio o en los paquetes.

## Dump comprobado

Esta tabla solo se activa para la ROM CRC32 `D16EA396`, porque los offsets todavía son específicos de esa revisión.

| Recurso | Offset relativo al PRG | Banco CHR |
|---|---:|---:|
| Paleta de juego | `0x2CF3` | 3 |
| Paleta de menús | `0x2D2B` | 0 |
| Selección de tipo | `0x367A` | 0 |
| Selección de nivel | `0x3ADB` | 0 |
| Marco de partida | `0x3F3C` | 3 |
| Entrada de récord | `0x439D` | 0 |
| Parche de tabla de récords | `0x47FE` | 0 |
| Parche A-Type para ocultar altura | `0x495D` | 0 |

## Pantallas usadas en v0.8

- Selección A-Type/B-Type reconstruida desde la ROM.
- Selección de nivel y altura reconstruida desde la ROM.
- Marco principal de partida reconstruido desde la ROM.
- Entrada de nombre reconstruida desde `enter_high_score_nametable`.
- Tabla de récords creada aplicando `high_scores_nametable` sobre la composición de entrada.
- Tablero alineado a la cuadrícula original de tiles de 8×8, escalada 2×.
- Piezas, siguiente pieza y bloques fijados al banco CHR de juego.
- Nombres, puntuaciones, niveles y cursores colocados sobre las posiciones del cartucho.

## Límites

- Los cursores aún son equivalentes SDL; no son sprites OAM traducidos exactamente.
- Los récords locales usan tres filas por modo; el cartucho conserva su propio formato interno en RAM.
- La vista de récords alterna A/B automáticamente, en vez de reproducir toda la máquina de estados original.
- Las animaciones y sprites de finales no están reconstruidos desde las tablas OAM.
- La ejecución no emula el PPU por ciclos; reconstruye el resultado visual estático de los streams.
- Una ROM compatible con otro CRC usa el renderer alternativo para evitar leer offsets incorrectos.

El siguiente objetivo es interpretar las tablas OAM de cursores y finales, reproducir la demo almacenada en el PRG y traducir el controlador APU.
