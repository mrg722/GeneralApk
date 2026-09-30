# District Fury — Informe final DF-013.2

Estado real a la fecha de entrega. Todo lo marcado como verificado fue
compilado (cross-compile Windows con llvm-mingw: `district_fury.exe`,
`combat_smoke_tests.exe`, `application_state_tests.exe`). Nada fue ejecutado
en pantalla: el entorno de trabajo no tenia GPU, display ni Wine.

## Hecho en esta rama

| Area | Estado |
|---|---|
| CombatWorld conectado a los 4 modos (hazards/impactos de specials) | Implementado |
| HUD comun (`ui::DrawPlayerVitals`) en Stage1/2/3/4/5 y VS | Implementado |
| Stage 4 Kessler Tower + Titan-X Mejorado | Implementado |
| Stage 5 Camara del Clon + Rayder Clone | Implementado |
| Sprites reales Titan-X, Titan-X Mejorado, Rayder Clone (sin matte blanco) | Implementado (poses por estado) |
| Sprites reales de Brakk (14 poses) y Grinder (7 poses) | Implementado (poses por estado) |
| Rayden clon seleccionable en VS, Rayden original intacto | Implementado |
| Dr. Kessler NPC narrativo en intros de Stage 4 y 5 | Implementado con sus sprites reales |
| `BossDefinition` + clase `Boss` compartida | Implementado, usado por VS |
| VS con 5 stages, 8 enemigos y 5 bosses | Implementado |
| Codigo legacy sin consumidores retirado | Hecho |

## Pendiente (no esta hecho)

- Encadenamiento automatico de campaña (Clear → siguiente stage), rewards,
  serum/upgrade entre stages, save/load extendido.
- Migrar los bosses de Historia a la clase `Boss` (hoy cada stage mantiene su logica).
- Animacion frame a frame de bosses: hoy cada boss tiene UNA pose por estado.
- Poses que faltan en los sets recibidos: caminar/bloqueo/derribo del clon,
  caminar y correr de Grinder, y 14 de las 17 animaciones de Kessler.
- Build Android (.apk): sin NDK/Gradle en el repo.

## Assets que hacen falta

- Tiras transparentes por animacion (una fila, frames separados) para el ciclo
  de caminar, correr y los frames intermedios de ataque de Brakk, Grinder,
  Rayder, Rayder Clone y ambos Titan-X.
- Resto de animaciones de Kessler (girar, observar, consola, etc.).
- Fondos: faltan ~17 escenarios.

## Compilar

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

raylib se descarga automaticamente (FetchContent) en la primera configuracion.
