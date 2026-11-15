# TAPERMATE v1.1 - controles tomados de la captura de TIRMAN 48

IMPORTANTE: no pude ver el video (YouTube bloquea la lectura automatica) ni encontre documentacion escrita. Lo que si tengo es la
captura de pantalla: amplie el panel y lei los nombres de los controles. **Lo que hace cada uno es mi interpretacion** de como
funciona una grabadora de cinta; si el video explica algo distinto, mandame sus notas (texto) y lo ajusto.

| Control (en la captura) | Que hace en TAPERMATE v1.1 |
|---|---|
| POWER (boton rojo + LED) | Enciende / apaga la maquina. Apagada = senal original sin tocar (derivacion limpia) |
| SPEED (interruptor) | 7.5 ips / 15 ips: 15 es mas brillante y limpio; 7.5 mas oscuro, mas hiss, mas wow/flutter y "head bump" mas grave |
| VARI | Vari-speed continuo (-50% .. +50%): cambia velocidad y tono, como en una grabadora real |
| PITCH | Tono en semitonos (-12 .. +12) |
| L.PITCH | Ajuste fino de tono (-100 .. +100 centesimas) |
| LF PITCH | Inestabilidad de baja frecuencia: wow + deriva lenta (y algo de flutter) |
| HISS | Siseo de cinta (mas fuerte a 7.5 ips) |
| NOISE | Imperfecciones: zumbido de 60 Hz, chasquidos y caidas de nivel (dropouts) |
| SAT MIX | Mezcla de la saturacion de cinta (INPUT empuja la senal contra ella) |
| DELAY + dos perillas pequenas | Eco de cinta: tiempo (20-1200 ms) + FB (realimentacion) + MIX |
| RESET | Devuelve los controles a su valor por defecto (doble clic en una perilla resetea solo esa) |
| REV / RWD / F.FWD / STOP / PLAY / REC | Transporte de una grabadora de 60 s: REC graba lo que entra, PLAY lo reproduce con todos los efectos, REV en reversa, RWD / F.FWD rebobinan / avanzan a 6x, STOP frena la cinta (efecto "tape stop") |
| LOOP | Repite la cinta grabada |
| TILT (icono de carrete) | Desalineacion del cabezal (azimut): el canal derecho llega un poco tarde y mas apagado |
| SYNC | La maquina sigue el transporte de FL Studio (con REC armado, graba al dar play en el host) |
| INPUT / OUTPUT | Ganancia de entrada (-24..+24 dB) y de salida (-24..+12 dB) |
| IN / OUT (interruptor junto a los VU) | Elige que miden los VU: entrada o salida |
| VU (x2) | Medidores con balistica de ~300 ms (0 VU = -18 dBFS) |
| Carretes | Giran segun la velocidad real de la cinta; la cinta pasa de un carrete al otro segun la posicion |
| Menu superior izquierdo "..." / lista superior derecha | Menu (tamano, extras, intro, reset) / presets de fabrica |

## Lo que se conserva de la v1.0 (en el menu "..." -> Extras)
Reverb y cortes de EQ (low cut, mid cut, high cut). El analizador de espectro se reemplazo por los VU. El efecto de frenado con retroceso
corto vive ahora en STOP (frena) y REV (retrocede). Las velocidades 1 / 1.5 / 2 se reemplazan por VARI y PITCH.

## Notas
- La cinta grabada (60 s, ~23 MB de RAM) NO se guarda dentro del proyecto de FL Studio; los ajustes de los controles si.
- El modo normal (STOP) procesa en vivo lo que entra, igual que la v1.0.
