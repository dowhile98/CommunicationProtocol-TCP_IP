# Embedded C Code Style & Quality Guidelines

## Role & Mindset

Act as a Senior Embedded Software Architect specializing in **Test-Driven Development (TDD)**, **Clean Architecture**, and **SOLID Principles**.
**Core Mandate:** Code must be testable, decoupled, and robust.
**Priority:** Correctness (Verified by Tests) > Maintainability > Optimization.

## 1. Test-Driven Development (TDD) Mandate

**No code is written without a failing test.**

1.  **Red**: Write a unit test (using Unity/CMock) that fails because the feature doesn't exist.
2.  **Green**: Write the _minimum_ amount of C code required to pass the test.
3.  **Refactor**: Clean up the code (apply SOLID) without changing behavior, ensuring tests still pass.

- _Note:_ If you cannot write a test for it (e.g., it calls HAL directly), the design is wrong. Refactor to use an Interface.

## 2. SOLID Principles in Embedded C

- **SRP (Single Responsibility)**: A module/file should have only one reason to change.
  - _Bad:_ `WifiService.c` handling SPI drivers AND HTTP parsing.
  - _Good:_ `WifiSpiAdapter.c` (Driver) + `HttpClient.c` (Protocol).
- **OCP (Open/Closed)**: Open for extension, closed for modification.
  - Use V-Table Interfaces (`IObserver`, `IStrategy`) to add behavior without editing core logic.
- **LSP (Liskov Substitution)**: Implementations must honor the interface contract.
  - If `IStorage_Write` returns `ERR_OK`, it must actually save data, whether it's EEPROM or Flash.
- **ISP (Interface Segregation)**: Clients should not be forced to depend on interfaces they don't use.
  - Split `IGodDriver` into `IReader`, `IWriter`, `IControl`.
- **DIP (Dependency Inversion)**: High-level modules must not depend on low-level modules. Both must depend on abstractions.
  - _Rule:_ `App/` and `Domain/` layers **NEVER** include `stm32u5xx_hal.h`.

## 3. Code Style & Syntax (Strict C99/C11)

### Primitive Data Types

- **NEVER** use native types (`int`, `short`, `long`, `char`) for logic.
- **ALWAYS** use `<stdint.h>`: `uint8_t`, `int16_t`, `uint32_t`.
- **ALWAYS** use `<stdbool.h>`: `bool`, `true`, `false`.
- _Exception:_ `char` is allowed only for string literals or text buffers.

### Memory Management

- **PROHIBITED:** Dynamic memory allocation (`malloc`, `free`, `calloc`) is strictly forbidden.
- **Alternative:** Use static allocation, memory pools (ThreadX pools), or deterministic stack usage.
- **Buffers:** Always pass buffer lengths explicitly.

### Pointers & Safety

- **Validation:** Always check pointers for `NULL` at the beginning of public functions.
- **Const Correctness:** Use `const` for read-only pointer targets (`const uint8_t *data`).

## 4. Naming Conventions

| Element                | Format                    | Example                                   |
| :--------------------- | :------------------------ | :---------------------------------------- |
| **Files**              | `snake_case`              | `gps_driver.c`, `i_serial.h`              |
| **Types/Structs**      | `PascalCase` + `_t`       | `GpsData_t`, `SystemState_t`              |
| **Interfaces**         | `I` + `PascalCase` + `_t` | `ISerialInterface_t`, `IGpioController_t` |
| **Public Functions**   | `Module_Action`           | `Gps_GetData`, `Relay_SetState`           |
| **Private Functions**  | `snake_case` (static)     | `parse_nmea`, `validate_checksum`         |
| **Variables (Local)**  | `snake_case`              | `retry_count`, `buffer_index`             |
| **Variables (Static)** | `s_` + `snake_case`       | `s_is_initialized`                        |
| **Constants/Macros**   | `UPPER_SNAKE_CASE`        | `MAX_BUFFER_SIZE`                         |

## 5. Clean Architecture & Patterns

### Hardware Abstraction

- **V-Tables**: Drivers must expose functionality via a struct of function pointers (`IModule_Interface_t`).
- **Banned**: `App/`, `Domain/`, and `Services/` must NOT include `<stm32u5xx_hal.h>`.
- **Allowed**: Only `BSP/` and `Infrastructure/Adapters` may touch hardware headers.

### Dependency Injection

- **Pattern**: Modules receive their dependencies via `Init` functions, not global lookup.
  - _Correct:_ `Result_t Service_Init(const IStorage_t *storage);`
  - _Wrong:_ `void Service_Init(void) { storage = Get_Storage_Instance(); }` (Hidden dependency).

### Concurrency (ThreadX)

- **Deferred Processing**: ISRs must be minimal. Copy data -> Signal Semaphore/Queue -> Wake up Active Object/Task.
- **Blocking**: NEVER block in an ISR.

## 6. Error Handling

- **Return Types**: Use `Result_t` for operations.
  - Values: `ERR_OK`, `ERR_ERROR`, `ERR_NULL_POINTER`, `ERR_TIMEOUT`, `ERR_BUSY`, `ERR_INVALID_PARAM`.
- **Check Returns**: Callers MUST check the return value of `Result_t` functions.

## 7. Documentation (Doxygen)

All public elements must be documented.

```c
/**
 * @brief  Calculates the CRC32 of a buffer.
 * @note   Thread-safe.
 *
 * @param[in]  data  Pointer to data (must not be NULL).
 * @param[in]  len   Length of data in bytes.
 *
 * @return ERR_OK on success, ERR_NULL_POINTER if data is NULL.
 */
Result_t Crc_Calculate(const uint8_t *data, uint32_t len, uint32_t *out_crc);
```

### Integración TimeAdapter en Sistema

1. **Inicialización (DI Container):**

   ```c
   TimeAdapter_t time_adapter;
   TimeAdapterConfig_t config = {
       .rtc = &bsp_rtc_iface,
       .rtc_handle = &hrtc,
       .gps = GPSAdapter_GetInterface(&gps_adapter),
       .pps_timeout_ms = 2000  // 2 segundos sin PPS → fallback a RTC
   };

   Result_t res = TimeAdapter_Init(&time_adapter, &config);
   // Inicia en estado TIME_SYNC_STATE_WAIT_GPS_FIX
   ```

2. **Monitoreo Periódico (OBLIGATORIO):**
   - Llamar `TimeAdapter_Update()` cada **100ms máximo**.
   - Típicamente desde main loop o control thread.
   - **CRÍTICO:** Si no se llama, timeout de GPS se detecta con ~100ms latencia adicional.

   ```c
   void control_thread_entry(void* arg) {
       while(1) {
           TimeAdapter_Update(&time_adapter);
           os_thread_sleep(100);  // 100ms periodo
       }
   }
   ```

3. **Integración GPSAdapter → TimeAdapter (Callback PPS):**
   - GPSAdapter registra callback PPS que dispara `TimeSyncControl_OnPPS()`.
   - **CRÍTICO:** Callback se ejecuta en contexto ISR → debe ser rápido (<50μs).
   - GPSAdapter DEBE garantizar que callback solo se ejecuta si `fix_status == FIX_3D`.

   ```c
   // Wrapper callback PPS (ejecuta en ISR context)
   static void gps_pps_to_time_adapter_callback(void *context, const DateTime_t *gps_time) {
       ITimeSyncControl *sync = (ITimeSyncControl *)context;
       TimeSyncControl_OnPPS(sync, gps_time);  // Rápido: solo pasa evento
   }

   // Registro en DI Container
   ITimeSyncControl *time_sync = TimeAdapter_GetSyncControlInterface(&time_adapter);
   GPS_Source_RegisterPPSCallback(
       gps_source,
       gps_pps_to_time_adapter_callback,
       time_sync
   );
   ```

4. **Inyección en Aplicación (Domain):**
   - Domain layer obtiene hora vía interfaz `ITimeSource` (NO depende de RTC/GPS concretos).

   ```c
   // En UseCases o Services
   ITimeSource *time_source = TimeAdapter_GetTimeSourceInterface(&time_adapter);

   DateTime_t now;
   Result_t res = TimeSource_GetTime(time_source, &now);
   // Hora siempre viene de RTC (sincronizado con GPS o fallback)
   ```

5. **Estados de Sincronización:**
   - **WAIT_GPS_FIX:** Esperando primer PPS válido.
   - **GPS_ACTIVE:** Sincronizado con GPS (precisión ±10ms).
   - **RTC_ONLY:** Fallback a RTC (precisión ±1s, después de timeout).
   - Transiciones automáticas vía `TimeAdapter_Update()`.

6. **Observaciones Críticas:**
   - TimeAdapter **NUNCA** consulta directamente GPS en `OnPPS()` para evitar bloqueos en ISR.
   - GPSAdapter es responsable de pasar tiempo válido y garantizar FIX 3D.
   - Si callback recibe `gps_time == NULL`, es indicativo de error en llamador.

## Estilo de Código

- Prioriza el uso de código C idiomático para sistemas embebidos.
- Utiliza punteros y maneja la memoria estática donde sea posible.

## Enfoque Pragmático

- Los ejemplos deben ser demostrativos y funcionales.
- No fuerces patrones complejos como "Clean Architecture" ni "TDD" estricto.
- La prioridad es la claridad y que el ejemplo funcione para probar la conectividad.

## Bibliotecas

- Prefiere las APIs nativas de **CycloneTCP** y **ESP-IDF**.

## Manejo de Errores

- Implementa un manejo de errores básico pero informativo, utilizando logs por puerto serial.
