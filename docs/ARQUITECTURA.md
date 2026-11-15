# TAPERMATE v1.1 - Arquitectura
```
Source/
  TapeMachine.*     motor DSP: grabadora (60 s), camino en vivo, reproduccion de cinta, efectos, medidores
  PluginProcessor.* parametros (APVTS), transporte atomico, SYNC con el host
  PluginEditor.*    interfaz dibujada en vectores (carretes, rodillos, cabezal, panel, transporte, VU), menus, presets, extras
  IntroOverlay.*    pantalla de inicio (cintas que se rebobinan) + logo TL
  Profile.*         nombre, idioma, tamano de ventana (global: %APPDATA%\TapeLab\TAPERMATE\profile.xml) y textos EN/ES
```
## Senal
`in -> INPUT -> saturacion (SAT MIX) -> curva de velocidad (head bump + perdida de agudos)` = senal de grabacion.
- STOP / REC: camino en vivo = cambio de velocidad (VARI / PITCH / L.PITCH, dos cabezales) + vibrato (wow / flutter).
- PLAY / REV / F.FWD / RWD: se lee la cinta con interpolacion Hermite a la velocidad del motor x VARI x PITCH x wow.
- Cambios de estado: el motor acelera / frena suavemente (el frenado produce el efecto "tape stop") y las dos fuentes se mezclan.
Despues: TILT (azimut) -> HISS / NOISE (zumbido, chasquidos, caidas) -> eco de cinta (FB saturado y oscuro) -> OUTPUT -> cortes de EQ -> reverb -> POWER.
## Tiempo real
Toda la memoria (cinta, retardos, reverb) se reserva en `prepareToPlay` (~30 MB). `process()` no reserva memoria; trabaja en trozos de 256 muestras.
## Ventana escalable
`TapeContent` (1530x722) se escala con `setTransform`; el editor hace `setSize(base * escala)` (50% - 400%) y guarda la escala en el perfil.
