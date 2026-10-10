# Bucle de juego

Rutas relativas a `src/ps2/veronica/prog/`.

## Arranque

1. `main()` ([njloop.c](../../src/ps2/veronica/prog/njloop.c)) llama a `njUserInit()` una vez. Después ejecuta `njUserMain()` + `njWaitVSync()` en bucle infinito.
2. `njUserInit()` ([main.c](../../src/ps2/veronica/prog/main.c)) configura el vídeo (`hws`), limpia `SYS_WORK` y reserva la memoria del juego: `freemem = syMalloc(12845056)`. La parte alta de ese bloque son los buffers de vértices del Ninja (`njpmemp`, `vwbmemp`, `vebmemp`). Termina con `sys->tk_flg = 0x1`, que solo activa la tarea 0 (`bhSysCallInit`).
3. Las tareas se encadenan solas: init → avisos → logos → vídeo inicial → título → opening → juego.

## Tabla de tareas

`njUserMain()` (main.c:189) incrementa `Ps2_sys_cnt` y ejecuta `bhSysTaskJumpTab[i]()` (main.c:60) para cada bit `i` que esté **activo** en `sys->tk_flg` y **no suspendido** en `sys->ts_flg`. Después llama a `PS2_jikken()` (fin de frame) y a `bhCheckSoftReset()`.

Ojo, los nombres engañan: `bhSysCallFirstmovie` muestra el menú de título, y `bhSysCallTitle` solo salta al Opening. `ts_flg` usa los mismos bits para indicar "tarea suspendida" (ver [events-and-flags.md](events-and-flags.md)). Las descripciones de las tareas 0-1 y 9-22 salen del nombre de la función; el resto está comprobado.

| Bit | Tarea | Qué hace |
| --- | --- | --- |
| 0 | `bhSysCallInit` | Inicialización |
| 1 | `bhSysCallWarning` | Pantalla de avisos |
| 2 | `bhSysCallIpl` | Logo de Capcom (`Adv_CapcomLogo`), y después pasa a la tarea 3 |
| 3 | `bhSysCallFirstmovie` | **Menú de título** (`Adv_BioCvTitle`, system.c:359) |
| 4 | `bhSysCallTitle` | Solo pone `tk_flg = 0x300020` para saltar al Opening (system.c:402) |
| 5 | `bhSysCallOpening` | Partida nueva / carga (`bhFirstGameStart`, system.c:416) |
| 6 | `bhSysCallPad` | Lee el mando: `bhSetPad` (system.c:514) |
| 7 | `bhSysCallGame` | Un frame de juego: `bhCheckSubTask` y luego `bhMainSequence` (system.c:539) |
| 8 | `bhSysCallEvent` | Pausa (L1+Start), mensajes y **scripts de evento** (`bhControlEvent`, system.c:734). Si `loop_ct >= 2` no hace nada, pero en la práctica siempre vale 1 |
| 9 | `bhSysCallItemselect` | Inventario |
| 10 | `bhSysCallMap` | Mapa |
| 11 | `bhSysCallDoordemo` | Animación de puerta |
| 12 | `bhSysCallMovie` | Vídeo |
| 13 | `bhSysCallEnding` | Final |
| 14 | `bhSysCallGameover` | Game over |
| 15 | `bhSysCallTypewriter` | Máquina de escribir |
| 16 | `bhSysCallOption` | Opciones |
| 17 | `bhSysCallCompEvent` | Terminal de ordenador |
| 18 | `bhSysCallDiscChange` | Cambio de disco |
| 19 | `bhSysCallSoundMuseum` | Museo de sonido |
| 20 | `bhSysCallMonitor` | **Cargador**: partida nueva, cambio de sala, cambio de personaje. Ver [rooms-and-memory.md](rooms-and-memory.md) |
| 21 | `bhSysCallSndMonitor` | Sonido |
| 22 | `bhSysCallScreenSaver` | Salvapantallas |

`bhCheckSubTask` (system.c:560) abre el inventario, el mapa o las opciones cuando el jugador pulsa el botón correspondiente.

## Un frame de juego: `bhMainSequence` (game.c:20)

```text
if (cb_flg & 0x1)    bhStartDoorDemo()         // hay una puerta pendiente
if (evt_tmd)         bhCheckEvtTimer()
for i in 0 .. sys->loop_ct-1:                  // bhSysCallGame fuerza loop_ct = 1
    fundido, bandas de cine
    bhControlEnemy()      // eneset.c:344 — todo ene[], solo si sp_flg & 0x2
    bhControlPlayer()     // player.c:1349 — solo si sp_flg & 0x1 y el jugador no está dormido
    bhControlEffect()
    bhControlObjItm()
    cámara de primera persona (si gm_flg & 0x40)
    bhControlCamera()
    bhControlLight()
    si es la última iteración: dibujar (bhAllDrawModel), espejo, cinesco, mira, termómetro, fundido
    fin de evento, restaurar sp_flg / pt_flg
    ...
    bhCheckCut()          // cambio de plano según la posición de plp (cut.c:12)
    si loop_ct > 1: bhControlMessage() + bhControlEvent()
```

**El dibujo usa la cámara del frame anterior:** `bhCheckCut` decide el plano al final del frame, después de dibujar, y los guiones (tarea 8) corren después de `bhSysCallGame`. Así, lo que cambien `bhCheckCut` o un guion (`cam`, `st_flg 0x1`, la cámara de evento con sus mallas ocultas) se ve en el dibujo del frame siguiente.

**Coste de un frame:** al entrar en `Ps2SwapDBuff` (ps2_NaSystem.c:67), `Ps2_vcount` cuenta los vsync que lleva el frame. El juego espera hasta tener 2 (30 fps); con 3 o más, la lógica se ralentiza. El hito 5 lo cuenta ahí (G31).

`bhSysCallGame` pone `sys->loop_ct = 1` en cada frame (system.c:543), así que esa última rama no se ejecuta nunca en la práctica. Los mensajes y los scripts de evento corren en la tarea 8 (`bhSysCallEvent`), después de `bhSysCallGame`.

Los enemigos se actualizan **antes** que el jugador. Los agarres (`_Nage`, `_PlayerControl`) modifican `plp` y `sys->pad_on` dentro de `bhControlEnemy`.

## Orden de dibujo: `bhAllDrawModel` (game.c:318)

1. Modelo de la sala (si `pt_flg & 0x20`).
2. Enemigos con modelo propio en `sys->en_obj[]` (`bhDrawEneObject`), si `pt_flg & 0x2`.
3. Efectos (`pt_flg & 0x10`). El momento depende de `Ps2_albinoid_flag`.
4. Objetos e items (`bhDrawObjItm`), que incluyen el arma y el pelo del jugador (`obwp[0..3]`).
5. Enemigos y NPCs de `ene[]` (`bhDrawEnemy`, eneset.c:797), si `pt_flg & 0x2`.
6. **El jugador**: `bhCheckClipModel(plp)` + `bhPutModel(plp)` (game.c:357-370), si `pt_flg & 0x1`.

`bhAllEasyDrawModel` (game.c:381) es una versión reducida que **no** dibuja al jugador. La usan algunas pantallas.

## Fin de frame

`PS2_jikken()` (ps2_dummy.c:230) vuelve a pedir el mando (`pdGetPeripheral(0)`), intercambia buffers (`Ps2SwapDBuff`) y ejecuta la función EOR `bhControlEOR` (sync.c:100). Esta comprueba si el mando sigue conectado (`bhCheckPadPort`, sync.c:261) y pausa el juego automáticamente si se desconecta.
