/*
  Aquaponics Controller
 
  This code is to be used in conjunction with a european or american two prong plug, resistors, and a temperature probe to
  measure the EC/PPM of a Aquaponics/Hydroponics System. You could modify this code to Measure other solutions if you change
  the resitor and values where appropriate (namely the K value and TemperatureCoef variables).
 
  This Program will serve as the controller of a greater system. It will monitor, record, and house the general commands of a
  hydroponic or other type of aquaponic system. It is broken down to be easily read and digested by anyone with basic object 
  oriented programming skills.

  It is my intention for this code to be easy to add on to and edit for your needs.
  
  @Author Michael Noe
  @Date   15/05/2018
  @Me     michael.noe.a@gmail.com
   
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
 
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
 
    You should have received a copy of the GNU General Public License
    along with this program. 

     
  Parts:
    -Arduino - Mega 2560
    -Standard American two prong plug (or european)
    -1 kohm resistor
    -DS18B20 Waterproof Temperature Sensor
    -TFT Touch Screen (model# EL-SM-004)

  Required Software
    -Elegoo Package (provided in zip. Place in library folder (*_/documents/arduino/libraries) 
    
  Concerns:
    -Power must be reliable and constant. Surges will likely fry your equipment.
    -Exposure to wet surfaces/enviornments is a constant threat to this system.
    
     
//############################################################################################################ 
//############################################################################################################ 
//############################################################################################################ 
 
  See [LINK HERE] for a Pinout and user guide or consult the Zip you got this code from
 
*/


 
//############################################################################################################ 
//************ Libraries Needed To Compile The Script [Links provided in README]****************************//
//############################################################################################################
// IMPORTANT: ELEGOO_TFTLCD LIBRARY MUST BE SPECIFICALLY
// CONFIGURED FOR EITHER THE TFT SHIELD OR THE BREAKOUT BOARD.
// SEE RELEVANT COMMENTS IN Elegoo_TFTLCD.h FOR SETUP.

//
#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library 
#include <OneWire.h>
#include <DallasTemperature.h>

//########################################################################################################## 
//********************************* LCD Variables ********************************************************//
//##########################################################################################################

// The control pins for the LCD can be assigned to any digital or
// analog pins...but we'll use the analog pins as this allows us to
// double up the pins with the touch screen (see the TFT paint example).
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin 

// When using the BREAKOUT BOARD only, use these 8 data lines to the LCD:
// For the Arduino Uno, Duemilanove, Diecimila, etc.:
//   D0 connects to digital pin 8  (Notice these are
//   D1 connects to digital pin 9   NOT in order!)
//   D2 connects to digital pin 2
//   D3 connects to digital pin 3
//   D4 connects to digital pin 4
//   D5 connects to digital pin 5
//   D6 connects to digital pin 6
//   D7 connects to digital pin 7
// For the Arduino Mega, use digital pins 22 through 29
// (on the 2-row header at the end of the board).

// Assign human-readable names to some common 16-bit color values:
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
// If using the shield, all control and data lines are fixed, and
// a simpler declaration can optionally be used:
// Elegoo_TFTLCD tft;

//##################################################################################
//********************* Power Plug EC Probe variables ******************************
//##################################################################################
 
//Do not Replace R1 with a resistor lower than 300 ohms 
int R1= 1000;
int Ra=25; //Resistance of powering Pins
int ECPin   = A11; //data pin
int ECGround= A13; 
int ECPower = A15;
 
//##################################################################################
//********************* PPM Conversion Standards ***********************************
//################################################################################## 
// Converting to ppm is different per region. I recommend using EC exclusively.
// Hana      [USA]        PPMconversion:  0.5
// Eutech    [EU]         PPMconversion:  0.64
// Tranchen  [Australia]  PPMconversion:  0.7
 
float PPMconversion=0.7;
 
//**************** EC Temperature Compensation ************************************//
//The value below will change depending on what chemical solution we are measuring.
//0.019 is generaly considered the standard for plant nutrients.
float TemperatureCoef = 0.019; //Google "Temperature compensation EC" for more info.
 
//##################################################################################
//********************* K Value Constant (CALIBRATION REQUIRED)*********************
//################################################################################## 
//Mine was around 7.9 with an american style plug. The plug was submerged in city tap water.
//You will need to calibrate this on your own. Calibration code is included in zip.
float K=7.9;
 
//************ Temp Probe Related *********************************************//
#define ONE_WIRE_BUS A10          // Data wire For Temp Probe is plugged into pin 10 on the Arduino
const int TempProbePossitive =22;  //Temp Probe power connected to pin 22
const int TempProbeNegative=24;    //Temp Probe Negative connected to pin 24

//##########################################################################################################
//                                          Instantiation 
//##########################################################################################################
OneWire oneWire(ONE_WIRE_BUS);// Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);// Pass our oneWire reference to Dallas Temperature.
 
float Temperature=5;
float EC=0;
float EC25 =0;
int ppm =0;
 
float raw= 0;
float Vin= 5;
float Vdrop= 0;
float Rc= 0;
float buffer=0;

//##########################################################################################################
//                                             Setup 
//########################################################################################################## 
//********************************* Runs once and sets pins etc ******************************************//
void setup()
{
  Serial.begin(9600);
  //////////////////////////////////////////////LCD Stuff///////////////////////////////////////////////////
  Serial.println(F("TFT LCD test"));

  #ifdef USE_Elegoo_SHIELD_PINOUT
    Serial.println(F("Using Elegoo 2.4\" TFT Arduino Shield Pinout"));
  #else
    Serial.println(F("Using Elegoo 2.4\" TFT Breakout Board Pinout"));
  #endif

  Serial.print("TFT size is "); Serial.print(tft.width()); Serial.print("x"); Serial.println(tft.height());

  tft.reset();

  uint16_t identifier = tft.readID();
   if(identifier == 0x9325) {
    Serial.println(F("Found ILI9325 LCD driver"));
  } else if(identifier == 0x9328) {
    Serial.println(F("Found ILI9328 LCD driver"));
  } else if(identifier == 0x4535) {
    Serial.println(F("Found LGDP4535 LCD driver"));
  }else if(identifier == 0x7575) {
    Serial.println(F("Found HX8347G LCD driver"));
  } else if(identifier == 0x9341) {
    Serial.println(F("Found ILI9341 LCD driver"));
  } else if(identifier == 0x8357) {
    Serial.println(F("Found HX8357D LCD driver"));
  } else if(identifier==0x0101)
  {     
      identifier=0x9341;
       Serial.println(F("Found 0x9341 LCD driver"));
  }
  else if(identifier==0x1111)
  {     
      identifier=0x9328;
       Serial.println(F("Found 0x9328 LCD driver"));
  }
  else {
    Serial.print(F("Unknown LCD driver chip: "));
    Serial.println(identifier, HEX);
    Serial.println(F("If using the Elegoo 2.8\" TFT Arduino shield, the line:"));
    Serial.println(F("  #define USE_Elegoo_SHIELD_PINOUT"));
    Serial.println(F("should appear in the library header (Elegoo_TFT.h)."));
    Serial.println(F("If using the breakout board, it should NOT be #defined!"));
    Serial.println(F("Also if using the breakout, double-check that all wiring"));
    Serial.println(F("matches the tutorial."));
    identifier=0x9328;
  
  }
  tft.begin(identifier);
  //////////////////////////////////////////END LCD Stuff/////////////////////////////////////////////////// 
  
  pinMode(TempProbeNegative , OUTPUT ); //seting ground pin as output for tmp probe
  digitalWrite(TempProbeNegative , LOW );//Seting it to ground so it can sink current
  pinMode(TempProbePossitive , OUTPUT );//for positive
  digitalWrite(TempProbePossitive , HIGH );
  pinMode(ECPin,INPUT);
  pinMode(ECPower,OUTPUT);//Setting pin for sourcing current
  pinMode(ECGround,OUTPUT);//setting pin for sinking current
  digitalWrite(ECGround,LOW);//We can leave the ground connected permanantly
 
  delay(100);// gives sensor time to settle
  sensors.begin();
  delay(100);
  //** Adding Digital Pin Resistance to [25 ohm] to the static Resistor *********//
  // Consule Read-Me for Why, or just accept it as true
  R1=(R1+Ra);// Taking into acount Powering Pin Resitance
 
  Serial.println("ElCheapo Arduino EC-PPM measurments");
  Serial.println("By: Michael Noe  michael.noe.a@gmail.com");
  Serial.println("Free software: you can redistribute it and/or modify it under GNU ");
  Serial.println("Adpated from Michael Radcliffe's 'ElCheapo Arduino EC - PPM Meter' project which can be found at");
  Serial.println("https://hackaday.io/project/7008-fly-wars-a-hackers-solution-to-world-hunger/log/24646-three-dollar-ec-ppm-meter-arduino");
  Serial.println("");
  Serial.println("Make sure Probe and Temp Sensor are in Solution and solution is well mixed");
  Serial.println("");
  Serial.println("");
  Serial.println("IMPORTANT! ");
  Serial.println("");
  Serial.println("1) Measurments must be made atleast 5s apart from each other or you wil polarize the liquid (or burn out your equipment");
  Serial.println("2) EC Probe prongs must not touch the side of the vessel. If any prong touches side of the vessel holding your liquid");
  Serial.println("   it will skew the reading");
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println("_________________________________________________________________________________________________________________________________");
 
};
//##########################################################################################################
//                                             Main Loop
//##########################################################################################################
//************************************* Main Loop - Runs Forever ***************************************************************//
void loop()
{

GetEC();          //Calls Code to Go into GetEC() Loop [Below Main Loop] dont call this more that 1/5 hhz [once every five seconds] or you will polarise the water
PrintReadings();  // Cals Print routine [below main loop]
delay(5000);
}
//##########################################################################################################
//                                             Get EC
//########################################################################################################## 
//************ This Loop Is called From Main Loop************************//
void GetEC(){
//*********Reading Temperature Of Solution *******************//
sensors.requestTemperatures();// Send the command to get temperatures
Temperature=sensors.getTempCByIndex(0); //Stores Value in Variable

//************Estimates Resistance of Liquid ****************//
digitalWrite(ECPower,HIGH);
raw= analogRead(ECPin);
raw= analogRead(ECPin);// This is not a mistake, First reading will be low.
digitalWrite(ECPower,LOW);

//***************** Converts to EC **************************//
Vdrop= (Vin*raw)/1024.0;
Rc=(Vdrop*R1)/(Vin-Vdrop);
Rc=Rc-Ra; //acounting for Digital Pin Resitance
EC = 1000/(Rc*K);
 
//*************Compensating For Temperaure********************//
EC25  =  EC/ (1+ TemperatureCoef*(Temperature-25.0));
ppm=(EC25)*(PPMconversion*1000) * 2;
 
;}
//##########################################################################################################
//                                             Print Readings
//##########################################################################################################  
//***This Loop Is called From Main Loop- Prints to serial usefull info ***//
void PrintReadings(){
tft.fillScreen(BLACK);
unsigned long start = micros();
tft.setCursor(0, 0);
  
tft.setTextColor(RED);  tft.setTextSize(4);
tft.print("Rc: ");
tft.print(Rc);
tft.println();
tft.println();
  
tft.setTextColor(WHITE); tft.setTextSize(3);
tft.print("EC: ");
tft.print(EC25);

tft.setTextColor(BLUE);  tft.setTextSize(3);
tft.print(" PPM: ");
tft.print(ppm);
tft.println();

tft.setTextColor(WHITE); tft.setTextSize(3);
tft.print(C*: ");
tft.print(Temperature);
tft.println();

tft.println();
  

  
  
//This set of output will print to seial monitor. Useful for without an lcd.
/*
Serial.print("Rc: ");
Serial.print(Rc);
Serial.print(" EC: ");
Serial.print(EC25);
Serial.print(" Simens  ");
Serial.print(ppm);
Serial.print(" ppm  ");
Serial.print(Temperature);
Serial.println(" *C ");
*/ 
 
/*
//##########################################################################################################
//                                                  DEBUG
//########################################################################################################## 
//********** Usued for Debugging ************
Serial.print("Vdrop: ");
Serial.println(Vdrop);
Serial.print("Rc: ");
Serial.println(Rc);
Serial.print(EC);
Serial.println("Siemens");
*/
};
