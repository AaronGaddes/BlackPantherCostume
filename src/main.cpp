#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <FastLED.h>

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

// Pin attached to LED strip's data line
#define LED_DATA_PIN        GPIO_NUM_5

// LED Strip type
#define LED_TYPE        WS2811

// LED Strip's color order
#define COLOR_ORDER     GRB

// Maximum number of LEDs.
// This is for on-the-fly calibration
// when connecting different crystals
// with different length LED strips
#define MAX_LEDS        32

// Maximum voltage and miliamp to use with the FAST_LED library
// to attempt to prevent against exces power usage
// and catastrophic failures
#define MAX_VOLTAGE     5
#define MAX_MILLI_AMPS  1000

// Create Array of LEDs
// using the Maximum number of LEDs
// expected to be used
CRGB leds[MAX_LEDS];

#define FRAMES_PER_SECOND  120

// Set the initial number of LEDs to be the maximum number
uint8_t NUM_LEDS = MAX_LEDS;

// Theses are used to control the pulse pattern
// As well as the maximum brightness of the LED strip
uint8_t MAX_BRIGHTNESS = 50;
uint8_t BRIGHTNESS = 0;
uint8_t pulseFadeAmount = 5;

// The current pattern, set by 
uint8_t currentPattern = 0;


// Initial colour of the LEDs 'Solid' state
long baseColor = CRGB::Purple;

// rotating "base color" used by some of the patterns
uint8_t gHue = 0;

// Touch Pins
#define TOUCH_PIN_0 GPIO_NUM_4
#define TOUCH_PIN_1 GPIO_NUM_13

int sensor0Read = 0;
int sensor1Read = 0;
uint8_t value0 = 0;
uint8_t value1 = 0;

#define serviceID BLEUUID((uint16_t)0x1700)

BLECharacteristic colorCharacteristic(
  BLEUUID((uint16_t)0x1A00),
  BLECharacteristic::PROPERTY_READ |
  BLECharacteristic::PROPERTY_WRITE
);

BLECharacteristic patternCharacteristic(
  BLEUUID((uint16_t)0x1A01),
  BLECharacteristic::PROPERTY_READ |
  BLECharacteristic::PROPERTY_WRITE
);

// BLECharacteristic brightnessCharacteristic(
//   BLEUUID((uint16_t)0x1A02),
//   BLECharacteristic::PROPERTY_READ |
//   BLECharacteristic::PROPERTY_WRITE
// );

bool deviceConnected = false;
char colorValueFromBLE[7] = "FF00FF";

class ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* MyServer) {
    deviceConnected = true;
    Serial.println("Connected to a device");
  };

  void onDisconnect(BLEServer* MyServer) {
    deviceConnected = false;
    Serial.println("Disconnected");
    MyServer->getAdvertising()->start();
  };
};


void changeColor(std::string colorString) {

  baseColor = strtol(colorString.c_str(), NULL, 16);

}

void changePattern(std::string patternNumAsString) {
  currentPattern = strtol(patternNumAsString.c_str(), NULL, 8);
}

// void changeBrightness(std::string brightnessNumAsString) {
//   MAX_BRIGHTNESS = strtol(brightnessNumAsString.c_str(), NULL, 8);
//   FastLED.setBrightness(MAX_BRIGHTNESS);
// }


class ColorCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *colorCharacteristic){
    std::string rcvString = colorCharacteristic->getValue();

    char value[rcvString.length() + 1];

    if(rcvString.length()>0){
      Serial.println("Color Value Received from BLE: ");
      for (int i = 0; i < rcvString.length(); ++i)
      {
        Serial.print(rcvString[i]);
        value[i]=rcvString[i];
      }
      for (int i = rcvString.length(); i < 50; ++i)
      {
        value[i]=NULL;
      }
      changeColor(value);
      colorCharacteristic->setValue((char*)&value);
    }
    else{
      Serial.println("Empty Value Received!");
    }
  }
};

class PatternCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *patternCharacteristic){
    std::string rcvString = patternCharacteristic->getValue();
    
    char value[rcvString.length() + 1];

    if(rcvString.length()>0){
      Serial.println("Pattern Value Received from BLE: ");
      for (int i = 0; i < rcvString.length(); ++i)
      {
        Serial.print(rcvString[i]);
        value[i]=rcvString[i];
      }
      for (int i = rcvString.length(); i < 50; ++i)
      {
        value[i]=NULL;
      }
      changePattern(value);
      patternCharacteristic->setValue((char*)&value);
    }
    else{
      Serial.println("Empty Value Received!");
    }
  }
};

// class BrightnessCharacteristicCallbacks: public BLECharacteristicCallbacks {
//   void onWrite(BLECharacteristic *brightnessCharacteristic){
//     std::string rcvString = brightnessCharacteristic->getValue();
    
//     char value[rcvString.length() + 1];

//     if(rcvString.length()>0){
//       Serial.println("Brightness Value Received from BLE: ");
//       for (int i = 0; i < rcvString.length(); ++i)
//       {
//         Serial.print(rcvString[i]);
//         value[i]=rcvString[i];
//       }
//       for (int i = rcvString.length(); i < 50; ++i)
//       {
//         value[i]=NULL;
//       }
//       changeBrightness(value);
//       brightnessCharacteristic->setValue((char*)&value);
//     }
//     else{
//       Serial.println("Empty Value Received!");
//     }
//   }
// };


// ############## Patterns ##############
// And supporting functions

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void solid() {
  for(int i = 0; i < NUM_LEDS; i++ )
   {
     leds[i] = baseColor;
  }
  FastLED.show();
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void setup() {
  // put your setup code here, to run once:
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,LED_DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  // set master brightness control
  FastLED.setBrightness(MAX_BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(MAX_VOLTAGE, MAX_MILLI_AMPS);

  Serial.begin(115200);

  BLEDevice::init("BlackPantherConstume");

  BLEServer *MyServer = BLEDevice::createServer();
  MyServer->setCallbacks(new ServerCallbacks());

  BLEService *customService = MyServer->createService(BLEUUID((uint16_t)0x1700));


  // Color Characteristic
  customService->addCharacteristic(&colorCharacteristic);

  colorCharacteristic.addDescriptor(new BLE2902());

  BLEDescriptor ColorDescriptor(BLEUUID((uint16_t)0x2901));
  ColorDescriptor.setValue("Color");
  colorCharacteristic.addDescriptor(&ColorDescriptor);

  colorCharacteristic.setCallbacks(new ColorCharacteristicCallbacks());


  // Pattern Characteristic
  customService->addCharacteristic(&patternCharacteristic);

  patternCharacteristic.addDescriptor(new BLE2902());

  BLEDescriptor PatternDescriptor(BLEUUID((uint16_t)0x2901));
  PatternDescriptor.setValue("Pattern");
  patternCharacteristic.addDescriptor(&PatternDescriptor);

  patternCharacteristic.setCallbacks(new PatternCharacteristicCallbacks());


  // Brightness Characteristic
  // customService->addCharacteristic(&brightnessCharacteristic);

  // brightnessCharacteristic.addDescriptor(new BLE2902());

  // BLEDescriptor BrightnessDescriptor(BLEUUID((uint16_t)0x2901));
  // BrightnessDescriptor.setValue("Brightness");
  // brightnessCharacteristic.addDescriptor(&BrightnessDescriptor);

  // brightnessCharacteristic.setCallbacks(new PatternCharacteristicCallbacks());


  // Initialize Advertising
  MyServer->getAdvertising()->addServiceUUID(serviceID);

  customService->start();

  MyServer->getAdvertising()->start();

  Serial.println(colorValueFromBLE);
  colorCharacteristic.setValue((char*)&colorValueFromBLE);

  Serial.println("Waiting for a Client to connect...");

}

void loop() {
  // solid();
  // put your main code here, to run repeatedly:
  // sensor0Read = touchRead(TOUCH_PIN_0);
  // sensor1Read = touchRead(TOUCH_PIN_1);
  // uint8_t newValue0 = map(sensor0Read, 120, 0, 0 , 1);
  // value1 = map(sensor1Read, 0, 120, 0 , 1);
  // Serial.print(newValue0);
  // Serial.print(',');
  // Serial.println(value1);

  // if(deviceConnected && newValue0 != value0) {
  //   value0 = newValue0;
  //   colorCharacteristic.setValue(&value0,1);
  //   colorCharacteristic.notify();
  // }


  if(currentPattern == 0) {
    // Serial.println("setting to solid");
    solid();
  }
  else if(currentPattern == 1) {
    // Serial.println("setting to rainbow");
    rainbow();
  }
  else if(currentPattern == 2) {
    // Serial.println("setting to confetti");
    confetti();
  }
  else if(currentPattern == 3) {
    // Serial.println("setting to sinelon");
    sinelon();
  }
  else if(currentPattern == 4) {
    // Serial.println("setting to bpm");
    bpm();
  }else {
      FastLED.clear();
    }
    // send the 'leds' array out to the actual LED strip
    FastLED.show();  
    // insert a delay to keep the framerate modest
    FastLED.delay(1000/FRAMES_PER_SECOND); 

    // slowly cycle the "base color" through the rainbow
    // for the 'rainbow' pattern
    EVERY_N_MILLISECONDS( 20 ) { gHue++; }

  // delay(100);
}