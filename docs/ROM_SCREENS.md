# Pantallas reconstruidas desde la ROM

La versión v0.9 incorpora intérpretes separados para los formatos `bulkCopyToPpu` y `patchToPpu` del cartucho. El port lee los streams directamente desde el PRG de la copia legal del usuario, reconstruye una nametable virtual, interpreta atributos y paletas y dibuja los tiles del banco CHR correspondiente.

No se guardan PNG, nametables descomprimidas, bancos CHR, tablas OAM ni otros recursos derivados en el repositorio o en los paquetes.

## Dump comprobado

Estas tablas solo se activan para la ROM CRC32 `D16EA396`, porque los offsets son específicos de esa revisión.

| Recurso | Offset relativo al PRG | Banco CHR |
|---|---:|---:|
| Paleta de juego | `0x2CF3` | 3 |
| Paleta de menús | `0x2D2B` | 0 |
| Paleta de finales | `0x2D43` | según final |
| Selección de tipo | `0x367A` | 0 |
| Selección de nivel | `0x3ADB` | 0 |
| Marco de partida | `0x3F3C` | 3 |
| Entrada de récord | `0x439D` | 0 |
| Parche de tabla de récords | `0x47FE` | 0 |
| Parche A-Type para ocultar altura | `0x495D` | 0 |
| Castillo B-Type nivel 9/19 | `0x49A6` | 1 |
| Final B-Type normal | `0x4E07` | 2 |

El stream del castillo termina exactamente en `0x4E07`, donde comienza el final normal. Ese límite contiguo se usa como comprobación adicional del offset.

## Parches del concierto por altura

El castillo contiene elementos estáticos del reparto completo. La rutina original aplica parches horizontales para ocultar los personajes no desbloqueados:

| Altura que comienza a ocultarse | Offset `patchToPpu` |
|---:|---:|
| 0 | `0x2834` |
| 1 | `0x284A` |
| 2 | `0x2862` |
| 3 | `0x287A` |
| 4 | `0x2896` |
| 5 | sin parche |

Los parches se aplican en cascada. Altura 0 aplica los cinco, altura 1 aplica los cuatro últimos y altura 4 aplica solo el último. Altura 5 conserva todo el escenario.

## Pantallas usadas en v0.9

- Selección A-Type/B-Type reconstruida desde la ROM.
- Selección de nivel y altura reconstruida desde la ROM.
- Marco principal de partida reconstruido desde la ROM.
- Entrada de nombre y tabla de récords reconstruidas desde sus streams.
- Cursores de tipo, nivel/altura y nombre reconstruidos desde tablas OAM.
- Final B-Type normal reconstruido desde su nametable y banco CHR.
- Seis variantes del castillo B-Type reconstruidas con los parches originales de altura.
- Reparto del concierto cargado desde `oamContentLookup`, con fotogramas y posiciones identificados en las rutinas del final.
- Puntuación previa, bonus de nivel, bonus de altura y total colocados en las direcciones PPU del cartucho.

## Límites

- Los valores dinámicos se dibujan con el renderer del port; todavía no se ejecuta toda la cola PPU/OAM de la NES por ciclo.
- La vista de récords alterna A/B automáticamente, en vez de reproducir toda la máquina de estados original.
- El concierto reproduce el reparto y sus fotogramas principales, pero todavía faltan la entrada completa de la catedral y algunos tiempos intermedios.
- Los finales A-Type y los cohetes normales de B-Type aún no están reconstruidos por completo.
- Una ROM con otro CRC usa el renderer alternativo para evitar leer offsets incorrectos.

Los siguientes objetivos son completar la máquina de estados de finales, reconstruir los cohetes y traducir el controlador APU.
