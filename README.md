# Tetris NES — port nativo para PC

Primera base jugable y portable de **Tetris (USA) para NES**, escrita en C99 con SDL2.
El repositorio **no contiene la ROM ni recursos gráficos de Nintendo**. Al iniciar, el programa
lee una copia legal de `Tetris (USA).nes` y decodifica su CHR de 2 bits por píxel para dibujar la
tipografía original.

## Estado actual

- Juego nativo; no ejecuta una CPU NES ni incorpora un emulador.
- Tablero 10×20, siete tetrominós, siguiente pieza, puntuación, líneas y niveles.
- Gravedad aproximada a la tabla NTSC del juego de NES.
- Rotación sin wall-kicks y randomizador de una repetición, al estilo NES.
- DAS horizontal, caída suave, pausa, reinicio y caída instantánea opcional de PC.
- Ventana redimensionable con escalado entero y actualización a 60.0988 Hz.
- Carga de la ROM por argumento, desde la carpeta del ejecutable o arrastrándola a la ventana.
- Compilación automática para Windows y Linux mediante GitHub Actions.

La lógica ya es jugable, pero esto todavía **no es una decompilación exacta bit a bit**. Consulta
[`docs/PORT_STATUS.md`](docs/PORT_STATUS.md) para ver qué falta para igualar por completo el binario.

## ROM comprobada

| Propiedad | Valor |
|---|---|
| Archivo | `Tetris (USA).nes` |
| Formato | NES 2.0 |
| Mapper | MMC1 / mapper 1 |
| PRG ROM | 32 KiB |
| CHR ROM | 16 KiB |
| CRC32 del archivo probado | `D16EA396` |
| SHA-1 del archivo probado | `3026d28b63d94c921fe58364f8b0659d10b5a0ac` |

Otras revisiones compatibles pueden abrir, pero el port muestra una advertencia porque no han sido
comparadas todavía.

## Compilar en Windows

Necesitas CMake, Git y Visual Studio 2022 con el componente «Desktop development with C++».
SDL2 se descarga durante la configuración.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

Copia tu ROM junto a `build\Release\tetris_pc.exe` con el nombre `Tetris (USA).nes`, o ejecútalo así:

```powershell
.\build\Release\tetris_pc.exe "C:\ruta\Tetris (USA).nes"
```

También puedes arrastrar la ROM sobre la ventana del juego.

## Compilar en Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/tetris_pc "/ruta/Tetris (USA).nes"
```

## Controles

| Tecla | Acción |
|---|---|
| Flechas izquierda/derecha | Mover |
| Flecha abajo | Caída suave |
| Z / X / flecha arriba | Rotar |
| Espacio | Caída instantánea |
| P | Pausa |
| R | Reiniciar tras perder |
| Esc | Salir |

## Pruebas

```bash
cmake -S . -B build -DTETRIS_BUILD_APP=OFF -DTETRIS_FETCH_SDL2=OFF
cmake --build build --target tetris_game_tests
ctest --test-dir build --output-on-failure
```

Para inspeccionar una ROM sin abrir el juego:

```bash
python tools/rom_info.py "Tetris (USA).nes"
```

## Aviso legal

Este proyecto exige que cada usuario proporcione su propia copia legal. No subas ROMs, gráficos,
música ni archivos extraídos al repositorio. «Tetris» y los recursos del juego original pertenecen
a sus respectivos titulares. Este proyecto es una reimplementación técnica no oficial.
