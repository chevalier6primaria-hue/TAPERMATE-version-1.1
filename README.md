# TAPERMATE v1.1

Maquina de cinta de carretes abiertos para FL Studio (VST3, Windows 64 bits): efecto en vivo + grabadora de cinta de 60 s.
Mismo codigo de plugin que la v1.0 (FL Studio lo toma como actualizacion).

## Novedades
- Diseno inspirado en la maquina de la captura: carretes que giran, rodillos, cinta, bloque de cabezal, panel de aluminio, transporte y VU.
- Controles: POWER, SPEED, VARI, PITCH, L.PITCH, LF PITCH, HISS, NOISE, SAT MIX, DELAY (+FB, MIX), RESET, REV/RWD/F.FWD/STOP/PLAY/REC,
  LOOP, TILT, SYNC, INPUT, OUTPUT, IN/OUT y VU. Ver `docs/FUNCIONES_TIRMAN.md` (incluye que es interpretacion mia).
- **Clic en la placa con el nombre "TAPERMATE"** (o en "...") -> tamano de la ventana de 50% a 400%; se recuerda.
- **Pantalla de inicio** (solo la primera vez): cintas dispersas sobre fondo negro que se mueven despacio como si se rebobinaran,
  logo TL y nombre del creador (TapeLab); pide tu nombre e idioma (English / Espanol). Para verla otra vez: "..." -> Mostrar intro.
- Presets de fabrica (lista superior derecha) y panel Extras (reverb + cortes de EQ de la v1.0).

## Compilar con GitHub (sin Visual Studio)
1. Repositorio nuevo -> sube el CONTENIDO de esta carpeta (`CMakeLists.txt`, `Source`, `docs` y la carpeta oculta `.github`).
2. Pestana **Actions** -> "Build TAPERMATE v1.1" corre solo (10-15 min).
3. Descarga **TAPERMATE-v1.1-VST3-Windows** (Artifacts), descomprime y copia `TAPERMATE.vst3` a `C:\Program Files\Common Files\VST3`
   (reemplaza la v1.0). En FL Studio: Options > Manage plugins > Find installed plugins.
