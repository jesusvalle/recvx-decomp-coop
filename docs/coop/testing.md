# Cómo probar el cooperativo

Guía práctica de lo aprendido en los hitos 1, 2a y 2b. Las rutas de herramientas son de esta máquina (ver `CLAUDE.local.md`).

## Herramientas

Están en `.superpowers/sdd/<plan>/tools/` (la más reciente: `2026-10-10-coop-hito2b`). **No están versionadas**: `.superpowers/` lo ignora git.

| Herramienta | Para qué |
| --- | --- |
| `check_build.py --sym X --str Y` | Que el último build no tiene errores y que el xMAP tiene los símbolos y el ELF las cadenas |
| `cmp_load.py a.elf b.elf` | Compara los segmentos `PT_LOAD` de dos ELF (la prueba de identidad sin `COOP`) |
| `firstdiff.py` | Dice qué objeto difiere de `baseline_main.elf` (primer símbolo con otra dirección o tamaño) |
| `baseline_main.elf` | ELF de referencia compilado sin el mod. **No se sube a git** (es el ejecutable del juego) |
| `mdcheck.py` | Enlaces y tablas de `CLAUDE.md` y `docs/` |
| `pcsx2.ps1` | Arnés de PowerShell (se carga con dot-source): `Set-TestConfig`, `Start-Game`, `Press`, `KeyDown`/`KeyUp`, `Snap`, `SaveState`, `Stop-Game`, `Restore-Config` |
| `ramread.py [--state X.p2s] [--xmap X.xMAP]` | Lee la RAM de un savestate: `memp`, `mempb`, `endp`, `Ps2_free_texmemsize`, sala, estado de P2 e inventario de P1 |
| `texmargin.py FREE RM_xxxx.RDX` | Peor caso del pool de texturas a partir de una medida en una sala |
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

Teclado del arnés: P1 flechas, Z (X), X (círculo), C (cuadrado), V (triángulo), Enter (Start), Q/E (L1/R1); P2 I/J/K/L, N (X), M (círculo), B (cuadrado), H (triángulo), P (Start), U/O (L1/R1); F1 guarda un savestate. Nunca pulsar Alt: Alt+Enter cambia a pantalla completa.

## Partida de prueba

La tarjeta del usuario tiene en el slot 1 "Claire / 01 / Prison: front" (sala 0-1, la de la máquina de escribir). P1 solo lleva el mechero, que no se puede desequipar, así que para probar armas hay que compilar con `COOP_TEST` (`#define COOP_TEST` tras el `#ifdef COOP` de coop.c): al cargar, P1 recibe una pistola (id 5) con 15 balas, que hay que equipar desde su inventario. Quitar el define antes del build final.

En el inventario, el cursor empieza en la lista de objetos; el mechero está en la casilla fija (STANDARD).

## Prueba de identidad sin `COOP`

Quitar `"COOP"` de `defines`, borrar `build/src/`, compilar y `cmp_load.py baseline_main.elf elf/main.elf`. Si difiere, `firstdiff.py` dice el objeto: si es `player.o`, `effsub1b.o` o `ps2_SystemSaveScreen.o` (MWCC no determinista), borrarlo y recompilar hasta que coincida; con `player.o` pueden hacer falta más de 10 intentos. Después volver a poner `"COOP"`, borrar `build/src/` y compilar.

## Lo que no se puede probar sin el usuario

- La vibración de los mandos.
- Cualquier cosa que necesite dos mandos reales a la vez o avanzar en la historia (puertas nuevas, otros personajes, enemigos concretos).
