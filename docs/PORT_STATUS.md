# Estado de decompilación y ports

## Resumen de v0.11

La undécima fase añade dos piezas necesarias para comparar el port con el juego
original: captura automática de RAM/APU en Mesen y renderizado del controlador
de sonido 6502 directamente desde la ROM legal.

El nuevo renderer ejecuta la entrada de inicialización `$E006`, llama `$E000`
por fotograma NTSC y sintetiza los registros escritos en `$4000-$4017`. También
produce una traza que puede compararse fotograma por fotograma con Mesen.

## Progreso estimado

| Área | Estado aproximado |
|---|---:|
| Port nativo jugable básico | 99% |
| Port Android jugable | 95% |
| Controles, tablero y puntuación | 99% |
| Modo A | 96% |
| Modo B | 92% |
| Integración y empaquetado | 98% |
| Carga legal de recursos desde ROM | 99% |
| Fidelidad de reglas y timings principales | 85% |
| Pantallas y animaciones originales/equivalentes | 94% |
| Efectos y música alternativos | 88% |
| Paquetes OGG del usuario en PC | 90% |
| Renderizado automático del APU original | 55% |
| Decompilación etiquetada/verificada del PRG 6502 | 38% |
| Correspondencia reproducible con la ROM | 15% |

El 55% de APU significa que ya se ejecuta el driver original y se genera WAV,
pero todavía falta exactitud por ciclo y sustituir el backend interactivo de
todas las plataformas.

## Implementado en v0.11

- Intérprete C99 del conjunto oficial NMOS 6502/2A03.
- Mapeo de 2 KiB de RAM y PRG de 32 KiB en `$8000-$FFFF`.
- Entrada original de inicialización del audio en `$E006`.
- Actualización del controlador original en `$E000` por fotograma.
- Selección de las pistas `1..10` mediante `musicTrack` en `$06F5`.
- Registros APU `$4000-$4013`, `$4015` y `$4017`.
- Dos pulsos, triángulo, ruido y reproducción DMC.
- Envolventes, contadores de longitud, barridos y contador lineal.
- Mezcla no lineal pulse/TND y filtro de componente continua.
- WAV mono PCM16 a 48 kHz con `tetris_apu_render`.
- Hash y CSV de escrituras APU por fotograma.
- Script Mesen para capturar RAM de juego y escrituras APU automáticamente.
- Pruebas sin ROM usando un PRG artificial.

## Uso rápido

```bash
tetris_apu_render "Tetris (USA).nes" 1 60 music-1.wav port-apu.csv
```

Comparación contra Mesen:

```bash
python tools/trace_compare.py mesen-apu.csv port-apu.csv --columns apu_writes
```

## Validación realizada

- Los módulos nuevos compilan como C99 con `-Wall -Wextra -Wpedantic -Werror`.
- Las 151 instrucciones oficiales tienen implementación en el intérprete.
- `apu tests: OK` con un PRG artificial.
- `apu_render --self-test` genera muestras no silenciosas y 16 escrituras APU.
- Firma local de la autoprueba: `8074cf27017db606`.
- El comparador de trazas sigue funcionando sin ROM.

## Diferencias conocidas

- El secuenciador APU se aproxima a nivel de muestra, no por ciclo de CPU.
- DMC no modela todavía los robos de ciclos al procesador.
- IRQ de frame y DMC no se conectan al intérprete.
- El renderer ejecuta el controlador musical aislado; no ejecuta todo el juego.
- El audio original aún no reemplaza el backend interactivo SDL en PC/Android.
- Debe compararse la secuencia exacta de escrituras contra una captura Mesen.
- La demo usa datos originales, pero las reglas principales siguen en C.
- No existe una construcción 6502 enlazable o binariamente idéntica.

## Próxima fase hacia exactitud

1. Capturar cada pista con `mesen_trace.lua` y comparar `apu_writes`.
2. Corregir la primera escritura divergente del controlador aislado.
3. Hacer el secuenciador de frames y barridos dependiente de ciclos 2A03.
4. Implementar robos de ciclos, IRQ y reinicio exacto del DMC.
5. Conectar el renderer verificado al backend interactivo de PC y Android.
6. Continuar la comparación de RAM de la demo y corregir su primera divergencia.
7. Completar la catedral B-Type y ampliar la traducción del PRG.
8. Crear una construcción 6502 enlazable.
