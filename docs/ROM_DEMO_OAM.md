# Demo, OAM y finales desde la ROM

La v0.9 amplía el uso directo de la ROM legal a la demo NTSC, cursores, personajes y finales. Todos los datos se interpretan en memoria y nunca se guardan como archivos derivados.

## Demo NTSC

- Comandos de botones y duraciones: `PRG+0x5D00`, 512 bytes.
- Secuencia de piezas: `PRG+0x5F00`, un byte por pieza; se usa el nibble alto.
- Nivel inicial: 0.
- Resultado actual del modelo C: 4,785 fotogramas y 40 piezas.
- Hash final: `807bd007f6ea6876`.
- Hash acumulado de traza: `227fdb211b9691e7`.

La firma detecta regresiones dentro del port. No demuestra todavía identidad con una ejecución del 6502.

## Cursores OAM

El port interpreta punteros desde `oamContentLookup` en `PRG+0x0C6C`. Cada descriptor contiene entradas `y, tile, atributos, x` y termina en `0xFF`.

Se soportan:

- paletas de sprite;
- volteo horizontal y vertical;
- transparencia del color cero;
- prioridad necesaria para el cursor de entrada de nombre;
- cursores de tipo, nivel/altura y récord.

Posiciones verificadas:

| Sprite | Offset relativo al PRG |
|---|---:|
| Cursor de nivel/altura | `0x0D20` |
| Cursor A-Type/B-Type | `0x0D31` |
| Cursor de nombre de récord | `0x0DE0` |

## Final B-Type

- Paleta final: `PRG+0x2D43`.
- Castillo nivel 9/19: `PRG+0x49A6`, CHR banco 1.
- Final normal: `PRG+0x4E07`, CHR banco 2.
- Los streams terminan respectivamente en `0x4E07` y `0x5268`.

Parches de altura del concierto:

| Altura ocultada | Offset | Fin |
|---:|---:|---:|
| 0 | `0x2834` | `0x284A` |
| 1 | `0x284A` | `0x2862` |
| 2 | `0x2862` | `0x287A` |
| 3 | `0x287A` | `0x2896` |
| 4 | `0x2896` | `0x28A8` |

Los parches se aplican en cascada. Altura 0 aplica los cinco; altura 5 no aplica ninguno.

El concierto dibuja progresivamente a Kid Icarus, Link, Samus, Donkey Kong, Bowser y, en altura 5, Peach, Mario y Luigi.

## Final A-Type

La documentación detallada se encuentra en [`ROM_TYPE_A_ENDING.md`](ROM_TYPE_A_ENDING.md).

Resumen:

- Fondo: `PRG+0x5268`, CHR banco 2.
- Parche ≥120,000: `PRG+0x28CC`.
- Cinco clases de cohete según 30k/50k/70k/100k/120k.
- Quince metasprites OAM para cuerpos y chorros.
- Espera de 240 fotogramas y ascenso con velocidad variable.

## Pruebas y herramientas

`tests/demo_rom_tests.c` usa un PRG artificial, sin bytes del juego, para comprobar duración, pulsaciones nuevas, botones mantenidos, piezas y fin de tabla.

Herramientas adicionales:

- `tetris_demo_verify`: ejecuta la demo y produce hashes/CSV.
- `rom_assets_verify.py`: verifica streams, parches y tabla OAM general.
- `type_a_ending_verify.py`: verifica fondo, parche y quince metasprites A-Type.

Estas herramientas solo imprimen límites, conteos y hashes. No escriben CHR, nametables, sprites ni audio extraídos.

## Límites actuales

- Las reglas se ejecutan en C; no se emula el 6502 por ciclo.
- La paridad de algunas animaciones se reconstruye desde el inicio de la pantalla.
- Falta comparar estados de RAM contra un emulador por fotograma.
- Falta completar el movimiento de entrada de la catedral.
- El APU original todavía no está traducido.
- Una ROM con otro CRC usa cursores, demo y finales alternativos para evitar offsets no verificados.
