# Tetris NES — port nativo para PC

Versión **0.5** de una reimplementación portable de **Tetris (USA) para NES**, escrita en C99 con SDL2.
El programa no incorpora una CPU NES ni distribuye la ROM, gráficos o música del cartucho. Cada
usuario proporciona su propia copia legal; el port valida el archivo y lee los tiles CHR y las
paletas de nivel durante la ejecución.

## Novedades de v0.5

- Opciones persistentes para audio, música, pantalla completa, escalado entero, demostración,
  caída instantánea y siguiente pieza.
- Recuerda el tamaño de la ventana, el último modo, nivel, altura y la última ROM válida.
- Selector de ROM desde el propio programa: diálogo nativo en Windows y selector externo compatible
  en Linux/macOS cuando está disponible.
- Grabación automática de la última partida en un formato de repetición determinista.
- Reproducción con **F8** y verificación del hash final para detectar desincronizaciones.
- Herramienta `tetris_replay_verify` para comprobar repeticiones sin abrir una ventana.
- Pantalla de título reconstruida en tiempo de ejecución desde el nametable, atributos, paleta y CHR de la ROM comprobada.
- Bloques originales de los tiles CHR `$7B`–`$7D`, con la paleta correspondiente a cada nivel.
- Paquetes portables mediante CPack y artefactos automáticos para Windows y Linux.
- Pruebas que permanecen activas también en compilaciones Release; ya no dependen de `assert`.
- Desensamblador recursivo NMOS 6502 por bancos MMC1, mapa de símbolos, referencias cruzadas y
  generador de un espacio de trabajo privado a partir de la ROM legal del usuario.

## Estado actual

- Modos A y B jugables a 60.0988 actualizaciones por segundo.
- Tablero 10×20, siete tetrominós, puntuación, líneas, niveles, estadísticas y récords.
- Orientaciones, gravedad, puntuación, progresión principal, RNG y borrado contrastados con tablas y
  rutinas identificadas en la ROM comprobada.
- Teclado y mando SDL con conexión en caliente.
- Fuente, título, bloques y paletas cargados desde la ROM legal; no se distribuyen imágenes extraídas.
- Audio nativo, tres composiciones chiptune originales, paquetes OGG opcionales, pantalla completa y opciones persistentes.
- Demostración automática, final equivalente de B-Type y repeticiones verificables.
- Compilación, pruebas, instalación y empaquetado automáticos para Windows y Linux.

Todavía no es una decompilación reproducible bit a bit. El título y los bloques ya usan la composición
y los tiles originales de la ROM comprobada, pero otros menús, finales, demostración y música siguen
siendo equivalentes nativos, no traducciones completas de sus rutinas originales. Consulta
[`docs/PORT_STATUS.md`](docs/PORT_STATUS.md), [`docs/ROM_MAP.md`](docs/ROM_MAP.md),
[`docs/REPLAY_FORMAT.md`](docs/REPLAY_FORMAT.md), [`docs/AUDIO_PACK.md`](docs/AUDIO_PACK.md) y
[`docs/DECOMP_TOOLS.md`](docs/DECOMP_TOOLS.md).

## ROM comprobada

| Propiedad | Valor |
|---|---|
| Archivo | `Tetris (USA).nes` |
| Formato | NES 2.0 |
| Mapper | MMC1 / mapper 1 |
| PRG ROM | 32 KiB |
| CHR ROM | 16 KiB |
| CRC32 | `D16EA396` |
| SHA-1 | `3026d28b63d94c921fe58364f8b0659d10b5a0ac` |
| NMI / RESET / IRQ | `$8005` / `$FF00` / `$804A` |

Otras revisiones estructuralmente compatibles pueden abrir, pero aparecen como no verificadas porque
sus offsets y comportamiento aún no se han contrastado.

## Ejecutar

Al primer inicio, coloca `Tetris (USA).nes` junto al ejecutable o selecciona tu archivo legal desde
el diálogo. También puedes arrastrar una ROM compatible sobre la ventana.

```powershell
.\tetris_pc.exe --rom "C:\ruta\Tetris (USA).nes"
```

El port recuerda la última ROM válida. Para reproducir un archivo concreto:

```powershell
.\tetris_pc.exe --rom "C:\ruta\Tetris (USA).nes" --replay "C:\ruta\partida.ttr"
```

## Compilar en Windows

Necesitas CMake, Git y Visual Studio 2022 con **Desktop development with C++**. Cuando SDL2 o
SDL2_mixer no están instalados, CMake descarga versiones estáticas durante la configuración.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix stage
cpack --config build\CPackConfig.cmake -C Release
```

También puedes ejecutar `build_windows.bat`.

## Compilar en Linux

En Debian o Ubuntu:

```bash
sudo apt install cmake build-essential libsdl2-dev libsdl2-mixer-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/tetris_pc --rom "/ruta/Tetris (USA).nes"
```


## Audio OGG opcional

El port puede usar música y efectos Ogg Vorbis creados localmente desde capturas de la copia legal del
usuario. La ROM no contiene archivos OGG: el NES genera el sonido mediante su APU, así que primero hay
que renderizar o capturar cada pista y efecto.

```bash
python tools/audio_pack.py recordings audio
./build/tetris_pc --rom "Tetris (USA).nes" --audio-pack ./audio
```

También se acepta la variable `TETRIS_AUDIO_PACK`. Si el paquete está incompleto o SDL2_mixer no está
disponible, el juego vuelve al sintetizador incluido. Consulta
[`docs/AUDIO_PACK.md`](docs/AUDIO_PACK.md) para los once nombres requeridos y el flujo legal completo.

## Menús y controles

| Tecla | Acción |
|---|---|
| Enter / espacio | Confirmar |
| Flechas | Navegar, mover y caída suave |
| Z / X | Rotar antihorario / horario |
| Espacio durante partida | Caída instantánea, si está habilitada |
| P | Pausa |
| Tab | Mostrar u ocultar siguiente pieza |
| M | Activar o desactivar audio |
| N | Cambiar música 1 / 2 / 3 / apagada |
| H | Ver récords |
| O | Abrir opciones desde el título |
| L | Elegir otra ROM desde el título |
| F8 | Reproducir y verificar la última partida |
| F11 | Pantalla completa |
| R | Reiniciar después de perder |
| Retroceso | Volver al menú anterior o al título |
| Letras y números | Editar el nombre de un récord |
| Esc | Salir |

### Mando

| Botón | Acción |
|---|---|
| Cruceta | Navegar, mover y caída suave |
| A / B | Confirmar o rotar |
| X | Opciones en el título / caída instantánea en partida |
| Y | Récords en el título / siguiente pieza en partida |
| Start | Confirmar, pausar o reiniciar |
| Back | Regresar |
| Hombro derecho | Reproducir la última partida desde el título |

## Archivos persistentes

SDL asigna una carpeta de preferencias a `YlPorts/TetrisNESPC`. Allí se guardan:

- `settings.ini`: opciones y última ROM;
- `scores.txt`: los tres mejores resultados de cada modo;
- `last_replay.ttr`: última partida grabada.

## Verificar una repetición

```bash
./build/tetris_replay_verify /ruta/last_replay.ttr
```

La herramienta devuelve código `0` cuando el hash reproducido coincide, `1` cuando hay una
sincronización distinta y `2` cuando el archivo no es válido. El formato está documentado en
[`docs/REPLAY_FORMAT.md`](docs/REPLAY_FORMAT.md).

## Herramientas de ROM y decompilación

Inspección básica:

```bash
python tools/rom_info.py "Tetris (USA).nes"
python tools/rom_tables.py "Tetris (USA).nes"
```

Autoprueba del desensamblador y generación de listados:

```bash
python tools/disassemble_prg.py --self-test
python tools/disassemble_prg.py "Tetris (USA).nes" --output tetris_recursive.asm --report report.json
python tools/disassemble_prg.py "Tetris (USA).nes" --aggressive --output tetris_aggressive.asm
```

Para crear un espacio de trabajo **privado** con los bancos y listados derivados de tu ROM:

```bash
python tools/create_decomp_workspace.py "Tetris (USA).nes" mi_decomp_privado
```

No subas esa carpeta: contiene bytes extraídos de tu copia. El repositorio solo guarda herramientas,
símbolos y documentación. Más detalles en [`docs/DECOMP_TOOLS.md`](docs/DECOMP_TOOLS.md).

## Aviso legal

Este proyecto exige que cada usuario proporcione su propia copia legal. No subas ROMs, gráficos,
música, bancos ni otros archivos extraídos. «Tetris» y los recursos del juego original pertenecen a
sus respectivos titulares. Este proyecto es una reimplementación técnica no oficial.
