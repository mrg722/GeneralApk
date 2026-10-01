# Probar District Fury en el celular (Samsung A57) con adb

El APK ya compilado está en `dist/district_fury-0.23-lab.apk` (arm64-v8a, Android 7.0 o superior).

## 1. Preparar el celular (una sola vez)

1. **Ajustes → Acerca del teléfono → Información de software**: toca **Número de compilación** 7 veces hasta que diga "Modo desarrollador activado".
2. **Ajustes → Opciones de desarrollador**: activa **Depuración USB**.
3. Conecta el celular por USB a la PC y, en el aviso que aparece en el celular, acepta **Permitir depuración USB** (marca "Permitir siempre").

## 2. Instalar adb en la PC

- **Windows**: descarga "SDK Platform-Tools" desde https://developer.android.com/tools/releases/platform-tools, descomprime y abre una terminal en esa carpeta.
- **Linux**: `sudo apt install adb`
- **macOS**: `brew install android-platform-tools`

Comprueba que ve el celular:

```bash
adb devices
```

Debe aparecer una línea con el número de serie y la palabra `device`. Si dice `unauthorized`, acepta el aviso en el celular.

## 3. Instalar y abrir el juego

Desde la raíz del repositorio (`GeneralApk`, rama `ccr-077ad675-rlrg28`):

```bash
adb install -r dist/district_fury-0.23-lab.apk
adb shell am start -n com.mrg722.districtfury/android.app.NativeActivity
```

`-r` reinstala sobre una versión anterior y conserva el progreso guardado. También puedes abrirlo desde el ícono **District Fury** del celular.

## 4. Ver el registro si algo falla

```bash
adb logcat -c                       # limpia el registro
adb logcat -s raylib:V AndroidRuntime:E DEBUG:V
```

Deja ese comando corriendo mientras juegas. Si el juego se cierra, copia las últimas líneas y envíamelas.

## 5. Desinstalar

```bash
adb uninstall com.mrg722.districtfury
```

## 6. Recompilar el APK después de cambios

Hace falta el Android SDK con `platforms;android-34`, `build-tools;34.0.0` y `ndk;26.3.11579264`, además de CMake 3.21 o superior y Java 17 o superior.

```bash
ANDROID_HOME=/ruta/al/Android/Sdk tools/android/build_apk.sh
adb install -r build-android/district_fury.apk
```

El script descarga raylib automáticamente la primera vez. Para usar una copia local, agrega `RAYLIB_SRC=/ruta/a/raylib`.

## Controles en el celular

| Pantalla | Controles |
|---|---|
| Combate | Joystick (izquierda) · **GOLPE** (J) · **PATADA** (K) · **habilidades 1 a 6** en arco alrededor de GOLPE (teclado 1-6), cada una con 15 s de espera · **ESPECIAL** (gasta energía) · **BLOQ** (mantener) · **DASH** · **FURIA** (cuando brilla) · **PAUSA** |
| Menús | Cruceta ▲▼◄► a la derecha · **OK** · **ATRÁS** (también el botón "atrás" del celular) |
| Derrota / etapa superada | **OK** · **REINTENTAR** · **MENÚ** |
| Recompensa entre stages | **1** · **2** · **3** |

Habilidades de Rayden y Rayder: 1 ONDA, 2 GANCHO, 3 TORBELLINO, 4 EMBESTIDA, 5 REMATE, 6 TRANSFORMAR (12 s con más daño y velocidad). Los personajes KF usan sus propias animaciones de poder. Mientras una habilidad espera, su botón se oscurece y muestra los segundos que faltan.

**Sonido:** el juego trae los sonidos de tu paquete y lluvia de ambiente. Se apagan en OPCIONES → SONIDO.

**Pantalla muy alargada:** en **OPCIONES → ANCHO DE PANTALLA** (70 % a 100 %) se reduce solo el ancho del juego; la altura no cambia.

## Qué probar primero

1. Nueva partida → elegir Rayden o Rayder → Nivel 1 completo (oleadas, "GO >>", guardianes y Brakk con su hoja nueva).
2. Las 6 habilidades y su espera; la transformación de Rayder (pelo blanco).
3. Modo VS // Laboratorio → **PERSONAJE**: cualquiera de los personajes **KF ...**. **RIVAL KF (LAB)**: un rival extraído. En combate no hay tecla N en el celular: las acciones del rival se ven peleando.
4. Que el juego se vea completo en horizontal: el área de juego 16:9 lleva bandas negras a los costados en la pantalla del A57.
