# Tetris NES — ports nativos para PC y Android

Versión **0.7** de una reimplementación portable de **Tetris (USA) para NES**, escrita en C99 con SDL2. Windows, Linux y Android ejecutan el mismo núcleo de reglas, carga de ROM, render, récords, ajustes y repeticiones.

El proyecto no distribuye la ROM, gráficos extraídos ni música del cartucho. Cada usuario proporciona su propia copia legal. El port valida el archivo y lee durante la ejecución los bancos CHR, paletas, tablas y streams PPU necesarios.

## Novedades de v0.7

- Selección A-Type/B-Type reconstruida desde el stream PPU original de la ROM.
- Selección de nivel y altura reconstruida desde la ROM.
- Marco principal de partida reconstruido desde nametable, atributos, paleta y CHR originales.
- Tablero alineado a la cuadrícula NES de 8×8 píxeles, escalada 2×.
- Cursores, puntuación, líneas, nivel, pieza siguiente y estadísticas superpuestos sobre los fondos originales.
- Renderer alternativo conservado para ROMs estructuralmente compatibles pero no verificadas.
- Android, Windows y Linux compilan el mismo decodificador PPU.
- APK Android actualizado a versión 0.7 para ARM64 y ARMv7.

## Estado actual

- Modos A y B jugables a 60.0988 actualizaciones por segundo.
- Tablero 10×20, siete tetrominós, puntuación, líneas, niveles, estadísticas y récords.
- Orientaciones, gravedad, puntuación, transiciones, RNG, DAS y borrado contrastados con tablas o rutinas identificadas en la ROM.
- Teclado, táctil y mando SDL con conexión en caliente.
- Título, fuente, bloques, paletas, selección de tipo, selección de nivel y marco de partida cargados desde la ROM legal.
- Opciones persistentes, selector de ROM, repeticiones deterministas y verificador por consola.
- Paquetes OGG opcionales en escritorio y sintetizador incorporado como respaldo.
- Desensamblador NMOS 6502 por bancos MMC1 con símbolos y referencias cruzadas.

Todavía no es una decompilación reproducible bit a bit. Los cursores OAM, récords exactos, demostración, finales y controlador APU continúan incompletos. Consulta:

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
| Tamaño total | 49,168 bytes |
| CRC32 | `D16EA396` |
| SHA-1 | `3026d28b63d94c921fe58364f8b0659d10b5a0ac` |
| NMI / RESET / IRQ | `$8005` / `$FF00` / `$804A` |

Los offsets visuales exactos de v0.7 solo se usan con ese CRC. Otra revisión compatible abre con el renderer alternativo y aparece como no verificada.

## Android

1. Instala el APK.
2. Abre **Tetris NES Port**.
3. Selecciona tu ROM legal mediante el selector del sistema.
4. La aplicación valida cabecera, mapper y tamaños y copia la ROM al almacenamiento privado.

La capa táctil incluye cruceta, A/B, caída, Start, Select, ROM y EDIT. En modo EDIT se pueden mover los botones; START cambia el tamaño y SEL cambia la opacidad. Al conectar un gamepad, la capa se oculta automáticamente.

El artefacto actual es un APK de depuración firmado automáticamente, apto para pruebas e instalación manual. No es una publicación de Play Store.

## Ejecutar en PC

Coloca la ROM junto al ejecutable, selecciónala desde el diálogo o pásala por argumento:

```powershell
.\tetris_pc.exe --rom "C:\ruta\Tetris (USA).nes"
```

Reproducir una partida:

```powershell
.\tetris_pc.exe --rom "C:\ruta\Tetris (USA).nes" --replay "C:\ruta\partida.ttr"
```

## Controles de PC

| Tecla | Acción |
|---|---|
| Enter / espacio | Confirmar |
| Flechas | Navegar, mover y caída suave |
| Z / X | Rotar antihorario / horario |
| Espacio durante partida | Caída instantánea |
| P | Pausa |
| Tab | Mostrar u ocultar siguiente pieza |
| M | Audio |
| N | Cambiar música |
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

El workflow descarga SDL2 oficial durante la compilación; no lo guarda en el repositorio.

```bash
cd android
gradle --no-daemon :app:assembleDebug
```

Requiere SDK 34, Build Tools 34.0.0, NDK 26.3.11579264, CMake 3.22.1 y Java 17.

## Legalidad y alcance

Este repositorio contiene código original de reimplementación, herramientas y documentación. No contiene ROM, bancos PRG/CHR, imágenes extraídas, música de Nintendo ni grabaciones OGG del cartucho. Los recursos se leen en memoria desde la copia legal suministrada por cada usuario.
