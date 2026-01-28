#include <ArduinoBLE.h>

#define L1 4
#define L2 5
#define R1 8
#define R2 9
#define EN 3

#define LED_PIN LED_BUILTIN

#define SERVICE_UUID "1ba9b5cf-7d16-41c3-bfb5-7043679fdb10"
#define XCHAR_UUID "1ba9b5cf-7d16-41c3-bfb5-7043679fdb11"
#define YCHAR_UUID "1ba9b5cf-7d16-41c3-bfb5-7043679fdb12"
#define SWITCHCHAR_UUID "1ba9b5cf-7d16-41c3-bfb5-7043679fdb13"

class Car {
 public:
  Car(uint8_t l1, uint8_t l2, uint8_t r1, uint8_t r2, uint8_t en)
      : _l1(l1), _l2(l2), _r1(r1), _r2(r2), _en(en), _speed(0), _stopped(true) {
    pinMode(_l1, OUTPUT);
    pinMode(_l2, OUTPUT);
    pinMode(_r1, OUTPUT);
    pinMode(_r2, OUTPUT);
    pinMode(_en, OUTPUT);
  };

  void directionForward() {
    digitalWrite(_l1, LOW);
    digitalWrite(_l2, HIGH);
    digitalWrite(_r1, LOW);
    digitalWrite(_r2, HIGH);
  }

  void directionBackward() {
    digitalWrite(_l1, HIGH);
    digitalWrite(_l2, LOW);
    digitalWrite(_r1, HIGH);
    digitalWrite(_r2, LOW);
  }

  void directionLeft() {
    digitalWrite(_l1, LOW);
    digitalWrite(_l2, HIGH);
    digitalWrite(_r1, HIGH);
    digitalWrite(_r2, LOW);
  }

  void directionRight() {
    digitalWrite(_l1, HIGH);
    digitalWrite(_l2, LOW);
    digitalWrite(_r1, LOW);
    digitalWrite(_r2, HIGH);
  }

  void setSpeed(uint8_t speed) {
    _speed = speed;
    if (!_stopped) analogWrite(_en, speed);
  }

  void start() {
    analogWrite(_en, _speed);
    _stopped = false;
  }

  void stop() {
    analogWrite(_en, 0);
    _stopped = true;
  }

  void toggle() {
    if (_stopped)
      start();
    else
      stop();
  }

 private:
  uint8_t _l1;
  uint8_t _l2;
  uint8_t _r1;
  uint8_t _r2;
  uint8_t _en;
  uint8_t _speed;
  bool _stopped;
};

Car car(L1, L2, R1, R2, EN);

void setup() {
  Serial.begin(9600);
  while (!Serial) {
  }

  if (!BLE.begin()) {
    Serial.println("BLE begin failed!");
    while (1);
  }

  Serial.println("Central scanning...");
  BLE.scanForUuid(SERVICE_UUID);
}

void loop() {
  BLEDevice device = BLE.available();

  if (device) {
    Serial.print("Found ");
    Serial.print(device.address());
    Serial.print(" '");
    Serial.print(device.localName());
    Serial.print("' ");
    Serial.print(device.advertisedServiceUuid());
    Serial.println();
    BLE.stopScan();

    if (!device.connect()) {
      Serial.println("Connect failed!");
      BLE.scanForUuid(SERVICE_UUID);
      return;
    }

    if (!device.discoverAttributes()) {
      Serial.println("Discover attributes failed!");
      BLE.scanForUuid(SERVICE_UUID);
      return;
    }

    BLECharacteristic xChar = device.characteristic(XCHAR_UUID);
    BLECharacteristic yChar = device.characteristic(YCHAR_UUID);

    if (!xChar.subscribe()) {
      Serial.println("Subscribe to X-axis failed!");
      BLE.scanForUuid(SERVICE_UUID);
      return;
    }

    if (!yChar.subscribe()) {
      Serial.println("Subscribe to Y-axis failed!");
      BLE.scanForUuid(SERVICE_UUID);
      return;
    }

    car.setSpeed(180);
    controlCar(device, xChar, yChar);

    BLE.scanForUuid(SERVICE_UUID);
  }
}

float x = 0.0f;
float y = 0.0f;

void controlCar(BLEDevice& device, BLECharacteristic& xChar, BLECharacteristic& yChar) {
  while (device.connected()) {
    BLE.poll();
    delay(1);

    bool updated = false;

    if (xChar.valueUpdated()) {
      if (!xChar.readValue(&x, sizeof(x))) x = 0.0f;
      updated = true;
    }

    if (yChar.valueUpdated()) {
      if (!yChar.readValue(&y, sizeof(y))) y = 0.0f;
      updated = true;
    }

    if (x == 0.0f && y == 0.0f) {
      car.stop();
    }

    if (updated) {
      if (abs(x) >= abs(y)) {
        uint8_t speed = static_cast<uint8_t>(abs(x) * 255);
        car.setSpeed(speed);

        if (x < 0.0f) {
          Serial.println("Turning right");
          car.directionRight();
          car.start();
        }

        if (x > 0.0f) {
          Serial.println("Turning left");
          car.directionLeft();
          car.start();
        }
      }

      if (abs(y) >= abs(x)) {
        uint8_t speed = static_cast<uint8_t>(abs(y) * 255);
        car.setSpeed(speed);

        if (y > 0.0f) {
          Serial.println("Going forward");
          car.directionForward();
          car.start();
        }

        if (y < 0.0f) {
          Serial.println("Going backward");
          car.directionBackward();
          car.start();
        }
      }
    }
  }
}
