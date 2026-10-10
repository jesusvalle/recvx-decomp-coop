# Especificación — Cooperativo, hito 5 (prototipo): pantalla partida

- **Fecha:** 2026-10-11
- **Estado:** pendiente de revisión del usuario. Decisiones del usuario:
  - pantalla partida en dos franjas de 640x240, **siempre** en juego normal;
  - las cinemáticas, a pantalla completa;
  - la primera persona (mira, lanzador lineal) ocupa solo la franja de su jugador;
  - por fases: primero las franjas y la primera persona de P1, después la de P2;
  - se puede elegir una cámara (como ahora) o dos con **L1+L2+R1+R2 en el mando 1**, y la elección se guarda con la partida; la fila del menú de opciones va en otro spec (hito 7).
- **Depende de:** hitos 1-3 (P2 con modelo, arma, inventario y vida propios).
- **Contexto:** [world-systems.md](../../architecture/world-systems.md#cámara) (cámara y dibujo), [game-loop.md](../../architecture/game-loop.md), [events-and-flags.md](../../architecture/events-and-flags.md), [input.md](../../architecture/input.md).

## 1. Objetivo

Un prototipo que responda dos preguntas: si la pantalla partida se ve y se juega bien en Code Veronica, y si la PS2 (y PCSX2) la mueve a 30 fps. Cada jugador ve su propio plano de cámara en su mitad de la pantalla.

## 2. Comportamiento

1. **Dos franjas en juego normal**: arriba P1 (filas 0-239), abajo P2 (240-479).
   - Cada franja es una ventana de 640x240 sobre el plano de su jugador, **a tamaño real** (sin zoom).
   - La ventana se desplaza en vertical para mantener a su jugador centrado y no sale nunca del encuadre original de 640x480. Así no se ven zonas de la sala sin modelar.
2. **Cada jugador tiene sus planos.** P2 cambia de plano según su posición, con las mismas zonas, las mismas cámaras que siguen y las mismas transiciones que P1.
3. **Todos se ven en las dos franjas**: jugadores, enemigos, efectos y objetos.
4. **Pantalla completa, con la cámara de P1 como el original**, cuando se cumpla cualquiera de estas condiciones:
   - la opción está en «una cámara»;
   - cinemática (`cb_flg & 0x4`);
   - cámara de guion, de evento o de examinar (`st_flg & 0x1`);
   - bandas de cine (`cine_an > 0`);
   - puerta (`cb_flg & 0x1`);
   - render a textura de pantalla completa (`gm_flg & 0x100`);
   - P2 no cargado, oculto (`coop_hidden`, por ejemplo con Chris), muerto (`coopP2Dead`) o en una demo (`coopDemo`);
   - la sala tiene más luces de las que caben en la copia de §4.5.

   El paso entre completa y partida es un corte seco.
5. **Encima de las dos franjas, una sola vez:** mensajes, fundido, termómetro y una línea divisoria negra de 2 px en la fila 240.
6. **Primera persona:** fase 1, la de P1, en su franja; fase 2, la de P2, en la suya. Su propio cuerpo no se dibuja en su franja, como en el original, y en la otra franja se le ve en tercera persona. Con «una cámara», P2 no entra en primera persona: apunta en tercera.
7. **Alternar una o dos cámaras:** L1+L2+R1+R2 del mando 1 a la vez, al pulsar el último de los cuatro. Solo en juego normal, no con menús, mensajes ni cinemáticas. La elección se guarda en el bloque de P2 y viaja con la partida, el reintento y la tarjeta. El valor inicial es «dos cámaras».
8. **Medidor (solo en el prototipo):**
   - contadores de frames por número de vsync, que se leen desde un savestate;
   - un cuadradito rojo de 8x8 en la esquina superior derecha mientras el último frame haya pasado de 2 vsync.

## 3. Activación y ganchos

`COOP_SPLIT` es un define nuevo en `"defines"` de `compile_config.json` y requiere `COOP`. Sin él, el cooperativo queda exactamente como ahora. Al cambiarlo hay que borrar `build/src/`.

| Gancho | Dónde | Qué hace |
| --- | --- | --- |
| G29 | `bhMainSequence` (game.c), justo después del `if/else` que llama a `bhCheckCut` | `coopUpdateCamera2()` |
| G30 | `bhMainSequence`, bloque de dibujo (`if (i == (sys->loop_ct - 1))`) | `if (coopSplitDraw() == 0) { bloque original }` |
| G31 | `Ps2SwapDBuff` (ps2_NaSystem.c), antes del `while (Ps2_vcount < 2)` | `coopFrameStat(Ps2_vcount)` |
| G32 | Fase 2: `coopBegin`/`coopEnd` (coop.c) | Poner y capturar los bits de primera persona de P2 (§5) |

Los ganchos de game.c y ps2_NaSystem.c van tras `#ifdef COOP_SPLIT`. La combinación de botones se lee desde `coopUpdateCamera2()`, así que no necesita gancho propio.

## 4. Componentes (fase 1)

Archivo nuevo `src/ps2/veronica/prog/coopcam.c` + `include/ps2/veronica/prog/coopcam.h`, añadido a `"source_files"`. Todo va dentro de `#ifdef COOP_SPLIT`.

### 4.1 Estado

| Global | Qué guarda |
| --- | --- |
| `CAM_WORK coop_cam2` | La cámara de P2. Es una global nueva: no toca `SYS_WORK` ni `BH_PWORK`. |
| `int coop_cam2_ok` | 0 si hay que reiniciar la cámara de P2 con `bhCheckCut(1)` (al activarse la partida, en una sala nueva o al volver de pantalla completa). |
| `int coop_split` | Si este frame va partido. Lo calcula G29 al final del frame y lo usa G30 en el siguiente, igual que el dibujo de P1 usa la cámara que dejó el `bhCheckCut` del frame anterior. Por eso la pantalla completa llega un frame después de que empiece una cinemática. |
| `float coop_band[2]`, `int coop_band_ok[2]` | Desplazamiento vertical `k` de cada franja (§4.4). |
| `itm[COOP_ITM + 19]` (`itm[275]`) | La opción: 0 = dos cámaras, 1 = una. Está libre: la firma, la vida y el veneno ocupan `272-274`. El 0 es el valor de las partidas que ya existen y del bloque recién sembrado, por eso «dos» es el inicial. |

### 4.2 `coopSplitWanted()`

Devuelve 1 si no se cumple ninguna condición de pantalla completa de §2.4. Se evalúa en G29, después de que P1 haya elegido su plano.

### 4.3 `coopUpdateCamera2()` (G29)

1. Lee la combinación (§4.7).
2. `coop_split = coopSplitWanted()`. Si es 0: `coop_cam2_ok = 0`, `coop_band_ok[] = 0`, y termina.
3. Guarda `cam`, `plp` y el estado global que toca un cambio de plano:
   - `sys->gm_flg`, `st_flg` y `pt_flg`;
   - `sys->fog_ct`, `fil_no`, `fil_rt` y `ef_flg`;
   - `rom->fog_col`, `fog_nr` y `fog_fr`;
   - `GameNear`/`GameFar` y el volumen de recorte (`ClipVolume`).
4. Si `coop_cam2_ok == 0`, `coop_cam2` parte de una copia de la `cam` de P1. Después, `cam = coop_cam2`, `plp = &ply2`, y se quitan de `gm_flg` los bits de primera persona y de plano fijo de P1 (`0x40`, `0x80`, `0x1000`, `0x2000`). Llama a `bhCheckCut(1)` si `coop_cam2_ok == 0` y a `bhCheckCut(0)` si no. Después, `coop_cam2_ok = 1`.
5. `coop_cam2 = cam` y restaura todo lo guardado en el paso 3.
6. `coopApplyCamState()` con la `cam` de P1, para que el estado de dibujo vuelva a ser el de su plano.

No se cambia el mando, porque la cámara solo lee la posición de `plp`. No se usa `coopBegin`, que restaura `cam` y hace más cosas.

### 4.4 `coopApplyCamState()` y seguimiento vertical

`coopApplyCamState()` aplica el estado de dibujo que depende del plano de la `cam` actual:

- **Mallas y luces ocultas:** `bhSetHideObjLgt(cam.ncut)`. Si es una vista en primera persona, en cambio, lo muestra todo, como `bhInitPlEyeCamera` (cut.c:2621).
- **Niebla:** la de `cam.fog_*`.
- **Recorte de vista y volumen:** si `sys->ts_flg & 0x200`, `bhChangeViewClipRM()` y `bhChangeClipVolumeRM()`, con la misma condición que `bhSetCut`.

**Desplazamiento `k` de cada franja:**

- Se proyecta el punto que sigue la cámara (`gpx`, `gpy + ci->h`, `gpz`, el mismo que `bhInitActiveCamera` pone en `cam.ply`) con la matriz de la cámara de la franja y la distancia de pantalla. La fila del plano de 640x480 que sale es `Y`.
- Objetivo: `clamp(Y - 120, 0, 240)`.
- Cada frame `k` se acerca 1/4 al objetivo. Salta directamente al objetivo si `coop_band_ok == 0` o si el plano ha cambiado.
- En primera persona, `k = 120` fijo.
- La función de proyección se escribe en `coopcam.c` con `njCalcPoint` y `_nj_screen_`.

### 4.5 Luces

`bhControlLight` anima las luces (parpadeos, giros) cada vez que se llama. En la pasada de P2:

1. se copia `rom->lgtp[0..lgt_n)` en un buffer estático (`LGT_WORK`, 0xE0 B por luz);
2. se llama a `bhControlLight` y a `bhSetLight` con la cámara de P2;
3. se dibuja;
4. se restaura la copia.

El tamaño del buffer se fija con el máximo de `lgt_n` en las 205 salas, que el plan medirá. Si una sala lo supera, va a pantalla completa (§2.4).

### 4.6 `coopSplitDraw()` (G30)

Si `coop_split == 0`, devuelve 0. Si no:

1. **Monitores:** si `gm_flg & 0x200`, `bhDrawSmallScreenRenderTexture()` una vez, como el original.
2. **Franja de P1:** su `cam` y su estado ya están aplicados.
   - `njSetScreen` de 640x480 con `cy = 240 - k1` (§4.8) y recorte `njUserClipping(2, {0,0}-{640,240})`;
   - `pt_flg 0x1` según §2.6;
   - `bhAllDrawModel()` y, si `st_flg & 0x100`, la pasada de espejo del bloque original;
   - si P1 está en primera persona con mira (`st_flg & 0x800000`), `bhDrawScope()` en la franja (§4.8);
   - `SyncPath`, `Ps2DrawOTag` y `Ps2ClearOT`, como hace el inventario (sub1.c:3540-3580).
3. **Franja de P2:**
   - guarda `cam`; pone `cam = coop_cam2`, `bhControlCamera()`, `coopApplyCamState()` y las luces de §4.5;
   - `cy = 480 - k2` y recorte `{0,240}-{640,480}`;
   - el mismo dibujo y envío que la franja de P1, con el `pt_flg 0x1` que le corresponda.
4. **Restaurar:**
   - `cam`, `bhControlCamera()`, `njSetScreen` de 640x480 centrado, `Ps2CalcScreenCone()` y `njUserClipping(0, …)`;
   - `coopApplyCamState()`, la copia de las luces, `bhSetLight()` y `pt_flg`.
5. **Superposición:** la línea divisoria, `bhDrawThermometer()` si `st_flg & 0x40000000`, `bhDrawScreenFade()` si `fade_an > 0` y el cuadradito del medidor. Devuelve 1.

### 4.7 Combinación de botones

- Se leen los botones físicos del mando 1 desde `sys->p1per`. En libpad, L2/R2/L1/R1 son `0x1/0x2/0x4/0x8`; el plan comprobará qué campo y qué máscara da `p1per`.
- Si los cuatro están pulsados y alguno se ha pulsado en este frame, alterna `itm[COOP_ITM + 19]`.
- Solo en juego normal: sin `st_flg & 0x1C040208` (subpantallas y mensajes), ni `cb_flg & 0x5`, ni `st_flg & 0x4`.
- Al pasar a «dos», `coop_cam2_ok = 0`.

### 4.8 Desplazar la imagen de la franja

`njSetScreen` (ps2_NaView.c:27) mueve el 3D: `fNaViwOffsetY = cy + 1808`. El 2D (la mira de `bhDrawScope`, screen.c:294) usa coordenadas fijas de 640x480 y no se mueve.

**Primera tarea del plan (prueba en PCSX2):** cambiar a mitad de frame el registro `XYOFFSET_1` del GS, con un paquete como el de `njUserClipping` y después de enviar la OT. Ese registro mueve el 3D y el 2D por igual.

- Si funciona, el desplazamiento de las franjas se hace con `XYOFFSET` y `njSetScreen` se queda centrado.
- Si no, `njSetScreen` mueve el 3D, y la mira se dibuja recortada a su franja con `k = 120`. *(Implementado: sin la prueba, se usa `njSetScreen` para el 3D y `XYOFFSET` solo alrededor de la mira, que así queda centrada en su franja; ver el registro de ejecución.)*

### 4.9 Medidor (G31)

- `coopFrameStat(vc)` recibe `vc`, el número de vsync que ha costado el frame, y suma 1 a uno de los 4 contadores de `coop_fstat`: `[0]` = 2 o menos, `[1]` = 3, `[2]` = 4, `[3]` = 5 o más.
- Actualiza `coop_fmax` y `coop_fslow = (vc > 2)`.
- Se reinicia al cambiar de sala, para medir sala a sala.
- Cuenta tanto con una cámara como con dos, para tener la referencia.

## 5. Primera persona de P2 (fase 2)

- **Bits propios:** `coop_pe2` guarda los bits que P2 deja al terminar su `bhControlPlayer`: `gm_flg 0x40/0x80/0x800/0x2000/0x80000`, `st_flg 0x800000` y si ha quitado `pt_flg 0x1`.
  - `coopEnd` los captura, como ya hace `coop_gm2` con el arma vacía y el crítico.
  - `coopBegin` los pone (G32).
  - Así `bhCPM2_act_scp` (playpch2.c) y el resto del código de mira ven el modo de P2.
- **Cámara de ojos en `coop_cam2`.** En `coopUpdateCamera2()`, con `cam = coop_cam2`, `plp = &ply2` y los bits de `coop_pe2` puestos, se repite lo que `bhMainSequence` hace para P1:
  - si `0x2000`: `bhInitPlEyeCamera()`;
  - en primera persona (`0x40`): `bhSetPlEyeCamera()` cada frame;
  - si no, `bhCheckCut`, que además hace la transición de vuelta (`bhControlPlEyeCamera`).

  Lo que esas funciones cambian en los bits se guarda en `coop_pe2`, y después se restaura el estado de P1 (§4.3).
- **`cam.pe_ax`/`cam.pe_pers`:** `bhCPM0_action` y el disparo (player.c:1789-1795, 4693, 6131) los escriben dentro del update de P2. `coopEnd` los copia a `coop_cam2` antes de restaurar `cam`.
- **Dibujo en la franja de P2 en primera persona:**
  - sala completa en una pasada (`gm_flg 0x80` durante la pasada);
  - sin el cuerpo de P2;
  - `bhDrawScope()` con la textura del arma de P2: durante la llamada, `sys->obwp[0]` se intercambia con `coop_wpn[0]`, como hace `coopSwapWeaponObj`.
- **Con «una cámara»:** `coopEnd` quita de `coop_pe2` la petición `0x2000`, así que P2 apunta en tercera persona.
- **Prueba:** una variante de `COOP_TEST` que da a P2 un arma de mira con munición. El plan elige el id.

## 6. Errores y robustez

- **Efectos secundarios de `bhCheckCut` para P2:** todo lo que no esté en la lista de §4.3 aparecerá en la franja de P1 (parpadeos de niebla, mallas o luces). Se corrige ampliando la lista.
- **Rendimiento:** si una sala no llega a 30 fps, el juego se ralentiza, porque la lógica avanza un paso por frame dibujado. El prototipo lo mide, sin intentar resolverlo. Opciones para después, de menor a mayor coste: subir el ciclo del EE en PCSX2, quitar el espejo en la franja de P2, o poner esa sala a una cámara.
- **Efectos calculados para la cámara de P1 durante la actualización** (billboards, destellos de luz, light.c:803): pueden verse mal en la franja de P2. Si aparecen, se apuntan como limitación.
- **Cambio de sala:** `coop_cam2_ok = 0` y `coop_band_ok[] = 0` en `coopRoomStart`. La cámara de P2 se reinicia con `bhCheckCut(1)`.
- **Game over, reintento y carga:** en esos momentos no hay juego normal, así que va a pantalla completa. La opción se lee del bloque de P2 recién cargado.

## 7. Verificación

1. **Identidad:**
   - sin `COOP`, objdiff idéntico al original;
   - con `COOP` y sin `COOP_SPLIT`, el mismo ejecutable que antes de este hito (salvo los objetos no deterministas de CLAUDE.md).
2. **Franjas:** separarse por una sala de varios planos. Cada franja cambia de plano con su jugador, las cámaras que siguen siguen a cada uno y no se ven zonas sin modelar al subir o bajar la ventana.
3. **Pantalla completa:** cinemática, puerta, examinar con cámara, P2 oculto (cambio a Chris) y P2 muerto. Vuelve a partida al terminar.
4. **Opción:** L1+L2+R1+R2 alterna. Se guarda en la máquina de escribir y vuelve al cargar y al reintentar.
5. **Salas especiales:** una con espejo y una con monitores.
6. **Primera persona:** la de P1 en su franja (fase 1) y la de P2 en la suya con la variante de `COOP_TEST` (fase 2). Al salir, cada cámara vuelve a su plano.
7. **Medidor:** leer `coop_fstat`/`coop_fmax` con `ramread.py` desde un savestate en salas normales y pesadas, con una y con dos cámaras.

## 8. Documentación al terminar

- `docs/coop/README.md`: estado del hito 5, ganchos G29-G32, decisiones nuevas, y la fila del menú de opciones en el hito 7.
- `docs/architecture/world-systems.md`: la cámara de P2, y el resultado de la prueba de `XYOFFSET`.
- `CLAUDE.md`: `COOP_SPLIT` y `coopcam.c` en la sección del cooperativo.

## 9. Fuera de alcance

- Partida dinámica (pantalla completa cuando los dos se ven desde un plano).
- La fila «Cámara: 1 / 2» del menú de opciones (otro spec, con la de activar el cooperativo).
- Color de fondo distinto por franja: unas pocas salas lo cambian por plano; se usa el de P1.
- Sonido según la cámara de P2.
- Monitores vistos desde la cámara de P2.
- Resolver la ralentización si la hay.
