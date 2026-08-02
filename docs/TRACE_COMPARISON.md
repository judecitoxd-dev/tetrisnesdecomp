# Comparación de trazas fotograma por fotograma

La v0.11 compara el estado del port C y las escrituras del APU contra capturas
obtenidas localmente en Mesen. El repositorio no incluye ROM, RAM, audio ni
datos capturados del juego.

## Traza del port

```bash
tetris_demo_verify "Tetris (USA).nes" port-trace.csv
```

El esquema v2 contiene entrada, tablero, piezas, posición, rotación,
puntuación, líneas, nivel, RNG, DAS, fases y contadores.

## Captura automática de Mesen

1. Abre la ROM legal en Mesen 2 o Mesen CE.
2. Permite acceso de Lua a I/O y funciones del sistema.
3. Ejecuta `tools/mesen_trace.lua`.
4. Reproduce la demo o escena deseada.

El script crea:

- `tetris-reference.csv`: RAM mapeada, hash del campo y escrituras APU;
- `tetris-apu-writes.csv`: solo `frame,apu_writes`.

Las escrituras se codifican en orden, por ejemplo:

```csv
frame,apu_writes
0,4017=C0|4015=0F|4000=9F|4002=FD|4003=08
```

## Render y traza del controlador original

```bash
tetris_apu_render "Tetris (USA).nes" 1 60 music.wav port-apu.csv
```

El renderer ejecuta el controlador 6502 de la ROM una vez por fotograma y
registra todas sus escrituras en `$4000-$4017`.

## Comparar APU

```bash
python tools/trace_compare.py tetris-apu-writes.csv port-apu.csv \
  --columns apu_writes --json apu-report.json
```

La salida muestra el primer fotograma cuya secuencia de registros difiere. Esto
permite separar errores del controlador 6502 de errores posteriores en la
síntesis de ondas.

## Comparar estado de juego

```bash
python tools/trace_compare.py tetris-reference.csv port-trace.csv \
  --columns input,x,y,level,fall_counter,das_counter,rng_seed
```

Los valores pueden escribirse en decimal, `0x`, `$`, `0b` o `%`. Las columnas
comunes se eligen automáticamente cuando no se especifica `--columns`.

## Resultados y códigos de salida

La herramienta informa cantidad de fotogramas, columnas, fotogramas ausentes,
primer campo divergente y hasta 20 diferencias.

| Código | Significado |
|---:|---|
| 0 | Coincidencia en las columnas comparadas |
| 1 | Existe una divergencia |
| 2 | Archivo o esquema inválido |

## Autopruebas

```bash
python tools/trace_compare.py --self-test
tetris_apu_render --self-test
```

## Alcance actual

La comparación se alinea por fotograma, no por ciclo de CPU/PPU/APU. El script
de Mesen ya automatiza la referencia, pero todavía deben verificarse los mapas
de RAM, la fase exacta de inicio y el orden de callbacks para cada escena.
