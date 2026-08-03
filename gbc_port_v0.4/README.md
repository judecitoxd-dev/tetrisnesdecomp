# Tetris NES → Game Boy Color v0.4 Compact UI (sin sonido)

Port específico de **Tetris (USA) para NES** a Game Boy Color, basado en el análisis, offsets y reglas del repositorio `tetrisnesdecomp`. No es un emulador ni un convertidor universal. El usuario proporciona su ROM legal exacta y el programa genera un `.gbc` jugable con los recursos procesados localmente.

## Uso en Windows

1. Abre `nes2gbc.exe`.
2. Selecciona `Tetris (USA).nes`.
3. Guarda `Tetris (USA)_GBC_v0.4.gbc`.
4. Abre el resultado en un emulador GBC o flashcart compatible.

Por consola:

```powershell
.\nes2gbc-cli.exe --rom ".\Tetris (USA).nes" --out ".\Tetris (USA)_GBC_v0.4.gbc"
```

La ROM verificada tiene 49168 bytes, CRC32 `D16EA396` y SHA-256 `ddb876c302cfd4ee19fabff8e3ede0b6801ded70c49980d1682a32d352953082`.

## Cambios principales de v0.4

- Interfaz rediseñada específicamente para el LCD GBC de 160×144.
- El campo lógico completo de 10×20 ahora se muestra: las veinte filas son visibles, sin ocultar las dos superiores.
- Cada tile de fondo empaqueta cuatro bloques de 4×4 píxeles. El conversor crea las 256 combinaciones posibles y las carga en el banco VRAM 1.
- Piezas activas y `NEXT` usan mini-bloques derivados de los tiles NES `0x7B`, `0x7C` y `0x7D`.
- Panel compacto completo: `A/B-TYPE`, `LINES`, `STAT`, `TOP`, `SCORE`, `NEXT` y `LV` visibles simultáneamente.
- `STATISTICS` funcional con contadores de tres cifras para las siete piezas.
- Pantalla de título recompuesta sin desplazamiento horizontal.
- Menús A-Type/B-Type y nivel/altura recompuestos para caber completos.
- Conserva la corrección de VRAM de v0.3: los bloques fijados no desaparecen.
- A-Type y B-Type siguen jugables; continúa sin música ni efectos.

## Controles

- Título: Start o A.
- Tipo: izquierda/derecha; Start o A confirma; Select regresa.
- Nivel A: izquierda/derecha cambia número; arriba/abajo cambia fila.
- Nivel B: izquierda/derecha cambia nivel; arriba/abajo cambia altura.
- Juego: izquierda/derecha mueve, abajo acelera, A/B rota y Start pausa.
- Tras perder o completar B-Type: Start reinicia.

## Diferencias pendientes

La entrega sigue siendo incremental. Faltan el RNG NES exacto de dos bytes y su `one-reroll`, los estados exactos de lock/row-check/ARE, animación de borrado, récords y entrada de nombre, demo automática, finales, concierto y sonido. `TOP` todavía se muestra como cero porque la tabla persistente de récords aún no se ha portado.

## Verificación

- Encabezado y checksums GBC.
- Conversión CHR y generación de los 256 tiles compactos.
- Flujo título → tipo → nivel/altura → juego.
- Tablero completo 10×20 y escritura de VRAM bajo restricciones reales.
- Lock, respawn, líneas, puntuación, nivel, paletas y B-Type.
- Contadores `STATISTICS` incrementados al aparecer cada pieza.

No se ha probado todavía en una Game Boy Color física.

## Compilar

```bash
python3 template/build_template.py
python3 tools/sync_template.py
cd builder
go test ./...
GOOS=windows GOARCH=amd64 CGO_ENABLED=0 go build -ldflags="-s -w" -o ../dist/nes2gbc-cli.exe .
GOOS=windows GOARCH=amd64 CGO_ENABLED=0 go build -ldflags="-s -w -H=windowsgui" -o ../dist/nes2gbc.exe .
```

## Legalidad

El código fuente y la plantilla no incluyen la ROM ni assets de Nintendo. El `.gbc` personalizado se genera localmente desde la copia legal del usuario.
