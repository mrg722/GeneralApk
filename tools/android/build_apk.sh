#!/usr/bin/env bash
# Compila un APK firmado (arm64-v8a). En este repo hay DOS aplicaciones:
#
#   tools/android/build_apk.sh            -> District Fury  (build-android/district_fury.apk)
#   tools/android/build_apk.sh gow        -> GowApk         (build-android/gow_apk.apk)
#
# Son APK independientes, con paquetes distintos: se pueden instalar las dos a
# la vez en el mismo telefono. GowApk lleva el contenido del paquete original
# (personajes, escenarios, jefes e interfaz propios); District Fury no lo lleva,
# salvo el guerrero del Modo VS, cuyas piezas viven en assets/kratos y las usan
# las dos aplicaciones.
#
# Requisitos: Android SDK con platforms;android-34, build-tools;34.0.0 y
# ndk;26.3.11579264 (ANDROID_HOME apunta al SDK), CMake >= 3.21, Java 17+.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
APP="${1:-district}"
SDK="${ANDROID_HOME:-$HOME/Android/Sdk}"
NDK="${ANDROID_NDK:-$SDK/ndk/26.3.11579264}"
BT="$SDK/build-tools/34.0.0"
PLATFORM_JAR="$SDK/platforms/android-34/android.jar"

case "$APP" in
  district) TARGET=district_fury; MANIFEST="$ROOT/android/AndroidManifest.xml"; NOMBRE=district_fury; CMAKE_APP=district ;;
  gow)      TARGET=gow_apk;       MANIFEST="$ROOT/GowApk/android/AndroidManifest.xml"; NOMBRE=gow_apk; CMAKE_APP=gow ;;
  *) echo "Uso: $0 [district|gow]"; exit 1 ;;
esac
OUT="$ROOT/build-android/$APP"

for f in "$NDK/build/cmake/android.toolchain.cmake" "$BT/aapt" "$BT/zipalign" "$BT/apksigner" "$PLATFORM_JAR" "$MANIFEST"; do
  [ -e "$f" ] || { echo "Falta $f (revisa ANDROID_HOME / ANDROID_NDK)"; exit 1; }
done

# 1) Libreria nativa (raylib + juego) -> libmain.so
EXTRA=()
[ -n "${RAYLIB_SRC:-}" ] && EXTRA+=("-DFETCHCONTENT_SOURCE_DIR_RAYLIB=$RAYLIB_SRC")
cmake -S "$ROOT" -B "$OUT/cmake" \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DANDROID_STL=c++_static \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DDF_APP="$CMAKE_APP" "${EXTRA[@]}"
cmake --build "$OUT/cmake" -j"$(nproc)" --target "$TARGET"

# 2) Contenido del APK: los assets van con la MISMA ruta relativa que en PC.
STAGE="$OUT/stage"
rm -rf "$STAGE"; mkdir -p "$STAGE/assets" "$STAGE/lib/arm64-v8a"

if [ "$APP" = district ]; then
  cp -r "$ROOT/assets" "$ROOT/data" "$STAGE/assets/"
  # hojas originales y pruebas: solo sirven para las herramientas, no van al APK
  find "$STAGE/assets" -type d -name source -prune -exec rm -rf {} +
  rm -rf "$STAGE/assets/assets/backgrounds/beta" \
         "$STAGE/assets/assets/characters/rayder/kf_clone/canva" \
         "$STAGE/assets/assets/characters/rayder/kf_clone/canva_sin_mangas" \
         "$STAGE/assets/assets/characters/rayder/kf_clone/"img_1_rig_* \
         "$STAGE/assets/assets/characters/rayder/kf_clone/"despiece_*
  # Laboratorio (temporal): datos para los personajes de prueba del Modo VS.
  if [ -f "$ROOT/apk_reference/king_fighter_iii/bin/animation.bin" ]; then
    mkdir -p "$STAGE/assets/apk_reference/king_fighter_iii/bin"
    cp "$ROOT/apk_reference/king_fighter_iii/bin/animation.bin" "$STAGE/assets/apk_reference/king_fighter_iii/bin/"
  fi
else
  # GowApk: sus propios datos + las piezas compartidas del guerrero.
  mkdir -p "$STAGE/assets/GowApk" "$STAGE/assets/assets" "$STAGE/assets/data"
  cp -r "$ROOT/GowApk/assets" "$ROOT/GowApk/data" "$STAGE/assets/GowApk/"
  cp -r "$ROOT/assets/kratos" "$STAGE/assets/assets/"
  cp -r "$ROOT/assets/audio"  "$STAGE/assets/assets/"
  cp -r "$ROOT/data/kratos"   "$STAGE/assets/data/"
  find "$STAGE/assets" -type d -name source -prune -exec rm -rf {} +
fi

cp "$OUT/cmake/libmain.so" "$STAGE/lib/arm64-v8a/"
"$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip" --strip-unneeded "$STAGE/lib/arm64-v8a/libmain.so"

# 3) Empaquetar, alinear y firmar (llave de depuracion local).
"$BT/aapt" package -f -M "$MANIFEST" -S "$ROOT/android/res" \
  -A "$STAGE/assets" -I "$PLATFORM_JAR" -F "$OUT/unsigned.apk"
(cd "$STAGE" && "$BT/aapt" add "$OUT/unsigned.apk" lib/arm64-v8a/libmain.so > /dev/null)
"$BT/zipalign" -f -p 4 "$OUT/unsigned.apk" "$OUT/aligned.apk"
KS="$ROOT/build-android/debug.keystore"
mkdir -p "$(dirname "$KS")"
[ -f "$KS" ] || keytool -genkeypair -keystore "$KS" -storepass android -keypass android \
  -alias districtfury -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=District Fury Debug" > /dev/null
"$BT/apksigner" sign --ks "$KS" --ks-pass pass:android --key-pass pass:android \
  --out "$OUT/$NOMBRE.apk" "$OUT/aligned.apk"
"$BT/apksigner" verify "$OUT/$NOMBRE.apk"
echo "APK listo: $OUT/$NOMBRE.apk ($(du -h "$OUT/$NOMBRE.apk" | cut -f1))"
