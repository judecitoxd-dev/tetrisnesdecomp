# Estado de decompilación y ports

## Resumen de v0.12

La duodécima fase convierte el renderer APU aislado de v0.11 en el backend de
audio interactivo del port. Windows, Linux y Android ejecutan ahora el
controlador 6502 original desde la ROM legal mientras se juega, durante la demo
y al reproducir una partida.

El callback SDL consume audio producido por `$E006`/`$E000`, mantiene las
escrituras `$4000-$4017` y usa el sintetizador alternativo únicamente cuando la
ROM no coincide con el dump verificado o el driver falla. Los paquetes OGG del
usuario siguen teniendo prioridad cuando están configurados.

## Progreso estimado

| Área | Estado aproximado |
|---|---:|
| Port nativo jugable básico | 99% |
| Port Android jugable | 97% |
| Controles, tablero y puntuación | 99% |
| Modo A | 96% |
| Modo B | 92% |
| Integración y empaquetado | 99% |
| Carga legal de recursos desde ROM | 99% |
| Fidelidad de reglas y timings principales | 85% |
| Pantallas y animaciones originales/equivalentes | 94% |
| Audio original interactivo | 92% |
| Paquetes OGG del usuario en PC | 90% |
| Renderizado automático del APU original | 78% |
| Decompilación etiquetada/verificada del PRG 6502 | 39% |
| Correspondencia reproducible con la ROM | 18% |

El 78% del APU significa que el driver original ya produce música y efectos
durante el juego en las tres plataformas. El porcentaje restante corresponde a
exactitud temporal: ciclos por instrucción, IRQ, secuenciador preciso y robos de
ciclos del DMC.

## Implementado en v0.12

- Backend `ROM APU` dentro del callback SDL mono de 48 kHz.
- Pistas originales A/B/C mediante los comandos 3, 4 y 5 del driver.
- Cambio automático a las variantes allegro 6, 7 y 8 cuando el tablero alcanza
  la zona alta usada por el juego original.
- Música desactivada mediante el comando `$FF`.
- Efectos originales de movimiento, rotación, bloqueo, línea, Tetris, subida de
  nivel, cortina de derrota y final.
- Eventos enviados a los slots originales en `$06F0-$06F4`.
- Música controlada por `musicTrack` en `$06F5`.
- Recarga segura del driver al seleccionar o arrastrar otra ROM.
- Desactivación automática del backend si el CRC no tiene offsets verificados.
- OGG como backend opcional y sintetizador original como respaldo.
- CPU 6502, APU y driver compilados también dentro del APK ARM64/ARMv7.
- Audio original activo durante partida, demo y verificación de replay.
- Pruebas unitarias para cada comando musical y efecto conectado.

## Validación de la fase

- La capa SDL y el puente del driver pasan análisis sintáctico C99 estricto.
- Las pruebas usan un PRG artificial y no contienen datos de Nintendo.
- Se comprueba selección de pista, apagado, siete clases de evento y salida no
  silenciosa.
- GitHub Actions compila Windows, Linux y Android antes de fusionar la fase.
- El APK continúa rechazando `.nes`, `.ttr`, `.ogg` y `.wav` incrustados.

## Diferencias conocidas

- El controlador se actualiza una vez por fotograma NTSC, pero las instrucciones
  6502 aún no avanzan el APU por su número real de ciclos.
- El secuenciador de frames y los barridos se aproximan a nivel de muestra.
- DMC no roba ciclos al procesador y sus IRQ no están conectadas.
- Las IRQ de frame tampoco llegan todavía al intérprete 6502.
- Falta comparar todas las escrituras de cada pista y efecto contra Mesen.
- La demo usa entradas y piezas originales, pero las reglas principales siguen
  ejecutándose en C.
- Falta completar el movimiento de entrada de la catedral B-Type.
- No existe una construcción 6502 enlazable o binariamente idéntica.

## Próxima fase hacia exactitud

1. Añadir conteo de ciclos a cada instrucción y modo de direccionamiento 6502.
2. Avanzar APU, frame counter, barridos y DMC por ciclos de CPU.
3. Implementar robos de ciclos y señales IRQ de DMC/frame.
4. Automatizar la comparación de todas las pistas y efectos contra Mesen.
5. Corregir la primera escritura APU divergente hasta lograr igualdad de traza.
6. Continuar la comparación de RAM de la demo y corregir su primera divergencia.
7. Completar la máquina de movimiento de la catedral B-Type.
8. Ampliar la traducción del PRG y preparar una construcción 6502 enlazable.
