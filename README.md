# Tetris NES — ports nativos para PC y Android

Versión **0.9** de una reimplementación portable de **Tetris (USA) para NES**, escrita en C99 con SDL2. Windows, Linux y Android ejecutan el mismo núcleo de reglas, carga de ROM, render, récords, ajustes, demo y repeticiones.

El proyecto no distribuye la ROM, gráficos extraídos ni música del cartucho. Cada usuario proporciona su propia copia legal. El port valida el archivo y lee durante la ejecución los bancos CHR, paletas, tablas, streams PPU, secuencias de demo y descriptores OAM necesarios.

## Novedades de v0.9

- Demo original NTSC reproducida desde los comandos de botones en `PRG+0x5D00`.
- Secuencia de piezas de la demo leída desde `PRG+0x5F00`.
- Duraciones y pulsaciones nuevas interpretadas como la rutina 6502 identificada.
- Cursores A-Type/B-Type, nivel/altura y récord reconstruidos desde OAM.
- Final B-Type normal reconstruido desde el PRG y CHR de la ROM.
- Castillo B-Type de nivel 9/19 con seis composiciones por altura.
- Concierto B-Type con personajes y fotogramas OAM originales.
- Final A-Type para 30k, 50k, 70k, 100k y 120k puntos.
- Dos fondos A-Type, incluido el parche especial para ≥120,000.
- Cinco cohetes y diez chorros reconstruidos desde quince metasprites OAM.
- Orden final A-Type → entrada de nombre → récords.
- Verificadores estructurales de demo, PPU, OAM y finales sin extraer assets.
- Android, Windows y Linux compilan la misma implementación.

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

Los recursos se decodifican en memoria desde la ROM CRC32 `D16EA396`; no se escriben PNG, bancos CHR, nametables descomprimidas ni tablas de demo. Una ROM con otro CRC usa los renderers y la demo alternativos.

## Estado actual

- Modos A y B jugables a 60.0988 actualizaciones por segundo.
- Orientaciones, gravedad, puntuación, transiciones, RNG, DAS y borrado contrastados con datos identificados en la ROM.
- Teclado, controles táctiles y gamepad sobre el mismo núcleo C99.
- Android horizontal e inmersivo, selector SAF, almacenamiento privado y controles editables.
- Opciones persistentes, selector de ROM, récords y repeticiones deterministas.
- Paquetes OGG opcionales en PC y sintetizador incorporado como respaldo.
- Desensamblador NMOS 6502 por bancos MMC1 con símbolos y referencias cruzadas.

Todavía no es una decompilación bit a bit. La demo usa las entradas y piezas originales, pero su resultado todavía depende del modelo de reglas C del port. El controlador APU, algunas entradas animadas de finales y la reconstrucción enlazable del PRG continúan pendientes.

Documentación:

- [`docs/PORT_STATUS.md`](docs/PORT_STATUS.md)
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

## Estado de CI

El código nuevo pasa validación local C99 estricta, análisis estático, pruebas Release y harnesses SDL2 con la ROM comprobada. Los últimos jobs de GitHub Actions terminan antes de crear `Set up job` y no generan pasos, registros ni artefactos. Por ello v0.9 permanece sin fusionar y el APK v0.8 continúa siendo la última versión Android publicada y validada.

## Legalidad

El repositorio contiene código original, herramientas y documentación. No contiene ROM, PRG/CHR extraído, imágenes, música de Nintendo, tablas de demo ni grabaciones OGG del cartucho.
