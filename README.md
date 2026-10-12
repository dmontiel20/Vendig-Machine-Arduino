# Controlador de Máquina Expendedora en Sistemas Empotrados y de Tiempo Real

[![Platform: Arduino](https://img.shields.io/badge/Platform-Arduino%20UNO-blue.svg)](https://www.arduino.cc/)
[![Language: C++](https://img.shields.io/badge/Language-C%2B%2B-00599C.svg)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Diseño e implementación del firmware de control para una máquina expendedora de café y bebidas calientes sobre microcontrolador ATmega328P (**Arduino UNO**). 

El proyecto aplica conceptos clave de **Sistemas Empotrados y de Tiempo Real**: arquitectura basada en **Máquina de Estados Finitos (FSM)**, multitarea periódica/cooperativa mediante hilos de ejecución, temporización por hardware (`TimerOne`), rutinas de interrupción (ISR) no bloqueantes y tolerancia a fallos mediante **Watchdog Timer (WDT)**.

---

## 🛠️ Componentes Hardware

| Componente | Rol en el Sistema | Tipo de Señal / Bus |
| :--- | :--- | :--- |
| **Arduino UNO** (ATmega328P) | Núcleo de procesamiento y temporización | MCU / GPIO / Timers |
| **Display LCD 16x2** | Interfaz visual del usuario y del modo de administración | Bus paralelo (LiquidCrystal) / I2C |
| **Joystick Analógico (XY + SW)** | Navegación por catálogo y modificación de valores | 2x ADC (A0/A1) + Entrada Digital Pull-up |
| **Sensor DHT11** | Monitorización de temperatura y humedad ambiente | Bus digital 1-Wire propietario |
| **Sensor Ultrasonidos (HC-SR04)** | Detección de proximidad/presencia de clientes | Digital (Trigger / Echo por tiempo de vuelo) |
| **Pulsador Externo** | Control temporal: Reset de servicio (2-3s) y Modo Admin (≥5s) | Digital con detección por interrupción / Debounce |
| **LED1 (Digital)** | Indicador de arranque / Diagnóstico en modo Admin | Salida digital GPIO |
| **LED2 (PWM)** | Indicador progresivo analógico de dispensación | Salida PWM analógica (`analogWrite`) |

---

## ⚙️ Arquitectura Software y Máquina de Estados (FSM)

El controlador opera como una máquina de estados finitos que garantiza transiciones deterministas y tiempos de respuesta acotados:

```
               [ 1. ARRANQUE / BOOT ]
                         │
                         │ (3 parpadeos LED1 @ 1s)
                         ▼
            ┌──> [ ESPERANDO CLIENTE ] <─────────┐
(Distancia  │            │                       │
 >= 1 m)    │            │ (Distancia < 1 m)     │ (Fin de servicio /
            │            ▼                       │  Pulsación botón 2-3 s)
            └─── [ 2. SERVICIO ACTIVO ] ─────────┘
                         │
                         │ (Pulsación botón >= 5 s)
                         ▼
                   [ 3. MODO ADMIN ]
                         │
                         │ (Pulsación botón >= 5 s)
                         └───> Vuelve a SERVICIO
```

### 1. Arranque (Boot)
- Al encender o reiniciar la placa, el LCD muestra el mensaje `CARGANDO …`.
- El **LED1** parpadea 3 veces en intervalos de 1 segundo (1s ON / 1s OFF).
- Finalizada la secuencia, el **LED1** se apaga y el sistema transiciona automáticamente a **Servicio**.

### 2. Modo Servicio
- **Detección por proximidad:** Mientras la distancia reportada por el HC-SR04 sea superior o igual a 1 metro, el LCD permanece en `ESPERANDO CLIENTE`. Al detectar un usuario a menos de 1 metro, se activa el proceso de compra.
- **Telemetría inicial:** El LCD muestra la lectura de temperatura y humedad durante 5 segundos.
- **Catálogo y navegación interactiva:**
  - Desplazamiento por la lista de bebidas mediante el joystick (**Arriba / Abajo**):
    - *Café Solo* — 1.00 €
    - *Café Cortado* — 1.10 €
    - *Café Doble* — 1.25 €
    - *Café Premium* — 1.50 €
    - *Chocolate* — 2.00 €
  - Selección de producto pulsando el botón integrado del joystick (`SW`).
- **Simulación de preparación y salida PWM:**
  - Tiempo de preparación estocástico entre 4 y 8 segundos ($t \in [4, 8]\text{ s}$).
  - Mensaje en pantalla: `Preparando Café...`.
  - El **LED2** incrementa gradualmente su ciclo de trabajo PWM de 0 a 255 proporcionalmente al progreso del tiempo calculado.
  - Al completar la preparación, la pantalla muestra `RETIRE BEBIDA` durante 3 segundos antes de retornar a la espera de cliente.
- **Reinicio rápido de servicio:** Pulsar el botón externo entre 2 y 3 segundos aborta la operación en curso y reinicia el flujo de servicio sin resetear el microcontrolador.

### 3. Modo Administrador (Admin)
- **Acceso/Salida segura:** Se activa o desactiva manteniendo presionado el botón externo durante **al menos 5 segundos continuos**.
- **Indicador visual:** Ambos LEDs (**LED1 y LED2**) permanecen permanentemente encendidos durante toda la estancia en este modo.
- **Menú de administración navegable** (Joystick vertical para scroll, clic para entrar, movimiento **Izquierda** para volver atrás):
  1. **Ver temperatura:** Refresco reactivo continuo de `Temp: XX ºC Hum: YY %`.
  2. **Ver distancia sensor:** Distancia en tiempo real en centímetros (`Distancia: XX cm`).
  3. **Ver contador:** Segundero continuo en pantalla con el tiempo total transcurrido (*uptime*) desde el inicio de la placa.
  4. **Modificar precios:** Permite ajustar los precios de las bebidas en incrementos/decrementos de **±0.05 €** usando el joystick (Arriba/Abajo). 
     - Botón del joystick: confirma y guarda el nuevo precio en memoria RAM.
     - Joystick a la izquierda: descarta y cancela la edición.
     - Los nuevos precios se reflejan de inmediato en el menú de servicio para la sesión activa.

---

## ⏱️ Técnicas de Tiempo Real y Concurrencia

- **Multitarea no bloqueante:** Separación de responsabilidades con `ArduinoThread` para la lectura de sensores, parpadeos y refresco visual sin recurrir a esperas activas (`delay()`).
- **Temporización por Hardware (`TimerOne`):** Control del contador global de tiempo (*uptime*) y gestión de tiempos críticos mediante interrupciones de temporizador periódicas.
- **Rutinas de Interrupción (ISR) Ligeras:** Las interrupciones externas solo registran marcas temporales (`millis()`) y conmutan banderas (*flags*), delegando la lógica pesada al bucle principal para evitar latencias indeseadas.
- **Watchdog Timer (`avr/wdt.h`):** Integración del perro guardián del ATmega328P para recuperar automáticamente el sistema en caso de bloqueos imprevistos o anomalías en el flujo de ejecución.

---

## 🔌 Asignación de Pines Sugerida

| Periférico | Pin Arduino UNO | Modo de Operación |
| :--- | :--- | :--- |
| **LED1** | `D8` | Salida digital |
| **LED2** | `D9` | Salida PWM analógica |
| **Botón externo** | `D2` | Entrada digital con interrupción (`INT0`) |
| **Joystick VRx** | `A0` | Entrada analógica (Eje X) |
| **Joystick VRy** | `A1` | Entrada analógica (Eje Y) |
| **Joystick SW** | `D3` | Entrada digital con interrupción (`INT1`) / Pull-up |
| **DHT11 Señal** | `D4` | Entrada/Salida digital monohilo |
| **HC-SR04 Trig** | `D5` | Salida digital (Disparo ultrasónico) |
| **HC-SR04 Echo** | `D6` | Entrada digital (Tiempo de vuelo) |
| **LCD (RS, E, D4-D7)** | `D7, D10, D11, D12, D13, A2` | Bus de control paralelo 4-bit |

---

## 🚀 Instalación y Compilación

1. Clona este repositorio:
   ```bash
   git clone https://github.com/tu-usuario/controlador-maquina-expendedora.git
   cd controlador-maquina-expendedora

   
   ```
2. Instala las dependencias necesarias desde el **Library Manager** de Arduino IDE o el `platformio.ini` de PlatformIO.
3. Conecta el **Arduino UNO** al puerto USB de tu equipo.
4. Compila y descarga el firmware en la placa.

---

ENLACE AL VIDEO: https://drive.google.com/file/d/1WChFGTd7DSJCJvSZWCxxEbXdxrZwhsn9/view?usp=sharing

