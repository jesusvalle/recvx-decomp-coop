# Cooperativo, hito 5 (prototipo): pantalla partida — plan de implementación

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** En juego normal, la pantalla se parte en dos franjas de 640x240: arriba, el plano de cámara de P1; abajo, el de P2, cada una a tamaño real y siguiendo a su jugador. Se puede elegir una o dos cámaras con L1+L2+R1+R2 del mando 1. La primera persona va en la franja de su jugador. Un medidor cuenta los frames que pasan de 2 vsync.

**Architecture:**

- Archivo nuevo `coopcam.c`, detrás del define `COOP_SPLIT`.
- **Cámara de P2 (G29):** una `CAM_WORK coop_cam2` que se calcula con el `bhCheckCut` original en un cambio de contexto (`cam ↔ coop_cam2`, `plp = &ply2`), guardando y restaurando el estado global que toca un cambio de plano.
- **Dibujo (G30):** dos pasadas de `bhAllDrawModel`. Cada una con su cámara, su estado de plano (mallas ocultas, luces, niebla, recorte de vista), el recorte del GS (`SCISSOR_1`) en su franja y la imagen desplazada en vertical. El desplazamiento se hace con `XYOFFSET_1` o, si la prueba de la tarea 2 falla, con el centro de `njSetScreen`. La lista de dibujo se envía entre pasadas.
- **Medidor (G31)**, en `Ps2SwapDBuff`.
- **Fase 2 (G32):** los bits de primera persona de P2 viajan con `coopBegin`/`coopEnd`.

**Tech Stack:** C (MWCC, estilo C89), Python 3.13, PowerShell (arnés de PCSX2).

**Spec:** [docs/superpowers/specs/2026-10-11-coop-hito5-split-design.md](../specs/2026-10-11-coop-hito5-split-design.md)

## Global Constraints

- Todo el código nuevo va dentro de `#ifdef COOP_SPLIT` (que requiere `COOP`):
  - sin `COOP`, los `PT_LOAD` son idénticos a `baseline_main.elf`;
  - con `COOP` y sin `COOP_SPLIT`, son idénticos a `coop_before.elf` (tarea 0).
- No se cambia la estructura de `SYS_WORK` ni de `BH_PWORK`. La opción va en `sys->itm[275]` (`COOP_ITM + 19`): 0 = dos cámaras, 1 = una.
- **Ganchos G29-G32.** G10-G28 ya están ocupados; el hito 7, sin commitear en el árbol de trabajo, usa G22-G28.
- Estilo del archivo que se toca: Allman, variables al principio de la función, flags en hex, comentarios `/* */`.
- **Sin commits ni PR** (lo tiene pedido el usuario). El árbol de trabajo tiene cambios sin commitear del hito 7: no se tocan ni se revierten.
- `COOP_TEST` y `COOP_TEST_SCOPE` nunca quedan activos en el build final.
- PCSX2 se usa con el arnés (`pcsx2.ps1`). **Antes de lanzarlo, comprobar que el usuario no tiene PCSX2 abierto**; si lo tiene, generar `RECVX_TEST.iso` y no tocar su ventana. Nunca pulsar Alt.
- Al cambiar `COOP` o `COOP_SPLIT` en `defines`, borrar `build/src/`.

## Review Focus

1. **El estado del plano de P2 se cuela en la franja de P1** (mallas que aparecen o desaparecen, luces o niebla de otro plano cuando P2 cambia de plano). La franja de P1 debe verse igual que con una cámara. *Prueba:* tarea 3, paso 9 (captura de la franja de P1 antes y después de que P2 cruce a otro plano, más la comparación con el modo de una cámara).
2. **Cambio de sala, reintento o carga con la pantalla partida:** `coop_cam2.ncut` de la sala anterior no puede indexar `rom->cutp` de la nueva, o se cuelga. Debe reiniciarse limpio. *Prueba:* tarea 3, paso 10 (`coopCamRoomStart` pone `coop_cam2_ok = 0` y G30 no dibuja partido hasta que G29 recalcula; savestate tras una puerta o un reintento).
3. **Mantener L1+L2+R1+R2 pulsados**, o pulsarlos con un menú o mensaje abierto: alterna una sola vez y nunca con menús. *Prueba:* tarea 1, paso 9 (pulsación larga de 1,5 s → `itm[275]` cambia una vez).
4. **Cosas que avanzan al dibujar** (luces animadas, `pl_sleep_cnt`, efectos dibujados dos veces): no deben ir al doble de velocidad con dos cámaras. *Prueba:* tarea 3, paso 8 (las luces y `pl_sleep_cnt` se guardan y restauran alrededor de la pasada de P2; el revisor busca más estado que cambie al dibujar en `bhAllDrawModel` y en `bhDrawEffect`, y lo que encuentre se apunta en la checklist del usuario).
5. **Paso de partida a completa** (cinemática, examinar, P2 muerto): el modo completo no debe quedarse con el recorte, el desplazamiento, la niebla o las luces de una franja. *Prueba:* tarea 3, paso 11 (captura en una cámara tras haber estado partida; `njUserClipping(0)` y la restauración siempre al final de `coopSplitDraw`).

---

## Mapa de archivos

| Archivo | Cambio |
| --- | --- |
| `include/ps2/veronica/prog/coopcam.h` | Nuevo: prototipos y constantes de la pantalla partida |
| `src/ps2/veronica/prog/coopcam.c` | Nuevo: cámara de P2, dibujo partido, medidor y combinación |
| `include/ps2/veronica/prog/coop.h` | Prototipos `coopP2Active`, `coopScopeSwap` (con `COOP_SPLIT`) |
| `src/ps2/veronica/prog/coop.c` | `coopP2Active`, `coopScopeSwap`, reinicio en `coopRoomStart`, visibilidad de P2 por franja, G32, `COOP_TEST_SCOPE` |
| `src/ps2/veronica/prog/game.c` | Include de `coopcam.h`; G29 y G30 |
| `src/ps2/veronica/prog/ps2_NaSystem.c` | Include de `coopcam.h`; G31 en `Ps2SwapDBuff` |
| `compile_config.json` | `"COOP_SPLIT"` en `defines`; `coopcam.c` en `source_files` |

`W = .superpowers/sdd/2026-10-11-coop-hito5`. Herramientas en `W/tools`, copiadas de `.superpowers/sdd/2026-10-11-coop-hito3/tools`, más las nuevas de la tarea 0.

Datos ya medidos para este plan:

- `lgt_n` máximo en las 205 salas = **44** (`RM_0020`, `RM_0021`); de ahí `COOP_LGT_MAX 48`.
- `cut_n` máximo = 14.
- En `sys->p1per->on` y `->press` (botones SCE ya invertidos, ps2_sg_pad.c `Pad_set`): L2 `0x1`, R2 `0x2`, L1 `0x4`, R1 `0x8`. Cuadra con `pad_tab_a`: R1 = `0x8` es apuntar y L1 = `0x4`.

---

### Task 0: Preparación (herramientas y línea base)

**Files:** solo `W/`.

**Interfaces:**

- Produces: `W/tools/coop_before.elf`, `W/tools/splitread.py`, `W/tools/firstdiff2.py`, `W/tools/pcsx2.ps1` con L2/R2 en el mando 1 (teclas `1` y `3`), `W/identity2.sh`.

- [ ] **Step 1: Copiar herramientas.**

```bash
cd /c/Users/jesus/Desktop/recvx-decomp
W=.superpowers/sdd/2026-10-11-coop-hito5
mkdir -p $W/tools
cp .superpowers/sdd/2026-10-11-coop-hito3/tools/{baseline_main.elf,cmp_load.py,mdcheck.py,mkiso_test.py,pcsx2.ps1,ramread.py} $W/tools/
cp .superpowers/sdd/2026-10-11-coop-hito3/check_build.py $W/
```

- [ ] **Step 2: Arnés con L2/R2.** En `W/tools/pcsx2.ps1`:
  - cambiar `PX_Dir` a `"C:/Users/jesus/Desktop/recvx-decomp/.superpowers/sdd/2026-10-11-coop-hito5/tools"`;
  - añadir a `$PX_Keys` las entradas `"p1l2"=0x31; "p1r2"=0x33;`;
  - en `Set-TestConfig`, añadir al final de `$p1` el texto `` `r`nL2 = Keyboard/1`r`nR2 = Keyboard/3 ``.

- [ ] **Step 3: `W/tools/splitread.py`:**

```python
"""Estado de la pantalla partida en un savestate: medidor, opción, cámara de P2 y XYOFFSET.

uso: splitread.py [--state X.p2s] [--xmap X.xMAP]
"""
import argparse, glob, os, pathlib, re, struct, sys, zipfile

root = pathlib.Path(__file__).resolve().parents[4]
ap = argparse.ArgumentParser()
ap.add_argument("--state")
ap.add_argument("--xmap")
a = ap.parse_args()
state = a.state or max(glob.glob(os.path.expandvars(r"$USERPROFILE/Documents/PCSX2/sstates/*.p2s")), key=os.path.getmtime)
ram = zipfile.ZipFile(state).read("eeMemory.bin")
syms = {}
for line in (pathlib.Path(a.xmap) if a.xmap else root / "elf/main.elf.xMAP").read_text(encoding="latin-1").splitlines():
    m = re.match(r"\s+([0-9A-Fa-f]{8})\s+[0-9A-Fa-f]{8}\s+\.\S+\s+(\S+)\s", line)
    if m:
        syms.setdefault(m.group(2), int(m.group(1), 16))

def u32(x): return struct.unpack_from("<I", ram, x & 0x1FFFFFF)[0]
def s32(x): return struct.unpack_from("<i", ram, x & 0x1FFFFFF)[0]
def f32(x): return struct.unpack_from("<f", ram, x & 0x1FFFFFF)[0]
def u64(x): return struct.unpack_from("<Q", ram, x & 0x1FFFFFF)[0]
def need(n):
    if n not in syms:
        sys.exit(f"falta {n} en el xMAP")
    return syms[n]

sysp = u32(need("sys"))
print("savestate =", state)
print("sala =", f"{ram[(sysp + 0x85) & 0x1FFFFFF]}-{ram[(sysp + 0x86) & 0x1FFFFFF]}")
print("opción itm[275] =", u32(sysp + 0x270 + 275 * 4), "(0 = dos cámaras)")
romp = need("romp")
print("rom->cut_n =", s32(romp + 0x80), " rom->lgt_n =", s32(romp + 0x84))
fs = [u32(need("coop_fstat") + 4 * i) for i in range(4)]
tot = sum(fs) or 1
print("frames <=2/3/4/>=5 vsync =", fs, f" lentos = {100.0 * (tot - fs[0]) / tot:.1f} %", " peor =", u32(need("coop_fmax")))
print("coop_split =", s32(need("coop_split")), " coop_cam2_ok =", s32(need("coop_cam2_ok")) if "coop_cam2_ok" in syms else "(aún no)")
if "coop_cam2" in syms:
    print("plano P1 / P2 (ncut) =", s32(need("cam") + 0x8C), "/", s32(need("coop_cam2") + 0x8C))
if "coop_band" in syms:
    print("bandas k =", f32(need("coop_band")), f32(need("coop_band") + 4))
if "coop_xy_dbg" in syms:
    for i in range(2):
        v = u64(need("coop_xy_dbg") + 8 * i)
        print(f"XYOFFSET draw{i}1 = OFX {(v & 0xFFFF) / 16:.1f} OFY {((v >> 32) & 0xFFFF) / 16:.1f}")
```

- [ ] **Step 4: `W/tools/firstdiff2.py`.** Es una copia de `.superpowers/sdd/2026-10-11-coop-hito3/firstdiff.py` con la línea `B=...` cambiada por:

```python
import sys
B = sys.argv[1] if len(sys.argv) > 1 else ".superpowers/sdd/2026-10-11-coop-hito5/tools/baseline_main.elf"
```

- [ ] **Step 5: `W/identity2.sh`.** Compila con `COOP` y sin `COOP_SPLIT`, compara con `coop_before.elf` y vuelve a poner `COOP_SPLIT`. Si `"COOP_SPLIT"` no está aún en `defines`, solo compila y compara.

```bash
set -u
cd /c/Users/jesus/Desktop/recvx-decomp
W=.superpowers/sdd/2026-10-11-coop-hito5
cp compile_config.json $W/compile_config.split.json
.venv/Scripts/python.exe - <<'PY'
p = "compile_config.json"
t = open(p, encoding="utf-8").read()
n = t.replace(',\n    "COOP_SPLIT"', '').replace(',\r\n    "COOP_SPLIT"', '')
open(p, "w", encoding="utf-8", newline="").write(n)
PY
rm -rf build/src
.venv/Scripts/python.exe compile.py > $W/build_nosplit.log 2>&1
for i in $(seq 1 30); do
  if .venv/Scripts/python.exe $W/tools/cmp_load.py $W/tools/coop_before.elf elf/main.elf > $W/cmp_nosplit.txt; then echo "intento $i: IDENTICOS"; break; fi
  f=$(.venv/Scripts/python.exe $W/tools/firstdiff2.py $W/tools/coop_before.elf)
  echo "intento $i: difiere $f"
  case "$f" in player.o|ps2_SystemSaveScreen.o|effsub1b.o) rm -f build/src/ps2/veronica/prog/$f; .venv/Scripts/python.exe compile.py > $W/build_loop.log 2>&1;; *) echo "NO es un objeto no determinista conocido"; break;; esac
done
tail -1 $W/cmp_nosplit.txt
cp $W/compile_config.split.json compile_config.json
rm -rf build/src
.venv/Scripts/python.exe compile.py > $W/build_split.log 2>&1
tail -1 $W/build_split.log
```

- [ ] **Step 6: Línea base `coop_before.elf`.** Con el árbol actual (que tiene `COOP` y no tiene `COOP_SPLIT`): `rm -rf build/src`, `.venv/Scripts/python.exe compile.py`, `cp elf/main.elf $W/tools/coop_before.elf`. Comprobar: `.venv/Scripts/python.exe $W/check_build.py --sym coopControlPlayer2` → PASA.

- [ ] **Step 7: Partida de prueba.** Comprobar que el usuario no tiene PCSX2 abierto (`Get-Process pcsx2-qt`). Después: `mkiso.py -m insert` (comprobar la fecha de la ISO), y en PowerShell:
  - `. $W/tools/pcsx2.ps1; Set-TestConfig; Start-Game 45`;
  - navegar a la partida del slot 1 (guía en [testing.md](../../coop/testing.md#flujo-de-una-prueba));
  - `SaveState`;
  - `Stop-Game; Restore-Config`.

  `splitread.py` imprime la sala, la opción y `rom->cut_n`, y después se detiene en «falta coop_fstat», porque ese símbolo aún no existe. Apuntar en `W/progress.md` el `cut_n` de la sala 0-1. **Si es 1**, las pruebas de «cada jugador en su plano» de la tarea 3 pasan a la checklist del usuario.

---

### Task 1: Base: define, archivo nuevo, ganchos, medidor y combinación

**Files:**

- Create: `include/ps2/veronica/prog/coopcam.h`, `src/ps2/veronica/prog/coopcam.c`
- Modify: `compile_config.json`, `include/ps2/veronica/prog/coop.h`, `src/ps2/veronica/prog/coop.c`, `src/ps2/veronica/prog/game.c` (bloque de dibujo y tras `bhCheckCut`), `src/ps2/veronica/prog/ps2_NaSystem.c` (`Ps2SwapDBuff`)

**Interfaces:**

- Consumes: `coop_loaded`, `coop_hidden`, `coopDemo()`, `coopP2Dead()` (static en coop.c), `COOP_ITM`.
- Produces:
  - `int coopP2Active(void)` (coop.c);
  - `void coopCamRoomStart(void)`, `void coopUpdateCamera2(void)`, `int coopSplitDraw(void)`, `void coopDrawMeter(void)`, `void coopFrameStat(unsigned int vc)`, `int coopSplitShowP2(void)` (coopcam.c);
  - las statics `coop_split`, `coop_fstat[4]`, `coop_fmax` y `coop_fslow`;
  - `COOP_CAM_OPT 275` y `COOP_LGT_MAX 48` (coopcam.h).

- [ ] **Step 1: Prueba que falla.** `.venv/Scripts/python.exe $W/check_build.py --sym coopFrameStat --sym coopUpdateCamera2 --sym coopSplitDraw` → FALLA (los símbolos no existen).

- [ ] **Step 2: `compile_config.json`.** En `"defines"`, después de `"COOP"`, añadir `"COOP_SPLIT"` (queda `"DEBUG", "COOP", "COOP_SPLIT"`, una por línea como las demás). En `"source_files"`, después de `"src/ps2/veronica/prog/coopsnd.c"`, añadir `"src/ps2/veronica/prog/coopcam.c"`. Respetar los finales de línea del archivo.

- [ ] **Step 3: `include/ps2/veronica/prog/coopcam.h`:**

```c
#ifndef _COOPCAM_H_
#define _COOPCAM_H_

#ifdef COOP_SPLIT

#include "types.h"

/* sys->itm[275] (COOP_ITM + 19): 0 = dos cámaras, 1 = una. Viaja con la partida. */
#define COOP_CAM_OPT 275
/* Máximo de rom->lgt_n en las 205 salas: 44 (RM_0020, RM_0021). */
#define COOP_LGT_MAX 48

void coopCamRoomStart(void);
void coopUpdateCamera2(void);
int coopSplitDraw(void);
void coopDrawMeter(void);
void coopFrameStat(unsigned int vc);
int coopSplitShowP2(void);

#endif

#endif
```

- [ ] **Step 4: `coop.h`.** Antes del `#endif` final de `#ifdef COOP`:

```c
#ifdef COOP_SPLIT
int coopP2Active(void);
#endif
```

- [ ] **Step 5: `coop.c`.**
  - Tras la lista de includes del principio, añadir `#ifdef COOP_SPLIT` / `#include "../../../ps2/veronica/prog/coopcam.h"` / `#endif`.
  - Justo después de la función `coopP2Dead`:

```c
#ifdef COOP_SPLIT
/* Para coopcam.c: P2 está en juego (cargado, sin demo, visible y vivo). */
int coopP2Active(void)
{
    if ((coop_loaded == 0) || (coopDemo() != 0) || (coop_hidden != 0) || (coopP2Dead() != 0))
    {
        return 0;
    }

    return 1;
}
#endif
```

  - En `coopRoomStart`, justo después de `int i;` y la línea en blanco:

```c
#ifdef COOP_SPLIT
    coopCamRoomStart();

#endif
```

- [ ] **Step 6: `src/ps2/veronica/prog/coopcam.c`** (versión de la tarea 1):

```c
#include "../../../ps2/veronica/prog/coopcam.h"

#ifdef COOP_SPLIT

#include "../../../ps2/veronica/prog/coop.h"
#include "../../../ps2/veronica/prog/ps2_NaDraw2D.h"
#include "../../../ps2/veronica/prog/ps2_NaSystem.h"
#include "../../../ps2/veronica/prog/main.h"

static int coop_split;
static unsigned int coop_fstat[4];
static unsigned int coop_fmax;
static int coop_fslow;

/* G31 (Ps2SwapDBuff): vc = vsync que ha costado el frame. [0] = 2 o menos, [1] = 3, [2] = 4, [3] = 5 o más. */
void coopFrameStat(unsigned int vc)
{
    int i;

    if (vc <= 2)
    {
        i = 0;
    }
    else if (vc >= 5)
    {
        i = 3;
    }
    else
    {
        i = vc - 2;
    }

    coop_fstat[i]++;

    if (vc > coop_fmax)
    {
        coop_fmax = vc;
    }

    coop_fslow = (vc > 2) ? 1 : 0;
}

static void coopDrawRect(float x0, float y0, float x1, float y1, unsigned int argb)
{
    NJS_POINT2COL p2c;
    NJS_POINT2 p[4];
    NJS_COLOR col[4];

    njColorBlendingMode(0, 8);
    njColorBlendingMode(1, 6);

    p2c.p = p;
    p2c.col = col;
    p2c.tex = NULL;
    p2c.num = 1;

    col[0].color = argb;
    col[1].color = argb;
    col[2].color = argb;
    col[3].color = argb;

    p[0].x = x0;
    p[0].y = y0;
    p[1].x = x1;
    p[1].y = y0;
    p[2].x = x1;
    p[2].y = y1;
    p[3].x = x0;
    p[3].y = y1;

    njDrawPolygon2D(&p2c, 4, -0.8f, 96);
}

/* G30, después del bloque de dibujo: cuadradito rojo si el último frame pasó de 2 vsync. */
void coopDrawMeter(void)
{
    if (coop_fslow != 0)
    {
        coopDrawRect(624.0f, 8.0f, 632.0f, 16.0f, 0xFFFF0000);
    }
}

void coopCamRoomStart(void)
{
    int i;

    for (i = 0; i < 4; i++)
    {
        coop_fstat[i] = 0;
    }

    coop_fmax = 0;
    coop_fslow = 0;
}

/* L1+L2+R1+R2 del mando 1 (botones SCE de p1per: L2 0x1, R2 0x2, L1 0x4, R1 0x8), al pulsar el último. */
static void coopCamToggle(void)
{
    const PDS_PERIPHERAL* pp;

    pp = sys->p1per;

    if ((pp == NULL) || (!(sys->sp_flg & 0x20)))
    {
        return;
    }

    if (((pp->on & 0xF) != 0xF) || (!(pp->press & 0xF)))
    {
        return;
    }

    /* Solo en juego normal: sin subpantallas, mensajes, jugador ocupado, cinemática ni puerta. */
    if ((sys->st_flg & 0x1C04020C) || (sys->cb_flg & 0x5))
    {
        return;
    }

    sys->itm[COOP_CAM_OPT] = (sys->itm[COOP_CAM_OPT] != 0) ? 0 : 1;
}

static int coopSplitWanted(void)
{
    if (sys->itm[COOP_CAM_OPT] != 0)
    {
        return 0;
    }

    if (coopP2Active() == 0)
    {
        return 0;
    }

    if ((sys->cb_flg & 0x5) || (sys->st_flg & 0x1) || (sys->cine_an > 0) || (sys->gm_flg & 0x100))
    {
        return 0;
    }

    if (rom->lgt_n > COOP_LGT_MAX)
    {
        return 0;
    }

    return 1;
}

/* G29 (bhMainSequence, tras bhCheckCut). */
void coopUpdateCamera2(void)
{
    coopCamToggle();

    coop_split = coopSplitWanted();
}

/* G30: 1 si ha dibujado la pantalla partida (el bloque original no se ejecuta). */
int coopSplitDraw(void)
{
    return 0;
}

/* coopDrawPlayer2: si P2 se dibuja en la pasada actual. Fuera de una pasada, como el original. */
int coopSplitShowP2(void)
{
    return (sys->pt_flg & 0x1) ? 1 : 0;
}

#endif
```

- [ ] **Step 7: `game.c`.**
  - Tras el bloque `#ifdef COOP` / `#include …coop.h` / `#endif` del principio, añadir `#ifdef COOP_SPLIT` / `#include "../../../ps2/veronica/prog/coopcam.h"` / `#endif`.
  - **G30.** Dentro de `if (i == (sys->loop_ct - 1))`, el bloque queda así (lo de dentro, sin cambios):

```c
        if (i == (sys->loop_ct - 1)) 
        {
#ifdef COOP_SPLIT
            if (coopSplitDraw() == 0)
            {
#endif
            if ((sys->gm_flg & 0x200))
            {
                bhDrawSmallScreenRenderTexture();
            }
            
            /* … el resto del bloque original, sin cambios, hasta el if (sys->fade_an > 0) { … } … */
#ifdef COOP_SPLIT
            }

            coopDrawMeter();
#endif
        }
```

  (El comentario de en medio es solo del plan, no se escribe: en el archivo va el bloque original entero.)
  - **G29.** Justo después del `if/else` que llama a `bhCheckCut(0)` / `bhCheckCut(1)`:

```c
#ifdef COOP_SPLIT
        coopUpdateCamera2();
#endif
```

- [ ] **Step 8: `ps2_NaSystem.c`.**
  - Tras los includes, añadir `#ifdef COOP_SPLIT` / `#include "../../../ps2/veronica/prog/coopcam.h"` / `#endif`.
  - **G31.** En `Ps2SwapDBuff`, entre `EorFunc();` y `while (Ps2_vcount < 2)`:

```c
#ifdef COOP_SPLIT
    coopFrameStat(Ps2_vcount);
#endif
```

- [ ] **Step 9: Compilar y probar.**
  - `rm -rf build/src`, `.venv/Scripts/python.exe compile.py`.
  - `check_build.py --sym coopFrameStat --sym coopUpdateCamera2 --sym coopSplitDraw --sym coopP2Active --sym coop_fstat` → PASA, sin avisos nuevos.
  - `git diff src/ps2/veronica/prog/game.c src/ps2/veronica/prog/ps2_NaSystem.c`: solo inserciones dentro de `#ifdef COOP_SPLIT`.
  - PCSX2 (si está libre), con el flujo de la tarea 0, paso 7:
    1. Tras cargar, esperar 10 s; `SaveState` → `splitread.py`: `frames … ` con `[0] > 0` y `coop_split = 1` (P2 visible).
    2. `Press p1l1,p1l2,p1r1,p1r2 1500 500` (pulsación larga); `SaveState` → `opción itm[275] = 1` y `coop_split = 0`. **Una sola alternancia** aunque se mantengan 1,5 s (Review Focus 3).
    3. Repetir la pulsación → `itm[275] = 0`.
    4. Abrir el inventario (`Press p1start`), pulsar la combinación, cerrar (`Press p1start`) → `itm[275]` no cambia.

  Apuntar los resultados en `W/progress.md`.

- [ ] **Step 10: Identidad.** `bash $W/identity2.sh` → `IDENTICOS` (con `COOP` y sin `COOP_SPLIT`, igual que `coop_before.elf`), y el build final con `COOP_SPLIT` compila.

---

### Task 2: Prueba de franjas y de `XYOFFSET` (las dos con la cámara de P1)

Objetivo: comprobar el mecanismo de dibujo antes de añadir la cámara de P2. Las dos franjas muestran el plano de P1: la de arriba se centra en P1 y la de abajo en P2.

**Files:** `src/ps2/veronica/prog/coopcam.c`

**Interfaces:**

- Consumes: lo de la tarea 1; `Db` y `Ps2_dbuff` (ps2_dummy.h), `Ps2_gs_save`, `SyncPath`/`D2_SyncTag`/`loadImage` (ps2_loadtim2.h), `Ps2DrawOTag`/`Ps2ClearOT`, `njUserClipping`, `njSetScreen`, `_nj_screen_`, `fNaViwOffsetY`, `njProjectScreen`, `njCalcPoint`.
- Produces:
  - `#define COOP_SPLIT_XYOFF` (1 o 0, lo decide el paso 5);
  - `static void coopGsSet(u_long data, u_long reg)`;
  - `static void coopFlush(void)`, `static void coopStripBegin(int s, float k)`, `static void coopStripEnd(void)`;
  - `static float coopBand(int s, BH_PWORK* pw, int fp)`;
  - las statics `coop_band[2]`, `coop_band_ok[2]`, `coop_band_cut[2]`, `coop_xy_base` y `coop_xy_dbg[2]`.

- [ ] **Step 1: Prueba que falla.** `check_build.py --sym coop_xy_dbg --sym coop_band` → FALLA.

- [ ] **Step 2: Includes y estado** (coopcam.c, tras los includes de la tarea 1):

```c
#include "../../../ps2/veronica/prog/cut.h"
#include "../../../ps2/veronica/prog/game.h"
#include "../../../ps2/veronica/prog/ps2_dummy.h"
#include "../../../ps2/veronica/prog/ps2_loadtim2.h"
#include "../../../ps2/veronica/prog/ps2_NaMatrix.h"
#include "../../../ps2/veronica/prog/ps2_NaView.h"

/* 1: la imagen de cada franja se desplaza con XYOFFSET_1 (3D y 2D); 0: con el centro de njSetScreen (solo 3D). */
#define COOP_SPLIT_XYOFF 1

static float coop_band[2];
static int coop_band_ok[2];
static int coop_band_cut[2];
static u_long coop_xy_base;
static u_long coop_xy_dbg[2];
```

  En `coopCamRoomStart`, añadir `coop_band_ok[0] = 0;` y `coop_band_ok[1] = 0;`.

- [ ] **Step 3: Funciones de GS y de franja** (antes de `coopSplitDraw`):

```c
/* Escribe un registro del GS al momento, como Ps2SetFogColor (ps2_dummy.c). */
static void coopGsSet(u_long data, u_long reg)
{
    D2_SyncTag();

    ((u_long*)WORKBASE)[0] = DMAend | 0x2;
    ((u_long*)WORKBASE)[1] = 0;

    ((u_long*)WORKBASE)[2] = SCE_GIF_SET_TAG(1, SCE_GS_TRUE, SCE_GS_FALSE, 0, SCE_GIF_PACKED, 1);
    ((u_long*)WORKBASE)[3] = SCE_GIF_PACKED_AD;

    ((u_long*)WORKBASE)[4] = data;
    ((u_long*)WORKBASE)[5] = reg;

    loadImage((void*)0xF0000000);

    D2_SyncTag();
}

/* Envía lo dibujado hasta ahora, como el inventario antes de cambiar el recorte (sub1.c:3540-3580). */
static void coopFlush(void)
{
    SyncPath();

    Ps2DrawOTag();
    Ps2ClearOT();

    SyncPath();
}

/* XYOFFSET del entorno de dibujo (el mismo en los dos búferes: lo comprueba la tarea 2). */
static u_long coopXyBase(void)
{
    coop_xy_dbg[0] = *(u_long*)&Db.draw01.xyoffset1;
    coop_xy_dbg[1] = *(u_long*)&Db.draw11.xyoffset1;

    return coop_xy_dbg[0];
}

/* Franja s (0 arriba, 1 abajo): recorte en sus 240 filas y la fila k del plano de 640x480 en su borde superior. */
static void coopStripBegin(int s, float k)
{
#if COOP_SPLIT_XYOFF
    int ofy;
#else
    NJS_SCREEN scr;
#endif
    u_long sc;

    coopFlush();

    sc = SCE_GS_SET_SCISSOR(0, 639, s * 240, (s * 240) + 239);

    Ps2_gs_save.SCISSOR = sc;

    coopGsSet(sc, SCE_GS_SCISSOR_1);

#if COOP_SPLIT_XYOFF
    ofy = (int)((coop_xy_base >> 32) & 0xFFFF) + (int)((k - (s * 240)) * 16.0f);

    coopGsSet(SCE_GS_SET_XYOFFSET(coop_xy_base & 0xFFFF, ofy & 0xFFFF), SCE_GS_XYOFFSET_1);
#else
    scr.dist = _nj_screen_.dist;
    scr.w = 640.0f;
    scr.h = 480.0f;
    scr.cx = 320.0f;
    scr.cy = (240.0f - k) + (s * 240);

    njSetScreen(&scr);
#endif
}

static void coopStripEnd(void)
{
#if !COOP_SPLIT_XYOFF
    NJS_SCREEN scr;
#endif

    coopFlush();

#if COOP_SPLIT_XYOFF
    coopGsSet(coop_xy_base, SCE_GS_XYOFFSET_1);
#else
    scr.dist = _nj_screen_.dist;
    scr.w = 640.0f;
    scr.h = 480.0f;
    scr.cx = 320.0f;
    scr.cy = 240.0f;

    njSetScreen(&scr);
#endif

    njUserClipping(0, NULL);
}

/* Fila superior k (0-240) de la ventana de la franja s: sigue el punto que sigue la cámara (cam.ply = gpy + ci->h).
   Se llama con la cámara de la franja ya construida (cam.mtx) y la pantalla centrada. */
static float coopBand(int s, BH_PWORK* pw, int fp)
{
    CAM_WRK* ci;
    NJS_POINT3 pos;
    NJS_POINT3 p3;
    NJS_POINT2 p2;
    float t;

    if (fp != 0)
    {
        t = 120.0f;
    }
    else
    {
        ci = &rom->cutp[cam.ncut].cam[cam.camver];

        pos.x = pw->gpx;
        pos.y = pw->gpy + ci->h;
        pos.z = pw->gpz;

        njCalcPoint(cam.mtx, &pos, &p3);

        if (p3.z > -1.0f)
        {
            t = 120.0f;
        }
        else
        {
            njProjectScreen(cam.mtx, &pos, &p2);

            t = ((p2.y - fNaViwOffsetY) + 240.0f) - 120.0f;
        }

        if (t < 0)
        {
            t = 0;
        }

        if (t > 240.0f)
        {
            t = 240.0f;
        }
    }

    if ((coop_band_ok[s] == 0) || (coop_band_cut[s] != cam.ncut))
    {
        coop_band[s] = t;

        coop_band_ok[s] = 1;
        coop_band_cut[s] = cam.ncut;
    }
    else
    {
        coop_band[s] += 0.25f * (t - coop_band[s]);
    }

    return coop_band[s];
}
```

  Si al compilar falta algún prototipo (`njUserClipping` está en `ps2_NaSystem.h`, `WORKBASE`/`DMAend` en `types.h`), incluir el header que lo declare (`grep -l "\bNOMBRE\s*(" include/ps2/veronica/prog/*.h`).

- [ ] **Step 4: `coopSplitDraw` de prueba** (sustituye al de la tarea 1):

```c
/* PRUEBA de la tarea 2: las dos franjas con la cámara de P1; arriba centrada en P1, abajo en P2. */
int coopSplitDraw(void)
{
    float k;

    if (coop_split == 0)
    {
        coop_band_ok[0] = 0;
        coop_band_ok[1] = 0;
        return 0;
    }

    coop_xy_base = coopXyBase();

    k = coopBand(0, &ply, 0);
    coopStripBegin(0, k);
    bhAllDrawModel();
    coopStripEnd();

    k = coopBand(1, &ply2, 0);
    coopStripBegin(1, k);
    bhAllDrawModel();
    coopStripEnd();

    coopDrawRect(0.0f, 239.0f, 640.0f, 241.0f, 0xFF000000);

    return 1;
}
```

- [ ] **Step 5: Probar `XYOFFSET` en PCSX2.**
  1. Compilar y pasar `check_build.py --sym coop_xy_dbg --sym coop_band`.
  2. ISO, cargar la partida y `Snap t2_xy`.
  3. Mover P2 lejos de P1 (`KeyDown p2up`, 2 s, `KeyUp p2up`), `Snap t2_xy2` y `SaveState` → `splitread.py`.

  Pasa si:
  - `XYOFFSET draw01` y `draw11` coinciden (OFX 1728.0, OFY 1808.0);
  - en las capturas hay dos franjas con imagen limpia, sin basura ni parpadeo, separadas por la línea negra, y la de abajo está desplazada respecto a la de arriba cuando P2 está lejos.

  **Si falla** (imagen rota, franja vacía o desplazamiento ignorado): cambiar `#define COOP_SPLIT_XYOFF 0`, recompilar, repetir las capturas (`t2_scr`, `t2_scr2`) y apuntar en `W/progress.md` que la mira 2D no se desplazará. **Si `draw01 != draw11`**: apuntarlo y usar en `coopXyBase` el del búfer en el que se dibuja. Para saber cuál es, probar devolviendo `coop_xy_dbg[1]`: la imagen correcta lo decide.

- [ ] **Step 6: Medida de la prueba.** Tras 30 s quieto en la sala con dos franjas, `SaveState` → apuntar `frames …` y `peor`. Repetir con una cámara (combinación). Las dos medidas van a `W/progress.md`.

---

### Task 3: Fase 1 completa: cámara de P2, estado por franja y primera persona de P1

**Files:** `src/ps2/veronica/prog/coopcam.c`, `src/ps2/veronica/prog/coop.c`

**Interfaces:**

- Consumes: lo de las tareas 1 y 2; `bhCheckCut`, `bhSetHideObjLgt` (cut.h); `bhChangeViewClipRM`, `bhChangeClipVolumeRM`, `bhChangeBackColor` (event.h, ps2_event.h); `bhControlLight`, `bhSetLight` (light.h); `bhControlCamera` (camera.h); `bhDrawScope`, `bhDrawThermometer`, `bhDrawScreenFade`, `bhDrawSmallScreenRenderTexture` (screen.h); `bhEtcMirrorDrawModel` (game.h); `njMirror`, `njSetMatrix` (ps2_NaMatrix.h); `njClipZ` (ps2_NaView.h); las funciones de niebla (ps2_NaFog.h); `njMemCopy` (ps2_NaMem.h); `pl_sleep_cnt` (system.h).
- Produces:
  - `CAM_WORK coop_cam2`, `coop_cam2_ok`, `coop_pass` y `coop_show_p2`;
  - `static void coopApplyCamState(int fp, BH_PWORK* who, int back)`, `static void coopLightSave(void)`, `static void coopLightRestore(void)`, `static void coopFogApply(CAM_WORK* p1)`, `static void coopFogRestore(void)` y `static void coopMirror(void)`;
  - `coopUpdateCamera2` y `coopSplitDraw` definitivas de la fase 1;
  - `COOP_TEST_SCOPE` en coop.c.

- [ ] **Step 1: Prueba que falla.** `check_build.py --sym coop_cam2 --sym coop_cam2_ok` → FALLA.

- [ ] **Step 2: Includes y estado nuevos** (coopcam.c):

```c
#include "../../../ps2/veronica/prog/camera.h"
#include "../../../ps2/veronica/prog/event.h"
#include "../../../ps2/veronica/prog/ps2_event.h"
#include "../../../ps2/veronica/prog/light.h"
#include "../../../ps2/veronica/prog/screen.h"
#include "../../../ps2/veronica/prog/system.h"
#include "../../../ps2/veronica/prog/ps2_NaFog.h"
#include "../../../ps2/veronica/prog/ps2_NaMem.h"

static CAM_WORK coop_cam2;
static int coop_cam2_ok;
static int coop_pass = -1;
static int coop_show_p2 = 1;
static CAM_WORK coop_cambk;
static LGT_WORK coop_lgt_bak[COOP_LGT_MAX];
static unsigned char coop_lsys_bak[0xA0];
static unsigned int coop_lst_bak;
static unsigned char coop_sleep_bak;
static float coop_fog_tbl[128];
static float coop_fog_rom[128];
static float coop_fog_f[3];
static float* coop_fog_top;
static unsigned int coop_fog_col;
static int coop_fog_on;

/* Estado de niebla de Ninja (ps2_NaFog.c): njGenerateFogTable3 escribe la tabla, la densidad y near/far. */
extern float fNaFogNear;
extern float fNaFogFar;
extern float fNaFogDensity;
extern float fNaFogTbl[128];
extern float* fpNaFogTblTop;
extern unsigned int ulNaFogA;
extern unsigned int ulNaFogR;
extern unsigned int ulNaFogG;
extern unsigned int ulNaFogB;
extern unsigned int ulNaFogState;
```

  En `coopCamRoomStart`, añadir `coop_cam2_ok = 0;`. En `coopCamToggle`, tras cambiar la opción, añadir `coop_cam2_ok = 0;`, `coop_band_ok[0] = 0;` y `coop_band_ok[1] = 0;`.

- [ ] **Step 3: Estado de dibujo del plano:**

```c
/* Mallas, luces y recorte de vista del plano de la cam actual. fp: vista en primera persona de who (como bhInitPlEyeCamera).
   back: también el color de fondo (solo para P1: la franja de P2 usa el de P1). */
static void coopApplyCamState(int fp, BH_PWORK* who, int back)
{
    NJS_CNK_OBJECT* obj;
    LGT_WORK* lp;
    int i;

    if (fp != 0)
    {
        for (i = 0; i < (int)rom->mdl.obj_num; i++)
        {
            obj = &rom->mdl.objP[i];

            if (obj->model != NULL)
            {
                obj->evalflags &= ~0x8;
            }
        }

        lp = &rom->lgtp[4];

        for (i = 4; i < rom->lgt_n; i++, lp++)
        {
            lp->flg &= ~0x2;
        }

        if ((who->wpnr_no == 10) || (who->wpnr_no == 19))
        {
            njClipZ(-1.1f, -20000.0f);
        }
        else
        {
            njClipZ(-1.0f, -20000.0f);
        }

        return;
    }

    bhSetHideObjLgt(cam.ncut);

    /* Como bhSetCut: solo con el inventario cerrado. */
    if ((sys->ts_flg & 0x200))
    {
        bhChangeViewClipRM();
        bhChangeClipVolumeRM();

        if (back != 0)
        {
            bhChangeBackColor();
        }
    }
}
```

- [ ] **Step 4: Luces y niebla:**

```c
/* bhControlLight anima las luces en cada llamada: la pasada de P2 se hace sobre una copia que después se descarta. */
static void coopLightSave(void)
{
    njMemCopy(coop_lgt_bak, rom->lgtp, rom->lgt_n * sizeof(LGT_WORK));
    njMemCopy(coop_lsys_bak, &sys->lgtp, (unsigned char*)&sys->mes_idx - (unsigned char*)&sys->lgtp);

    coop_lst_bak = sys->st_flg;
    coop_sleep_bak = pl_sleep_cnt;
}

static void coopLightRestore(void)
{
    njMemCopy(rom->lgtp, coop_lgt_bak, rom->lgt_n * sizeof(LGT_WORK));
    njMemCopy(&sys->lgtp, coop_lsys_bak, (unsigned char*)&sys->mes_idx - (unsigned char*)&sys->lgtp);

    sys->st_flg = (sys->st_flg & ~0x1000000) | (coop_lst_bak & 0x1000000);
    pl_sleep_cnt = coop_sleep_bak;
}

/* Niebla del plano de la cam actual (la de P2), si la sala tiene niebla y la de P1 (p1) es distinta. */
static void coopFogApply(CAM_WORK* p1)
{
    unsigned int col;

    coop_fog_on = 0;

    if ((!(sys->st_flg & 0x2)) || (ulNaFogState == 0))
    {
        return;
    }

    if ((cam.fog_nr == p1->fog_nr) && (cam.fog_fr == p1->fog_fr) && (cam.fog_col == p1->fog_col))
    {
        return;
    }

    coop_fog_on = 1;

    njMemCopy(coop_fog_tbl, fNaFogTbl, sizeof(coop_fog_tbl));
    njMemCopy(coop_fog_rom, rom->fog, sizeof(coop_fog_rom));

    coop_fog_f[0] = fNaFogNear;
    coop_fog_f[1] = fNaFogFar;
    coop_fog_f[2] = fNaFogDensity;

    coop_fog_top = fpNaFogTblTop;

    coop_fog_col = (ulNaFogA << 24) | (ulNaFogR << 16) | (ulNaFogG << 8) | ulNaFogB;

    /* Como sync.c al refrescar la niebla de un plano. */
    if (!(sys->st_flg & 0x100000))
    {
        col = cam.fog_col;
    }
    else
    {
        col = sys->fog_col & 0xFFFFFF;
    }

    njSetFogColor(col);
    njGenerateFogTable3(rom->fog, cam.fog_nr, cam.fog_fr);
    njSetFogTable(rom->fog);
}

static void coopFogRestore(void)
{
    if (coop_fog_on == 0)
    {
        return;
    }

    njMemCopy(fNaFogTbl, coop_fog_tbl, sizeof(coop_fog_tbl));
    njMemCopy(rom->fog, coop_fog_rom, sizeof(coop_fog_rom));

    fNaFogNear = coop_fog_f[0];
    fNaFogFar = coop_fog_f[1];
    fNaFogDensity = coop_fog_f[2];

    fpNaFogTblTop = coop_fog_top;

    njSetFogColor(coop_fog_col);

    coop_fog_on = 0;
}

/* La pasada de espejo del bloque original (game.c), para la franja actual. */
static void coopMirror(void)
{
    if ((sys->st_flg & 0x100))
    {
        sys->gm_flg |= 0x4000;

        njSetMatrix(cam.mtxb, cam.mtx);

        njMirror(cam.mtx, &sys->mr_pl);

        bhSetLight();

        bhEtcMirrorDrawModel();

        njSetMatrix(cam.mtx, cam.mtxb);

        sys->gm_flg &= ~0x4000;
    }
}
```

- [ ] **Step 5: `coopUpdateCamera2` definitiva de la fase 1** (sustituye a la de la tarea 1):

```c
/* G29 (bhMainSequence, tras bhCheckCut): la cámara de P2 con el código original, en un cambio de contexto. */
void coopUpdateCamera2(void)
{
    BH_PWORK* plp_bak;
    unsigned int gm;
    unsigned int st;
    unsigned int pt;
    unsigned int ef;
    int fog_ct;
    short fil_no;
    short fil_rt;
    unsigned int rfog_col;
    float rfog_nr;
    float rfog_fr;
    int p1fp;

    coopCamToggle();

    coop_split = coopSplitWanted();

    if (coop_split == 0)
    {
        coop_cam2_ok = 0;

        coop_band_ok[0] = 0;
        coop_band_ok[1] = 0;
        return;
    }

    p1fp = (sys->gm_flg & 0x40) ? 1 : 0;

    coop_cambk = cam;
    plp_bak = plp;

    gm = sys->gm_flg;
    st = sys->st_flg;
    pt = sys->pt_flg;
    ef = sys->ef_flg;

    fog_ct = sys->fog_ct;
    fil_no = sys->fil_no;
    fil_rt = sys->fil_rt;

    rfog_col = rom->fog_col;
    rfog_nr = rom->fog_nr;
    rfog_fr = rom->fog_fr;

    if (coop_cam2_ok == 0)
    {
        coop_cam2 = cam;
    }

    cam = coop_cam2;
    plp = &ply2;

    /* Primera persona y plano fijo son de P1. */
    sys->gm_flg &= ~0x30C0;

    bhCheckCut((coop_cam2_ok == 0) ? 1 : 0);

    coop_cam2_ok = 1;

    coop_cam2 = cam;

    plp = plp_bak;
    cam = coop_cambk;

    sys->gm_flg = gm;
    sys->st_flg = st;
    sys->pt_flg = pt;
    sys->ef_flg = ef;

    sys->fog_ct = fog_ct;
    sys->fil_no = fil_no;
    sys->fil_rt = fil_rt;

    rom->fog_col = rfog_col;
    rom->fog_nr = rfog_nr;
    rom->fog_fr = rfog_fr;

    coopApplyCamState(p1fp, &ply, 1);
}
```

- [ ] **Step 6: `coopSplitDraw` y `coopSplitShowP2` definitivas de la fase 1** (sustituyen a las anteriores):

```c
/* G30: las dos franjas. 1 si ha dibujado (el bloque original no se ejecuta). */
int coopSplitDraw(void)
{
    unsigned int pt;
    unsigned int gm;
    int p1fp;
    float k;

    if ((coop_split == 0) || (coop_cam2_ok == 0))
    {
        return 0;
    }

    p1fp = (sys->gm_flg & 0x40) ? 1 : 0;

    pt = sys->pt_flg;
    gm = sys->gm_flg;

    coop_xy_base = coopXyBase();

    if ((sys->gm_flg & 0x200))
    {
        bhDrawSmallScreenRenderTexture();
    }

    /* P2 se ve si el juego dibuja al jugador, o si no lo dibuja solo porque P1 está en primera persona. */
    coop_show_p2 = ((pt & 0x1) || (p1fp != 0)) ? 1 : 0;

    /* Franja de P1: su cámara y su estado ya están puestos. */
    coop_pass = 0;

    k = coopBand(0, &ply, p1fp);

    coopStripBegin(0, k);

    bhAllDrawModel();

    coopMirror();

    if ((sys->st_flg & 0x800000))
    {
        bhDrawScope();
    }

    coopStripEnd();

    /* Franja de P2. */
    coop_cambk = cam;

    coopLightSave();

    cam = coop_cam2;

    sys->gm_flg &= ~0xC0;
    sys->pt_flg = (pt & ~0x1) | coop_show_p2;

    coop_pass = 1;

    bhControlCamera();

    coopApplyCamState(0, &ply2, 0);

    bhControlLight();
    bhSetLight();

    coopFogApply(&coop_cambk);

    k = coopBand(1, &ply2, 0);

    coopStripBegin(1, k);

    bhAllDrawModel();

    coopMirror();

    coopStripEnd();

    /* Vuelta al estado de P1. */
    coop_pass = -1;

    cam = coop_cambk;

    sys->gm_flg = gm;
    sys->pt_flg = pt;

    coopFogRestore();
    coopLightRestore();

    bhControlCamera();

    coopApplyCamState(p1fp, &ply, 1);

    bhSetLight();

    coopDrawRect(0.0f, 239.0f, 640.0f, 241.0f, 0xFF000000);

    if ((sys->st_flg & 0x40000000))
    {
        bhDrawThermometer();
    }

    if (sys->fade_an > 0)
    {
        bhDrawScreenFade();
    }

    return 1;
}

int coopSplitShowP2(void)
{
    if (coop_pass < 0)
    {
        return (sys->pt_flg & 0x1) ? 1 : 0;
    }

    return coop_show_p2;
}
```

- [ ] **Step 7: Visibilidad de P2 por franja (coop.c, `coopDrawPlayer2`).** La primera condición queda:

```c
#ifdef COOP_SPLIT
    if ((coopSplitShowP2() == 0) || (ply2.stflg & 0x1000000) || (ply2.mdflg & 0x1))
#else
    if ((!(sys->pt_flg & 0x1)) || (ply2.stflg & 0x1000000) || (ply2.mdflg & 0x1))
#endif
```

- [ ] **Step 8: `COOP_TEST_SCOPE` (coop.c).** Tras el `#endif` que cierra `coopTestGiveHandgun`:

```c
#ifdef COOP_TEST_SCOPE
/* Solo para pruebas: lanzador lineal (id 11) equipado en el bloque de P2 y otro, sin equipar, en el de P1. */
static void coopTestGiveScope(void)
{
    unsigned int* pip;
    int i;

    pip = &sys->itm[COOP_ITM];

    for (i = 2; i < 10; i++)
    {
        if ((((pip[i] >> 16) & 0xFF) == 11) || (pip[i] == 0))
        {
            pip[i] = 0x080B0001;
            pip[0] = i;
            break;
        }
    }

    pip = &sys->itm[sys->ply_id * 16];

    for (i = 2; i < 10; i++)
    {
        if (((pip[i] >> 16) & 0xFF) == 11)
        {
            break;
        }

        if (pip[i] == 0)
        {
            pip[i] = 0x080B0001;
            break;
        }
    }
}
#endif
```

  En `coopLoadPlayer2`, justo antes del `#ifdef COOP_TEST` que llama a `coopTestGiveHandgun()`:

```c
#ifdef COOP_TEST_SCOPE
            coopTestGiveScope();
#endif
```

  (`COOP_TEST_SCOPE` se activa con `#define COOP_TEST_SCOPE` tras el `#ifdef COOP` de coop.c y se quita antes del build final.)

- [ ] **Step 9: Compilar y probar en PCSX2.**
  - `check_build.py --sym coop_cam2 --sym coop_cam2_ok --sym coopSplitShowP2` → PASA, sin avisos nuevos.
  - Pruebas:
    1. Cargar, `Snap t3_a` (las dos franjas; P1 arriba y P2 abajo, cada uno visible en las dos si cabe en el plano).
    2. Mover P2 (`KeyDown p2up/p2left…` 1-3 s) hasta que `splitread.py` dé `plano P1 / P2` distintos; `Snap t3_b`. La franja de abajo muestra otro plano.
    3. **Review Focus 1:** `Snap t3_c` con P2 vuelto al plano de P1 y comparar la franja de arriba de `t3_a`, `t3_b` y `t3_c`. No deben cambiar mallas, luces ni niebla de la franja de P1 por el plano de P2.
    4. Combinación → una cámara, `Snap t3_d`. Debe ser idéntica al juego sin el hito, sin restos de recorte ni desplazamiento.

    Si `cut_n` de la sala es 1, los pasos 2-3 pasan a la checklist del usuario. Se prueba solo el seguimiento vertical moviendo a P2 hacia la cámara y alejándolo.
  - **Review Focus 4:** revisar en el código de `bhAllDrawModel`, `bhDrawEffect`, `bhDrawEnemy` y `bhDrawObjItm` (solo leer) qué estado global cambian al dibujar (contadores, `--`/`++` sobre `sys->` o globales). Apuntar en `W/progress.md` lo encontrado; lo que no sean luces ni `pl_sleep_cnt` va a la checklist del usuario como «efecto X al doble de velocidad con dos cámaras».

- [ ] **Step 10: Cambio de sala y reintento (Review Focus 2).** Con dos franjas:
  - Si el arnés llega a una puerta de la sala 0-1 (P1 hacia la puerta y X), atravesarla, esperar 5 s y `SaveState`. `splitread.py`: `coop_cam2_ok = 1`, el `ncut` de P2 es menor que `rom->cut_n`, y en `Snap t3_e` se ven las dos franjas en la sala nueva.
  - Si no se llega a una puerta, revisar en el código que `coopCamRoomStart` (que llama `coopRoomStart`) pone `coop_cam2_ok = 0` y que `coopSplitDraw` no dibuja con `coop_cam2_ok == 0`. Pasa a la checklist del usuario.

- [ ] **Step 11: Partida → completa (Review Focus 5).** Con dos franjas, abrir el mapa o el inventario y cerrarlo, y examinar algo con P1 (la máquina de escribir: acción con X) para que salga el mensaje; `Snap t3_f` con el mensaje (sigue partida) y `Snap t3_g` tras cerrarlo. Después, una cámara con la combinación y `Snap t3_h`: imagen completa correcta.

- [ ] **Step 12: Primera persona de P1** (con `COOP_TEST_SCOPE`). Compilar con el define, ISO de prueba (`mkiso_test.py`) y cargar.
  - Equipar el lanzador lineal a P1 desde su inventario: Start, mover el cursor a la casilla del lanzador, X, «equipar», X, cerrar. Si el arnés no llega, pasa a la checklist del usuario.
  - Apuntar con P1 (`KeyDown p1r1`) y `Snap t3_fp`. La vista en primera persona y la mira van solo en la franja de arriba; abajo se ve a P1 en tercera persona apuntando.
  - Soltar y `Snap t3_fp2`: la franja de P1 vuelve a su plano.
  - Quitar `COOP_TEST_SCOPE`.

- [ ] **Step 13: Medida** (como la tarea 2, paso 6) con la cámara de P2 real: 30 s quieto, `SaveState`, y lo mismo con una cámara. Apuntar las dos en `W/progress.md`.

- [ ] **Step 14: Identidad.** `bash $W/identity2.sh` → `IDENTICOS`.

---

### Task 4: Fase 2: primera persona de P2

**Files:** `src/ps2/veronica/prog/coopcam.c`, `include/ps2/veronica/prog/coopcam.h`, `src/ps2/veronica/prog/coop.c`, `include/ps2/veronica/prog/coop.h`

**Interfaces:**

- Consumes: lo de la tarea 3; `bhInitPlEyeCamera`, `bhSetPlEyeCamera` (cut.h); `coopBegin`/`coopEnd` (coop.c); `coop_wpn[0]`, `coop_wpn_ok[0]` (static en coop.c).
- Produces:
  - `void coopCamP2Begin(void)` y `void coopCamP2End(void)` (G32, coopcam.c);
  - `void coopScopeSwap(void)` (coop.c);
  - `#define COOP_PE_GM 0x828C0`;
  - las statics `coop_pe2_gm`, `coop_pe2_st` y `coop_pe2_camin`.

- [ ] **Step 1: Prueba que falla.** `check_build.py --sym coopCamP2Begin --sym coopScopeSwap` → FALLA.

- [ ] **Step 2: coopcam.h.** Añadir `void coopCamP2Begin(void);` y `void coopCamP2End(void);`.

- [ ] **Step 3: coop.h y coop.c: `coopScopeSwap`.** En coop.h, dentro del `#ifdef COOP_SPLIT`, añadir `void coopScopeSwap(void);`. En coop.c, justo después de `coopSwapWeaponObj`:

```c
#ifdef COOP_SPLIT
/* bhDrawScope toma la textura y el tipo de sys->obwp[0]: durante la mira de P2, los de su arma. Se llama dos veces (poner y quitar). */
void coopScopeSwap(void)
{
    ML_WORK* w;
    unsigned short t;

    if (coop_wpn_ok[0] == 0)
    {
        return;
    }

    w = sys->obwp->mlwP;
    sys->obwp->mlwP = coop_wpn[0].mlwP;
    coop_wpn[0].mlwP = w;

    t = sys->obwp->type;
    sys->obwp->type = coop_wpn[0].type;
    coop_wpn[0].type = t;
}
#endif
```

- [ ] **Step 4: G32 (coop.c).**
  - En `coopBegin`, justo después de `sys->gm_flg = (sys->gm_flg & ~0x10040000) | coop_gm2;`:

```c
#ifdef COOP_SPLIT
    coopCamP2Begin();
#endif
```

  - En `coopEnd`, justo antes de `sys->st_flg = coop_save.st_flg;`:

```c
#ifdef COOP_SPLIT
    coopCamP2End();
#endif
```

- [ ] **Step 5: Estado y G32 (coopcam.c).** El `#define` y las tres statics van **al principio del archivo**, con las demás (las usan `coopCamToggle` y `coopUpdateCamera2`). Las dos funciones van al final, antes del `#endif`. También va arriba `static int coop_p2fp;` (paso 7).

```c
/* Bits de primera persona de P2: gm_flg 0x40/0x80/0x800/0x2000/0x80000 (st_flg 0x800000 aparte). */
#define COOP_PE_GM 0x828C0

static unsigned int coop_pe2_gm;
static unsigned int coop_pe2_st;
static int coop_pe2_camin;

/* G32, al final de coopBegin: P2 ve su modo de vista; en primera persona, también su cámara de ojos. */
void coopCamP2Begin(void)
{
    sys->gm_flg = (sys->gm_flg & ~COOP_PE_GM) | coop_pe2_gm;
    sys->st_flg = (sys->st_flg & ~0x800000) | coop_pe2_st;

    if ((coop_pe2_gm & 0x40))
    {
        cam = coop_cam2;

        coop_pe2_camin = 1;
    }
    else
    {
        cam.pe_ax = coop_cam2.pe_ax;
        cam.pe_pers = coop_cam2.pe_pers;

        coop_pe2_camin = 0;
    }
}

/* G32, al principio de coopEnd (antes de que restaure los flags y cam de P1). */
void coopCamP2End(void)
{
    coop_pe2_gm = sys->gm_flg & COOP_PE_GM;
    coop_pe2_st = sys->st_flg & 0x800000;

    if (coop_pe2_camin != 0)
    {
        coop_cam2 = cam;
    }
    else
    {
        coop_cam2.pe_ax = cam.pe_ax;
        coop_cam2.pe_pers = cam.pe_pers;
    }

    coop_pe2_camin = 0;

    /* Con una cámara no hay franja para P2: apunta en tercera persona. */
    if (sys->itm[COOP_CAM_OPT] != 0)
    {
        coop_pe2_gm &= ~0x2000;
    }
}
```

  En `coopCamRoomStart`, añadir `coop_pe2_gm = 0;` y `coop_pe2_st = 0;`. En `coopCamToggle`, al pasar a una cámara (valor nuevo 1), añadir:

```c
    if (sys->itm[COOP_CAM_OPT] != 0)
    {
        coop_pe2_gm = (coop_pe2_gm & ~0x20C0) | 0x800;
        coop_pe2_st = 0;
    }
```

- [ ] **Step 6: `coopUpdateCamera2` (fase 2).** Sustituir la línea `sys->gm_flg &= ~0x30C0;` y la llamada a `bhCheckCut` por:

```c
    /* El modo de vista de P2 (no el de P1), sin plano fijo. Mismo orden que bhMainSequence (game.c). */
    sys->gm_flg = (sys->gm_flg & ~(COOP_PE_GM | 0x1000)) | coop_pe2_gm;
    sys->st_flg = (sys->st_flg & ~0x800000) | coop_pe2_st;

    if ((sys->gm_flg & 0x2000))
    {
        bhInitPlEyeCamera();
    }

    bhCheckCut(((coop_cam2_ok == 0) || (sys->gm_flg & 0x800)) ? 1 : 0);

    if ((sys->gm_flg & 0x40) && (sys->sp_flg & 0x40))
    {
        bhSetPlEyeCamera();
    }

    coop_pe2_gm = sys->gm_flg & COOP_PE_GM;
    coop_pe2_st = sys->st_flg & 0x800000;
```

  En la rama `if (coop_split == 0)` del principio, añadir (P2 no puede quedarse en primera persona sin franja):

```c
        coop_pe2_gm = (coop_pe2_gm & ~0x20C0) | ((coop_pe2_gm & 0x40) ? 0x800 : 0);
        coop_pe2_st = 0;
```

- [ ] **Step 7: Franja de P2 en primera persona (`coopSplitDraw`).** Declarar `int p2fp;` con las demás variables. Antes de `coop_show_p2 = …`, poner `p2fp = (coop_pe2_gm & 0x40) ? 1 : 0;`. En la pasada de P2:
  - sustituir `sys->gm_flg &= ~0xC0;` por:

```c
    sys->gm_flg &= ~0xC0;

    if (p2fp != 0)
    {
        sys->gm_flg |= 0xC0;
    }
```

  - sustituir `coopApplyCamState(0, &ply2, 0);` por `coopApplyCamState(p2fp, &ply2, 0);`;
  - sustituir `k = coopBand(1, &ply2, 0);` por `k = coopBand(1, &ply2, p2fp);`;
  - entre `coopMirror();` y `coopStripEnd();` de la pasada de P2, añadir:

```c
    if ((p2fp != 0) && (coop_pe2_st & 0x800000))
    {
        coopScopeSwap();
        bhDrawScope();
        coopScopeSwap();
    }
```

  Y que P2 no se vea en su propia franja: con `static int coop_p2fp;` (declarada arriba en el paso 5), poner `coop_p2fp = p2fp;` junto a `coop_show_p2 = …`, y en `coopSplitShowP2`, antes del último `return`:

```c
    if ((coop_pass == 1) && (coop_p2fp != 0))
    {
        return 0;
    }
```

- [ ] **Step 8: Compilar y probar.**
  - `check_build.py --sym coopCamP2Begin --sym coopCamP2End --sym coopScopeSwap` → PASA.
  - Con `COOP_TEST_SCOPE` (P2 empieza con el lanzador lineal equipado), ISO de prueba y cargar:
    1. P2 apunta (`KeyDown p2r1`) → `Snap t4_a`: abajo, la primera persona de P2 con la mira del lanzador; arriba, P2 en tercera persona y P1 normal.
    2. Soltar → `Snap t4_b`: la franja de P2 vuelve a su plano.
    3. Con P2 apuntando, combinación a una cámara → `Snap t4_c`: pantalla completa normal, sin vista en primera persona, y P2 sigue pudiendo moverse y apuntar en tercera persona.
    4. `SaveState` y `splitread.py` en cada caso; `coop_pe2_gm` se puede leer en el xMAP con `ramread`, si hace falta.
  - Quitar `COOP_TEST_SCOPE`.

- [ ] **Step 9: Identidad.** `bash $W/identity2.sh` → `IDENTICOS`.

---

### Task 5: Verificación final y documentación

- [ ] **Step 1: Builds de prueba.** Compilar con `COOP_TEST` y con `COOP_TEST_SCOPE` (que compilen), quitarlos y hacer el build final. `grep -c "coopTestGiveHandgun\|coopTestGiveScope" elf/main.elf.xMAP` → 0.

- [ ] **Step 2: Identidad sin `COOP`.** Copiar `.superpowers/sdd/2026-10-11-coop-hito3/identity.sh` a `W/identity.sh` y cambiar dentro:
  - `W=` por la carpeta de este hito;
  - el `replace` de defines, para que quite también `COOP_SPLIT`: `'"DEBUG",\n    "COOP",\n    "COOP_SPLIT"'` → `'"DEBUG"'` (y la variante con `\r\n`);
  - el `--sym` final, por `coopUpdateCamera2 --sym coopSplitDraw --sym coopControlPlayer2`;
  - `firstdiff.py` por `tools/firstdiff2.py`.

  Ejecutar → `IDENTICOS`, y el build final vuelve a `COOP` + `COOP_SPLIT`.

- [ ] **Step 3: Identidad con `COOP` sin `COOP_SPLIT`.** `bash $W/identity2.sh` → `IDENTICOS`.

- [ ] **Step 4: ISO final.** Con PCSX2 cerrado, `mkiso.py -m insert`, comprobando que la ISO es posterior al ELF.

- [ ] **Step 5: Documentación.**
  - `docs/coop/README.md`:
    - estado del hito 5 (prototipo): qué está probado y qué no;
    - hoja de ruta: marcar «pantalla partida (prototipo)» y añadir al hito 7 «Fila "Cámara: 1 / 2" del menú de opciones»;
    - ganchos G29-G32;
    - decisiones nuevas (dos franjas siempre; cinemáticas a pantalla completa; primera persona en la franja; L1+L2+R1+R2; `itm[275]`);
    - las medidas del medidor.
  - `docs/architecture/world-systems.md`:
    - en «Cámara», la cámara de P2 (`coopcam.c`);
    - en «Dibujo», el resultado de la prueba de `XYOFFSET` (si funciona a mitad de frame y si los dos búferes tienen el mismo);
    - que `bhAllDrawModel` decrementa `pl_sleep_cnt`;
    - que `bhControlLight` anima las luces en cada llamada;
    - que `njGenerateFogTable3` escribe `fNaFogTbl` y no la tabla que recibe.
  - `CLAUDE.md`, sección «Cooperativo (código)»: `coopcam.c`, `COOP_SPLIT`, `COOP_TEST_SCOPE` y los ganchos.
  - `docs/coop/testing.md`: `splitread.py`, las teclas L2/R2 del arnés y la checklist de la pantalla partida para el usuario (lo que no se pudo probar con el arnés: dos mandos reales, planos distintos si la sala 0-1 solo tiene uno, puertas, cinemáticas, P2 muerto, salas con espejo o monitores, efectos al doble de velocidad).
  - `mdcheck.py` → ok.

- [ ] **Step 6: Revisión final** con un revisor nuevo sobre todo el diff del hito (sin el del hito 7, que ya estaba en el árbol).
