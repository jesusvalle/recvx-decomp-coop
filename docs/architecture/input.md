# Entrada: del mando a `sys->pad_*`

Rutas relativas a `src/ps2/veronica/prog/`.

## Resumen

- Todo el juego lee el mando a través de **campos lógicos de `SYS_WORK`**: `pad_on` (pulsado), `pad_ps` (recién pulsado), `pad_rs` (recién soltado), `pad_old`, `pad_ax/pad_ay` (stick), `pad_dx/pad_dy` (acumuladores de dirección), `pad_ar/pad_al` (gatillos).
- **Solo se lee el puerto 0.** El puerto 1 se abre pero nunca se consulta.
- No hay multitap ni código para él.

## Capa PS2 ([ps2_sg_pad.c](../../src/ps2/veronica/prog/ps2_sg_pad.c))

| Función | Línea | Qué hace |
| --- | --- | --- |
| `Pad_init` | 646 | `scePadInit` y `scePadPortOpen` para el puerto 0 **y el 1** (buffers `Padd1`/`Padd2`). La llama `ps2_dummy.c:164`. |
| `Ps2_pad_read` | 194 | **Fijado al puerto 0**: `scePadGetState(0,0)`, máquina de conexión, modo analógico y presión con el estado global `Pad_status`, `scePadRead(0,0,Pad_rdata1)` y `Pad_set(&Ps2_pad.pad1, 1)`. Al final llama a `Ps2_pad_actuater` (vibración). |
| `pdGetPeripheral(port)` | 74 | Devuelve un `PDS_PERIPHERAL` **estático y único** (`pp`). Solo refresca una vez por frame (`Ps2_sys_cnt != Old_sys_cnt`), así que pedir el puerto 1 después del 0 devuelve los datos del 0. Con `port == 1` elige `Pad_rdata2`/`pad2`, que nunca se rellenan. El stick derecho se ignora (`x2 = y2 = 0`). |
| `Ps2_Read_Key` | 379 | Copia el periférico en `Pad[0..3]` (estructura Dreamcast, padman.c) y gestiona `SoftResetFlag`. En el código decompilado `Pad[].SoftReset` nunca se pone a un valor distinto de 0, así que **no hay reset por software** (`bhCheckSoftReset`, sync.c:283, nunca llega a `njUserExit`). Comprobado con grep. |
| `Pad_set` | 524 | Ya admite `pad_num == 2` → `Pad_rdata2` (líneas 531-534). |
| `pdGetPeripheralInfo(port)` | 148 | Ignora el argumento y comprueba el puerto 0. |

Hay piezas preparadas para el puerto 2 que no se usan: `Pad_rdata2[32]`, `Ps2_pad.pad2`, `sys->p2per` (types.h:621, nunca asignado) y un `PAD_STATUS Pad_status2` comentado en ps2_sg_pad.c:32-33.

La vibración también es solo del puerto 0 (`ps2_sg_pdvib.c`, `scePadSetActDirect(0,0,…)`). `StartVibrationEx(atributo, n)` recibe un atributo, no el puerto: las armas llaman con `StartVibrationEx(0, vib_tp)` (pwksub.c:2738, playpch2.c:213) y siempre vibra el puerto 0. Ver [combat.md](combat.md).

## Capa lógica: `bhSetPad` ([pad.c](../../src/ps2/veronica/prog/pad.c):17)

La llama la tarea 6, `bhSysCallPad` (system.c:518), solo si `sys->sp_flg & 0x20` (si no, pone `pad_on = 0`). Durante las cinemáticas aplica la máscara `0x1188F` a `pad_on`/`pad_ps`.

1. `sys->p1per = njGetPeripheral(pd_port)` (pad.c:126). Esto es lo que provoca la lectura real. `pd_port` (main.c:55) vale 0, o -1 si el mando está desconectado (lo decide `bhCheckPadPort`, sync.c:261).
2. `pad_ar/al` = gatillos; `pad_ax/ay` = −stick izquierdo (pad.c:139-143).
3. **Configuración de botones:** `pad_type[sys->keytype]` con las tablas `pad_tab_a..d` (pad.c:8-12) traduce 18 bits lógicos a botones SCE.
4. Convierte el stick en dirección digital, salvo si `gm_flg & 0x80001`.
5. `pad_dx/pad_dy` son acumuladores que suben o bajan de 4 en 4 por frame, así que **guardan estado entre frames**.
6. `pad_ps = pad & ~oncpy`, `pad_rs`, y `pad_on = pad_oncpy = pad` (pad.c:275-278).
7. En modo demo (`ss_flg & 0x400000`, pad.c:50-117) reproduce una entrada grabada (`sys->pdm_pd`).

`sys->pad_port` se escribe en sync.c:113 pero nadie lo lee; el que cuenta es el global `pd_port`.

## Bits lógicos

Deducidos de la tabla de tipo A y de cómo los usa el código.

| Bit | Significado |
| --- | --- |
| `0x1` / `0x2` | Adelante / atrás |
| `0x4` / `0x8` | Girar |
| `0x10` | Apuntar (R1) |
| `0x20` / `0x40` | Apuntar arriba / abajo |
| `0x80` | L1 (cambiar de blanco; también modificador; con la mira telescópica, zoom) |
| `0x100` | Disparar |
| `0x200` | Acción / examinar |
| `0x400` | Correr |
| `0x800` | Aceptar |
| `0x1000` | Cancelar |
| `0x2000` | Mapa |
| `0x4000` | Inventario (Start o R3: máscara física `0xC00` en la tabla A, pad.c:8; deducido) |
| `0x8000` | Opciones |
| `0x10000` | Saltar cinemática |
| `0x20000` | Start (L1+Start = pausa). Start también genera `0x4000` |

**Un botón físico activa varios bits lógicos** en la configuración A (pad.c:8, deducido de la tabla):

- `0x1` y `0x20` comparten el físico `0x1000`; `0x2` y `0x40` comparten `0x4000`.
- `0x100` (disparar), `0x200` (acción) y `0x800` (aceptar) comparten `0xC0`.
- `0x400` (correr) y `0x1000` (cancelar) comparten `0x20`.
- El stick vertical activa `0x21` (arriba, `pad_ay < -63`) y `0x42` (abajo, `pad_ay > 63`) (pad.c:162-170), salvo con `gm_flg & 0x80001`.

## Quién lee la entrada

**El jugador** lee directamente `sys->pad_*`, sin variable intermedia:

| Archivo | Referencias a `sys->pad_*` |
| --- | --- |
| player.c | 66 (`bhControlPlayerPad` :1828 decodifica el movimiento; `bhCPM2_act_atk` :5270 tiene 18) |
| playpch.c | 22 |
| playpch2.c | 20 (mira: `pad_ax/ay/dx/dy`) |
| hitchk.c | 14 (`bhCheckExmAtari`, el botón de acción) |
| pl_evt.c | 1 |

El código del jugador también **escribe** `sys->pad_on &= ~0xF` (player.c:2283 y :5253).

**Otros consumidores** (siempre del jugador 1):

- **Sistema:** `bhCheckSubTask` (máquina de escribir, terminal, inventario, mapa y opciones; system.c:568-640), saltar cinemática (system.c:553) y pausa (system.c:676). La reanudación tras la pausa lee directamente `Ps2_pad.pad1.push`.
- **Inventario, ItemView, FileView y mensajes:** leen solo `sys->pad_*` (sub1.c, itemview.c, fileview.c, message.c); no usan `Pad[]`, `pdGetPeripheral` ni `p1per`. Ver [inventory.md](inventory.md).
- **Eventos y mensajes:** guardan y restauran el mando en `pad_onb/psb/oldb` (se guardan en hitchk.c:4796, :4831; se restauran en game.c:149, :182 y message.c:318); `bhPadCheck` en los scripts (event.c:4766).
- **Pantallas:** inventario (`sub1.c`, 56 refs; ver [inventory.md](inventory.md)), mapa (`MapPadMain`), archivos, items, guardar/cargar, game over, ranking.
- **Puzles:** `effsub1b.c` (`bhEff133/134/137/138`), `objitm.c` (`bhObj007`), `zonzon.c` (`bhEne_LeverCheck`), `sp_evt.c`.
- **Agarres de enemigos:** se ejecutan dentro de `bhControlEnemy`, antes de actualizar al jugador, y hacen `sys->pad_on &= ~0xF`. Están en en01, en01sub, en04, en09, en17, en22 y en26.
- **Lectores directos de `Pad[]`:** `adv.c` (menús de título) y `sdfunc.c:3280,3325` (cancelar vídeo).
- **Al cambiar de sala** se ponen a cero los campos del mando (system.c:238-244).

## Cooperativo (build `COOP`)

- **Puerto 2:**
  - `pdGetPeripheral(1)` devuelve `coopGetPeripheral2()`, en el bloque `#ifdef COOP` al final de ps2_sg_pad.c.
  - Ese bloque sondea el puerto 1 con su propia máquina de conexión (`Pad_status2`, copia de la de `Ps2_pad_read`) y su propia caché por frame.
  - Devuelve `NULL` si no hay mando.
- **Estado lógico de P2:**
  - `coopSetPad2()` (coop.c), llamado al final de `bhSysCallPad`, ejecuta el `bhSetPad()` original con los campos `sys->pad_*` intercambiados y `pd_port = 1`. Así P2 usa la misma configuración de botones y conversión de stick que P1.
  - Al final se enmascara: `0x40F` (moverse, girar, correr) en el hito 1 y `0x5FF` desde el hito 2b (además apuntar, apuntar arriba/abajo, cambiar de blanco y disparar). Con armas de mira se quita `0x10`.
- **Detalles:** en demo, o con `!(sp_flg & 0x20)`, no se lee. Nadie más llama a `pdGetPeripheral(1)`.
- **Pantalla de inventario de P2 (hito 2d):** `coopSetPad2` guarda también el mando de P2 sin máscara (`coop_pad2raw`). Mientras la pantalla es de P2, lo copia a `sys->pad_*` al final de la tarea 6, así que la pantalla (tarea 9) y los mensajes sí/no (tarea 8) se manejan con el mando 2. Al abrir se guarda el estado de P1 y al cerrar se repone: `bhSetPad` calcula `pad_ps`/`pad_rs` comparando con el `pad_oncpy` anterior. El Start de P2 se detecta en `coop_pad2raw.ps & 0x4000` y la acción en `& 0x200`, que no entra en la máscara de juego de P2 (`0x5FF`). **`oncpy` no se enmascara:** es el historial con el que `bhSetPad` calcula `ps = pad & ~oncpy` (pad.c:275), y nadie más lo lee; si se enmascara, los botones de fuera de la máscara dan una pulsación nueva en cada frame que se mantienen.
- **Vibración del mando 2 (hito 2b):**
  - La vibración del juego va siempre al puerto 0: `StartVibrationEx` usa `CurrentPortId` (padman.c, siempre 0) y las funciones `pdVibMx*` (ps2_sg_pdvib.c) ignoran el puerto y escriben en la tabla única `Pad_act[20]`, que `Ps2_pad_actuater` envía con `scePadSetActDirect(0, …)` al final de `Ps2_pad_read`.
  - En la build `COOP`, `coopBegin` pone `CurrentPortId = 1` durante la ventana de P2, así que sus vibraciones llegan como puerto lógico 8. Los ganchos de `pdVibMxIsReady`/`pdVibMxStart`/`pdVibMxStop` desvían el puerto 8 a una tabla propia (`Pad_act2`), que `coopPadActuater2` envía al puerto 1 al final de `Coop_pad_read2`. `pdVibMxStop` para también la de P2, y `coopSetPad2` la para cuando no se lee el mando 2 (pausa, demo).
  - La opción de vibración del menú (`EnadleVibrationFlag`, vibman.c), `EventVibrationMode` y el filtro del demo se comprueban antes de `pdVibMx*`, así que valen para los dos mandos.
