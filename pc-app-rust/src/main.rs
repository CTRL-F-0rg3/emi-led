use btleplug::api::{Central, Manager as _, Peripheral as _, ScanFilter, WriteType};
use btleplug::platform::Manager;
use eframe::egui;
use std::sync::mpsc::{self, Sender};
use std::time::Duration;
use tokio::time;
use uuid::Uuid;

const SERVICE_UUID: Uuid = Uuid::from_u128(0x0000ffe0_0000_1000_8000_00805f9b34fb);
const CHAR_UUID: Uuid = Uuid::from_u128(0x0000ffe1_0000_1000_8000_00805f9b34fb);
const DEVICE_NAME: &str = "EMI-LED";

// Typ wiadomości wysyłanej z GUI do wątku BLE
enum BleCommand {
    SetColor(u8, u8, u8),
}

// Struktura przechowująca stan naszej aplikacji GUI
struct LedApp {
    r: u8,
    g: u8,
    b: u8,
    status: String,
    tx: Sender<BleCommand>,
}

impl LedApp {
    fn new(tx: Sender<BleCommand>) -> Self {
        Self {
            r: 0,
            g: 0,
            b: 0,
            status: "Szukanie adaptera i łączenie...".to_owned(),
            tx,
        }
    }
}

// Implementacja interfejsu graficznego
impl eframe::App for LedApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        egui::CentralPanel::default().show(ctx, |ui| {
            ui.heading("EMI-LED Controller 🦀");
            ui.separator();

            // Suwaki RGB
            ui.add(egui::Slider::new(&mut self.r, 0..=255).text("Czerwony (R)"));
            ui.add(egui::Slider::new(&mut self.g, 0..=255).text("Zielony (G)"));
            ui.add(egui::Slider::new(&mut self.b, 0..=255).text("Niebieski (B)"));

            ui.separator();

            // Podgląd koloru
            let color = egui::Color32::from_rgb(self.r, self.g, self.b);
            let (rect, _response) = ui.allocate_exact_size(egui::vec2(ui.available_width(), 50.0), egui::Sense::hover());
            ui.painter().rect_filled(rect, 5.0, color);

            ui.add_space(10.0);

            // Przycisk wysyłający kolor
            if ui.button("🚀 Wyślij kolor do ESP32").clicked() {
                if let Err(e) = self.tx.send(BleCommand::SetColor(self.r, self.g, self.b)) {
                    self.status = format!("Błąd wysyłania: {}", e);
                } else {
                    self.status = format!("Wysłano RGB: {}, {}, {}", self.r, self.g, self.b);
                }
            }

            ui.add_space(20.0);
            ui.label(format!("Status: {}", self.status));
        });
    }
}

// Wątek w tle obsługujący Bluetooth
fn run_bluetooth_thread(rx: std::sync::mpsc::Receiver<BleCommand>) {
    let rt = tokio::runtime::Runtime::new().unwrap();
    rt.block_on(async {
        let manager = match Manager::new().await {
            Ok(m) => m,
            Err(_) => return, // Brak adaptera, kończymy cicho (GUI pokaże błąd)
        };

        let adapters = match manager.adapters().await {
            Ok(a) => a,
            Err(_) => return,
        };

        let central = match adapters.into_iter().next() {
            Some(c) => c,
            None => return,
        };

        let _ = central.start_scan(ScanFilter::default()).await;
        time::sleep(Duration::from_secs(3)).await;

        let peripherals = central.peripherals().await.unwrap_or_default();
        let mut target = None;

        for p in peripherals {
            if let Ok(Some(props)) = p.properties().await {
                if props.local_name.as_deref() == Some(DEVICE_NAME) {
                    target = Some(p);
                    break;
                }
            }
        }

        if let Some(device) = target {
            if device.connect().await.is_ok() && device.discover_services().await.is_ok() {
                let chars = device.characteristics();
                if let Some(char_write) = chars.iter().find(|c| c.uuid == CHAR_UUID) {
                    // Pętla odbierająca komendy z GUI
                    while let Ok(cmd) = rx.recv() {
                        match cmd {
                            BleCommand::SetColor(r, g, b) => {
                                let data = [r, g, b];
                                let _ = device.write(&char_write, &data, WriteType::WithResponse).await;
                            }
                        }
                    }
                }
            }
        }
    });
}

fn main() -> Result<(), eframe::Error> {
    // Kanał do komunikacji GUI -> Bluetooth
    let (tx, rx) = mpsc::channel();

    // Uruchamiamy wątek Bluetooth w tle
    std::thread::spawn(move || {
        run_bluetooth_thread(rx);
    });

    // Konfiguracja okna GUI
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size([400.0, 350.0])
            .with_min_inner_size([300.0, 300.0]),
        ..Default::default()
    };

    // Uruchomienie okna
    eframe::run_native(
        "EMI-LED Rust Client",
        options,
        Box::new(|_cc| Box::new(LedApp::new(tx))),
    )
}
