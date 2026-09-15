# CS2 Pointer Reference

glennf  27th December 2025 09:28 AM

*Source: [UnknownCheats - CS2 Understanding Offset Chains](https://www.unknowncheats.me/forum/counter-strike-2-a/731242-cs2-understanding-offset-chains-entitylist-money-names-viewmatrix-etc.html?s=57a27889578ec737510bc0efc8e1b3d4)*

---

## Notation

- `base` = `client.dll` base address
- `addr` = read pointer/value at address `addr`
- "handle" = 32-bit integer encoding an index into `dwEntityList`

---

## 1. Core `client.dll` Offsets

| Offset                    | Description                           | Math Example                                                                |
| ------------------------- | ------------------------------------- | --------------------------------------------------------------------------- |
| `dwLocalPlayerPawn`       | Pointer to your pawn (in-world model) | `local_pawn = *(base + dwLocalPlayerPawn)`                                  |
| `dwLocalPlayerController` | Pointer to your player controller     | `local_controller = *(base + dwLocalPlayerController)`                      |
| `dwEntityList`            | Root of global entity list            | `entity_list = *(base + dwEntityList)`                                      |
| `dwViewMatrix`            | 4×4 view-projection matrix (ESP)      | `view_matrix_addr = base + dwViewMatrix`                                    |
| `dwViewAngles`            | Current view angles (pitch/yaw)       | `pitch = *(base + dwViewAngles + 0-4)` `yaw = *(base + dwViewAngles + 1-4)` |

---

## 2. Player Pawn / Controller Fields

**Pawn fields:**

- `m_iHealth`: `health = *(pawn + m_iHealth)`
- `m_iTeamNum`: `team = *(pawn + m_iTeamNum)`
- `m_ArmorValue`: `armor = *(pawn + m_ArmorValue)`
- `m_vOldOrigin`: `pos = *(pawn + m_vOldOrigin)` → Vec3(x, y, z)
- `m_vecVelocity`: `vel = *(pawn + m_vecVelocity)` → Vec3(x, y, z)
- `m_fFlags`: `flags = *(pawn + m_fFlags)`
- `m_bIsScoped`: `scoped = *(pawn + m_bIsScoped) != 0`
- `m_flFlashDuration`: `flash_duration = *(pawn + m_flFlashDuration)`
- `m_lifeState`: `life_state = *(pawn + m_lifeState)`

**Controller fields:**

- `m_iszPlayerName`: `name_ptr = controller + m_iszPlayerName` (read bytes, split at `\0`)
- `m_pInGameMoneyServices` + `m_iAccount`: `money_services = *(controller + m_pInGameMoneyServices)`
- `money = *(money_services + m_iAccount)`
- `m_iObserverMode`: `observer_mode = *(controller + m_iObserverMode)`

---

## 3. Handles and `dwEntityList` (hi/lo logic)

Handles (32-bit) are used for:

- `m_hPawn` (controller → pawn)
- `m_hPlayerPawn` (controller → pawn)
- `m_hOwnerPawn` (weapon → owner pawn)
- `m_hObserverTarget` (observer → spectated entity)

**Handle math:**

```python
index = handle & 0x7FFF
hi = index >> 9
lo = index & 0x1FF

entity_list = *(base + dwEntityList)
list_entry = *(entity_list + 0x10 + 8 * hi)
entity_ptr = *(list_entry + 112 * lo)  # 112 decimal = 0x70 stride
```

This is used in ESP, radar, aimbot, triggerbot, world ESP, glow, etc.

---

## 4. Scene Node, Bones, Position, and Skeleton

- `m_pGameSceneNode` + `m_vecAbsOrigin`:
  - `scene_node = *(pawn + m_pGameSceneNode)`
  - `abs_pos = *(scene_node + m_vecAbsOrigin)`
- `m_pBoneArray` (bones for aimbot/skeleton ESP):
  - `scene_node = *(pawn + m_pGameSceneNode)`
  - `bones_base = *(scene_node + m_pBoneArray)`
  - `bone_pos = *(bones_base + bone_index * 32)`

If bones fail, fallback to: `pos = *(pawn + m_vOldOrigin)`

---

## 5. View Matrix & World-to-Screen

Read a 4×4 float matrix from `dwViewMatrix` and project 3D world points to 2D screen space.

**Matrix read:**

```python
def read_matrix(handle, addr):
        return struct.unpack('f' * 16, read_bytes(handle, addr, 64))
```

**Projection:**

```python
def world_to_screen(matrix, pos, width, height):
        if pos is None:
                return None
        x = matrix[0] * pos.x + matrix[1] * pos.y + matrix[2] * pos.z + matrix[3]
        y = matrix[4] * pos.x + matrix[5] * pos.y + matrix[6] * pos.z + matrix[7]
        w = matrix[12] * pos.x + matrix[13] * pos.y + matrix[14] * pos.z + matrix[15]
        if w < 0.1:
                return None
        inv_w = 1.0 / w
        return {
                "x": width / 2 + (x * inv_w) - width / 2,
                "y": height / 2 - (y * inv_w) - height / 2
        }
```

---

## 6. Names and Money (Controller Chains)

- **Names:**
  - `controller = local or remote player controller pointer`
  - `name_buffer = controller + m_iszPlayerName` (read ~32 bytes, split at `\0`)
- **Money:**
  - `money_services = *(controller + m_pInGameMoneyServices)`
  - `money = *(money_services + m_iAccount)`

---

## 7. Weapons and Weapon IDs

- **Active weapon pointers:**
  - `weapon_services = *(pawn + m_pWeaponServices)`
  - `weapon_ptr = *(pawn + m_pClippingWeapon)`
- **Weapon definition ID:**
  - `attribute_mgr = weapon_ptr + m_AttributeManager`
  - `item_base = attribute_mgr + m_Item`
  - `weapon_id = *(item_base + m_iItemDefinitionIndex)`

---

## 8. Glow (CS2GlowManager)

1. Local team:
        - `local_pawn = *(base + dwLocalPlayerPawn)`
        - `local_team = *(local_pawn + m_iTeamNum)`
2. Entity list root:
        - `entities = *(base + dwEntityList)`
3. Controller → pawn via handle:
        - `pawn_handle = *(controller + m_hPlayerPawn)`
        - `idx = pawn_handle & 0x7FFF`
        - `hi = idx >> 9`, `lo = idx & 0x1FF`
        - `entry = *(entities + 0x10 + 8 * hi)`
        - `pawn = *(entry + 0x70 * lo)`
4. Glow struct:
        - `glow_struct = pawn + m_Glow`
        - `color_addr = glow_struct + m_glowColorOverride`
        - `flag_addr = glow_struct + m_bGlowing`
        - `type_addr = glow_struct + m_iGlowType`

---

## 9. Radar and Triggerbot Chains

**Radar:**

- Read local player info:
  - `local_pawn = *(base + dwLocalPlayerPawn)`
  - `local_team = *(local_pawn + m_iTeamNum)`
  - `local_origin = *(local_pawn + m_vOldOrigin)`
  - `local_yaw = *(base + dwViewAngles + 0x4)`
- Walk controllers with `dwEntityList` using hi/lo:
  - For i in [1..64]:
    - `idx = i & 0x7FFF`
    - `list_entry = *(ent_list + 0x10 + 8 * (idx >> 9))`
    - `controller = *(list_entry + 112 * (idx & 0x1FF))`
- Then `m_hPlayerPawn` handle → pawn, and use `m_iHealth`, `m_iTeamNum`, `m_vOldOrigin` to draw dots on the radar.

**Triggerbot:**

- Start from local pawn: `player = *(base + dwLocalPlayerPawn)`
- Get target index from crosshair: `entityId = *(player + m_iIDEntIndex)`
- Same handle-like math on `entityId`:
  - `entList = *(base + dwEntityList)`
  - `entEntry = *(entList + 0x10 + 0x8 * (entityId >> 9))`
  - `entity = *(entEntry + 112 * (entityId & 0x1FF))`
- Then:
  - `entityTeam = *(entity + m_iTeamNum)`
  - `entityHp = *(entity + m_iHealth)`

---

## 10. BHop Flags

The BHop script reads flags and simulates space:

- `local_pawn = *(base + dwLocalPlayerPawn)`
- `flags = *(local_pawn + m_fFlags)`

Check the "on ground" bit and, if set, send jump input on space hold.

---

## 11. FOV Changer

Works on the controller:

- `controller = *(base + dwLocalPlayerController)`
- `current_fov = *(controller + m_iDesiredFOV)`
- If different from desired value, write new value back to `controller + m_iDesiredFOV`.

---

## 12. Bomb (C4) Info

Bomb ESP uses extra offsets:

- `dwPlantedC4` (read via offsets manager):
  - `c4_handle_addr = base + dwPlantedC4`
  - `c4_ptr = *(c4_handle_addr)`
  - `planted_flag = *(c4_handle_addr - 0x8)` (1 byte)
  - If `planted_flag == 0`, no active bomb.
  - If planted:
    - `c4_class = *(c4_ptr)`
    - `c4_node = *(c4_class + m_pGameSceneNode)`
    - `bomb_pos = *(c4_node + m_vecAbsOrigin)`
    - `timer_length = *(c4_class + m_flTimerLength)`

---

## 13. Spectator List

Combines several of the above offsets:

1. Get local controller & pawn:
        - `local_controller = *(base + dwLocalPlayerController)`
        - `local_pawn_handle = *(local_controller + m_hPawn) & 0x7FFF`
        - `local_pawn = handle → dwEntityList → pawn` (hi/lo math)
2. From an observer pawn:
        - `observer_services = *(observer_pawn + m_pObserverServices)`
        - `target_handle = *(observer_services + m_hObserverTarget) & 0x7FFF`
3. From local controller directly:
        - `observer_mode = *(local_controller + m_iObserverMode)`
        - `observer_target = *(local_controller + m_hObserverTarget)`
4. Use `observer_target` handle to resolve which player you’re spectating via `dwEntityList`.

---

## 14. Quick Reference List (All Offsets)

**Global (`client.dll`):**

- `dwLocalPlayerPawn` – pointer to your pawn
- `dwLocalPlayerController` – pointer to your controller
- `dwEntityList` – global entity list root
- `dwViewMatrix` – 4×4 float view matrix (ESP)
- `dwViewAngles` – view angles (radar, aimbot helper)

**Pawn / Controller Fields:**

- `m_iHealth` – health
- `m_iTeamNum` – team
- `m_ArmorValue` – armor
- `m_vOldOrigin` – pawn position (world)
- `m_vecVelocity` – pawn velocity
- `m_fFlags` – movement flags (bhop)
- `m_bIsScoped` – scoped flag
- `m_flFlashDuration` – flashbang duration
- `m_lifeState` – alive/dead state
- `m_iszPlayerName` – player name string
- `m_pInGameMoneyServices` + `m_iAccount` – money chain
- `m_pGameSceneNode` + `m_vecAbsOrigin` – scene node world position
- `m_pBoneArray` – bone array for skeleton / aimbot
- `m_pWeaponServices` – weapon service (active weapon, etc.)
- `m_pClippingWeapon` – direct active weapon pointer
- `m_hPawn` – controller → pawn handle
- `m_hPlayerPawn` – controller → pawn handle (used a lot)
- `m_hOwnerPawn` – weapon → owning pawn handle
- `m_pObserverServices` + `m_hObserverTarget` – observer system
- `m_iObserverMode` – observer mode (spectating type)

**Weapon / Item Fields:**

- `m_AttributeManager` – weapon attribute manager
- `m_Item` – item struct inside attribute manager
- `m_iItemDefinitionIndex` – actual weapon ID

**Glow Fields:**

- `m_Glow` – base of glow struct on pawn
- `m_glowColorOverride` – ARGB color override
- `m_bGlowing` – enable/disable flag
- `m_iGlowType` – glow type/mode

---

All modules (ESP, world ESP, radar, aimbot, glow, trigger, bhop, FOV, bomb ESP, spectator list) are just different combinations of:

> base + global offset → pointer → struct + member offset → value (or another pointer)

plus the special handle → `dwEntityList` math (`hi`, `lo`, `112` stride).
