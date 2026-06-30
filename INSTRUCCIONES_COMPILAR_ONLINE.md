# Ark Shimmer Reverb - Compilación online con GitHub Actions

Esta versión incluye un workflow de GitHub Actions para compilar el plugin en la nube y descargar el VST3 ya generado.

## Pasos rápidos

1. Entra a GitHub y crea un repositorio nuevo llamado `ArkShimmerReverb`.
2. Descomprime este ZIP en tu computadora.
3. Sube a GitHub todo el contenido de la carpeta `ArkShimmerReverb_CloudBuild`.
   - Debes subir `CMakeLists.txt`, `Source`, `Presets`, `README.md` y la carpeta oculta `.github`.
4. En GitHub, entra a la pestaña **Actions**.
5. Selecciona **Build Windows VST3**.
6. Haz clic en **Run workflow**.
7. Cuando termine, entra al resultado de la ejecución.
8. Abajo, en **Artifacts**, descarga `ArkShimmerReverb-Windows-VST3`.
9. Descomprime el artifact. Dentro estará `Ark Shimmer Reverb.vst3`.
10. Copia el VST3 a:

```text
C:\Program Files\Common Files\VST3
```

11. Abre tu DAW y haz rescan de plugins VST3.

## Importante

- No necesitas instalar Visual Studio en tu PC.
- No necesitas instalar JUCE en tu PC.
- GitHub Actions descargará JUCE automáticamente durante la configuración CMake.
- Si el build falla, copia el error completo o mándame una captura del log.
