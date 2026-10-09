# Entrada: del mando a `sys->pad_*`

Rutas relativas a `src/ps2/veronica/prog/`.

## Resumen

- Todo el juego lee el mando a través de **campos lógicos de `SYS_WORK`**: `pad_on` (pulsado), `pad_ps` (recién pulsado), `pad_rs` (recién soltado), `pad_old`, `pad_ax/pad_ay` (stick), `pad_dx/pad_dy` (acumuladores de dirección), `pad_ar/pad_al` (gatillos).
- **Solo se lee el puerto 0.** El puerto 1 se abre pero nunca se consulta.
- No hay multitap ni código para él.

## Capa PS2 ([ps2_sg_pad.c](../../src/ps2/veronica/prog/ps2_sg_pad.c))

| Función | Línea | Qué hace |
| --- | --- | --- |
| `Pad_init` | 634 | `scePadInit` y `scePadPortOpen` para el puerto 0 **y el 1** (buffers `Padd1`/`Padd2`). La llama `ps2_dummy.c:164`. |
| `Ps2_pad_read` | 182 | **Fijado al puerto 0**: `scePadGetState(0,0)`, máquina de conexión, modo analógico y presión con el estado global `Pad_status`, `scePadRead(0,0,Pad_rdata1)` y `Pad_set(&Ps2_pad.pad1, 1)`. Al final llama a `Ps2_pad_actuater` (vibración). |
| `pdGetPeripheral(port)` | 69 | Devuelve un `PDS_PERIPHERAL` **estático y único** (`pp`). Solo refresca una vez por frame (`Ps2_sys_cnt != Old_sys_cnt`), así que pedir el puerto 1 después del 0 devuelve los datos del 0. Con `port == 1` elige `Pad_rdata2`/`pad2`, que nunca se rellenan. El stick derecho se ignora (`x2 = y2 = 0`). |
| `Ps2_Read_Key` | 367 | Copia el periférico en `Pad[0..3]` (estructura Dreamcast, padman.c) y gestiona `SoftResetFlag`. |
| `Pad_set` | 512 | Ya admite `pad_num == 2` → `Pad_rdata2` (líneas 519-522). |
| `pdGetPeripheralInfo(port)` | 136 | Ignora el argumento y comprueba el puerto 0. |

Hay piezas preparadas para el puerto 2 que no se usan: `Pad_rdata2[32]`, `Ps2_pad.pad2`, `sys->p2per` (types.h:621, nunca asignado) y un `PAD_STATUS Pad_status2` comentado en ps2_sg_pad.c:27-28.

La vibración también es solo del puerto 0 (`ps2_sg_pdvib.c`, `scePadSetActDirect(0,0,…)`).

## Capa lógica: `bhSetPad` ([pad.c](../../src/ps2/veronica/prog/pad.c):17)

La llama la tarea 6, `bhSysCallPad` (system.c:514), solo si `sys->sp_flg & 0x20` (si no, pone `pad_on = 0`). Durante las cinemáticas aplica la máscara `0x1188F` a `pad_on`/`pad_ps`.

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
| `0x80` | L1 (cambiar de blanco; también modificador) |
| `0x100` | Disparar |
| `0x200` | Acción / examinar |
| `0x400` | Correr |
| `0x800` | Aceptar |
| `0x1000` | Cancelar |
| `0x2000` | Mapa |
| `0x4000` | Inventario |
| `0x8000` | Opciones |
| `0x10000` | Saltar cinemática |
| `0x20000` | Start (L1+Start = pausa) |

## Quién lee la entrada

**El jugador** lee directamente `sys->pad_*`, sin variable intermedia:

| Archivo | Referencias a `sys->pad_*` |
| --- | --- |
| player.c | 66 (`bhControlPlayerPad` :1820 decodifica el movimiento; `bhCPM2_act_atk` :5262 tiene 18) |
| playpch.c | 22 |
| playpch2.c | 20 (mira: `pad_ax/ay/dx/dy`) |
| hitchk.c | 14 (`bhCheckExmAtari`, el botón de acción) |
| pl_evt.c | 1 |

El código del jugador también **escribe** `sys->pad_on &= ~0xF` (player.c:2275 y :5245).

**Otros consumidores** (siempre del jugador 1):

- **Sistema:** `bhCheckSubTask` (inventario, mapa y opciones; system.c:600-632), saltar cinemática (system.c:545) y pausa (system.c:668). La reanudación tras la pausa lee directamente `Ps2_pad.pad1.push`.
- **Eventos y mensajes:** guardan y restauran el mando en `pad_onb/psb/oldb` (game.c:142, :175; message.c:318); `bhPadCheck` en los scripts (event.c:4766).
- **Pantallas:** inventario (`sub1.c`, 57 refs), mapa (`MapPadMain`), archivos, items, guardar/cargar, game over, ranking.
- **Puzles:** `effsub1b.c` (`bhEff133/134/137/138`), `objitm.c` (`bhObj007`), `zonzon.c` (`bhEne_LeverCheck`), `sp_evt.c`.
- **Agarres de enemigos:** se ejecutan dentro de `bhControlEnemy`, antes de actualizar al jugador, y hacen `sys->pad_on &= ~0xF`. Están en en01, en01sub, en04, en09, en17, en22 y en26.
- **Lectores directos de `Pad[]`:** `adv.c` (menús de título) y `sdfunc.c:3280,3325` (cancelar vídeo).
- **Al cambiar de sala** se ponen a cero los campos del mando (system.c:234-240).

## Cooperativo (build `COOP`)

- **Puerto 2:**
  - `pdGetPeripheral(1)` devuelve `coopGetPeripheral2()`, en el bloque `#ifdef COOP` al final de ps2_sg_pad.c.
  - Ese bloque sondea el puerto 1 con su propia máquina de conexión (`Pad_status2`, copia de la de `Ps2_pad_read`) y su propia caché por frame.
  - Devuelve `NULL` si no hay mando.
- **Estado lógico de P2:**
  - `coopSetPad2()` (coop.c), llamado al final de `bhSysCallPad`, ejecuta el `bhSetPad()` original con los campos `sys->pad_*` intercambiados y `pd_port = 1`. Así P2 usa la misma configuración de botones y conversión de stick que P1.
  - Al final se enmascara a `0x40F` (moverse, girar, correr).
- **Detalles:** en demo, o con `!(sp_flg & 0x20)`, no se lee. Nadie más llama a `pdGetPeripheral(1)`.
