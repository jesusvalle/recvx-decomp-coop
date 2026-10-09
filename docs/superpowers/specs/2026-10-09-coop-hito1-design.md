# Especificación — Cooperativo, hito 1: P2 aparece y anda

- **Fecha:** 2026-10-09
- **Estado:** aprobada en conversación sección por sección; el usuario revisará spec, plan e implementación al final.
- **Contexto:** [docs/coop/README.md](../../coop/README.md) (decisiones D1-D4) y [docs/architecture/](../../architecture/README.md).

## 1. Objetivo

Un segundo personaje (P2), controlado con el mando 2, aparece junto a Claire cuando ella recibe el control en la celda (stage 0, sala 0) y se mueve por el juego a su lado. Es la base técnica (segunda instancia del jugador, mando 2, cambio de contexto) sobre la que se construirán los hitos siguientes.

## 2. Comportamiento

### 2.1 Lo que hace P2

1. **Aspecto:** P2 es un **clon del personaje actual de P1** (Claire). Usa el mismo modelo, texturas y animaciones. Si P1 cambia de personaje, P2 se vuelve a clonar.
2. **Presencia:** existe siempre en el build `COOP`. Sin mando 2 conectado, se queda quieto. El mando 2 se puede conectar y desconectar en cualquier momento.
3. **Controles:** con el mando 2, P2 anda, corre, gira y hace el giro rápido de 180°. Solo pasan los bits lógicos `0x1`, `0x2`, `0x4`, `0x8` y `0x400`; apuntar, disparar, acción/examinar y menús quedan anulados.
4. **Eventos.** Hay dos niveles (§4.4):
   - **Ocultar y recolocar:** cinemáticas, puertas, y P1 escondido o controlado por guion. P2 no se dibuja (ni él ni su sombra), no se actualiza y se recoloca junto a P1 en cada frame. Por eso aparece al lado de Claire al terminar la intro, una cinemática o una puerta.
   - **Congelar:** mensajes en pantalla y momentos en que el juego no actualiza al jugador. P2 sigue visible donde estaba, pero no se mueve. Así no salta junto a Claire cada vez que ella examina algo.
5. **Colisión:** P2 choca con las paredes igual que P1. P2 no se superpone con P1: si se acercan a menos de la suma de sus radios, **solo P2** se desplaza hacia fuera (P1 empuja a P2; P2 no mueve a P1).
6. **Invulnerabilidad:** la vida de P2 se fija antes de cada actualización, así que P2 no puede morir ni lanzar el game over.

### 2.2 Limitaciones aceptadas

- La cámara sigue solo a P1, y P2 puede salirse de plano.
- Los enemigos ignoran a P2.
- Las pisadas de P2 suenan en la posición de P1.
- P2 no tiene el objeto del pelo (la coleta). El pelo de Claire es `sys->obwp[2]`, enganchado al hueso 5 de P1.
- P2 no abre puertas, no recoge objetos, no recibe daño y no se guarda en la partida.

## 3. Estructura

### 3.1 Archivos

- **Nuevos:** `src/ps2/veronica/prog/coop.c` e `include/ps2/veronica/prog/coop.h`. El contenido entero de ambos va dentro de `#ifdef COOP`.
- **Bloque nuevo en `ps2_sg_pad.c`:** un `#ifdef COOP` al final con la lectura del puerto 2 (`Pad_status2`, `Coop_pad_read2`, `coopGetPeripheral2`).
- **`compile_config.json`:** se añade `src/ps2/veronica/prog/coop.c` a `source_files` (hereda MWCC con `sdata 0`) y `COOP` a `defines`.
- **Build sin cooperativo:** se quita `COOP` de `defines`. Todas las funciones del juego deben quedar idénticas a las del build sin cambios (se comprueba con objdiff, §6).

### 3.2 Ganchos en el código original

Cada gancho es una sola línea dentro de `#ifdef COOP`.

| # | Archivo y función | Posición | Llamada |
| --- | --- | --- | --- |
| G1 | ps2_sg_pad.c `pdGetPeripheral` | Al principio | `if (port == 1) return coopGetPeripheral2();` |
| G2 | system.c `bhSysCallPad` | Tras el bloque que llama a `bhSetPad()` | `coopSetPad2();` |
| G3 | player.c `bhInitPlayer` | Justo antes de `sys->mempb = sys->memp;` | `coopInitMemory();` |
| G4 | dread.c `bhReadPlayerData` | Al final | `coopCloneModel();` |
| G5 | room.c `bhFinishRoom` | Al final | `coopRoomStart();` |
| G6 | game.c `bhMainSequence` | Tras `bhControlPlayer();` | `coopControlPlayer2();` |
| G7 | game.c `bhAllDrawModel` | Tras el bloque que dibuja a P1 | `coopDrawPlayer2();` |

No se modifica ningún struct existente (`SYS_WORK`, `BH_PWORK`, etc.): el estado nuevo vive en `coop.c` (decisión D4).

## 4. Componentes

### 4.1 Mando 2

- **`coopGetPeripheral2()`:**
  - Vive en un bloque `#ifdef COOP` al final de `ps2_sg_pad.c` y no en `coop.c`, porque necesita `Pad_set`, que es `static`.
  - Se ejecuta una vez por frame, con su propia comprobación de `Ps2_sys_cnt`.
  - Mantiene su propia máquina de conexión para el puerto 1 (`scePadGetState(1,0)`, paso a modo analógico, `scePadRead(1,0,Pad_rdata2)`), copiada de `Ps2_pad_read` y parametrizada.
  - Procesa los datos con `Pad_set(&Ps2_pad.pad2, 2)`.
  - Construye un `PDS_PERIPHERAL` estático propio: `on`, `press`, `release`, `l`, `r`, y `x1`/`y1` = byte − 128, o centrados si el mando no es analógico.
  - Devuelve `NULL` si no hay mando estable.
  - No llama a `Ps2_Read_Key` ni a la vibración.
- **`coopSetPad2()`:**
  - Si `sys->ss_flg & 0xC00000` (demo) o `!(sys->sp_flg & 0x20)`, deja a cero el mando de P2.
  - Si no:
    1. intercambia los campos de mando de `sys` con el estado de P2 (`pad_on`, `pad_oncpy`, `pad_ps`, `pad_rs`, `pad_old`, `pad_onb`, `pad_psb`, `pad_oldb`, `pad_ax`, `pad_ay`, `pad_dx`, `pad_dy`, `pad_ar`, `pad_al`);
    2. guarda `pd_port` y `sys->p1per`;
    3. pone `pd_port = 1` si hay mando 2 o `-1` si no;
    4. llama al `bhSetPad()` original;
    5. restaura `pd_port` y `sys->p1per` y deshace el intercambio.
  - Al final, enmascara `on/oncpy/ps/rs/old` de P2 con `0x40F`.

### 4.2 Memoria

- **`coopInitMemory()`** reserva con `bhGetFreeMemory` un bloque persistente, situado por debajo de `mempb`, que sobrevive a los cambios de sala:

  | Pieza | Tamaño |
  | --- | --- |
  | `exp0` (`EXP_WORK`) | 0x7C |
  | `exp1` | 124 B |
  | Pool de clonado | 64 KB |

  `exp3` no se reserva: la auditoría confirmó que el update del jugador nunca lo lee (solo lo usa el objeto del pelo), así que `ply2.exp3 = NULL`.

- La `exp2` de P2 (`PP_WORK`) la reserva `PlyPchInit(&ply2)` en la memoria de la sala, en cada `coopRoomStart`.
- El propio `BH_PWORK ply2` y el estado de su mando son variables estáticas de `coop.c` (unos 1.5 KB de BSS).

### 4.3 Clonado del modelo

`coopCloneModel()` se ejecuta tras cada `bhReadPlayerData` y trabaja así:

1. Vacía el pool.
2. Para cada `i < ply.mdl_n`:
   - copia `ply.mdl[i]` (`ML_WORK`);
   - copia el array `objP` (`obj_num` × `NJS_CNK_OBJECT`) al pool y **reubica `child` y `sibling`** (un puntero no nulo dentro del array original pasa a apuntar a la misma posición de la copia);
   - copia el array `owP` (`obj_num` × `O_WORK`).
3. Copia los punteros compartidos `skp[]`, `mbp[]`, `txp[]` y `mdl_n`.
4. Si el pool no basta, marca P2 como desactivado y lo avisa con `printf`.

### 4.4 Update por frame (`coopControlPlayer2`)

1. Si P2 está desactivado, no hace nada.
2. **Condición de ocultar.** Se cumple si alguna de estas es cierta:
   - `ply.stflg & 0x1000000`
   - `sys->cb_flg & 0x5` (puerta o cine)
   - `ply.mode0 == 7` (control por guion: `bhLoadWork` siempre lo pone)

   No se usa `ply.flg & 0x10000`: el juego lo pone también en la animación de espera, al empujar y al recibir daño. En la prueba hacía desaparecer a P2 cuando P1 se quedaba quieto.

   Si se cumple, P2 se oculta (`ply2.stflg |= 0x1000000`, lo que también esconde su sombra) y se recoloca junto a P1 dentro del contexto protegido. Fin del frame.
3. Al dejar de cumplirse, P2 se vuelve visible.
4. **Condición de congelar.** Se cumple con `sys->st_flg & 0x200` (mensaje) o `!(sys->sp_flg & 0x1)`. Si se cumple, P2 no se actualiza y se acaba el frame.
5. Si no, P2 se actualiza dentro del **contexto protegido** (`coopBegin`/`coopEnd`). Este contexto guarda y restaura `sys->st_flg`, `cb_flg`, `gm_flg`, `pt_flg`, `flr_idx`, `etc_idx`, `pl_htp`, `door` y `cam` entero; además intercambia el mando y pone `plp = &ply2`. Es la lista que salió de la auditoría del árbol de llamadas de `bhControlPlayer`, documentada en el plan. Pasos:
   1. Fija `ply2.hp`, pone `ply2.psh_ct = 0` y quita el bit 0x80 de `ply2.stflg`. Así se impide el empuje automático de cajas, que acabaría poniendo `mode3 = 6` a P1 desde el objeto caja.
   2. Ejecuta `bhControlPlayer()`.
   3. Vuelve a anular el empuje de cajas.
   4. Aplica el empuje P1 → P2 (§2.1.5) con `bhCheckWallEx` y recalcula el modelo.
6. **Todas las llamadas a `bhCheckWallEx` sobre P2 se hacen dentro del contexto protegido.** La función aplica el daño de las paredes peligrosas a `plp`, y fuera del contexto se lo aplicaría a P1.

### 4.5 Inicialización por sala y colocación

- **`coopRoomStart()`**, si P2 está activo:
  1. Pone a cero `ply2` y aplica el subconjunto inocuo de `bhSetPlayer`:
     - flags `0x119` y `mdflg 0x20`;
     - `ar`/`ah`/`car`/`cah` de `PlyInfo[ply_id]`;
     - punteros `exp` propios y `mtx = mtxbuf`;
     - escalas a 1;
     - `wpnr_no = 0`;
     - `clp_jno` y `cpcl` como P1;
     - `exp1` inicial;
     - `owP[7]` y `owP[11]` con el flag 0x8;
     - animación de reposo (`PlMtnAct[0][0][0]`, `mtn_tp = PlyFlip`);
     - `PlyPchInit(&ply2)`.
  2. **No** llama a `bhPushGameData`, ni al pelo, ni a `bhCheckCut`, ni a `bhCheckMothEgg`, ni a `bhCheckSubPack`.
  3. Crea su sombra con `bhSetShadow(NULL, &ply2, 1, 4.5f, 4.0f, 3.5f)` y pone `mode0 = 1`.
  4. Lo coloca junto a P1.
- **Colocación junto a P1:**
  - candidato a 2 × `ar` a la derecha de P1 según `ply.ay`, a la altura del piso de P1;
  - se valida con `bhCheckWallEx` desde la posición de P1 hacia el candidato; si choca, se usa la posición de P1;
  - copia `flr_no`, inicializa los campos de posición de `EXP_WORK` como `bhFinishRoom` (`spx`/`plx`/`bpx`…), pone el ángulo igual al de P1, la animación de reposo, y llama a `bhCalcModel`.

### 4.6 Dibujo

- **`coopDrawPlayer2()`:** si P2 está activo, no oculto, y se cumplen `sys->pt_flg & 0x1` y `!(ply2.mdflg & 1)`, dibuja con `bhCheckClipModel(&ply2) == 0` → `bhPutModel(&ply2)`, igual que P1.
- **Sombra:** el efecto de sombra (`bhEff001`, effsub1.c:718-722) se oculta solo cuando su dueño tiene `stflg & 0x1000000`, el mismo bit que oculta a P2. Se recrea en cada sala, porque `bhClearEffect` borra `eff[]` al cargar.

## 5. Errores y robustez

| Situación | Comportamiento |
| --- | --- |
| Mando 2 ausente o desconectado | P2 quieto; se reconecta solo |
| Pool de clonado insuficiente | P2 desactivado con aviso; el juego sigue normal |
| Falta memoria al reservar | `bhGetFreeMemory` devuelve `NULL`: P2 se desactiva con aviso. Se mide con `malloc area`; la mitigación es recortar el pool |
| Cambio de personaje de P1 | Se vuelve a clonar (G4) y se reinicia en la siguiente sala (G5) |
| Cargar partida | No cambia el formato; P2 reaparece junto a P1 |
| Modo demo | P2 sin entrada; `bhSetPad` no se invoca para P2 |

## 6. Verificación

1. **Build `COOP`:** compila y enlaza sin errores nuevos.
2. **Build sin `COOP`:** objdiff (o la comparación de los objetos) confirma que las funciones del juego son idénticas a las del build sin cambios.
3. **Prueba manual en PCSX2 con dos mandos:**
   1. intro y aparición de P2;
   2. controles independientes;
   3. botones bloqueados;
   4. paredes y empuje;
   5. escena de Rodrigo (P2 oculto y reaparece);
   6. puertas y varias salas;
   7. pausa, inventario y mapa;
   8. desconectar y reconectar el mando 2;
   9. cargar partida;
   10. `malloc area` con la consola del EE.

## 7. Fuera de alcance

Disparar, daño, IA de enemigos contra P2, cámara cooperativa, pelo de P2, otro personaje como P2, puertas o items activados por P2, guardado de P2.
