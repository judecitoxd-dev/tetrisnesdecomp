# Tetris NES — ports nativos para PC y Android

Versión **0.8** de una reimplementación portable de **Tetris (USA) para NES**, escrita en C99 con SDL2. Windows, Linux y Android ejecutan el mismo núcleo de reglas, carga de ROM, render, récords, ajustes y repeticiones.

El proyecto no distribuye la ROM, gráficos extraídos ni música del cartucho. Cada usuario proporciona su propia copia legal. El port valida el archivo y lee durante la ejecución los bancos CHR, paletas, tablas y streams PPU necesarios.

## Novedades de v0.8

- Pantalla de récords reconstruida desde `enter_high_score_nametable` y el parche `high_scores_nametable` de la ROM.
- Nombres, puntuaciones y niveles locales colocados sobre las filas del marco original.
- Alternancia automática entre tablas A-Type y B-Type.
- Entrada de nombre sobre la pantalla original de “TETRIS MASTER”.
- Cursor de letra, puntuación, nivel y fila del récord alineados a la cuadrícula NES.
- Las mejoras se compilan simultáneamente para Android, Windows y Linux.

## Recursos originales ya utilizados

- Cuatro bancos CHR de 4 KiB.
- Fuente y tiles de los tetrominós.
- Paletas de menús y niveles.
- Pantalla de título.
- Selección A-Type/B-Type.
- Selección de nivel/altura.
- Marco principal de partida.
- Pantallas de récords y entrada de nombre.

Los recursos se decodifican en memoria desde la ROM CRC32 `D16EA396`; no se escriben PNG, bancos CHR ni nametables descomprimidas. Una ROM con otro CRC usa el renderer alternativo.

## Estado actual

- Modos A y B jugables a 60.0988 actualizaciones por segundo.
- Orientaciones, gravedad, puntuación, transiciones, RNG, DAS y borrado contrastados con datos identificados en la ROM.
- Teclado, controles táctiles y gamepad sobre el mismo núcleo C99.
- Android horizontal e inmersivo, selector SAF, almacenamiento privado y controles editables.
- Opciones persistentes, selector de ROM, récords y repeticiones deterministas.
- Paquetes OGG opcionales en PC y sintetizador incorporado como respaldo.
- Desensamblador NMOS 6502 por bancos MMC1 con símbolos y referencias cruzadas.

Todavía no es una decompilación bit a bit. Los cursores OAM exactos, la demo original, los finales, los músicos y el controlador APU continúan incompletos.

Documentación:

- [`docs/PORT_STATUS.md`](docs/PORT_STATUS.md)
- [`docs/ROM_SCREENS.md`](docs/ROM_SCREENS.md)
- [`docs/ANDROID_PORT.md`](docs/ANDROID_PORT.md)
- [`docs/ROM_MAP.md`](docs/ROM_MAP.md)
- [`docs/REPLAY_FORMAT.md`](docs/REPLAY_FORMAT.md)
- [`docs/AUDIO_PACK.md`](docs/AUDIO_PACK.md)
- [`docs/DECOMP_TOOLS.md`](docs/DECOMP_TOOLS.md)

## ROM comprobada

| Propiedad | Valor |
|---|---|
| Archivo | `Tetris (USA).nes` |
| Formato | NES 2.0 |
| Mapper | MMC1 / mapper 1 |
| PRG ROM | 32 KiB |
| CHR ROM | 16 KiB |
| Tamaño | 49,168 bytes |
| CRC32 | `D16EA396` |
| SHA-1 | `3026d28b63d94c921fe58364f8b0659d10b5a0ac` |
| NMI / RESET / IRQ | `$8005` / `$FF00` / `$804A` |

## Android

1. Instala el APK.
2. Abre **Tetris NES Port**.
3. Selecciona tu ROM legal con el selector del sistema.
4. La app valida cabecera, mapper y tamaños y copia el archivo al almacenamiento privado.

La capa táctil incluye cruceta, A/B, caída, Start, Select, ROM y EDIT. En modo EDIT se mueven los botones; START cambia el tamaño y SEL la opacidad. Al conectar un mando físico, la capa se oculta.

## Ejecutar en PC

```powershell
.\tetris_pc.exe --rom "C:\ruta\Tetris (USA).nes"
```

Repetición:

```powershell
.\tetris_pc.exe --rom "C:\ruta\Tetris (USA).nes" --replay "C:\ruta\partida.ttr"
```

## Controles de PC

| Tecla | Acción |
|---|---|
| Enter / espacio | Confirmar |
| Flechas | Navegar, mover y caída suave |
| Z / X | Rotar antihorario / horario |
| Espacio en partida | Caída instantánea |
| P | Pausa |
| Tab | Siguiente pieza |
| M / N | Audio / música |
| H | Récords |
| O | Opciones |
| L | Elegir ROM |
| F8 | Reproducir última partida |
| F11 | Pantalla completa |
| R | Reiniciar después de perder |
| Retroceso | Volver |
| Esc | Salir |

## Compilar PC

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

## Compilar Android

```bash
cd android
gradle --no-daemon :app:assembleDebug
```

Requiere SDK 34, Build Tools 34.0.0, NDK 26.3.11579264, CMake 3.22.1 y Java 17. El APK generado por CI es una build de depuración para instalación manual.

## Legalidad

El repositorio contiene código original, herramientas y documentación. No contiene ROM, PRG/CHR extraído, imágenes, música de Nintendo ni grabaciones OGG del cartucho.
