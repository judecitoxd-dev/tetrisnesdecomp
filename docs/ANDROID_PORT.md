# Port Android v0.6

La versión Android ejecuta el mismo núcleo C99 de reglas, ROM, render, récords, ajustes y repeticiones que las versiones de Windows y Linux. El frontend usa SDL2 2.30.11 y una actividad Java mínima para seleccionar la ROM mediante Storage Access Framework.

## Requisitos

- Android 6.0 o posterior (`minSdk 23`).
- Procesador ARM64 o ARMv7.
- Una copia legal de `Tetris (USA).nes`.

El APK no incluye ROM, bancos, gráficos extraídos, audio del cartucho ni archivos de repetición.

## Primer inicio

1. Instala el APK.
2. Abre **Tetris NES Port**.
3. Selecciona tu ROM legal con el selector del sistema.
4. La aplicación valida cabecera iNES, tamaño, mapper MMC1 y disposición PRG/CHR.
5. La ROM se copia al almacenamiento privado de la aplicación y no se solicita de nuevo mientras permanezca instalada.

La revisión probada tiene 49,168 bytes y CRC32 `D16EA396`. Una ROM estructuralmente compatible puede abrir, pero se mostrará como no verificada.

## Controles táctiles

La capa incluye:

- cruceta;
- A y B;
- caída instantánea;
- Start y Select;
- botón **ROM** para elegir otra copia;
- botón **EDIT** para personalizar la disposición.

En modo EDIT:

- arrastra cualquier botón para moverlo;
- toca **START** para cambiar el tamaño;
- toca **SEL** para cambiar la opacidad;
- toca **EDIT** otra vez para terminar.

Posición, tamaño y opacidad se guardan en `settings.ini`. Cuando SDL detecta un gamepad físico, la capa táctil se oculta automáticamente.

## Compilar

La compilación automática descarga el código oficial de SDL2 y no lo guarda en el repositorio.

```bash
cd android
gradle --no-daemon :app:assembleDebug
```

Antes de ejecutar Gradle deben estar disponibles Android SDK 34, Build Tools 34.0.0, NDK 26.3.11579264 y CMake 3.22.1. El workflow `.github/workflows/android.yml` prepara estas dependencias y genera un APK para `arm64-v8a` y `armeabi-v7a`.

## Firma

El artefacto actual es un APK de depuración firmado automáticamente por Android Gradle Plugin. Es instalable para pruebas, pero no es una versión firmada para Play Store. Una publicación estable necesitará un keystore privado y un bloque `signingConfig` de release fuera del repositorio.

## Límites actuales

- Android usa el sintetizador nativo; los paquetes OGG de escritorio todavía no están conectados al selector móvil.
- No se ha probado todavía en todos los fabricantes, relaciones de aspecto o mandos Bluetooth.
- Los menús y finales que aún son equivalentes nativos en PC también lo son en Android.
- Esta versión no convierte el proyecto en una decompilación bit a bit; comparte la misma base para continuar esa reconstrucción sin duplicar lógica.
