# Sistemas del mundo: cámara, colisión, objetos, luces y sonido

Rutas relativas a `src/ps2/veronica/prog/`. Cada sección indica si el sistema recibe la entidad como **parámetro** o usa el **global `plp`**. Esto es lo que decide si un sistema sirve tal cual para un segundo personaje.

## Cámara

Hay una sola cámara global, `CAM_WORK cam` (types.h:1062, main.c:39). No hay pantalla partida.

### Planos fijos

- `bhCheckCut` (cut.c:12) se llama en cada frame desde `bhMainSequence`. Comprueba `plp->gpx/gpz/flr_no` contra las zonas de `rom->cutp[]` con `bhCheckCutArea` (cut.c:155, que **recibe parámetros**). Para cambiar de plano, los 4 puntos de prueba (±1.0) tienen que haber salido del plano actual (cut.c:112-117). Entonces llama a `bhSetCut` (cut.c:281).
- `bhSetCut` también oculta mallas y luces de la sala según el plano (`hidobj`/`hidlgt`). La geometría que no se ve desde el plano actual puede no dibujarse.
- `bhControlCamera` (camera.c:23) solo construye la matriz a partir de `cam`.

### Cámaras que siguen

- `bhInitActiveCamera` (cut.c:663) y `bhControlActiveCamera` (cut.c:1174) siguen a `cam.plx/ply/plz`, que salen de `plp->exp0` (EXP_WORK `plx/ply/plz`).
- **Para una cámara cooperativa, este es el punto de enganche natural** (por ejemplo, apuntar al punto medio entre los dos jugadores).

### Cámaras de evento

- Los fotogramas clave llevan un enlace `lkflg/lkno/lkono`. `bhGetEvtCamLockPosition` (cut.c:2391) lo resuelve: 1 = `plp`, 2 = `ene[lkno]`, 3 = `obwp`, 4 = `itwp`, 5 = `eff`, 6 = `rom->posp`.
- Las luces usan el mismo patrón de enlace (light.c:720-743).

### Primera persona

- `bhInitPlEyeCamera`, `bhControlPlEyeCamera` y `bhSetPlEyeCamera` (cut.c:2472-2800) usan el hueso de la cabeza de `plp` (`owP[5]`).

Si un personaje que no es `plp` sale del encuadre, no pasa nada: se sigue actualizando y `bhCheckClipModel` (pwksub.c:2465) lo descarta al dibujar.

## Colisión con la sala

| Función | Línea | ¿Parámetro o `plp`? | Notas |
| --- | --- | --- | --- |
| `bhCheckWallEx(pw, npos, opos, r, h)` | hitchk.c:641 | Parámetro | Paredes de `rom->walp` + `sys->mwalp`. Si no choca, ajusta `npos->y` a la altura del piso. Excepciones: el atributo 0x10000 solo se trata si `pw == plp` (:722); las paredes que hacen daño (atributo 0x40) dañan a **`plp`** (:967-1001). El jugador la llama dos veces por frame (cuerpo player.c:1622, pies :1647). |
| `bhCheckWall(pw)` | hitchk.c:12 | Parámetro | La usan los enemigos. Mismo problema de daño a `plp` (:259-287). |
| `bhSetFloorNum(pw)` | pwksub.c:85 | Parámetro | Calcula el piso. |
| `bhGetGroundPosition(p)` | hitchk.c:3557 | Parámetro | Altura del suelo; escribe el global `sys->htp`. |
| `bhCheckFloorP(pp)` | hitchk.c:5245 | **Mixto** | Lee `plp->exp0` y `plp->flg`, borra y activa los activadores globales `sys->cb_flg` (0x200) / `flr_idx`. Los resultados por personaje (sonido de pisada, agua, escalón) van a `pp`. |
| `bhCheckExmAtari(pp)` | hitchk.c:4588 | **`plp`** | Botón de acción. Sondea desde `plp`. Tipos de zona: 0 = puerta (rellena `sys->door` y activa el bit 0x1 de `cb_flg`), 1/2 = escalera o escalón (`sys->pl_htp`), 3 = examinar o cámara fija, 4 = coger item u objeto. |
| `bhCheckEnemies(pp)` | hitchk.c:5788 | Parámetro | Separa a `pp` de los enemigos. Tiene una rama `pp == plp` (:5844). |
| `bhCheckPlayer(pp)` | hitchk.c:5712 | **`plp`** | Lo llaman los enemigos para no atravesar al jugador; solo considera a `plp`. |
| `bhCollisionCheckLine*` | hitchkl.c | Parámetro | Líneas de visión y balas. |
| `bhCheckRoute` | rutchk.c:5 | Parámetro | |

Las escaleras (`kaidan`) y escalones (`dansa`) leen el global `sys->pl_htp` (player.c:2882, 3144, 3939, 4134).

## Objetos e items ([objitm.c](../../src/ps2/veronica/prog/objitm.c))

- `sys->obwp[32]` son los objetos (`O_WRK`): los índices 0-1 son las armas del jugador, el 2 su pelo o accesorio, el 3 está reservado, y los objetos de la sala empiezan en el 4. `sys->itwp` son los items.
- Las cajas que se empujan (`bhObj001`) usan `plp->psh_idx` (objitm.c:1063-1138). Los objetos que giran solo empujan a `plp` (objitm.c:1673-1833).
- El pelo (`bhObjClpn`) ya admite un dueño que no sea `plp` (objitm.c:2146-2155).

## Luces y sombras

- La iluminación es **global y depende del plano de cámara**, no del personaje. `bhControlLight` (light.c:332) elige las luces de `rom->cutp[cam.ncut]`. `bhSetLight` y `bhSetHalfLight` se aplican antes de dibujar enemigos y jugador.
- Una luz con `lkflg == 1` sigue a `plp` (light.c:724-726). El mechero es `rom->lgtp[1]` (player.c:1459-1495).
- **Sombras:** `bhSetShadow(jtb, owner, lkono, …)` (effect.c:644) enlaza un efecto de `eff[]` a cualquier dueño. El jugador la llama en player.c:791. Como `bhClearEffect` borra las sombras en cada carga de sala, hay que recrearlas.

## Dibujo

- `bhPutModel(pw)` (MdlPut.c:17), con la pasada de espejo incluida, y `bhCalcModel(pw)` (MdlPut.c:240) **reciben parámetro**.
- La llamada para el jugador está fija en `bhAllDrawModel` (game.c:357-370).

## Sonido

- `PlayerPos` se toma de `plp` (sdfunc.c:3010-3012).
- `CallPlayerFootStepSe` (player.c:1433) no recibe posición, así que suena donde está `plp`. Existe `CallPlayerFootStepSeEx(…, pPos)` (sdfunc.c:1447), que sí la recibe.

## Interfaz y salud

- No se muestra la vida durante el juego; solo en la pantalla de estado: `StatusInit` (sub1.c:1389) lee `plp->hp`/`stflg`, y `Pulse*` (sub1.c:9075-9275) dibuja el ECG.
- La curación está en `Use_00` (sub1.c:6283).
- La muerte normal lanza el game over poniendo a 0 el bit `0x4000` de `ts_flg` (`bhCPM0_die`, player.c:6424). `gm_flg 0x400` indica otro caso: game over por fin de una cuenta atrás (player.c:1298).
