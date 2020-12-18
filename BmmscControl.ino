#include <LiquidCrystal.h>
#include <HID.h>
#include <BMDSDIControl.h>
#include <SoftwareSerial.h>

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

unsigned long currentTime;
unsigned long loopTime;
const int pin_A = 2;
const int pin_B = 3;
const int pin_back = 4;
const int pin_go = 5;
unsigned char encoder_A;
unsigned char encoder_B;
unsigned char encoder_A_prev=0;

const int                shieldAddress = 0x6E;
BMD_SDICameraControl_I2C sdiCameraControl(shieldAddress);

int navLevel = 0;
boolean backReady = true;
boolean goReady = true;

int incomingModeByte = 0;
int incomingValByte = 0;
int incomingBreakByte = 0;

char cameraFunctions[][14] = {"camera       ",
                              "video mode   ",
                              "sensor gain  ",
                              "white balance",
                              "shutter speed",
                              "aperture     ",
                              "focus        ",
                              "zoom         ",
                              "overlays     ",
                              "zebras       "};
const int FUNC_CAMERA = 0;
const int FUNC_MODE = 1;
const int FUNC_GAIN = 2;
const int FUNC_TEMP = 3;
const int FUNC_SHUTTER = 4;
const int FUNC_APERTURE = 5;
const int FUNC_FOCUS = 6;
const int FUNC_ZOOM = 7;
const int FUNC_OVERLAYS = 8;
const int FUNC_ZEBRAS = 9;

int cameraFunctionIndex = 0;

int cameraNum = 2;

char videoModes[][12] = {"720p50    ", "720p59.94 ", "720p60    ", "1080i25   ", "1080i29.97", "1080i30   ",
                         "1080p23.98", "1080p24   ", "1080p25   ", "1080p29.97", "1080p30   ", "1080p50   ", "1080p59.94", "1080p60   ",
                         "2160p23.98", "2160p24   ", "2160p25   ", "2160p29.97", "2160p30   "};
int videoModeIndex = 9;

short sensorGains[] = {1, 2, 4, 8, 16};
short isoOptions[] = {100, 200, 400, 800, 1600};
int sensorGainIndex = 4;

short whiteBalances[] = {2500, 2800, 3000, 3200, 3400, 3600, 4000, 4500, 4800, 5000, 5200, 5400, 5600, 6000, 6500, 7000, 7500, 8000};
int whiteBalanceIndex = 9;

short shutterSpeeds[] = {50, 60, 75, 90, 100, 120, 150, 180, 250, 360, 500, 725, 1000, 1450, 2000};
int shutterSpeedIndex = 1;

float fStops[] = {1.0, 1.2, 1.4, 1.7, 2.0, 2.4, 2.8, 3.3, 4.0, 4.8, 5.6, 6.7, 8.0, 9.5, 11.0, 13.0, 16.0};
int fStopIndex = 10;

float focus = 0.7;
short zoom = 25;

bool overlays = true;
bool zebras = true;

void updateLcd() {
  switch (navLevel) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("select:       ");
      lcd.setCursor(0, 1);
      lcd.print("  ");
      lcd.print(cameraFunctions[cameraFunctionIndex]);
      break;
    case 1:
      lcd.setCursor(0, 0);
      lcd.print(cameraFunctions[cameraFunctionIndex]);
      lcd.setCursor(0, 1);
      switch(cameraFunctionIndex) {
        case FUNC_CAMERA:
          lcd.print("      ");
          if (cameraNum < 10) lcd.print(" ");
          if (cameraNum < 100) lcd.print(" ");
          lcd.print(cameraNum);
          lcd.print("      ");
          break;
        case FUNC_MODE:
          lcd.print("    ");
          lcd.print(videoModes[videoModeIndex]);
          lcd.print("    ");
          break;
        case FUNC_GAIN:
          lcd.print("    ISO ");
          lcd.print(isoOptions[sensorGainIndex]);
          lcd.print("    ");
          break;
        case FUNC_TEMP:
          lcd.print("      ");
          lcd.print(whiteBalances[whiteBalanceIndex]);
          lcd.print("      ");
          break;
        case FUNC_SHUTTER:
          lcd.print("      ");
          if (shutterSpeedIndex < 4) lcd.print(" ");
          if (shutterSpeedIndex < 12) lcd.print(" ");
          lcd.print(shutterSpeeds[shutterSpeedIndex]);
          lcd.print("      ");
          break;
        case FUNC_APERTURE:
          lcd.print("     f/");
          lcd.print(fStops[fStopIndex]);
          lcd.print("     ");
          break;
        case FUNC_FOCUS:
          lcd.print("      ");
          lcd.print(focus);
          lcd.print("      ");
          break;
        case FUNC_ZOOM:
          lcd.print("      ");
          if (zoom < 10) lcd.print(" ");
          if (zoom < 100) lcd.print(" ");
          lcd.print(zoom);
          lcd.print("      ");
          break;
        case FUNC_OVERLAYS:
          lcd.print("      ");
          if (overlays) {
            lcd.print(" on");
          } else {
            lcd.print("off");
          }
          lcd.print("      ");
          break;
        case FUNC_ZEBRAS:
          lcd.print("      ");
          if (zebras) {
            lcd.print(" on");
          } else {
            lcd.print("off");
          }
          lcd.print("      ");
          break;
      }
      break;
  }
}

void clockwise() {
  switch (navLevel) {
    case 0:
      switch(cameraFunctionIndex) {
        case 9:
          cameraFunctionIndex = 0;
          break;
        default:
          cameraFunctionIndex += 1;
          break;
      }
      break;
    case 1:
      switch(cameraFunctionIndex) {
        case FUNC_CAMERA:
          if (cameraNum >= 99) {
            cameraNum = 100;
          } else {
            cameraNum += 1;
          }
          break;
        case FUNC_MODE:
          switch(videoModeIndex) {
            case 15:
              videoModeIndex = 0;
              break;
            default:
              videoModeIndex += 1;
              break;
          }
          writeVideoMode();
          break;
        case FUNC_GAIN:
          switch(sensorGainIndex) {
            case 4:
              sensorGainIndex = 0;
              break;
            default:
              sensorGainIndex += 1;
              break;
          }
          writeSensorGain();
          break;
        case FUNC_TEMP:
          switch(whiteBalanceIndex) {
            case 17:
              whiteBalanceIndex = 0;
              break;
            default:
              whiteBalanceIndex += 1;
              break;
          }
          writeWhiteBalance();
          break;
        case FUNC_SHUTTER:
          switch(shutterSpeedIndex) {
            case 14:
              shutterSpeedIndex = 0;
              break;
            default:
              shutterSpeedIndex += 1;
              break;
          }
          writeShutterSpeed();
          break;
        case FUNC_APERTURE:
          switch(fStopIndex) {
            case 16:
              fStopIndex = 0;
              break;
            default:
              fStopIndex += 1;
              break;
          }
          writeAperture();
          break;
        case FUNC_FOCUS:
          if (focus >= 0.99) {
            focus = 1.0;
          } else {
            focus += 0.01;
          }
          writeFocus();
          break;
        case FUNC_ZOOM:
          if (zoom >= 199) {
            zoom = 200;
          } else {
            zoom += 1;
          }
          writeZoom();
          break;
        case FUNC_OVERLAYS:
          overlays = !overlays;
          writeOverlays();
          break;
        case FUNC_ZEBRAS:
          zebras = !zebras;
          writeZebras();
          break;
      }
      break;
  }
}

void counterClockwise() {
  switch (navLevel) {
    case 0:
      switch(cameraFunctionIndex) {
        case 0:
          cameraFunctionIndex = 9;
          break;
        default:
          cameraFunctionIndex -= 1;
          break;
      }
      break;
    case 1:
      switch(cameraFunctionIndex) {
        case FUNC_CAMERA:
          if (cameraNum <= 2) {
            cameraNum = 1;
          } else {
            cameraNum -= 1;
          }
          break;
        case FUNC_MODE:
          switch(videoModeIndex) {
            case 0:
              videoModeIndex = 15;
              break;
            default:
              videoModeIndex -= 1;
              break;
          }
          writeVideoMode();
          break;
        case FUNC_GAIN:
          switch(sensorGainIndex) {
            case 0:
              sensorGainIndex = 4;
              break;
            default:
              sensorGainIndex -= 1;
              break;
          }
          writeSensorGain();
          break;
        case FUNC_TEMP:
          switch(whiteBalanceIndex) {
            case 0:
              whiteBalanceIndex = 17;
              break;
            default:
              whiteBalanceIndex -= 1;
              break;
          }
          writeWhiteBalance();
          break;
        case FUNC_SHUTTER:
          switch(shutterSpeedIndex) {
            case 0:
              shutterSpeedIndex = 14;
              break;
            default:
              shutterSpeedIndex -= 1;
              break;
          }
          writeShutterSpeed();
          break;
        case FUNC_APERTURE:
          switch(fStopIndex) {
            case 0:
              fStopIndex = 16;
              break;
            default:
              fStopIndex -= 1;
              break;
          }
          writeAperture();
          break;
        case FUNC_FOCUS:
          if (focus <= 0.01) {
            focus = 0.0;
          } else {
            focus -= 0.01;
          }
          writeFocus();
          break;
        case FUNC_ZOOM:
          if (zoom <= 9) {
            zoom = 8;
          } else {
            zoom -= 1;
          }
          writeZoom();
          break;
        case FUNC_OVERLAYS:
          overlays = !overlays;
          writeOverlays();
          break;
        case FUNC_ZEBRAS:
          zebras = !zebras;
          writeZebras();
          break;
      }
      break;
  }
}

void goBack() {
  if (navLevel > 0) {
    navLevel = navLevel - 1;
  }
}

void goForward() {
  if (navLevel < 1) {
    navLevel = navLevel + 1;
  }
}

long shutterUs() {
  return 1000000/shutterSpeeds[shutterSpeedIndex];
}

float fStopConverted() {
  return log(pow(fStops[fStopIndex], 2)) / log(2);
}

float zoomConverted() {
  if (zoom < 25) {
    return 0.0;
  } else if (zoom > 25) {
    return 1.0;
  } else {
    return 0.5;
  }
}

void writeVideoMode() {
  byte modeBytes[13] = {lowByte(cameraNum), 0x09, 0x00, 0x00,
                        0x01, 0x00, 0x01, 0x00,
                        0x1E, 0x00, 0x03, 0x00, 0x00};
  switch (videoModeIndex) {
    case 0:
      modeBytes[8] = 0x32;
      modeBytes[9] = 0x00;
      modeBytes[10] = 0x02;
      modeBytes[11] = 0x00;
      break;
    case 1:
      modeBytes[8] = 0x3C;
      modeBytes[9] = 0x01;
      modeBytes[10] = 0x02;
      modeBytes[11] = 0x00;
      break;
    case 2:
      modeBytes[8] = 0x3C;
      modeBytes[9] = 0x00;
      modeBytes[10] = 0x02;
      modeBytes[11] = 0x00;
      break;
    case 3:
      modeBytes[8] = 0x19;
      modeBytes[9] = 0x00;
      modeBytes[10] = 0x03;
      modeBytes[11] = 0x01;
      break;
    case 4:
      modeBytes[8] = 0x1E;
      modeBytes[9] = 0x01;
      modeBytes[10] = 0x03;
      modeBytes[11] = 0x01;
      break;
    case 5:
      modeBytes[8] = 0x1E;
      modeBytes[9] = 0x00;
      modeBytes[10] = 0x03;
      modeBytes[11] = 0x01;
      break;
    case 6:
      modeBytes[8] = 0x18;
      modeBytes[9] = 0x01;
      modeBytes[10] = 0x03;
      modeBytes[11] = 0x00;
      break;
    case 7:
      modeBytes[8] = 0x18;
      modeBytes[9] = 0x00;
      modeBytes[10] = 0x03;
      modeBytes[11] = 0x00;
      break;
    case 8:
      modeBytes[8] = 0x19;
      modeBytes[9] = 0x00;
      modeBytes[10] = 0x03;
      modeBytes[11] = 0x00;
      break;
    case 9:
      modeBytes[8] = 0x1E;
      modeBytes[9] = 0x01;
      modeBytes[10] = 0x03;
      modeBytes[11] = 0x00;
      break;
    case 10:
      modeBytes[8] = 0x1E;
      modeBytes[9] = 0x00;
      modeBytes[10] = 0x03;
      modeBytes[11] = 0x00;
      break;
    case 11:
      modeBytes[8] = 0x32;
      modeBytes[9] = 0x00;
      modeBytes[10] = 0x03;
      modeBytes[11] = 0x00;
      break;
    case 12:
      modeBytes[8] = 0x3C;
      modeBytes[9] = 0x01;
      modeBytes[10] = 0x03;
      modeBytes[11] = 0x00;
      break;
    case 13:
      modeBytes[8] = 0x3C;
      modeBytes[9] = 0x00;
      modeBytes[10] = 0x03;
      modeBytes[11] = 0x00;
      break;
    case 14:
      modeBytes[8] = 0x18;
      modeBytes[9] = 0x01;
      modeBytes[10] = 0x06;
      modeBytes[11] = 0x00;
      break;
    case 15:
      modeBytes[8] = 0x18;
      modeBytes[9] = 0x00;
      modeBytes[10] = 0x06;
      modeBytes[11] = 0x00;
      break;
    case 16:
      modeBytes[8] = 0x19;
      modeBytes[9] = 0x00;
      modeBytes[10] = 0x06;
      modeBytes[11] = 0x00;
      break;
    case 17:
      modeBytes[8] = 0x1E;
      modeBytes[9] = 0x01;
      modeBytes[10] = 0x06;
      modeBytes[11] = 0x00;
      break;
    case 18:
      modeBytes[8] = 0x1E;
      modeBytes[9] = 0x00;
      modeBytes[10] = 0x06;
      modeBytes[11] = 0x00;
      break;
  }
  sdiCameraControl.write(modeBytes);
}

void writeSensorGain() {
  sdiCameraControl.writeCommandInt8(cameraNum, 0x01, 0x01, 0x00, sensorGains[sensorGainIndex]);
}

void writeWhiteBalance() {
  sdiCameraControl.writeCommandInt16(cameraNum, 0x01, 0x02, 0x00, whiteBalances[whiteBalanceIndex]);
}

void writeShutterSpeed() {
  sdiCameraControl.writeCommandInt32(cameraNum, 0x01, 0x05, 0x00, shutterUs());
}

void writeAperture() {
  sdiCameraControl.writeCommandFixed16(cameraNum, 0x00, 0x02, 0x00, fStopConverted());
}

void writeFocus() {
  sdiCameraControl.writeCommandFixed16(cameraNum, 0x00, 0x00, 0x00, focus);
}

void writeZoom() {
  sdiCameraControl.writeCommandInt16(cameraNum, 0x00, 0x07, 0x00, zoom);
}

void writeOverlays() {
  sdiCameraControl.writeCommandInt16(cameraNum, 0x03, 0x00, 0x00, overlays ? 65535 : 0);
}

void writeZebras() {
  sdiCameraControl.writeCommandInt16(cameraNum, 0x04, 0x01, 0x00, zebras ? 65535 : 0);
}

void refresh() {
  updateLcd();
}

void handleSerialData() {
  incomingModeByte = Serial.read();
  if (incomingModeByte == 255) {
    return;
  }
  incomingValByte = Serial.read();
  if (incomingValByte == 255) {
    return;
  }
  incomingBreakByte = Serial.read();
  if (incomingBreakByte != 255) {
    return;
  }
  switch (incomingModeByte) {
    case FUNC_CAMERA:
      cameraNum = incomingValByte;
      break;
    case FUNC_MODE:
      videoModeIndex = incomingValByte;
      writeVideoMode();
      break;
    case FUNC_GAIN:
      sensorGainIndex = incomingValByte;
      writeSensorGain();
      break;
    case FUNC_TEMP:
      whiteBalanceIndex = incomingValByte;
      writeWhiteBalance();
      break;
    case FUNC_SHUTTER:
      shutterSpeedIndex = incomingValByte;
      writeShutterSpeed();
      break;
    case FUNC_APERTURE:
      fStopIndex = incomingValByte;
      writeAperture();
      break;
    case FUNC_FOCUS:
      focus = float(incomingValByte) / 100.0;
      writeFocus();
      break;
    case FUNC_ZOOM:
      zoom = incomingValByte;
      writeZoom();
      break;
    case FUNC_OVERLAYS:
      overlays = incomingValByte == 1 ? true : false;
      writeOverlays();
      break;
    case FUNC_ZEBRAS:
      zebras = incomingValByte == 1 ? true : false;
      writeZebras();
      break;
  }
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.print("               >");
  
  pinMode(pin_A, INPUT_PULLUP);
  pinMode(pin_B, INPUT_PULLUP);
  pinMode(pin_back, INPUT_PULLUP);
  pinMode(pin_go, INPUT_PULLUP);

  sdiCameraControl.begin();
  sdiCameraControl.setOverride(true);

  refresh();
  writeFocus();
}

void loop() {
  currentTime = millis();
  if(currentTime >= (loopTime + 2)){
    encoder_A = digitalRead(pin_A);
    encoder_B = digitalRead(pin_B);
    if((!encoder_A) && (encoder_A_prev)){
      if(encoder_B) {
        clockwise();
        refresh();
      }
      else {
        counterClockwise();
        refresh();
      }
    }
    encoder_A_prev = encoder_A;

    loopTime = currentTime;
  }
  
  if (digitalRead(pin_back) == LOW) {
    if (backReady) {
      goBack();
      refresh();
    }
    backReady = false;
  } else {
    backReady = true;
  }
  if (digitalRead(pin_go) == LOW) {
    if (goReady) {
      goForward();
      refresh();
    }
    goReady = false;
  } else {
    goReady = true;
  }

  if (Serial.available() > 2) {
    handleSerialData();
  }
}
