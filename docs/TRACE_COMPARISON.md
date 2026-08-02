# Comparación de trazas fotograma por fotograma

La v0.10 añade una ruta reproducible para comparar el estado del port C con una
captura obtenida en un emulador. La herramienta no incluye ROM, RAM, CHR, audio
ni imágenes del juego: solo lee CSV creados localmente por el usuario.

## Generar la traza del port

```bash
tetris_demo_verify "Tetris (USA).nes" port-trace.csv
```

El formato de traza v2 contiene una fila por fotograma y añade:

- máscara de entrada interpretada;
- hash del tablero;
- pieza activa y siguiente;
- posición y rotación;
- puntuación, líneas y nivel;
- semilla RNG;
- contadores de caída, DAS, fase y borrado;
- offsets consumidos de las tablas de demo.

Los hashes son firmas de regresión del port. No sustituyen una comparación de
RAM contra el programa 6502.

## Preparar una traza de emulador

Exporta un CSV con una columna `frame` y las columnas que puedas mapear al
estado del port. Los nombres comunes se comparan automáticamente. También se
puede elegir un subconjunto explícito.

Ejemplo mínimo:

```csv
frame,active,next,x,y,rotation,score,lines,level,rng_seed
0,6,0,3,-1,0,0,0,0,$8988
1,6,0,3,-1,0,0,0,0,$44C4
```

Los valores pueden escribirse en decimal, `0x` hexadecimal, `$` hexadecimal,
`0b` binario o `%` binario. Las columnas cuyo nombre contiene `hash` también
aceptan hexadecimal sin prefijo.

## Comparar

```bash
python tools/trace_compare.py emulator.csv port-trace.csv
```

Elegir columnas:

```bash
python tools/trace_compare.py emulator.csv port-trace.csv \
  --columns active,next,x,y,rotation,score,lines,level,rng_seed
```

Crear un informe JSON:

```bash
python tools/trace_compare.py emulator.csv port-trace.csv \
  --json trace-report.json
```

La salida indica:

- cantidad de fotogramas de cada archivo;
- columnas comparadas;
- fotogramas ausentes;
- primer fotograma divergente;
- primer campo distinto;
- hasta 20 diferencias por defecto.

Códigos de salida:

| Código | Significado |
|---:|---|
| 0 | Las trazas coinciden en las columnas comparadas |
| 1 | Existe al menos una divergencia |
| 2 | Archivo o esquema inválido |

## Autoprueba

```bash
python tools/trace_compare.py --self-test
```

GitHub Actions ejecuta esta autoprueba en Windows, Linux y Android antes de
compilar.

## Alcance actual

La comparación es por fotograma lógico del port, no por ciclo de CPU o PPU.
Para llegar a paridad total todavía se necesita una captura de referencia con
un mapeo estable de RAM, mapper, NMI, OAM y escrituras PPU/APU.
