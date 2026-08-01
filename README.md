# Tetris NES — port nativo para PC

Versión **0.2** de una reimplementación portable de **Tetris (USA) para NES**, escrita en C99 con SDL2.
El ejecutable no incorpora una CPU NES ni distribuye la ROM, gráficos, música o datos propietarios.
Cada usuario proporciona su copia legal de `Tetris (USA).nes`; el programa valida el archivo y
decodifica su CHR en tiempo de ejecución para dibujar la tipografía.

## Novedades de v0.2

- Coordenadas, pivotes y orientaciones iniciales de las 19 configuraciones de piezas traducidas del
  programa 6502.
- Tabla NTSC de gravedad del cartucho y progresión especial de niveles iniciales 10–19.
- Randomizador LFSR de dos bytes con contador de apariciones y una repetición, siguiendo la rutina
  original.
- Animación de borrado centro hacia afuera: cinco pasos, uno cada cuatro fotogramas.
- Retardo de entrada según la altura donde se bloquea la pieza y cortina de fin de partida.
- Pantalla de título y selector de nivel 0–19.
- Soporte de teclado y mando SDL, conexión y desconexión en caliente.
- Efectos sintetizados para movimiento, rotación, bloqueo, líneas, Tetris, nivel y derrota.
- Cola de pulsaciones: las entradas rápidas ya no se pierden entre actualizaciones fijas.
- Estadísticas por pieza, ocultar la siguiente pieza, pantalla completa y audio conmutable.
- Herramienta `rom_tables.py` para verificar offsets y vectores con una ROM legal.

La caída instantánea y la selección directa de niveles 10–19 son mejoras opcionales de PC; no
formaban parte del modo A original.

## Estado actual

- Juego nativo, redimensionable y jugable a 60.0988 actualizaciones por segundo.
- Tablero 10×20, siete tetrominós, puntuación, líneas, niveles y estadísticas.
- DAS, caída suave, pausa, siguiente pieza y reinicio.
- Reglas principales y pequeñas tablas contrastadas con la ROM comprobada.
- Compilación y pruebas automáticas para Windows y Linux.

Todavía no es una decompilación reproducible bit a bit. Faltan los menús completos del cartucho,
modo B, récords, finales, cohetes, paletas exactas por nivel y el motor musical/APU original. El
estado detallado está en [`docs/PORT_STATUS.md`](docs/PORT_STATUS.md), y los offsets identificados
en [`docs/ROM_MAP.md`](docs/ROM_MAP.md).

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

El cargador puede abrir otras revisiones estructuralmente compatibles, pero muestra una advertencia
porque sus offsets y comportamiento todavía no han sido contrastados.

## Compilar en Windows

Necesitas CMake, Git y Visual Studio 2022 con **Desktop development with C++**. SDL2 se descarga
durante la configuración.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

Copia tu ROM junto a `build\Release\tetris_pc.exe` con el nombre `Tetris (USA).nes`, o ejecútalo así:

```powershell
.\build\Release\tetris_pc.exe "C:\ruta\Tetris (USA).nes"
```

También puedes arrastrar la ROM sobre la ventana.

## Compilar en Linux

Instala SDL2 y CMake; por ejemplo, en Debian/Ubuntu:

```bash
sudo apt install cmake build-essential libsdl2-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/tetris_pc "/ruta/Tetris (USA).nes"
```

## Controles de teclado

| Tecla | Acción |
|---|---|
| Flechas izquierda/derecha | Mover o cambiar nivel |
| Flecha abajo | Caída suave o bajar nivel |
| Flecha arriba | Subir nivel / rotar en partida |
| Z / X | Rotar antihorario / horario |
| Espacio | Caída instantánea de PC |
| Enter | Confirmar pantalla |
| P | Pausa |
| Tab | Mostrar u ocultar siguiente pieza |
| M | Activar o desactivar audio |
| F11 | Pantalla completa |
| R | Reiniciar tras perder |
| Retroceso | Volver al título |
| Esc | Salir |

## Mando

| Botón | Acción |
|---|---|
| Cruceta | Mover, caída suave y selector de nivel |
| A / B | Rotar |
| X | Caída instantánea |
| Y | Mostrar u ocultar siguiente pieza |
| Start | Confirmar, pausar o reiniciar tras perder |
| Back | Volver al título |

## Pruebas

Las pruebas no necesitan SDL2:

```bash
cmake -S . -B build-core -DTETRIS_BUILD_APP=OFF -DTETRIS_FETCH_SDL2=OFF
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
```

Inspección básica de la ROM:

```bash
python tools/rom_info.py "Tetris (USA).nes"
python tools/rom_tables.py "Tetris (USA).nes"
```

## Aviso legal

Este proyecto exige que cada usuario proporcione su propia copia legal. No subas ROMs, gráficos,
música ni archivos extraídos al repositorio. «Tetris» y los recursos del juego original pertenecen
a sus respectivos titulares. Este proyecto es una reimplementación técnica no oficial.
