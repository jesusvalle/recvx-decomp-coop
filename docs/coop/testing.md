# Cómo probar el cooperativo

Guía práctica de lo aprendido en los hitos 1, 2a y 2b. Las rutas de herramientas son de esta máquina (ver `CLAUDE.local.md`).

## Herramientas

Están en `.superpowers/sdd/<plan>/tools/` (la más reciente: `2026-10-11-coop-hito5`; `check_build.py`, `firstdiff.py`, `identity.sh`, `test_wpnfiles.py`, `fnscan.py` y los `check_fix.py` de cada revisión están en la carpeta del plan). **No están versionadas**: `.superpowers/` lo ignora git.

| Herramienta | Para qué |
| --- | --- |
| `check_build.py --sym X --str Y` | Que el último build no tiene errores y que el xMAP tiene los símbolos y el ELF las cadenas |
| `cmp_load.py a.elf b.elf` | Compara los segmentos `PT_LOAD` de dos ELF (la prueba de identidad sin `COOP`) |
| `symdiff.py a.elf b.elf` (hito 7) | Primeros símbolos con otra dirección o tamaño: dice qué objeto recompilar cuando la identidad falla por el no determinismo de MWCC |
| `firstdiff.py` | Dice qué objeto difiere de `baseline_main.elf` (primer símbolo con otra dirección o tamaño) |
| `baseline_main.elf` | ELF de referencia compilado sin el mod. **No se sube a git** (es el ejecutable del juego) |
| `mdcheck.py` | Enlaces y tablas de `CLAUDE.md` y `docs/` |
| `pcsx2.ps1` | Arnés de PowerShell (se carga con dot-source): `Set-TestConfig`, `Start-Game`, `Press`, `KeyDown`/`KeyUp`, `Snap`, `SaveState`, `Stop-Game`, `Restore-Config` |
| `ramread.py [--state X.p2s] [--xmap X.xMAP]` | Lee la RAM de un savestate: `memp`, `mempb`, `endp`, `Ps2_free_texmemsize`, sala, estado de P2, inventario de P1 y (desde el 2c) bloque de P2, `coop_wpn2_no` y `coop_wpn_ok` |
| `test_wpnfiles.py` | (2c) Recorre los ficheros de arma de `SYSTEM.AFS[20..40]` y comprueba que caben en los buffers de P2 de coop.h |
| `fnscan.py NOMBRE…` | Para cada función, cuenta los `plp` de su cuerpo y lista las líneas que dañan al jugador (para saber qué enemigos o efectos usan `plp`) |
| `check_fix.py [ARCHIVO.c] [prueba…]` | (2d, 3) Pruebas de los arreglos de cada revisión final sobre coop.c; la del 2d simula `bhSetPad` con el enmascarado del mando de P2. Con un `ARCHIVO.c` se pasan a otra versión (para ver que fallaban antes del arreglo) |
| `texmargin.py FREE RM_xxxx.RDX` | Peor caso del pool de texturas a partir de una medida en una sala |
| `splitread.py [--state X.p2s] [--xmap X.xMAP]` | (hito 5) Pantalla partida en un savestate: la opción `itm[275]`, `rom->cut_n`/`lgt_n`, el medidor (`coop_fstat`, `coop_fmax`), `coop_split`, el plano de P1 y de P2, las bandas `k` y el `XYOFFSET` de los dos búferes |
| `identity.sh` / `identity2.sh` | (hito 5) Identidad sin `COOP` frente a `baseline_main.elf`, y con `COOP` sin `COOP_SPLIT` frente a `coop_before.elf` (el build de antes del hito). Recompilan cualquier objeto que difiera, hasta 50 intentos |
| `mkiso_test.py` | Genera `iso/RECVX_TEST.iso` (`mkiso.py` siempre escribe `RECVX_NEW.iso`) |

## Flujo de una prueba

1. `python compile.py` y `check_build.py`.
2. Con PCSX2 **cerrado**, `python mkiso.py -m insert` y comprobar que la fecha de la ISO es posterior a la del ELF. Si el usuario tiene PCSX2 abierto con `RECVX_NEW.iso`, generar `RECVX_TEST.iso` con `mkiso_test.py` y no tocar su ventana.
3. `Set-TestConfig` (hace copia del `PCSX2.ini`, añade teclado a los dos mandos y pone los savestates sin comprimir) y `Start-Game 45`.
4. Navegar hasta la partida:
   - el primer Start en el título a veces se pierde: si sigue el "PRESS START", pulsar otra vez;
   - menú: LOAD GAME (ya seleccionado) → X → slot 1 → X → partida → X → YES → X, y esperar unos 13 s;
   - si se espera unos 3 minutos en el título empieza el demo, y una X en el demo vuelve a los logos.
5. Capturas con `Snap`; medidas con `SaveState` y `ramread.py`.
6. Al terminar: `Stop-Game` y `Restore-Config` (comprobar que el ini queda igual que la copia).

Teclado del arnés: P1 flechas, Z (X), X (círculo), C (cuadrado), V (triángulo), Enter (Start), Q/E (L1/R1), 1/3 (L2/R2, desde el hito 5: L1+L2+R1+R2 = Q+1+E+3 alterna una o dos cámaras); P2 I/J/K/L, N (X), M (círculo), B (cuadrado), H (triángulo), P (Start), U/O (L1/R1); F1 guarda un savestate. Nunca pulsar Alt: Alt+Enter cambia a pantalla completa.

## Partida de prueba

La tarjeta del usuario tiene en el slot 1 "Claire / 01 / Prison: front" (sala 0-1, la de la máquina de escribir). P1 solo lleva el mechero, que no se puede desequipar. Desde el hito 2c, P2 lleva su propia arma (el cuchillo en el build normal). Para probar armas de fuego hay que compilar con `COOP_TEST` (`#define COOP_TEST` tras el `#ifdef COOP` de coop.c): al cargar, el bloque de P2 (`itm[256..]`) recibe una pistola (id 5) con 15 balas, ya equipada. Hasta el hito 2b, `COOP_TEST` se la daba a P1. Quitar el define antes del build final. Si se guarda partida con `COOP_TEST`, la pistola queda en el bloque de P2 de esa partida.

**Desde el 2026-10-11 la tarjeta es otra:** el slot 1 tiene 1 vacía, 2 "Claire / 01 / NO_DATA" y 3 "Chris / 01 / T Facility: UG". En la pantalla de la tarjeta, la flecha abajo del teclado del arnés no movió el cursor (X sí funciona); para cargar una partida concreta, que la elija el usuario. Dos Start en el título llevan a la intro y a la pantalla de la tarjeta.

En el inventario, el cursor empieza en la lista de objetos; el mechero está en la casilla fija (STANDARD).

## Prueba de identidad sin `COOP`

Quitar `"COOP"` de `defines`, borrar `build/src/`, compilar y `cmp_load.py baseline_main.elf elf/main.elf`. Si difiere, `firstdiff.py` dice el objeto: si es `player.o`, `effsub1b.o` o `ps2_SystemSaveScreen.o` (MWCC no determinista), borrarlo y recompilar hasta que coincida; con `player.o` pueden hacer falta más de 10 intentos. Después volver a poner `"COOP"`, borrar `build/src/` y compilar.

## Sin emulador

Los hitos 2c, 2d y 3 se hicieron sin PCSX2 (petición del usuario). La verificación fue: build y `check_build`, identidad sin `COOP` (con `player.o` pueden hacer falta más de 30 recompilaciones: si `identity.sh` acaba en DIFERENTES, repetirlo), pruebas sobre los datos del juego (`test_wpnfiles.py`), pruebas de código (`check_fix.py`) y una revisión final por hito con un revisor nuevo. El resumen y las checklists para el usuario están en [revision-2026-10-11.md](revision-2026-10-11.md).

## Sonido

Los `printf` no salen en el log, pero se puede verificar el sonido en un savestate. Herramientas en `.superpowers/sdd/2026-10-10-coop-sonido/tools/`:

| Herramienta | Para qué |
| --- | --- |
| `chk.py X.p2s` | Estado de `coopsnd.c` (`coop_se_state`, muestras y tamaño del banco de P2), `SE_BANK` y el HD fusionado |
| `chk2.py X.p2s` | Estructura del HD fusionado, muestras de P2 comparadas byte a byte con el `.SPQ` en la SPU2, `se_max`/`HdAddr`/`SE_TBL` del IOP |
| `hd.py` | Lector de los HD de los `.SPQ` de `MULTSPQ1.AFS` |
| `irx.py syms` / `irx.py FUNC…` | Símbolos y desensamblado de `TSNDDRV.IRX` |

Para ver si un sonido ha llegado a sonar, guardar el savestate justo después y buscar en `SPU2.bin` (antes del byte 0x10004) direcciones en medias palabras dentro de la zona de sus muestras.

## Diagnosticar un cuelgue

Herramientas del hito 8 en `.superpowers/sdd/2026-10-10-coop-hito8/tools/`: `isoelf.py ISO SALIDA` (saca `SLUS_201.84` de una ISO), `syms.py A.elf B.elf` (símbolos que solo están en uno o cambian de tamaño) y `rd8.py ESTADO.p2s XMAP` (variables del hito 8, flags de `sys` y estado de P1/P2).

1. **Reproducir con el arnés** sobre la ISO que falla: `$Global:PX_Iso = "...\RECVX_TEST.iso"` después de cargar `pcsx2.ps1`. `FPS: no` en la barra de PCSX2 significa que el EE no termina frames (bucle de espera).
2. **Guardar un savestate** (`SaveState`, F1) y leer el estado:
   - cargador: `ts_flg 0x80` y `cb_flg 0x800000` con `ply` a cero = aún cargando; `coop_ld_mode` dice en qué paso está la carga de P2 (6 = esperando a `coopSeStep`);
   - sonido: `SpqFileReadRequestFlag`, `SdReadMode`, `trans_level`, `get_iop_snddata.se_sum[]` frente a `SE_HD_CHECK[]`, y la cola `sndque_tbl` con `sque_r_idx`/`sque_w_idx` (ver [world-systems.md](../architecture/world-systems.md#driver-de-sonido-iop-y-memoria-spu2)).
3. **Comparar con un build que funciona:** sacar el ELF de cada ISO con `isoelf.py`, ver qué cambia con `syms.py` y, si lo que cambia no se ejecuta en el cuelgue, sospechar de direcciones fijas: buscar en la RAM de los dos savestates qué punteros apuntan a la zona dañada. Así se encontró el heap de la libc ([rooms-and-memory.md](../architecture/rooms-and-memory.md#memoria)).

## Lo que no se puede probar sin el usuario

- La vibración de los mandos.
- Oír los sonidos (el arnés solo puede comprobar la RAM).
- Cualquier cosa que necesite dos mandos reales a la vez o avanzar en la historia (puertas nuevas, otros personajes, enemigos concretos).
