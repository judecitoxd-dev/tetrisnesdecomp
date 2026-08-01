# Tetris NES — port nativo para PC

Versión **0.4** de una reimplementación portable de **Tetris (USA) para NES**, escrita en C99 con SDL2.
El ejecutable no incorpora una CPU NES ni distribuye la ROM, gráficos, música o datos propietarios.
Cada usuario proporciona su copia legal de `Tetris (USA).nes`; el programa valida el archivo y lee
los tiles CHR y las paletas de nivel directamente de esa copia durante la ejecución.

## Novedades de v0.4

- Introducción editable de seis caracteres para los récords, con teclado y mando.
- Clasificación previa del resultado para abrir la pantalla de nombre solo cuando corresponde.
- Demostración automática después de 12 segundos de inactividad en el título.
- El modo demostración ejecuta el juego real con un controlador determinista que evalúa líneas,
  huecos, altura acumulada y desnivel del tablero.
- Final básico animado para B-Type con estrellas, cohete y resumen de nivel, altura y puntuación.
- Pruebas nuevas para edición de nombres, clasificación de récords y supervivencia del controlador
  de demostración.
- Validación C99 estricta y análisis estático de todos los módulos sin diagnósticos.

## Estado actual

- Juego nativo a 60.0988 actualizaciones por segundo.
- Modos A y B, tablero 10×20, siete tetrominós, puntuación, líneas, niveles y estadísticas.
- Orientaciones, gravedad, puntuación, progresión principal, RNG y borrado contrastados con tablas y
  rutinas de la ROM comprobada.
- Teclado y mando SDL con conexión en caliente.
- Audio, pantalla completa, siguiente pieza conmutable y récords persistentes.
- Compilación y pruebas automáticas para Windows y Linux.

Todavía no es una decompilación reproducible bit a bit. Faltan el menú y presentación exactos del
cartucho, finales y cohetes fieles, demostración basada en las entradas originales, paletas de
sprites completas y el controlador musical/APU original. Consulta [`docs/PORT_STATUS.md`](docs/PORT_STATUS.md)
y [`docs/ROM_MAP.md`](docs/ROM_MAP.md).

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

Otras revisiones estructuralmente compatibles pueden abrir, pero se muestran como no verificadas
porque sus offsets y comportamiento no se han contrastado.

## Compilar en Windows

Necesitas CMake, Git y Visual Studio 2022 con **Desktop development with C++**. SDL2 se descarga en
la configuración cuando no está instalado.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

Copia tu ROM junto a `build\Release\tetris_pc.exe` con el nombre `Tetris (USA).nes`, o pásala como
argumento:

```powershell
.\build\Release\tetris_pc.exe "C:\ruta\Tetris (USA).nes"
```

También puedes arrastrar otra ROM compatible sobre la ventana.

## Compilar en Linux

En Debian o Ubuntu:

```bash
sudo apt install cmake build-essential libsdl2-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/tetris_pc "/ruta/Tetris (USA).nes"
```

## Menús y controles

| Tecla | Acción |
|---|---|
| Enter / espacio | Confirmar |
| Flechas | Navegar, mover y caída suave |
| Z / X | Rotar antihorario / horario |
| Espacio durante partida | Caída instantánea opcional de PC |
| P | Pausa |
| Tab | Mostrar u ocultar siguiente pieza |
| M | Activar o desactivar todo el audio |
| N | Cambiar música 1 / 2 / 3 / apagada |
| H | Ver récords |
| F11 | Pantalla completa |
| R | Reiniciar después de perder; salir del final B-Type |
| Retroceso | Volver al menú anterior o al título |
| Letras y números | Editar el nombre de un récord |
| Esc | Salir |

### Mando

| Botón | Acción |
|---|---|
| Cruceta | Navegar, mover y caída suave |
| A / B | Confirmar o rotar |
| X | Caída instantánea |
| Y | Récords en el título / siguiente pieza durante partida |
| Start | Confirmar, pausar o reiniciar |
| Back | Regresar |

## Récords locales

Se guardan tres entradas para cada modo. El port usa la ruta de preferencias que SDL asigna a
`YlPorts/TetrisNESPC`; si no puede obtenerla, utiliza `tetris_scores.txt` junto al ejecutable.
Al conseguir un puesto aparece una pantalla para escribir seis letras, números o guiones. También
puedes usar las flechas para cambiar cada carácter con teclado o mando.

## Pruebas

El núcleo no necesita SDL2 para compilarse:

```bash
cmake -S . -B build-core -DTETRIS_BUILD_APP=OFF -DTETRIS_FETCH_SDL2=OFF
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
```

Inspección de la ROM legal:

```bash
python tools/rom_info.py "Tetris (USA).nes"
python tools/rom_tables.py "Tetris (USA).nes"
```

## Aviso legal

Este proyecto exige que cada usuario proporcione su propia copia legal. No subas ROMs, gráficos,
música ni archivos extraídos al repositorio. «Tetris» y los recursos del juego original pertenecen
a sus respectivos titulares. Este proyecto es una reimplementación técnica no oficial.
