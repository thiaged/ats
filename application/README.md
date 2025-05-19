# Solar Energy Transfer Control System

This project is a solar energy transfer control system designed to run on the Lilygo T-Display S3 AMOLED. It manages the transfer of energy between solar and utility sources while monitoring the voltage of both energy sources and the battery.

## Project Structure

```
solar-energy-transfer-control

│── src/
│   ├── core/                  # 📌 Camada de Domínio
│   │   ├── domain/            # 📌 Entidades e Regras de Negócio
│   │   │   ├── EnergySource.h # 🔥 Classe base (Solar, Utility)
│   │   │   ├── SolarSource.h  # ☀️ Representa a rede solar
│   │   │   ├── UtilitySource.h # ⚡ Representa a rede utility
│   │   ├── ports/             # 📌 Interfaces para comunicação
│   │   │   ├── EnergyPort.h   # Interface para fontes de energia
│   ├── adapters/              # 📌 Adapters (implementação das interfaces)
│   │   ├── input/             # 📌 Sensores, comunicação de entrada
│   │   │   ├── SolarSensorAdapter.h
│   │   ├── output/            # 📌 Dispositivos de saída (display, relés)
│   │   │   ├── DisplayAdapter.h
|   ├── main.cpp
└── README.md
```

## Features

- **Energy Management**: Monitors and manages energy from solar and utility sources.
- **Battery Control**: Monitors battery voltage and controls charging/discharging.
- **User Interface**: Provides a graphical interface with home, configuration, and waveform screens.
- **Voltage Monitoring**: Continuously monitors the voltage levels of solar, utility, and battery sources.

## Setup Instructions

1. Clone the repository:
   ```
   git clone <repository-url>
   ```
2. Navigate to the project directory:
   ```
   cd solar-energy-transfer-control
   ```
3. Build the project using CMake:
   ```
   mkdir build
   cd build
   cmake ..
   make
   ```
4. Upload the firmware to the Lilygo T-Display S3 AMOLED.

## Usage Guidelines

- Use the home screen to view the current energy status.
- Navigate to the configuration screen to adjust settings.
- The waveform screen visualizes the energy production and consumption.

## Contributing

Contributions are welcome! Please submit a pull request or open an issue for any enhancements or bug fixes.

## License

This project is licensed under the MIT License. See the LICENSE file for more details.