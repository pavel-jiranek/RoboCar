#include <ArduinoBLE.h>
#include <Joystick-KY023.h>

#define SERVICE_UUID "1ba9b5cf-7d16-41c3-bfb5-7043679fdb10"
#define XCHAR_UUID "1ba9b5cf-7d16-41c3-bfb5-7043679fdb11"
#define YCHAR_UUID "1ba9b5cf-7d16-41c3-bfb5-7043679fdb12"
#define SWITCHCHAR_UUID "1ba9b5cf-7d16-41c3-bfb5-7043679fdb13"

#define XPIN A0
#define YPIN A1
#define SWITCHPIN A2
#define ZERO_TOLERANCE_RANGE 0.1f

class JoystickBLE {
 public:
  JoystickBLE(uint8_t xPin, uint8_t yPin, uint8_t switchPin);

  void begin();
  void loop();

 private:
  JoystickKY023 _joy;

  float _prevX;
  float _prevY;
  bool _prevSwitch;

  BLEService _service;
  BLEFloatCharacteristic _xAxisChar;
  BLEFloatCharacteristic _yAxisChar;
  BLEBoolCharacteristic _switchChar;
};

JoystickBLE::JoystickBLE(uint8_t xPin, uint8_t yPin, uint8_t switchPin)
    : _joy(xPin, yPin, switchPin, ZERO_TOLERANCE_RANGE),
      _prevX(0.0f),
      _prevY(0.0f),
      _prevSwitch(false),
      _service(SERVICE_UUID),
      _xAxisChar(XCHAR_UUID, BLERead | BLENotify),
      _yAxisChar(YCHAR_UUID, BLERead | BLENotify),
      _switchChar(SWITCHCHAR_UUID, BLERead | BLENotify) {}

void JoystickBLE::begin() {
  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (true);
  }

  _joy.calibrate();

  BLE.setLocalName("Joystick");
  BLE.setAdvertisedService(_service);

  _service.addCharacteristic(_xAxisChar);
  _service.addCharacteristic(_yAxisChar);
  _service.addCharacteristic(_switchChar);

  BLE.addService(_service);

  _xAxisChar.writeValue(0.0f);
  _yAxisChar.writeValue(0.0f);
  _switchChar.writeValue(false);

  BLE.advertise();
}

void JoystickBLE::loop() {
  BLEDevice central = BLE.central();

  if (!central) return;

  long prevMillis = millis();

  while (central.connected()) {
    const long currMillis = millis();

    if (currMillis - prevMillis >= 200) {
      prevMillis = currMillis;

      float x = _joy.getX();
      float y = _joy.getY();
      bool isSwitchPressed = _joy.isSwitchPressed();

      bool xyUpdated = false;
      if (x != _prevX) {
        _xAxisChar.writeValue(x);
        xyUpdated = true;
        _prevX = x;
      }

      if (y != _prevY) {
        _yAxisChar.writeValue(y);
        xyUpdated = true;
        _prevY = y;
      }

      if (xyUpdated) {
        Serial.print("X = ");
        Serial.print(x);
        Serial.print(", Y = ");
        Serial.println(y);
      }

      if (isSwitchPressed != _prevSwitch) {
        _switchChar.writeValue(isSwitchPressed);
        _prevSwitch = isSwitchPressed;
      }
    }
  }
}

JoystickBLE joy(XPIN, YPIN, SWITCHPIN);

void setup() {
  Serial.begin(9600);
  joy.begin();
}

void loop() { joy.loop(); }
