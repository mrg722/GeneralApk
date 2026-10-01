#!/usr/bin/env bash
# Compila District Fury para Android (arm64-v8a) y genera un APK firmado.
#
# Requisitos: Android SDK con platforms;android-34, build-tools;34.0.0 y
# ndk;26.3.11579264 (ANDROID_HOME apunta al SDK), CMake >= 3.21, Java 17+.
# Uso (desde la raiz del repo):
#   ANDROID_HOME=/ruta/al/sdk tools/android/build_apk.sh
# Salida: build-android/district_fury.apk
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SDK="${ANDROID_HOME:-$HOME/Android/Sdk}"
NDK="${ANDROID_NDK:-$SDK/ndk/26.3.11579264}"
BT="$SDK/build-tools/34.0.0"
PLATFORM_JAR="$SDK/platforms/android-34/android.jar"
OUT="$ROOT/build-android"
for f in "$NDK/build/cmake/android.toolchain.cmake" "$BT/aapt" "$BT/zipalign" "$BT/apksigner" "$PLATFORM_JAR"; do
  [ -e "$f" ] || { echo "Falta $f (revisa ANDROID_HOME / ANDROID_NDK)"; exit 1; }
done

# 1) Libreria nativa (raylib + juego) -> libmain.so
EXTRA=()
[ -n "${RAYLIB_SRC:-}" ] && EXTRA+=("-DFETCHCONTENT_SOURCE_DIR_RAYLIB=$RAYLIB_SRC")
cmake -S "$ROOT" -B "$OUT/cmake" \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DANDROID_STL=c++_static \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF "${EXTRA[@]}"
cmake --build "$OUT/cmake" -j"$(nproc)" --target district_fury

# 2) Contenido del APK: assets del juego con las mismas rutas relativas que en PC.
STAGE="$OUT/stage"
rm -rf "$STAGE"; mkdir -p "$STAGE/assets" "$STAGE/lib/arm64-v8a"
cp -r "$ROOT/assets" "$ROOT/data" "$STAGE/assets/"
find "$STAGE/assets" -type d -name source -prune -exec rm -rf {} +   # hojas originales: solo para las herramientas
# despiece del clon: el juego solo usa rig_rojo.* e img_1.png; las hojas de Canva y pruebas no van al APK
rm -rf "$STAGE/assets/assets/characters/rayder/kf_clone/canva" "$STAGE/assets/assets/characters/rayder/kf_clone/"img_1_rig_* \
       "$STAGE/assets/assets/characters/rayder/kf_clone/"despiece_*
# Laboratorio (temporal): datos de la APK para los personajes de prueba del Modo VS.
if [ -f "$ROOT/apk_reference/king_fighter_iii/bin/animation.bin" ]; then
  mkdir -p "$STAGE/assets/apk_reference/king_fighter_iii/bin"
  cp "$ROOT/apk_reference/king_fighter_iii/bin/animation.bin" "$STAGE/assets/apk_reference/king_fighter_iii/bin/"
fi
cp "$OUT/cmake/libmain.so" "$STAGE/lib/arm64-v8a/"
"$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip" --strip-unneeded "$STAGE/lib/arm64-v8a/libmain.so"

# 3) Empaquetar, alinear y firmar (llave de depuracion local).
"$BT/aapt" package -f -M "$ROOT/android/AndroidManifest.xml" -S "$ROOT/android/res" \
  -A "$STAGE/assets" -I "$PLATFORM_JAR" -F "$OUT/unsigned.apk"
(cd "$STAGE" && "$BT/aapt" add "$OUT/unsigned.apk" lib/arm64-v8a/libmain.so > /dev/null)
"$BT/zipalign" -f -p 4 "$OUT/unsigned.apk" "$OUT/aligned.apk"
KS="$OUT/debug.keystore"
[ -f "$KS" ] || keytool -genkeypair -keystore "$KS" -storepass android -keypass android \
  -alias districtfury -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=District Fury Debug" > /dev/null
"$BT/apksigner" sign --ks "$KS" --ks-pass pass:android --key-pass pass:android \
  --out "$OUT/district_fury.apk" "$OUT/aligned.apk"
"$BT/apksigner" verify "$OUT/district_fury.apk"
echo "APK listo: $OUT/district_fury.apk ($(du -h "$OUT/district_fury.apk" | cut -f1))"
