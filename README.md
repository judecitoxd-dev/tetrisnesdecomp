# Tetris NES — ports nativos para PC y Android

Versión **0.6** de una reimplementación portable de **Tetris (USA) para NES**, escrita en C99 con SDL2. Windows, Linux y Android ejecutan el mismo núcleo de reglas, carga de ROM, render, récords, ajustes y repeticiones.

El proyecto no distribuye la ROM, gráficos extraídos ni música del cartucho. Cada usuario proporciona su propia copia legal; durante la ejecución se leen los tiles CHR, la composición del título y las paletas desde esa copia.

## Novedades de v0.6

- APK horizontal e inmersivo para Android 6.0 o posterior, ARM64 y ARMv7.
- Selector Android SAF: importa la ROM legal sin permiso general de almacenamiento.
- Controles multitouch con cruceta, A/B, caída, Start, Select, ROM y EDIT.
- Botones movibles; tamaño, opacidad y posiciones persistentes.
- La capa táctil se oculta automáticamente al conectar un gamepad.
- Botón Atrás integrado con los menús y la partida.
- Windows y Linux pasan a v0.6 usando exactamente el mismo núcleo C99.
- CI genera APK y paquetes de escritorio, ejecuta pruebas Release y verifica que no se incluyan `.nes`, `.ttr` ni `.ogg`.

## Estado actual

- Modos A y B jugables a 60.0988 actualizaciones por segundo.
- Tablero 10×20, siete tetrominós, puntuación, líneas, niveles, estadísticas y récords.
- Orientaciones, gravedad, puntuación, progresión principal, RNG y borrado contrastados con tablas y rutinas identificadas en la ROM comprobada.
- Teclado, táctil y mando SDL con conexión en caliente.
- Fuente, título, bloques y paletas cargados desde la ROM legal.
- Opciones persistentes, selector de ROM, pantalla completa y repeticiones verificables.
- Paquetes OGG opcionales en escritorio y sintetizador incorporado como respaldo.
- Desensamblador recursivo NMOS 6502 por bancos MMC1, símbolos y referencias cruzadas.

Todavía no es una decompilación reproducible bit a bit. El título y los bloques ya usan composición y tiles del cartucho, pero otros menús, finales, demostración y música continúan como equivalentes nativos. Consulta:

- [`docs/PORT_STATUS.md`](docs/PORT_STATUS.md)
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

Otras revisiones estructuralmente compatibles pueden abrir, pero aparecen como no verificadas porque sus offsets y comportamiento aún no se han contrastado.

## Android

Instala el APK, ábrelo y selecciona tu ROM legal mediante el selector del sistema. La aplicación valida la cabecera, mapper y tamaños, y copia el archivo al almacenamiento privado. No solicita permiso para explorar todo el almacenamiento.

En modo **EDIT**, arrastra los controles para moverlos. **START** cambia el tamaño y **SEL** cambia la opacidad. La configuración se conserva entre sesiones.

El artefacto actual es un APK de depuración firmado automáticamente, apto para instalación y pruebas. No es todavía una publicación firmada para Play Store. Consulta [`docs/ANDROID_PORT.md`](docs/ANDROID_PORT.md).

## Ejecutar en PC

Coloca `Tetris (USA).nes` junto al ejecutable, selecciónala desde el diálogo o pásala por argumento:

```powershell
.\tetris_pc.exe --rom "C:\ruta\Tetris (USA).nes"
```

Para reproducir una partida:

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
| M | Activar o desactivar audio |
| N | Cambiar música |
| H | Ver récords |
| O | Opciones |
| L | Elegir otra ROM |
| F8 | Reproducir la última partida |
| F11 | Pantalla completa |
| R | Reiniciar después de perder |
| Retroceso | Volver |
| Esc | Salir |

### Mando

| Botón | Acción |
|---|---|
| Cruceta | Navegar, mover y caída suave |
| A / B | Confirmar o rotar |
| X | Opciones / caída instantánea |
| Y | Récords / siguiente pieza |
| Start | Confirmar, pausar o reiniciar |
| Back | Regresar |
| Hombro derecho | Reproducir última partida |

## Compilar PC

Windows:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix stage
```

Linux:

```bash
sudo apt install cmake build-essential libsdl2-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

CMake descarga SDL2_mixer estático cuando es necesario para proporcionar Ogg Vorbis sin una dependencia dinámica adicional.

## Compilar Android

El workflow descarga SDL2 2.30.11 oficialmente y lo usa como dependencia temporal. Requiere Java 17, Android SDK 34, Build Tools 34.0.0, NDK 26.3.11579264 y CMake 3.22.1.

```bash
cd android
gradle --no-daemon :app:assembleDebug
```

## Audio OGG opcional en escritorio

```bash
python tools/audio_pack.py recordings audio
./build/tetris_pc --rom "Tetris (USA).nes" --audio-pack ./audio
```

La ROM no contiene OGG; estos archivos deben crearse localmente desde capturas legales. Si el paquete está incompleto, el juego vuelve al sintetizador.

## Herramientas de decompilación

```bash
python tools/rom_info.py "Tetris (USA).nes"
python tools/rom_tables.py "Tetris (USA).nes"
python tools/disassemble_prg.py --self-test
python tools/disassemble_prg.py "Tetris (USA).nes" --output tetris_recursive.asm --report report.json
python tools/create_decomp_workspace.py "Tetris (USA).nes" mi_decomp_privado
```

No subas el espacio de trabajo privado: contiene bytes derivados de la ROM. El repositorio guarda únicamente código, símbolos, herramientas y documentación.

## Aviso legal

Este proyecto exige que cada usuario proporcione su propia copia legal. No subas ROMs, bancos, gráficos, música ni otros archivos extraídos. «Tetris» y los recursos originales pertenecen a sus respectivos titulares. Este proyecto es una reimplementación técnica no oficial.
