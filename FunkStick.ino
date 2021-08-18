

//#include <TCA9548A.h>
#include <Wire.h>
#include <ESPRotary.h>
#include <LiquidCrystal_I2C.h>
#include <USB-MIDI.h>

USBMIDI_CREATE_DEFAULT_INSTANCE();

#define X_PIN 0
#define Y_PIN 1
#define Z_PIN 2
#define JOYSTICK_BUTTON_PIN 3
#define SELECTION_ENCODER_PUSH_PIN 5

#define CLICKS_PER_STEP 4

ESPRotary ccSelectionEncoder;

LiquidCrystal_I2C lcd(0x27, 16, 2);

int ccChannel[] = {0, 1, 2};
int ccValue[] = {0, 0, 0};
int midiChannel = 1;
String encoderModes[] = {"MIDI Channel", "CC", "CC", "CC"};
int encoderModeIndex = 1;
boolean selectionPressed = false;
boolean joystickPressed = false;
int tick = 0;
boolean dirty = true;

void setup() {
  Serial.begin(115200);
  Serial.print("Starting Setup\n");
  setup_joystick();
  
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  digitalWrite(11, HIGH);

  setup_encoders();

  lcd.init();
  lcd.backlight();

  MIDI.begin(midiChannel);

  Serial.print("Complete Setup\n");
}

void setRGB(int red, int green, int blue) {
    digitalWrite(9, red % 256);
    digitalWrite(10, green % 256);
    digitalWrite(11, blue % 256);
}

void setup_joystick() {
  pinMode(X_PIN, INPUT);
  pinMode(Y_PIN, INPUT);
  pinMode(Z_PIN, INPUT);
  pinMode(JOYSTICK_BUTTON_PIN, INPUT);
}

void setup_encoders() {
    pinMode(SELECTION_ENCODER_PUSH_PIN, INPUT);

    ccSelectionEncoder.begin(7, 11, CLICKS_PER_STEP);
    ccSelectionEncoder.setUpperBound(127);
    ccSelectionEncoder.setLowerBound(0);
    ccSelectionEncoder.setChangedHandler(rotateCCSelectionEncoder);
}

void loop() {
  int x = analogRead(X_PIN);
  int y = analogRead(Y_PIN);
  int z = analogRead(Z_PIN);

  tick = (tick + 1) % 100;
  if(tick == 0) {
    printPosition(x, y, z);
  }

  setCCValues(x, y, z);

  setRGB((int)(pow(x, 2)/pow(1024.0, 2)*255.0), (int)(pow(y, 2)/pow(1024.0, 2)*255.0), (int)(pow(z, 2)/pow(1024.0,2)*255.0));

  loopEncoders();

  loopButtons();

  if(dirty && tick == 0) {
    lcdPrint();
    sendMIDI();
  }
}

void setCCValues(int x, int y, int z) {
  int new0 = scaleInt(x, 1024, 128);
  int new1 = scaleInt(y, 1024, 128);
  int new2 = scaleInt(z, 1024, 128);

  dirtySetValue(0, new0, 2);
  dirtySetValue(1, new1, 2);
  dirtySetValue(2, new2, 2);
}

void dirtySetValue(int index, int newValue, int minIncrement) {
  if(abs(ccValue[index] - newValue) > minIncrement) {
    ccValue[index] = newValue;
    dirty = true;
  }
}

int scaleInt(int x, int from, int to) {
  return (int)((to*1.0)/(from*1.0)*(x*1.0));
}

void sendMIDI() {
  MIDI.sendControlChange(ccChannel[0], ccValue[0], midiChannel);
  MIDI.sendControlChange(ccChannel[1], ccValue[1], midiChannel);
  MIDI.sendControlChange(ccChannel[2], ccValue[2], midiChannel);
}

void lcdPrint() {
  switch(encoderModeIndex) {
    case 0:
      lcd.setCursor(0,0);
      lcd.print("MIDI Channel: ");
      lcd.print(convertToTwoDigits(midiChannel));
      break;
    default:
      printCC(0, ccChannel[0], ccValue[0], encoderModeIndex == 1);
      printCC(5, ccChannel[1], ccValue[1], encoderModeIndex == 2);
      printCC(10, ccChannel[2], ccValue[2], encoderModeIndex == 3);
      break;
  }
  dirty = false;
}

void printCC(int position, int channel, int value, boolean selected) {
      lcd.setCursor(position,0);
      lcd.print(printCCName(channel, selected));
      lcd.setCursor(position,1);
      lcd.print(convertToThreeDigits(value));
}

String printCCName(int channel, bool selected) {
  String text = "CC" + convertToTwoDigits(channel);
  if(selected) {
    text = text + "*";
  }
  return text;
}

String convertToTwoDigits(int value) {
  if(value < 10) {
    return "0"+String(value);
  }
  return String(value);
}

String convertToThreeDigits(int value) {
  if(value < 10) {
    return "00"+String(value);
  }
  if(value < 100) {
    return "0"+String(value);
  }
  return String(value);
}

void loopButtons() {
  if(isJoystickButtonClicked()) {
    onJoystickButtonClicked();
  } 

  if(isSelectionButtonClicked()) {
    onSelectionButtonClicked();
  }
}

void loopEncoders() {
  ccSelectionEncoder.loop();
}

void printPosition(int x, int y, int z) {

  Serial.print(x);
  Serial.print(", ");
  Serial.print(y);
  Serial.print(", ");
  Serial.print(z);
  Serial.print("  :  ");
  Serial.print(encoderModes[encoderModeIndex]);
  Serial.print(", ");
  Serial.print(ccChannel[0]);
  Serial.print(", ");
  Serial.print(ccValue[0]);
  Serial.print(", ");
  Serial.print(ccChannel[1]);
  Serial.print(", ");
  Serial.print(ccValue[1]);
  Serial.print(", ");
  Serial.print(ccChannel[2]);
  Serial.print(", ");
  Serial.print(ccValue[2]);
  Serial.print(", ");
  Serial.print(midiChannel);
  Serial.print("  :  ");
  Serial.print(joystickPressed);
  Serial.print(", ");
  Serial.print(selectionPressed);
  Serial.print("\n");
}

void onJoystickButtonClicked() {
    Serial.println("Joystick Clicked!");
}

void onSelectionButtonClicked() {
  Serial.println("Selection Clicked!");
  encoderModeIndex = (encoderModeIndex + 1) % 4;

  if(encoderModeIndex == 0) {
    ccSelectionEncoder.setUpperBound(16);
    ccSelectionEncoder.setLowerBound(1);
  } else {
    ccSelectionEncoder.setUpperBound(127);
    ccSelectionEncoder.setLowerBound(0);
  }

  dirty = true;
  lcd.clear();
}


bool isSelectionButtonClicked() {
  int val = digitalRead(SELECTION_ENCODER_PUSH_PIN);
  if(val < 1 && selectionPressed == false) {
    selectionPressed = true;
    return true;
  }
  if(val >= 1) {
    selectionPressed = false;
  }
  return false;
}

bool isJoystickButtonClicked() {
  int val = analogRead(JOYSTICK_BUTTON_PIN);
  if(val < 10 && joystickPressed == false) {
    joystickPressed = true;
    return true;
  } 
  if(val >= 10) {
    joystickPressed = false;
  }
  return false;
}

void rotateCCSelectionEncoder(ESPRotary& r) {
  switch(encoderModeIndex) {
    case 0:
      midiChannel = r.getPosition();
      break;
    case 1:
      ccChannel[0] = r.getPosition();
      break;
    case 2:
      ccChannel[1] = r.getPosition();
      break;
    case 3:
      ccChannel[2] = r.getPosition();
      break;
  }
  dirty = true;
}
