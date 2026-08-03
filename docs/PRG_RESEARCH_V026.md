# Investigación PRG v0.26 — G6 completado

Esta fase cierra la última puerta de investigación sin modificar el runtime
jugable v0.23.

## Resultado

- G1 reconstrucción binaria: 100%.
- G2 partición completa: 100%.
- G3 símbolos semánticos: 100%.
- G4 grafo de control: 100%.
- G5 contratos RAM/PPU/APU: 100%.
- G6 trazas dinámicas: **18/18, 100%**.
- G7 toolchain reproducible: 100%.
- Preparación total: **100%**.

La correspondencia funcional del ejecutable sigue en 50%. Esta fase crea la
referencia necesaria para corregirlo después; no afirma que el port ya sea
idéntico.

## Familias dinámicas

Las 18 recetas ejecutan directamente rutinas del PRG original con estados RAM y
registros deterministas:

1. rotación válida;
2. gravedad y caída;
3. desplazamiento DAS;
4. colisión bloqueada;
5. aparición de pieza;
6. selección de pieza de demo;
7. selección pseudoaleatoria;
8. estadísticas por pieza;
9. bloqueo en el campo;
10. cortina de game over;
11. fila completada;
12. recepción de basura;
13. puntuación y transición de nivel;
14. altura de bloqueo;
15. cambio allegro;
16. avance del stream de demo;
17. selección de pista musical;
18. LFSR del RNG.

Cada traza incluye registros finales, instrucciones, ciclos, cambios de RAM y
escrituras a hardware. El archivo completo se conserva localmente; el
repositorio almacena únicamente los hashes SHA-256 y las recetas.

## Reproducción local

```powershell
python tools\prg_dynamic_traces.py "C:\ROMs\Tetris (USA).nes" `
  --output g6-traces-local.json `
  --summary g6-summary-local.json `
  --expect research\g6_trace_reference_v026.json
```

La salida final esperada es:

```text
G6_REFERENCE_MATCH=YES
```

## Evidencia local

Las dos copias de la ROM proporcionadas por el usuario tienen el mismo SHA-256:

`ddb876c302cfd4ee19fabff8e3ede0b6801ded70c49980d1682a32d352953082`

Ambas generaron exactamente el mismo archivo de resumen. El hash agregado de
las 18 familias es:

`d44aa967522aca04f672bd2ba455a1274d8f9118f66601f08369e070ea9c4840`

## Evidencia en CI

Actions recompila la fuente 6502 fijada, confirma el PRG/CHR exacto, normaliza
solo los 16 bytes de metadatos iNES y vuelve a ejecutar las 18 recetas. El job
falla si cambia cualquier hash individual o el hash agregado.

Antes de publicar el artefacto de investigación se eliminan la ROM reconstruida
y la traza completa. Solo se publica el resumen de hashes y los demás informes
sin contenido propietario.

## Runtime congelado

Esta fase no cambia `src/`, jugabilidad, audio, render, controles, PC ni Android.
El runtime continúa congelado en v0.23 dentro de este merge. La siguiente fase
podrá aplicar los hallazgos de manera incremental, comparando cada cambio con
estas 18 referencias.
