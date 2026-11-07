# EMI-LED: Smart RGB Lighting Control

An educational project aimed at learning wireless communication (Bluetooth Low Energy) between a microcontroller (ESP32-S3) and client applications (Android, PC).

## About the Project
The goal of this project is to build a complete system for controlling an RGB LED (WS2812) using a smartphone or a computer.

### Components:
* **Firmware (ESP32-S3)**: Written in C using ESP-IDF. Responsible for receiving data via BLE and controlling the LED.
* **Android Application**: Written in Kotlin (IntelliJ IDEA). Allows color control using RGB sliders.
* **PC Application**: Written in Rust (GUI based on `egui`). Allows control from a desktop computer.

## How to Run

### ESP32-S3
1. Install [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/).
2. In the root directory, run: `idf.py build flash monitor`

### Android Application
1. Open the `app` folder in Android Studio / IntelliJ IDEA.
2. Build and run the application on an Android 12+ smartphone.

### PC Application (Rust)
1. Install [Rust](https://rustup.rs/).
2. Navigate to the folder: `cd pc-app-rust`
3. Run: `cargo run`

## License and Guidelines

This project is distributed under the **MIT License** (full text in the `LICENSE` file).

**Author's Note:** This project was created for educational purposes. We strongly discourage using this code for commercial purposes without making your own significant contribution to its development. It is best to treat it as a learning base and write your own implementation.
# emi-led
