# Tetris NES — ports nativos para PC y Android

Versión **0.10** de una reimplementación portable de **Tetris (USA) para NES**,
escrita en C99 con SDL2. Windows, Linux y Android ejecutan el mismo núcleo de
reglas, carga de ROM, render, récords, demo, finales, ajustes y repeticiones.

El proyecto no distribuye la ROM, gráficos extraídos ni música del cartucho.
Cada usuario proporciona su propia copia legal. El port valida el archivo y lee
durante la ejecución los bancos CHR, paletas, streams PPU, secuencias de demo y
descriptores OAM necesarios.

## Novedades de v0.10

- Traza de demo **v2** con una fila por fotograma.
- Máscara de entrada, hash del tablero, semilla RNG y contadores internos.
- Estado de pieza, posición, rotación, puntuación, líneas, nivel, DAS y fases.
- `tools/trace_compare.py` para comparar el port contra una captura de emulador.
- Detección del primer fotograma y campo divergente.
- Informes legibles y JSON, selección de columnas y normalización decimal/hex/bin.
- Autoprueba sin ROM para el comparador.
- GitHub Actions vuelve a ejecutarse al sincronizar cambios de un pull request.
- Artefactos y versiones de Android, Windows y Linux actualizados a v0.10.

La comparación de trazas crea la infraestructura para localizar diferencias
reales entre el modelo C y el programa 6502. Todavía no significa paridad por
ciclo ni reconstrucción binaria del PRG.

## Recursos originales ya utilizados

- Cuatro bancos CHR de 4 KiB.
- Fuente y tiles de los tetrominós.
- Paletas de menús, niveles y finales.
- Pantalla de título.
- Selección A-Type/B-Type.
- Selección de nivel/altura.
- Marco principal de partida.
- Pantallas de récords y entrada de nombre.
- Cursores OAM de tipo, nivel/altura y nombre.
- Entradas y secuencia de piezas de la demo NTSC.
- Fondos y reparto de finales B-Type.
- Fondos, cohetes y chorros de finales A-Type.

Los recursos se decodifican en memoria desde la ROM CRC32 `D16EA396`; no se
escriben PNG, bancos CHR, nametables descomprimidas, tablas de demo ni sprites
extraídos. Una ROM con otro CRC usa renderers y secuencias alternativas para
evitar offsets no verificados.

## Estado actual

- Modos A y B jugables a 60.0988 actualizaciones por segundo.
- Orientaciones, gravedad, puntuación, transiciones, RNG, DAS y borrado
  contrastados con datos identificados en la ROM.
- Demo NTSC ejecutada desde sus comandos de botones y secuencia de piezas.
- Menús, récords y finales reconstruidos desde PPU, CHR y OAM de la ROM legal.
- Teclado, controles táctiles y gamepad sobre el mismo núcleo C99.
- Android horizontal e inmersivo, selector SAF, almacenamiento privado y
  controles editables.
- Opciones persistentes, selector de ROM, récords y repeticiones deterministas.
- Paquetes OGG opcionales en PC y sintetizador original como respaldo.
- Desensamblador NMOS 6502 por bancos MMC1 con símbolos y referencias cruzadas.
- Comparación reproducible de trazas fotograma por fotograma.

Todavía no es una decompilación bit a bit. El controlador APU, parte del
movimiento de la catedral B-Type, la validación contra RAM de emulador y una
construcción 6502 enlazable continúan pendientes.

## Documentación

- [`docs/PORT_STATUS.md`](docs/PORT_STATUS.md)
- [`docs/TRACE_COMPARISON.md`](docs/TRACE_COMPARISON.md)
- [`docs/ROM_SCREENS.md`](docs/ROM_SCREENS.md)
- [`docs/ROM_DEMO_OAM.md`](docs/ROM_DEMO_OAM.md)
- [`docs/ROM_TYPE_A_ENDING.md`](docs/ROM_TYPE_A_ENDING.md)
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
4. La app valida cabecera, mapper y tamaños y copia el archivo al
   almacenamiento privado.

La capa táctil incluye cruceta, A/B, caída, Start, Select, ROM y EDIT. En modo
EDIT se mueven los botones; START cambia el tamaño y SEL la opacidad. Al
conectar un mando físico, la capa se oculta.

## Ejecutar en PC

```powershell
.\tetris_pc.exe --rom "C:\ruta\Tetris (USA).nes"
```

Repetición:

```powershell
.\tetris_pc.exe --rom "C:\ruta\Tetris (USA).nes" --replay "C:\ruta\partida.ttr"
```

## Generar y comparar una traza

```bash
tetris_demo_verify "Tetris (USA).nes" port-trace.csv
python tools/trace_compare.py emulator.csv port-trace.csv
```

Elegir campos concretos:

```bash
python tools/trace_compare.py emulator.csv port-trace.csv \
  --columns active,next,x,y,rotation,score,lines,level,rng_seed
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

Requiere SDK 34, Build Tools 34.0.0, NDK 26.3.11579264, CMake 3.22.1 y Java 17.

## Autopruebas sin ROM

```bash
python tools/disassemble_prg.py --self-test
python tools/rom_assets_verify.py --self-test
python tools/type_a_ending_verify.py --self-test
python tools/cathedral_verify.py --self-test
python tools/trace_compare.py --self-test
```

## Legalidad

El repositorio contiene código original, herramientas y documentación. No
contiene ROM, PRG/CHR extraído, imágenes, música de Nintendo, tablas de demo,
capturas de RAM ni grabaciones OGG del cartucho.
