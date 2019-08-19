// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_STMPE610_RK.h>

// This #include statement was automatically added by the Particle IDE.
#if defined(PARTICLE)
 #include <Adafruit_mfGFX.h>
 #include "Adafruit_ILI9341.h"

 // For the Adafruit shield, these are the default.
 #define TFT_DC D5
 #define TFT_CS D4

#else
 #include "SPI.h"
 #include <Adafruit_mfGFX.h>
 #include "Adafruit_ILI9341.h"

 // For the Adafruit shield, these are the default.
 #define TFT_DC 9
 #define TFT_CS 10

#endif
#define TS_MINX 3800
#define TS_MAXX 100
#define TS_MINY 100
#define TS_MAXY 3750

const int STMPE_CS = D3;

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, 0);
Adafruit_STMPE610 touch = Adafruit_STMPE610(STMPE_CS);
// If using the breakout, change pins as desired
//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

const uint32_t baud = 19200;

int makeCoffee(String command);
int len;
char szTX[64];
char szRX[64];
String inData;
char space = 32;
String weight;
int brewWeight;
boolean makingCoffee = false;

//adam variables
int ms = 0;
int s = 0;
int coffee = 36;
int brewtime = 25;
int oldy,oldx = 0;
long brewTimeStarted=0;

void setup() {



  Serial1.begin(baud);   // baud rates below 1200 can't be produced by USART
  Serial.begin(9600);
  strcpy(szTX, "TARE$0d$0a"); // add some exotic chars too

  len = strlen(szTX) + 1;
  Serial1.write((uint8_t*)szTX, len);
  //Serial1.write((uint8_t*)szTX, len);

  // TOUCHSCREEN SETUP
  tft.begin();
  touch.begin();
  setupSettingsScreen();

  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);

/*
Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX);
*/

#define BACKCOLOR 0x0000
#define PRINTCOL 0xFFFF
#define CHARBACK 0x001F

  //COFFEE RELAY
  pinMode(A0, OUTPUT);  // set pin as an output
  digitalWrite(A0,LOW); // Turns OFF Relays 1

  Particle.function("makeCoffee", makeCoffee);
  Particle.publish("signal", "start");

  randomSeed(analogRead(A0) + analogRead(A1));

}

void loop() {



    if (!makingCoffee){
    TS_Point p = touch.getPoint();
    p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

      if (touch.touched() ) {
        TS_Point p = touch.getPoint();
        p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
        p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
        while ( !touch.bufferEmpty() || touch.touched())
        {
            p = touch.getPoint();
        }
        p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
        p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());


        if (oldx == p.x || oldy == p.y) ;
        else{ pointTouchedNotBrewing(p.x,p.y);}

        oldx = p.x;
        oldy = p.y;
      }
    }

    if(Serial1.available())
    {
            String scaleInputLine = Serial1.readStringUntil('\r\n');
            char inputStr[64];
            scaleInputLine.toCharArray(inputStr,64);
            char *op = strtok(inputStr," ");
            String first = String(op);
            op = strtok(NULL," ");
            weight = String(op);
            op = strtok(NULL," ");
            String third = String(op);

            weight.remove(0,3); //remove first 4 zeros
            weight.remove(7,4); // last 2 post decimal point
            String theTime;

            if (makingCoffee){
                tft.setCursor(140,100);
                tft.setCursor(95, 50);
                tft.fillRect(95,50, 70, 50, ILI9341_BLACK);
                int weightDisplay = weight.toInt();
                String weightDisplayer = String(weightDisplay);
                weightDisplayer.remove(3,5);
                tft.print(weightDisplayer);
                tft.setCursor(85, 170);
                tft.fillRect(85, 170, 80, 50, ILI9341_BLACK);
                tft.setTextSize(3);
                theTime =  String((millis() /1000.0) - brewTimeStarted).remove(4,10);
                tft.print(theTime);
                tft.setTextSize(4);

            }

            Particle.publish("weight", weight);

            if((weight.toInt() > (brewWeight - 2)) && makingCoffee) // stop making coffee
            {
                Particle.publish("Stopped here weight reading", weight);
                Particle.publish("Stopped here scale reading", scaleInputLine);
                toggleRelay();
                Particle.publish("time", theTime);
                makingCoffee = false;
            }

    }


}


//Buttons logic from settings page
void pointTouchedNotBrewing(int x, int y){
    if (x > (5) && x < (75) && y < 80 && y >10) {
         coffee = coffee-1;
         tft.setCursor(95, 50);
         tft.fillRect(95,50, 50, 50, ILI9341_BLACK);
         tft.print(String(coffee));
    }
    else if (x > (165) && x < (235) && y < 80 && y >10) {
         coffee = coffee+1;
         tft.setCursor(95, 50);
         tft.fillRect(95,50, 50, 50, ILI9341_BLACK);
         tft.print(String(coffee));
    }
    else if (x > (5) && x < (75) && y < 200 && y >130) {
        brewtime = brewtime-1;
        tft.setCursor(95, 170);
        tft.fillRect(95, 170, 50, 50, ILI9341_BLACK);
        tft.print(String(brewtime));
    }
    else if (x > (165) && x < (235) && y < 200 && y >130) {
        brewtime =  brewtime+1;
        tft.setCursor(95, 170);
        tft.fillRect(95, 170, 50, 50, ILI9341_BLACK);
        tft.print(String(brewtime));
    }
    else if (x > 5 && x<235 && y > 230 && y <310){ brew(); }
}


//called when the brew button is press
void brew(){
    makeCoffee(String(coffee));
    setupBrewingScreen();
    tft.setCursor(70,250);
    while (makingCoffee){
        loop();
    }
    setupReadyScreen();
}

void setupBrewingScreen(){
    tft.fillRect(5, 230, 230, 80, ILI9341_BLUE);
    tft.setTextSize(4);
    tft.setCursor(30,250);
    tft.print("BREWING");
    tft.drawRect(5, 230, 230, 80, ILI9341_WHITE);

}

void setupReadyScreen(){
    tft.fillRect(5, 230, 230, 80, ILI9341_CYAN);
    tft.setTextSize(4);
    tft.setCursor(75,250);
    tft.print("BREW");
    tft.drawRect(5, 230, 230, 80, ILI9341_WHITE);
    tft.fillRect(85, 170, 80, 50, ILI9341_BLACK);
    tft.fillRect(95,50, 70, 50, ILI9341_BLACK);
    tft.setCursor(95, 50);
    tft.print(String(coffee));
    tft.setCursor(95, 170);
    tft.print(String(brewtime));


}


//makes the coffee
int makeCoffee(String command)
{
 makingCoffee = true;
 brewWeight = command.toInt();
 brewWeight = brewWeight - 3; //adjust for "last second espresso dripping"
  ms = 0; // reset timers
  s = 0; // reset timers

  //tare the scale
  strcpy(szTX, "TARE$0d$0a"); // add some exotic chars too
  len = strlen(szTX) + 1;
  Serial1.write((uint8_t*)szTX, len);

    toggleRelay(); // start making espresso

}

void toggleRelay()
{
    digitalWrite(A0,HIGH); // Turns ON Relays 1
    delay(500); // Wait 2 seconds
    digitalWrite(A0,LOW); // Turns Relay Off
    brewTimeStarted = millis() / 1000.0;

}



void setupSettingsScreen(){
    tft.fillScreen(ILI9341_BLACK);

    tft.fillCircle(40, 45, 35, ILI9341_RED);
    tft.fillCircle(200, 45, 35, ILI9341_GREEN);

    tft.fillCircle(40, 165,  35, ILI9341_RED);
    tft.fillCircle(200, 165, 35, ILI9341_GREEN);

    tft.fillRect(5, 230, 230, 80, ILI9341_CYAN);

    tft.drawRect(5, 230, 230, 80, ILI9341_WHITE);


    tft.fillRect(15, 40, 50, 10, ILI9341_WHITE);
    tft.fillRect(195, 20, 10, 50, ILI9341_WHITE);
    tft.fillRect(175, 40, 50, 10, ILI9341_WHITE);

    tft.fillRect(15, 160, 50, 10, ILI9341_WHITE);
    tft.fillRect(195, 140, 10, 50, ILI9341_WHITE);
    tft.fillRect(175, 160, 50, 10, ILI9341_WHITE);

    tft.setCursor(85, 20);
    tft.setTextSize(2);

    tft.println("weight");
    tft.setCursor(95, 140);

    tft.println("time");
    tft.setTextSize(4);
    tft.setCursor(75,250);
    tft.print("BREW");

    tft.setCursor(95, 50);
    tft.fillRect(95,50, 50, 50, ILI9341_BLACK);
    tft.print(String(coffee));

    tft.setCursor(95, 170);
    tft.fillRect(95, 170, 50, 50, ILI9341_BLACK);
    tft.print(String(brewtime));

    loop();
}
