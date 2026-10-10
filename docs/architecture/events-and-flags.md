# Flags y sistema de eventos

Rutas relativas a `src/ps2/veronica/prog/`. Confianza: **C** = comprobado en el código, **P** = probable, **S** = suposición.

Los flags del motor son literales hex sin nombre. Este glosario dice qué significa cada bit conocido. En los scripts de evento, los bits se numeran **empezando por el más significativo**: el bit *n* es la palabra `n>>5` con máscara `0x80000000 >> (n & 31)` (flag.c:6, event.c:550). Así, `cb_flg 0x200` es el bit 22 de script y `sp_flg 0x10` es el 27.

## Parte A — Flags de `SYS_WORK`

### `tk_flg` (tarea activa) y `ts_flg` (tarea suspendida)

Los dos usan el mismo orden de bits que la tabla de tareas ([game-loop.md](game-loop.md)). Una tarea se ejecuta si su bit está en `tk_flg` y **no** en `ts_flg`. Muchas comprobaciones del tipo `ts_flg & 0x200` significan en realidad "el inventario no está abierto".

**Con el inventario abierto** la tarea Game (7) está suspendida (`ts_flg 0x80`) pero la tarea Event (8) sigue corriendo con `sp_flg = 0x30` (solo el planificador de eventos y la lectura del mando). Ver [inventory.md](inventory.md).

| Bit | Máscara | Tarea | Significado de `ts_flg` cuando está a 1 |
| --- | --- | --- | --- |
| 6 | 0x40 | Pad | Mando desactivado (al empezar una demo; se reactiva al final de la carga de sala) |
| 7 | 0x80 | Game | Juego congelado (cargas, menús, vídeo) |
| 8 | 0x100 | Event | Eventos congelados (puerta, muerte por cuenta atrás) |
| 9 | 0x200 | Itemselect | Inventario cerrado |
| 10 | 0x400 | Map | Mapa cerrado |
| 11 | 0x800 | Doordemo | Además significa "animación de puerta terminada" (system.c:1835, room.c:998) |
| 12 | 0x1000 | Movie | Se pone a 0 para reproducir un vídeo (system.c:764) |
| 14 | 0x4000 | Gameover | Se pone a 0 para lanzar el game over (player.c:6424, event.c:8685) |
| 15 | 0x8000 | Typewriter | Se pone a 0 para abrir la pantalla de guardar |
| 16 | 0x10000 | Option | Se pone a 0 para abrir las opciones |
| 17 | 0x20000 | CompEvent | Lo pone a 0 el cargador en el modo 7 |
| 20 | 0x100000 | Monitor | Congelado durante el fundido de puerta (room.c:961) |

`(ts_flg & 0x3DE00) == 0x3DE00` significa "no hay ninguna subpantalla abierta"; es la condición para poder pausar (system.c:668).

Valores típicos de `tk_flg` (C):

| Valor | Dónde | Situación |
| --- | --- | --- |
| 0x1 | main.c:179 | Arranque |
| 0x300004 | system.c:335 | Logo de Capcom |
| 0x300008 | system.c:348 | Menú de título |
| 0x300020 | system.c:402; room.c:1053 | Opening (partida nueva) o reintento tras morir |
| 0x308040 | system.c:376 | Pantalla de cargar partida |
| 0x73DFC0 añadido y luego quitado 0x20 | system.c:451, :460 | Conjunto de tareas durante el juego |
| 0x702040 | event.c:9068 | Final y ranking |

En partida nueva, `ts_flg = 0x3DF80` (system.c:453).

### `sp_flg`: qué sistemas se actualizan

Se pone a todo 1 en system.c:140 y al terminar de cargar la sala (room.c:545). Todo **C**.

| Máscara | Activa | Dónde se comprueba |
| --- | --- | --- |
| 0x1 | `bhControlPlayer` | player.c:1365 |
| 0x2 | `bhControlEnemy` | eneset.c:352 |
| 0x4 | Objetos e items | objitm.c:289 |
| 0x8 | Efectos | effect.c:750 |
| 0x10 | Planificador de tareas de evento | event.c:13024 |
| 0x20 | `bhSetPad` (leer el mando) | system.c:522 |
| 0x40 | Cambio de plano de cámara y primera persona | cut.c:24, game.c:53 |
| 0x80 | Luces | light.c:411 |
| 0x200 | Cuenta atrás de evento | sync.c:64 |

Valores típicos: menús e inventario `0x30` (system.c:617); puerta `0x48` (room.c:898); vídeo `4` (system.c:760). Un mensaje de script congela `0x7` (event.c:1076).

### `pt_flg`: qué se dibuja

Se pone a todo 1 al cambiar de sala (system.c:193). Todo **C**.

| Máscara | Dibuja |
| --- | --- |
| 0x1 | El jugador y sus objetos (ids ≥ 1210) — game.c:357, objitm.c:368 |
| 0x2 | Enemigos y sus objetos (ids 1000-1209) — game.c:336, :353 |
| 0x4 | Objetos de la sala |
| 0x8 | Items |
| 0x10 | Efectos |
| 0x20 | Modelo de la sala |

### `gm_flg`: estado de juego

Al cambiar de sala se conservan los bits `0x9B8C00CB` (system.c:185).

| Máscara | Significado | Referencias | Conf. |
| --- | --- | --- | --- |
| 0x2 | Datos de sala (RDT) cargados | room.c:30 | P |
| 0x4 | Subida de paleta pendiente | sync.c:104 | C |
| 0x10 | Refrescar niebla y cámara (en cada cambio de plano) | cut.c:57 | C |
| 0x20 | El plano de cámara ha cambiado en este frame | cut.c:298 | P |
| 0x40 | Vista en primera persona | cut.c:28, game.c:53 | C |
| 0x80 | Dibujar la sala en una sola pasada (va con 0x40) | game.c:329 | P |
| 0x100 / 0x200 | Render a textura a pantalla completa / en ventana | game.c:63-68 | C |
| 0x400 | Game over por **fin de la cuenta atrás** (no por muerte normal) | player.c:1298, :1319 | P |
| 0x800 | Forzar comprobación del plano (`bhCheckCut(1)`) | game.c:289 | C |
| 0x1000 | Plano fijo de guion activo | game.c:273 | C |
| 0x2000 | Reiniciar la cámara de primera persona | game.c:284, player.c:1786 | C |
| 0x4000 | Pasada de espejo en curso | game.c:79-91 | C |
| 0x10000 | La cámara de examinar usa un fotograma de cámara de evento | hitchk.c:4806 | C |
| 0x20000 | Mantener la cámara de evento al cerrar el mensaje | event.c:4613 | P |
| 0x40000 | **Arma vacía**: la pone `bhCountBullet` al llegar a 0 balas (weapon.c:462); activa la recarga automática y desactiva el fuego automático. Ver [combat.md](combat.md) | weapon.c:462, player.c:4940-4955 | C |
| 0x80000 | Zoom de la mira (con `st_flg 0x800000`) | playpch2.c:44 | P |
| 0x1000000 | Modo primera persona persistente (`bhSyukanModeSet`) | event.c:8885 | P |
| 0x8000000 | El personaje tiene la mochila (2 casillas más, `itm[10..11]`), según `ev_flg` 6/7/8; siempre activa en el Battle Game. Ver [inventory.md](inventory.md) | player.c:1088-1094, sub1.c:3392-3399 | C |
| 0x10000000 | **Crítico de la pistola especial** (id de objeto 131): lo activa `WeaponSet` al equiparla y lo lee el disparo. Ver [combat.md](combat.md) | sub1.c:4555, player.c:4966 | C |
| 0x20000000 | Ya ha corrido al menos un frame de juego | system.c:554 | P |
| 0x80000000 | Reloj de tiempo de juego en marcha | ps2_NaSystem.c:104 | C |

### `st_flg`: estado

Se pone a 0 al cambiar de sala (system.c:187).

| Máscara | Significado | Referencias | Conf. |
| --- | --- | --- | --- |
| 0x1 | Cámara controlada por evento o plano fijo, no por las zonas | cut.c:26, :1690 | C |
| 0x2 | Niebla activada | sync.c:174 | C |
| 0x4 | **Jugador ocupado o bloqueado** (cinemática, puerta, escaleras, recoger objeto, daño); bloquea los menús. Lo pone cada frame `bhControlPlayer` si `plp->flg & 0x4` (player.c:1738) y lo borra el jugador al volver al reposo (player.c:1846) | player.c:1738, system.c:600-632 | C |
| 0x8 | Pantalla de estado o inventario abierta (subpantalla; la pone `bhCheckSubTask` al abrir y `ItemTaskCheck` la quita al cerrar) | system.c:612, sub1.c:3103-3226 | C |
| 0x40 | La sala tiene agua | objitm.c:1317 | P |
| 0x80 | Examinar con cámara en curso | hitchk.c:4813 | P |
| 0x100 | Sala con espejo | game.c:77 | C |
| 0x200 | **Mensaje en pantalla** | message.c:43 | C |
| 0x400 … 0x80000 | Estado interno de los mensajes (página completa, espera, esperar botón, sí/no…) | message.c:81-200 | C |
| 0x2000 | Mensaje lanzado al examinar (al cerrarlo se restauran el mando y `sp_flg`) | hitchk.c:4840 | C |
| 0x40000 | Mapa abierto | system.c:624 | C |
| 0x800000 | Dibujar la mira | game.c:99 | C |
| 0x1000000 | Usando las luces de evento | light.c:356 | P |
| 0x4000000 | Terminal de ordenador activo | system.c:585 | C |
| 0x8000000 | Máquina de escribir activa | system.c:576-590 | C |
| 0x10000000 | Opciones abiertas | system.c:641 | C |
| 0x20000000 | Mechero encendido | pwksub.c:3464 | P |
| 0x40000000 | Termómetro en pantalla | game.c:104 | P |

Máscaras compuestas: `0x1C040008` = hay alguna subpantalla abierta; `0x1C040208` = lo mismo o un mensaje (system.c:668).

### `cb_flg`: peticiones y activadores

Al cambiar de sala se conservan los bits `0xAF8000BB` (system.c:191). Algunos bits **solo los activan los scripts de la sala**.

| Máscara | Significado | Referencias | Conf. |
| --- | --- | --- | --- |
| 0x1 | **Puerta pendiente o en curso** | hitchk.c:4662, room.c:907, game.c:24 | C |
| 0x2 | Fundido de pantalla en curso | screen.c:23 | C |
| 0x4 | **Modo cinemática** (bandas negras) | event.c:1632-1673; el final se procesa en system.c:736 | C |
| 0x8 | Examinar con cámara fija pedido | hitchk.c:4815 | P |
| 0x10 | Objeto cogido: abre la pantalla de objeto obtenido (petición automática de inventario) | hitchk.c:4885, player.c:3936 | C |
| 0x20 | Mensaje de examinar lanzado | hitchk.c:4838 | P |
| 0x40 | Modo cinemática sin bandas | event.c:1657 | C |
| 0x80 | Cambio de personaje pendiente | event.c:6276, system.c:1725 | C |
| 0x100 | Activador de examinar; la zona está en `sys->etc_idx` (bit de script 23) | hitchk.c:4643 | C |
| 0x200 | Activador de suelo; la zona está en `sys->flr_idx` (bit 22). Se borra en cada frame | hitchk.c:5270, :5295 | C |
| 0x400 | Item usado en un punto de examinar (`sb_id`) | sub1.c:3937 | P |
| 0x800 | Item cogido (activa `it_flg`) | sub1.c:3813 | P |
| 0x1000 / 0x2000 | Cerrar mensaje / mensaje terminado (los scripts esperan a 0x2000) | message.c:244-310 | P |
| 0x4000 | Coger un objeto sin preguntar (petición de inventario). Lo consume `GetItem` (sub1.c:3809-3857). Quién lo activa: sin confirmar (probablemente un script) | sub1.c:3809 | P |
| 0x10000 | Abrir el mapa desde un evento | event.c:6843 | C |
| 0x20000 | Abrir el visor de archivos (petición de inventario) | hitchk.c:4860 | C |
| 0x40000 / 0x80000 / 0x100000 | Abrir el baúl general / especial A / especial B (peticiones de inventario; zona de examinar tipo 4 o tapa `bhObjItmBox`) | hitchk.c:4893-4905, objitm.c:820 | C |
| 0x200000 | Petición de máquina de escribir (la pone el script; tiene prioridad sobre el inventario) | system.c:576 | P |
| 0x2000000 | **Bloquea el inventario** (`bhCheckSubTask`, system.c:608). Ningún código C lo activa: lo pone un guion (deducido) | system.c:608 | P |
| 0x400000 | Petición de terminal de ordenador | event.c:6430 | C |
| 0x800000 | Colocar al jugador en la posición guardada | player.c:669, room.c:477 | C |
| 0x4000000 | Petición de vídeo | event.c:1399 | C |
| 0x10000000 | Saltar cinemática pedido (cinemática + botón 0x10000) | system.c:545 | C |
| 0x40000000 | Game over: solo se actualizan los objetos del jugador | gameover.c:276 | P |

### Otros flags de `SYS_WORK`

- **`rm_flg`**: memoria de trabajo de la sala; se pone a 0 al cambiar de sala. Su significado depende de la sala. Algunos enemigos usan los bits 0x1-0x10 (por ejemplo, el "Spotter" activa 0x1 al ver al jugador). **C**
- **`ef_flg`**: 0x1 = los efectos siguen durante la carga (C); 0x2 = lanzador lineal (arma 18): lo activa la mira (playpch2.c:134), lo lee player.c:4925 y lo borran los efectos (effsub1.c:9094) (C); 0x4 = buffer de estelas reservado (P).
- **`ss_flg`** (sesión):

  | Máscara | Significado | Conf. |
  | --- | --- | --- |
  | 0x1 | Disco 2 | P |
  | 0x100 | **Partida nueva** | C |
  | 0x200 | Restaurando un estado (cargar, reintentar, demo) | C |
  | 0x20000 | Saliendo al menú (soft reset desactivado) | P |
  | 0x400000 | **Reproduciendo la demo** | C |
  | 0x80000000 | Pausa | C |

- **`opt_flg`**: declarado pero no se usa en ningún sitio. **C**

## Parte B — Flags de `BH_PWORK`

### El jugador (`plp`)

`flg`:

| Máscara | Significado | Conf. |
| --- | --- | --- |
| 0x1 | Activo | C |
| 0x2 | Atrapado o anulado (agarre de enemigo, muerte forzada) | P |
| 0x4 | Recibiendo un golpe (activa `st_flg 0x4`) | P |
| 0x8 | Choca con los enemigos | P |
| 0x10 | Choca con las paredes | C |
| 0x800 | Tiene sombra | S |
| **0x10000** | **Controlado por script o enemigo: ignora el mando** (player.c:1804) | C |
| 0x20000 | El arma equipada se puede apuntar | P |
| 0x100000 | Girar hacia `ayp` en varios frames | C |
| 0x400000 | La animación ha llegado al último frame | C |
| 0x1000000 | Llama del mechero encendida | C |
| 0x10000000 | Empujando un objeto | P |

`stflg` (al cambiar de sala y al guardar se conserva la máscara `0x78280000`, room.c:1022):

| Máscara | Significado | Conf. |
| --- | --- | --- |
| 0x8 | Sin sombra | P |
| 0x10 | En unas escaleras | C |
| 0x400 | Apuntando | P |
| 0x1000 | Daño continuo por trampa | P |
| 0x10000 / 0x18000 | Acción forzada por script (0x18000 = recoger un objeto) | C |
| 0x40000 | Muerte bloqueada | P |
| 0x80000 / 0x200000 | Envenenado (dos tipos) | P |
| 0x100000 | Dentro del agua | P |
| **0x1000000** | **Escondido y congelado:** no se actualiza ni se dibuja (player.c:1365, game.c:357) | C |
| 0x8000000 … 0x20000000 | Huevos de polilla de tipo 0/1/2 | P |
| 0x40000000 | "Es un personaje jugador" (siempre a 1) | C |
| 0x80000000 | Pasando por una puerta o escalera; los enemigos lo ignoran | C |

`mdflg`: 0x1 = no dibujar; 0x20 = sin prueba de recorte (C); 0x40 = se refleja en el suelo espejo (P). `flg2`: 0x1 = escaleras, no se le puede empujar; 0x8 = suelo con agua (P).

### Enemigos y NPCs (bits genéricos)

Cada archivo de enemigo usa además bits privados.

| Campo | Bits |
| --- | --- |
| `flg` | 0x1 activo; 0x2 muerto; 0x8 y 0x40 participa en los empujes; 0x10 choca con paredes; **0x20 se le puede fijar como blanco**; 0x80 enlazado a un padre (`lkwkp`); 0x8000 id > 40 / no se le busca; 0x80000000 se dibuja como `en_obj` (P) |
| `stflg` | 0x1000000 o 0x40000000 desactivado o escondido; 0x10000 controlado por script (P) |
| `mdflg` | 0x1 escondido; 0x2 morph; 0x4 recalcular antes de dibujar; 0x20 sin prueba de recorte; 0x100 máscara facial; 0x200 solo en la pasada de render a textura (C) |
| `mode0` | Por convención: 0 init, 1 mover, 2 agarrar, 3 daño, 4 morir, **5 `bhEne_Event` (movido por script)** |

## Parte C — El cargador (`bhSysCallMonitor`)

- **Cómo se pide un modo:** se escribe en `mn_mode0`. Las peticiones se encolan en `mn_stack[8]`.
- **Estado de trabajo:** `mn_md0` = modo actual, `mn_md1` = paso. Al terminar, `SET_SYS_MN_MD(0,…)` saca la siguiente petición de la cola.
- **Durante la carga** se dibuja "Now Loading".

| Modo | Para qué | Pasos clave |
| --- | --- | --- |
| 1 | Carga inicial (`bhFirstGameStart`, system.c:462) | 1-2 `sysmes.ald`; 4 init de objetos, efectos, cámara, jugador y enemigos; 5 `AllItemInit` en partida nueva; 6-7 modelo del jugador; 8-9 arma; 10 pasa a modo 4; 20-21 demo |
| 2 | Cambio de sala (`bhStartDoorDemo`, room.c:993) | 0 `memp = mempb`; 1 banco de sonido de la sala; 2 pasa a modo 4 |
| 3 | Cambio de arma (`bhArmsItemChange`, event.c:1285; `WeaponSet`, sub1.c:4456; system.c:1573-1608) | Lee `SYSTEM.AFS[plp->wpnr_no + ply_id*30 + 20]` → `bhReadWeaponData` (modelos `wrmdlp`/`wlmdlp`, `bhSetWeapon`, animaciones en `plp->mnwP[100+]`) → `RequestArmsSoundBank`. Ver [combat.md](combat.md) |
| 4 | Carga de sala (system.c:1601) | 0 `rm_*.rdx` + `bhInitReadRDT`; 1 `Expand`; 2 `bhSetRDT`; 3 texturas + `bhFinishRoom` (1723); 4-7 cambio de personaje; **10 `bhInitEvent()` (1816)**; 11 espera al sonido y a la puerta; **16 `ts_flg &= ~0x180` reanuda Game y Event (1886)** |
| 5 | Inventario (`bhCheckSubTask`, system.c:608; modo en system.c:1962-2036) | 0 copia las texturas de la sala a memoria principal; 1 lee `ITEM.AFS[145]` (interfaz); 2 al acabar el fundido suspende Game (`ts_flg \|= 0x80`) y libera texturas; 3 `SbsTextureInit`; 4 `ItemTaskCheck` activa Itemselect. Ver [inventory.md](inventory.md) |
| 6 | Modelo 3D del objeto examinado (`Model_Read_Start`, itemview.c:284-315; lo pide `StatusMain` con el inventario abierto). Ver [inventory.md](inventory.md) | |
| 7 | Terminal de ordenador (system.c:591) | |

El sonido lo carga `bhSysCallSndMonitor` (system.c:2202) según `sdm_flg`.

## Parte D — Scripts de evento

### Dónde están

- **Fichero:** cada sala es `rm_SRRC.rdx`, dentro de `RDX_LNK.AFS`, comprimido con un LZ equivalente a PRS de Sega (lo descomprime `Expand`, expand.c).
- **Carga:** se descomprime en `sys->rdtp`.
- **Cabecera** (`RDT_WORK`, include/ps2/veronica/prog/system.h:8):
  - `hed00` → imagen de `ROM_WORK`;
  - `hed01` → modelos;
  - `hed02` → animaciones;
  - **`hed03` → bloque de eventos**: `rom->evtp = rdtp + hed03` (room.c:152);
  - `hed04` → texturas.
- **Bloque de eventos** (`EVT_WORK`, types.h:968), con offsets relativos a `evtp`:
  - `scd0` = script de inicio de sala;
  - `scd1` = script que se ejecuta en cada frame;
  - `evd[]` = scripts de tarea, indexados por número de evento.

### Intérprete

| Pieza | Dónde | Qué hace |
| --- | --- | --- |
| Globales | event.c:31-41 | `bhScePtr` (puntero de instrucción), `bhCetask` (tarea actual), **`BH_SCEWORK bhEtask[16]`** (16 tareas simultáneas), `G_Sp` (salto del IF), `Event_T_timer` |
| `BH_SCEWORK` | types.h:1948 | `status`, `data` (punto de reanudación), **`work` (el `BH_PWORK` controlado)**, pila de bucles, parámetros de animación y de splines |
| `bhInitEvent` | event.c:339 | Al cargar la sala (system.c:1816): limpia las 16 tareas, ejecuta `scd0` una vez y una pasada del planificador |
| `bhControlEvent` | event.c:379 | En cada frame, desde la **tarea 8** (system.c:734): ejecuta `scd1` desde el principio y luego el planificador `bhEventScheduler2` (event.c:13017) |
| Planificador | event.c:13017 | Solo si `sp_flg & 0x10`. Ejecuta las tareas 0..15; cada una avanza hasta que un opcode devuelve 0 |

Cada opcode devuelve 1 (seguir en este frame) o 0 (parar hasta el siguiente). Dentro de un IF, un 0 significa "condición falsa" y salta a `G_Sp`.

El terminal de ordenador (sp_evt.c, `comevt_script_tab`) y el mapa (map.c) tienen sus propios intérpretes.

### Opcodes principales

La tabla es `bhScenarioJmpT[256]` (event.c:59): 223 opcodes reales, y el resto apunta a `dm0`. Los operandos van en little-endian.

| Op | Handler (línea en event.c) | Qué hace |
| --- | --- | --- |
| 00 | `bhEnd` 387 | Fin de `scd0`/`scd1` |
| 01/02/03 | `bhIfelCk` 395 / `bhElseCk` 413 / `bhEndif` 425 | IF / ELSE / ENDIF; las condiciones seguidas se combinan con AND |
| **04** | **`bhCk` 443** | Comprueba un flag. Tipos: 1 `ev`, 2 `ky`, 3 `ed`, 4 `rm`, 5 `st`, 6 `sp`, 7 `it`, 8 `mp`, 9 `ic`, 10 `cb` (los bits 22/23 comparan también `flr_idx`/`etc_idx`), 11 `gm`, 12 `ts`, 13 `plp->flg`, 14 `plp->stflg`, 15 `plp->flg2` |
| **05** | **`bhSet` 554** | Activa, limpia o invierte un flag (mismos tipos) |
| 06/07 | `bhCmpB` / `bhCmpW` | Compara `stg_no`, `rom_no`, `pos_no`, `ply_id`, `hp`… |
| 08/09 | `bhSv` 796 / `bhSvW` 856 | Escribe esas variables. **No** activa flags |
| **12** | **`bhCineSet` 1618** | 0 = empieza cinemática con bandas, 3 = sin bandas; **1/4 = termina (`sp_flg = ~0`)** |
| **13** | **`bhCamSet` 936** | Cámara de evento (`13 0 evc key`) o volver a la de juego (`13 1`) |
| **14** | **`bhEvtOn` 920** | Lanza un evento en una tarea |
| 1F | `bhMessageSet` 1056 | Mensaje; puede congelar `sp_flg 0x7` |
| 24 | `bhInitModelSet` 2802 | Esconder o mostrar un modelo. Tipo 0 = jugador → `stflg 0x1000000` |
| **33** | **`bhSetDoorCall` 2333** | Abrir puerta → `bhSetDoorDemo` |
| 5D | `bhPadCheck` 4766 | Esperar a que se pulse un botón |
| **64** | **`bhPlCtr` 9259** → `Player_controll` 9271 | Mover al jugador por guion. Subop 128 = devolver el control (`mode0 = 1`, limpia `flg 0x210000` y `stflg 0x10000`); 131 andar o girar; 140 ir a un punto; 137 teletransportar a `posp[n]` |
| **65** | **`bhLoadWork` 12713** | Elegir qué controla la tarea: 0 jugador (**`mode0 = 7`**, `flg` y `stflg 0x10000`), 1 enemigo, 2 objeto, 3 item, 4 efecto |
| **67** | **`bhSubCtr` 9748** → `Sub_controll` 9758 | Mover un NPC o enemigo por guion; 128/144 lo devuelven a su IA |
| 69 | `bhCommonCtr` → `Common_controll` 9963 | Posición, ángulo, velocidad y splines de la entidad controlada |
| 89 | `bhPlayerChangeSet` 6264 | Cambio de personaje |
| BD / C6 | `bhReTryPointSet` 8172 / `bhGameOverSet` 8681 | Punto de reintento / game over |
| F8 | `bhSleep` 9100 | Esperar n frames |
| FA/FB, FC/FD | `bhFor`/`bhNext`, `bhWhile`/`bhEwhile` | Bucles |
| **FE / FF** | **`bhEvtNext` 9196 / `bhEvtEnd` 9212** | Ceder el frame / terminar la tarea |

### Cómo una cinemática toma el control

**Del jugador:**

1. `bhLoadWork` tipo 0 pone `plp->mode0 = 7` y activa los bits 0x10000 de `flg` y `stflg`.
2. `bhControlPlayer` ejecuta `bhCPM0_event` (pl_evt.c:62) con los movimientos `pl_smove00..08`.
3. El script lo suelta con `Player_controll 128`.
4. Cuando termina la cinemática, `bhSysCallEvent` (system.c:736-744) detecta que `cb_flg 0x4` ha pasado a 0 y limpia `plp->flg 0x10000` y `stflg 0x18000`.

Un script también puede **esconder y congelar** al jugador con `bhInitModelSet(0,0,0)` (`stflg 0x1000000`).

**De los NPCs:** `bhLoadWork` tipo 1 apunta a `ene[rom->enep[n].wrk_no]`; `Sub_controll` pone `mode0 = 5` (`bhEne_Event`) o, para los subpl, `mode0 5/6` (`em_sce`).

### Cómo saber si hay un evento en curso

| Comprobación | Significa |
| --- | --- |
| `sys->cb_flg & 0x4` | Modo cinemática (`& 0x40` = sin bandas) |
| `sys->st_flg & 0x4` | Jugador bloqueado en una acción forzada (también puertas y escaleras) |
| `plp->mode0 == 7` o `plp->flg & 0x10000` | Jugador controlado por script |
| `plp->stflg & 0x1000000` | Jugador escondido y congelado por script |
| `sys->st_flg & 0x200` | Mensaje en pantalla |
| `sys->cb_flg & 0x1` | Puerta en curso |
| `sys->ts_flg & 0x80` | Juego suspendido |

`bhEtask[i].status` **no** sirve: muchas tareas son bucles de ambiente que no terminan nunca.

### Flags de historia (todos se guardan con la partida, salvo `en_flg`)

Funciones: `bhStFlg`/`bhCrFlg`/`bhCkFlg` (flag.c); `bhFlagCk(type, n, want)` (event.c:13096, devuelve `want ^ activado`); `bhFlagSet(type, n, op)` (event.c:13138).

| Array | Bits | Para qué |
| --- | --- | --- |
| `ev_flg[32]` | 1024 | Progreso de la historia. Ejemplos: 6/7/8 = mochila de cada personaje; 74 = modo ráfaga del arma de id 10 (`WeaponSet`, sub1.c:4527-4534) y la leen `WpnTab.flg 0x200` (player.c:4981); 75 = se activa al disparar el arma 20 (o la 18 con `gm_mode == 3`; player.c:5117, :5132, playpch2.c:161) y lo lee el ranking (ranking.c:154); 67/69/70 = cuentas atrás; 99 = intro de la celda terminada |
| `it_flg[16]` | 512 | Items de la sala ya cogidos |
| `ic_flg[16]` | 512 | Item examinado pero no cogido (el mapa reutiliza bits) |
| `ed_flg[32]` | 1024 | Enemigos muertos |
| `ky_flg[16]` | 512 | Solo lo usan los scripts; parece el estado de puertas y llaves |
| `mp_flg[8]` | 256 | Mapa: zonas descubiertas y marcador del personaje |
| `en_flg[4]` | 128 | No se guarda: tipos de enemigo con animaciones cargadas en la sala |

## Parte E — El comienzo de la partida (comprobado con la ISO)

1. **Del título a la partida:** título → `bhFirstGameStart` (system.c:416) pone `ss_flg |= 0x100`, `mn_mode0 = 1`, 160 de vida (320 si `gm_mode == 2`) y **`stg_no = 0`, `rom_no = 0`, `ply_id = 0`**.
2. **Carga:** el cargador carga `rm_0000.rdx`, que es la celda de Rockfort donde empieza Claire con Rodrigo (los mensajes de la sala lo confirman). `bhFinishRoom` coloca a Claire en `posp[pos_no]` y pone `mode0 = 0`. `bhInitEvent` ejecuta `scd0`.
3. **Primer frame:** `bhSetPlayer` → `mode0 = 1`.
4. **Intro (evento 29):** `bhInitModelSet(0,0,0)` **esconde y congela a Claire** (`stflg 0x1000000`), empieza la cinemática, cámara de evento y vídeo (FMV 0).
5. **Se entrega el control (evento 16):** se espera a que termine el mensaje (`cb_flg 0x2000`), y luego `bhCineSet(1)` termina la cinemática, `bhCamSet(1)` vuelve a la cámara de juego y **`bhInitModelSet(0,0,1)` quita `stflg 0x1000000`**.
6. A partir de ahí, `bhControlPlayer` vuelve a ejecutarse (player.c:1365) y `bhCPM0_action` lee el mando.

En esta intro Claire **no** pasa por `mode0 = 7`: se la congela escondiéndola. Las cinemáticas posteriores de la misma sala (eventos 0, 10 y 34) sí usan `bhLoadWork(0)` + `Player_controll 128`.

**Preguntas abiertas:**
- Bits sin setter en C: `gm_flg` 0x1/0x8/0x400000, `cb_flg` 0x2000000/0x80000000, varios de `ss_flg`. Probablemente los activan scripts.
- Qué significan `gm_mode` 0 y 1.
- Cuántos bytes ocupan algunos subops de `Common_controll` (10, 22, 24, 34, 35); hace falta saberlo antes de escribir un desensamblador de scripts.

## Parte F — Game over

- Lo lanza el update del jugador que muere (`bhCPM0_die`, `bhCPM0_enedie`): `ts_flg &= ~0x4000` y `gov_md0/gov_md1 = 0`, solo si el bit estaba puesto (una segunda muerte no lo reinicia).
- Tarea de game over (gameover.c:26): `bhSelectContinue` → `bhInitGameOver` (`ts_flg |= 0x100`, fundido) → `bhMainGameOver`. Durante 70 frames el juego sigue (gameover.c:236-342); después `sp_flg = 0x20` y `pt_flg = 0`.
- "Continue" lee `sys->pad_ps` (el mando de P1, gameover.c:467-509): "Sí" → `bhPopGameData` (reintento); "No" → `bhReturnTitle`. La voz final es `CallPlayerDeadVoice(sys->ply_id)`.
- `plp->flg 0x2` = muerte terminada por el enemigo, lanzar game over; `stflg 0x40000` = muerte bloqueada mientras un enemigo agarra (en01.c:6839); `stflg 0x200000` = veneno de Nosferatu.

## Parte G — Scripts: detalles útiles

- La tabla `evtp` de cada sala son offsets `u32` relativos: `[0] = scd0`, `[1] = scd1`, `[2 + n] = evd[n]`; el número de entradas es `primer_offset / 4` (event.c:12963).
- Opcodes de objetos: 0x0E `bhItmCk` (quita del mundo el objeto cogido, ver [inventory.md](inventory.md#recoger-un-objeto)), 0x23 `bhItmSetCk` (oculta al cargar la sala los ya cogidos), 0x8A `bhPlayerPoisonCk` (mira `plp`), 0xAE `bhPlayerPoison2Cr` (limpia `ply_stflg[0]`).
- `cb_flg 0x100` (zona de acción encontrada) se mantiene hasta la siguiente pulsación de acción. `cb_flg 0x10` y `0x20` sobreviven al cambio de sala (máscara `0xAF8000BB`).
