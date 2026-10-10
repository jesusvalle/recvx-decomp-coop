# Especificación — Cooperativo, hito 2d: inventario de P2, recoger objetos y baúl compartido

- **Fecha:** 2026-10-11
- **Estado:** pendiente de revisión del usuario. Decisiones del usuario: cada jugador recoge los objetos a su propio inventario (también los objetos clave); P1 y P2 se pasan cosas por el baúl general; P2 empieza con el cuchillo (hito 2c).
- **Depende de:** hito 2c (bloque de P2 en `sys->itm[256..]`, arma propia y su cargador por el modo 3 con `mn_md3 = COOP_MN_P2`).
- **Contexto:** [inventory.md](../../architecture/inventory.md) (apertura, `StatusMain`, recoger un objeto, baúles), [input.md](../../architecture/input.md), [events-and-flags.md](../../architecture/events-and-flags.md), [docs/coop/README.md](../../coop/README.md).

## 1. Objetivo

P2 gestiona su propio inventario: lo abre con Start en el mando 2, usa, combina, examina y equipa sus objetos; recoge objetos del mundo con su botón de acción; y comparte objetos con P1 a través del baúl general.

## 2. Comportamiento

1. **Abrir:** Start en el mando 2 abre la pantalla de inventario con los objetos de P2 (`itm[256..]`), su vida y su arma. Como en el original, el juego se pausa para los dos mientras está abierta. La maneja el mando 2.
2. **Usar, combinar, examinar y equipar** funcionan como para P1, sobre P2:
   - curarse cura a P2 (se verá en el hito 3, cuando P2 tenga vida de verdad);
   - equipar un arma la carga con el cargador de P2 (2c);
   - **usar objetos clave** (tipo `0x40`) desde la pantalla de P2 no hace nada, con un mensaje, hasta el hito 4 (resolver puzles con P2): su activador de suelo es el de P1.
3. **Recoger:** P2 pulsa acción (X) delante de un objeto. Se agacha si hace falta y se abre la pantalla de "coger objeto" de P2, con el mando 2. Si acepta, el objeto pasa a su inventario y desaparece del mundo (también para P1). Con el inventario lleno, no lo coge.
4. **Baúl:** P2 abre el baúl general con acción, y la pantalla del baúl muestra el inventario de P2 y el baúl compartido. Los baúles especiales A y B (de la historia) siguen siendo solo de P1.
5. **Lo que el botón de acción de P2 todavía no hace** (hito 4): abrir puertas, subir escaleras, examinar o leer mensajes, y usar escalones o salientes. En esas zonas, acción no hace nada.
6. **Los guiones solo miran el inventario de P1:** un objeto clave que tenga P2 no cuenta para la historia. Para avanzar, P2 lo deja en el baúl y P1 lo coge.
7. **Mochila:** compartida (P2 también es Claire; la mochila es de Claire).
8. **Mapa** desde la pantalla de P2: el de siempre, con los objetos de P1.
9. **Máquina de escribir:** solo P1 (como hoy). La partida guarda también el bloque de P2.

## 3. Ganchos

| Gancho | Dónde | Qué hace |
| --- | --- | --- |
| G2 (amplía) | `coopSetPad2` | Guarda una copia del mando de P2 **sin máscara**. Con la pantalla de P2 abierta, la copia a `sys->pad_*` (la leen la tarea 9 y los mensajes sí/no de la tarea 8) en lugar del de P1. Detecta el Start de P2. Se añade `0x200` (acción) a la máscara de juego |
| G13 | `bhSysCallGame`, alrededor de `bhCheckSubTask` (system.c:558) | `coopPreSubTask()` / `coopPostSubTask()`: inyecta las peticiones de P2 y decide el dueño de la pantalla |
| G14 | `StatusMain`, `case 1`, justo antes de `CursorInit` (sub1.c ≈ 3387) | Si la pantalla es de P2: `st->pip = &sys->itm[256]` (en cada inicialización, también al volver del mapa) |
| G15 | `bhSysCallItemselect` (system.c:913-944) | `plp = &ply2` alrededor de `ItemTaskCheck` si la pantalla es de P2; marca el modo 3 como de P2; detecta el cierre |
| G16 | `ItemUse` (sub1.c:2662) | En la pantalla de P2, los objetos clave muestran un mensaje y no se usan |

## 4. Componentes

### 4.1 Estado nuevo (coop.c)

`coop_inv_owner` (0 = nadie, 1 = P1, 2 = P2); peticiones pendientes de P2 (`coop_p2_req`, bits `cb_flg 0x10/0x20000/0x40000`), con su `sb_id`, `etc_idx` y bit `cb_flg 0x100`; los de P1 guardados mientras dura la pantalla de P2; `coop_box_lid_p2`; la copia sin máscara del mando de P2; `sb_id` en `COOP_SAVE`.

### 4.2 Acción de P2 (dentro de su ventana)

- **Filtro de zonas:** mientras se ejecuta `bhControlPlayer()` de P2 y P2 pulsa acción, se quita temporalmente `flg & 1` (zona activa) a las zonas de `rom->etcp`/`sys->metcp` que no sean de tipo 4 (objeto o baúl general), y a las de tipo 4 de los baúles A/B. Se restaura justo después. Si P1 se está agachando para coger algo o ya hay una petición pendiente, se quita `0x200` del mando de P2.
- **Capturar la petición:** `coopEnd` restaura `cb_flg`, `etc_idx` y `sb_id` de P1, así que antes compara: los bits nuevos `sys->cb_flg & ~coop_save.cb_flg & 0x60010`, junto con `sb_id`, `etc_idx` y el bit `0x100`, se guardan en `coop_p2_req`. Si la petición viene de agacharse (`bhCPM2_act_cro`, que pone `0x10` al terminar), se captura igual en el frame en que aparece.
- **Tapa del baúl:** `bhObjItmBox` corre fuera de la ventana; si la tapa la activó P2 (`stflg 0x18000` de P2 en la zona de la tapa), se recuerda en `coop_box_lid_p2` y su `cb_flg 0x40000` se trata como petición de P2.

### 4.3 Abrir la pantalla de P2 (G13)

Antes de `bhCheckSubTask`, solo si no hay pausa, P2 está visible y no agarrado, no hay petición de P1 en curso (`cb_flg & 0x64010`) ni puerta (`cb_flg & 4`):

- Start de P2: se inyecta `pad_ps |= 0x4000` (y se quita `pad_on 0x80`, para no confundirlo con la pausa).
- Petición de coger o de baúl de P2: se guardan `sb_id`, `etc_idx` y `cb_flg 0x100` de P1 y se ponen los de P2; se inyectan los bits en `cb_flg`.

Después de `bhCheckSubTask`: si se abrió (`st_flg & 0x8`), `coop_inv_owner = 2`, se borra la petición y se para la vibración del mando 2. Si no, se deshace la inyección.

### 4.4 Durante la pantalla de P2 (G2, G14, G15, G16)

- **Mando:** `sys->pad_*` = mando de P2 sin máscara.
- **Inventario:** `st->pip = &sys->itm[256]` en cada inicialización de `StatusMain`; `swork.pip` igual.
- **`plp = &ply2`** solo durante `ItemTaskCheck`/`StatusMain` (tarea 9). Event (8) y Monitor (20) siguen con P1.
- **`etc_idx` de P2** durante toda la pantalla: lo usa `bhItmCk` (tarea 8) para quitar del mundo el objeto cogido. Sin esto, el objeto se duplicaría.
- **Equipar:** si tras `StatusMain` hay `mn_mode0 == 3`, se pone `mn_md3 = COOP_MN_P2` y G11 (2c) carga el arma de P2.
- **Objetos clave:** G16.

### 4.5 Cerrar (G15)

Fin real de la sesión: `(ts_flg & 0x200) && !(st_flg & 0x40000)` tras `ItemTaskCheck` (el paso por el mapa no la cierra). Entonces:

- `ItemTaskCheck` ya ha llamado a `bhStandPlayerMotion()` sobre P2: deja de estar agachado o en acción forzada;
- `swork.pip = &sys->itm[sys->ply_id * 16]`;
- se restauran `sb_id`, `etc_idx` y `cb_flg 0x100` de P1;
- `coop_inv_owner = 0`.

## 5. Errores y robustez

| Situación | Resultado |
| --- | --- |
| P1 y P2 piden pantalla en el mismo frame | Gana P1; la petición de P2 espera al frame siguiente a que se cierre |
| P2 oculto, agarrado o en evento | Su Start y su acción no abren nada |
| Inventario de P2 lleno | Mensaje y no coge (como P1) |
| Cerrar sin restaurar | Imposible por diseño: el cierre es un único punto (G15); si falla, P1 gastaría la munición de P2 (riesgo crítico, cubierto por la revisión) |
| Mechero (id 55) en el inventario de P2 | Puede llevarlo, pero equiparlo deja a P2 sin arma (2c) |

## 6. Verificación

1. Build, `check_build`, identidad sin `COOP`.
2. PCSX2 con la partida del usuario:
   - Start del mando 2 abre el inventario de P2 (cuchillo), con el mando 2; cerrar y volver a jugar; P1 no se ve afectado;
   - P2 recoge un objeto de la sala (con `COOP_TEST`, si la sala no tiene: un objeto de prueba); desaparece del mundo y aparece en el inventario de P2, no en el de P1 (`ramread`);
   - P2 abre el baúl general, deja un objeto; P1 lo saca;
   - P2 equipa otra arma desde su pantalla (con `COOP_TEST`): su arma cambia y la de P1 no.
3. Checklist manual: objetos clave de P2 bloqueados; puertas, escaleras y examinar con P2 no hacen nada; inventario lleno; pausa con la pantalla de P2 abierta; guardar y cargar con objetos en el bloque de P2.

## 7. Fuera de alcance

- Puertas, escaleras, examinar y usar objetos clave con P2 (hito 4).
- Retrato propio de Claire B en la pantalla de estado.
- Mapa con los objetos de P2.
- Pasarse objetos sin baúl.
