# Investigación PRG v0.25

Esta fase mantiene congelado el runtime v0.23. No modifica `src/`, audio, render, controles, PC ni Android.

## Fuente y objetivo fijados

- Fuente pública: `CelestialAmber/TetrisNESDisasm`
- Commit: `94581c0306fa0009fe11fde48222875549d15b8b`
- PRG objetivo SHA-256: `a2de35dfa7333b0458762a1485fb612b6bdc9b5a15269f0fcf5a6be67c0d89de`
- CHR objetivo SHA-256: `23789834e66947081aa5d34efe092beb69a91273abc62089a8bb7c00f67ff751`

La reconstrucción produjo PRG y CHR idénticos. La imagen NES completa difiere únicamente en su cabecera iNES de 16 bytes.

## Puertas completadas

### G1. Reconstrucción binaria: 100%

El PRG recompilado de 32 KiB coincide byte por byte mediante SHA-256.

### G2. Partición completa: 100%

ld65 relaciona los 32.768 bytes del PRG con spans de fuente. El informe registra 30 archivos fuente, 8.304 registros de línea y 1.464 registros de símbolos.

### G3. Símbolos semánticos: 100%

Además de las 1.071 etiquetas ya semánticas, se clasificaron las 114 etiquetas genéricas detectadas por la regla ampliada. Todas tienen alias único, tipo, subsistema y evidencia. No quedan etiquetas omitidas, duplicadas o inválidas en el inventario fijado.

### G4. Grafo estático de control: 100%

- 1.028/1.028 ramas, JSR y JMP directos resueltos.
- 368/368 referencias de tablas `.addr` resueltas.
- 2/2 saltos calculados clasificados: dispatcher por tabla de retorno y dispatcher de métodos de efectos de sonido.

### G5. Contratos de estado: 100%

Se generaron 302 contratos para las 302 rutinas o bloques globales con instrucciones. Clasifican 4.963 instrucciones, lecturas y escrituras RAM, accesos PPU/APU, memoria indirecta, llamadas, saltos terminales, pila y retornos.

### G7. Toolchain reproducible: 100%

La revisión de fuente, hashes, workflow, formatos de informe y comprobaciones están fijados.

## Puerta pendiente

### G6. Familias de trazas dinámicas: 0% — 0/18

Falta capturar y verificar localmente con la ROM legal las 18 familias de ejecución. Cada familia deberá incluir receta reproducible, columnas de estado, cantidad de frames, hash del archivo y resultado de comparación. Las trazas propietarias no se publicarán; GitHub recibirá solamente hashes y metadatos.

## Estado actual

| Puerta | Estado |
|---|---:|
| G1. Reconstrucción binaria | 100% |
| G2. Partición completa | 100% |
| G3. Símbolos semánticos | 100% |
| G4. Grafo de control | 100% |
| G5. Contratos de estado | 100% |
| G6. Trazas dinámicas | 0% — 0/18 |
| G7. Toolchain reproducible | 100% |
| **Preparación total** | **85,7143%** |

La correspondencia funcional del ejecutable continúa congelada en 50%. El runtime no se descongelará hasta completar G6 y alcanzar 100% en las siete puertas.

## Informes

El artefacto de investigación contiene únicamente JSON y hashes: equivalencia PRG/CHR, mapa ld65, grafo de control, contexto y auditoría semántica, contratos de estado y revisión de fuente. No contiene ROM, PRG, CHR, audio ni trazas propietarias.
