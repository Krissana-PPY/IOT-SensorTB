#define PUL_PIN 26 // Pulse signal pin
#define DIR_PIN 27 // Direction signal pin
#define ENA_PIN 25 // Enable signal pin

void setup() {
  Serial.begin(9600);
  pinMode(PUL_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);
  digitalWrite(ENA_PIN, LOW); // Enable the driver
  delay(1000); // Wait for system to be ready
}

void loop() {
  // Enable TB6600 driver
  digitalWrite(ENA_PIN, LOW); // Or HIGH depending on your wiring

  // Rotate motor 180°
  Serial.println("Rotate 180 degrees");
  digitalWrite(DIR_PIN, HIGH);
  stepMotor(3200); // Rotate 3200 microsteps (180°) with 1/16 microstepping
  delay(1000); // Wait 1 second

  // Rotate motor back 90°
  Serial.println("Rotate back 90 degrees");
  digitalWrite(DIR_PIN, LOW);
  stepMotor(1600); // Rotate 1600 microsteps (90°)
  delay(1000); // Wait 1 second

  // Disable TB6600 driver
  digitalWrite(ENA_PIN, HIGH); // Or LOW depending on your wiring
}

void stepMotor(int steps) {
  for (int i = 0; i < steps; i++) {
    digitalWrite(PUL_PIN, HIGH);
    delayMicroseconds(500);
    digitalWrite(PUL_PIN, LOW);
    delayMicroseconds(500);
  }
}
