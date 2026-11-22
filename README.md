# RoboTune – autotune basico para FL Studio (VST3)

Controles: **Key** (12 notas), **Scale** (14 escalas) y **Retune Speed** (0–400 ms).
Para el efecto trap/T-Pain: **Retune Speed = 0 ms**.

Escalas: Chromatic, Major, Minor, Harmonic Minor, Melodic Minor, Pentatonic Major,
Pentatonic Minor, Blues, Dorian, Phrygian, Lydian, Mixolydian, Locrian, Phrygian Dominant.

## Compilar en Windows (una sola vez)
1. Instala **Visual Studio 2022 Community** (workload "Desarrollo para el escritorio con C++"),
   **CMake** y **Git**.
2. Abre "x64 Native Tools Command Prompt for VS 2022", entra a esta carpeta y ejecuta:

       cmake -B build -G "Visual Studio 17 2022"
       cmake --build build --config Release

   (la primera vez descarga JUCE automaticamente; tarda unos minutos)
3. El `.vst3` se copia a `C:\Program Files\Common Files\VST3\RoboTune.vst3`
   (si falla la copia, ejecuta la consola como Administrador, o toma el archivo de
   `build\RoboTune_artefacts\Release\VST3\`).
4. En FL Studio: Options > Manage plugins > **Find installed plugins** (con "Verify plugins").
   Lo encuentras en Add > Plugin database > Installed > Effects.
5. Ponlo en un canal del mixer con tu voz. El plugin tiene ~35 ms de latencia
   y se la informa a FL, que la compensa solo.

## Consejos de uso
- Pon la tonalidad de tu beat en Key/Scale. Si no la sabes, usa Chromatic.
- Funciona mejor con voz sola (monofonica) y sin reverb/delay *antes* del plugin.
- Si hay saltos raros, graba la voz mas cerca del micro / con menos ruido de fondo.
