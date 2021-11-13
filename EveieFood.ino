// Version 1.5.1 11/13/2020
//---------------------------------------------------------------------------------------------------------------------------------------------------------
#include <SPI.h>
#include <SD.h>
#include <Servo.h>
#include <DS3231.h>
#include <Wire.h>
#include <pitches.h>
#include <LiquidCrystal.h>

#define BUZZER_PIN 3
#define LCD_MASTER 28
#define LEFT_BUTTON 22
#define RIGHT_BUTTON 23
#define SELECT_BUTTON 24
#define BACK_BUTTON 25
#define SDCSPIN 53
#define SERVOAPIN 5
#define SERVOBPIN 6
#define CLEARWRITESPEED 25
#define LOOPSPEED 5
#define RESET_TRANS 27
#define SERVOA_POWER_TRANS 30
#define SERVOB_POWER_TRANS 31
#define SERVOUPPOSE 179
#define SERVODOWNPOSE 0
#define GATEFALLTIME 5000
#define GATESWAPTIME 3000
#define DONEFLASH 750
#define RINGS 10
#define TIMERCOOLDOWNMS 180000
#define LOGOPTIONSLENGTH 6
#define UNSIGNEDLONGINTARDUINO 4294967296
#define FILELOGPATH "CDL/CDLLOG.TXT"
#define FEEDLOGTEXT "Feeding Complete"
#define ARROW " ---> "
#define LOGOPTIONS {"Test Log        ","Walk            ","Dog went pee    ","Dog went poo    ","Ate             ","Vet             "}

uint16_t beepNote = NOTE_G7; //G7
uint8_t beepNoteIndex = 82;
uint16_t badBeepNote = NOTE_C7; //C7
uint8_t badBeepNoteIndex = 75;
uint16_t ringNote = NOTE_C8; //A4
uint8_t ringNoteIndex = 85;
const uint16_t notes[89] = {NOTE_B0, NOTE_C1, NOTE_CS1, NOTE_D1, NOTE_DS1, NOTE_E1, NOTE_F1, NOTE_FS1, NOTE_G1, NOTE_GS1, NOTE_A1, NOTE_AS1, NOTE_B1, NOTE_C2, NOTE_CS2, NOTE_D2, NOTE_DS2, NOTE_E2,
                            NOTE_F2, NOTE_FS2, NOTE_G2, NOTE_GS2, NOTE_A2, NOTE_AS2, NOTE_B2, NOTE_C3, NOTE_CS3, NOTE_D3, NOTE_DS3, NOTE_E3, NOTE_F3, NOTE_FS3, NOTE_G3, NOTE_GS3, NOTE_A3, NOTE_AS3, NOTE_B3, NOTE_C4,
                            NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4, NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5,
                            NOTE_A5, NOTE_AS5, NOTE_B5, NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6, NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7,
                            NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7, NOTE_B7, NOTE_C8, NOTE_CS8, NOTE_D8, NOTE_DS8
                           };
const uint8_t daysPerMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

DS3231 clockI2C;
RTCDateTime dt;
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
Servo loadGate;
Servo releaseGate;
File logFile;

bool lastLeft = false, lastRight = false, lastSelect = false, lastBack = false;
bool currentLeft = false, currentRight = false, currentSelect = false, currentBack = false;

int menuLocation = 1;
int subMenuLocation = 1;
long unsigned int lastWake = millis();
short int sleepDelay = 30;

uint16_t year;
uint8_t month;
uint8_t day;
uint8_t hour;
uint8_t minute;
uint8_t second;
char timeString[32];
char temp[4];

uint16_t setyear;
uint8_t setmonth;
uint8_t setday;
uint8_t sethour;
uint8_t setminute;
uint8_t setsecond;
uint8_t setCyclesToSkip;
uint8_t logChoice;

uint8_t hourtimer1 = 8;
uint8_t minutetimer1 = 0;
bool timer1 = true;
uint8_t hourtimer2 = 18;
uint8_t minutetimer2 = 0;
bool timer2 = true;
uint8_t hourtimer3 = 14;
uint8_t minutetimer3 = 0;
bool timer3 = false;
uint8_t feedTimerOn = 0;
uint8_t cyclesToSkip = 0;

//RECOMMENDED 1/7 SCOOP PER DAY PER POUND OF DOG (ex: dog is 28 pounds, give him 1 scoop breakfast, 1 scoop lunch, 2 scoop dinner). Changeable in UI
uint8_t scoopsPerFeed1 = 1;
uint8_t scoopsPerFeed2 = 1;
uint8_t scoopsPerFeed3 = 1;
unsigned long int lastTimerFeed = 0; //a millisecond log like lastWake
bool coolDown = 1; // a variable to say if the timer has NOT fed in the last TIMERCOOLDOWNMS, in other words if it is ready to feed again. To avoid repeat-feeding timers.

void setup() {
  Serial.begin(9600);
  //while(!Serial){}
  if(!SD.begin(SDCSPIN)){Serial.println("ERROR With SD card module. Try using a smaller SD card, make sure it is correctly FAT formatted, and try not using Quick Format. Make sure the wiring is correct.");}
  pinMode(LCD_MASTER, OUTPUT);
  pinMode(LEFT_BUTTON, INPUT);
  pinMode(RIGHT_BUTTON, INPUT);
  pinMode(SELECT_BUTTON, INPUT);
  pinMode(BACK_BUTTON, INPUT);
  pinMode(RESET_TRANS, OUTPUT);
  pinMode(SERVOA_POWER_TRANS, OUTPUT);
  pinMode(SERVOB_POWER_TRANS, OUTPUT);
  servoPower(HIGH);
  loadGate.attach(SERVOAPIN);
  releaseGate.attach(SERVOBPIN);
  loadGate.write(SERVOUPPOSE);
  releaseGate.write(SERVOUPPOSE);
  digitalWrite(LCD_MASTER, HIGH);
  digitalWrite(RESET_TRANS, LOW);
  clockI2C.begin();
  //clockI2C.setDateTime(__DATE__, __TIME__); //comment out if you don't want to reset date/time on upload
  dt = clockI2C.getDateTime();
  //strcpy(timeString[32],clockI2C.dateFormat("d-m-Y H:i:s", dt));
  fillTimeVars();
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  servoPower(LOW);
  /*Serial.print("begin");
  Serial.println(lastWake);
  Serial.println(millis());*/
}

//fillButtonVars(): this is a function that feeds the button presses into the readRising variables. It checks if a button just switched from off to on, and it is used once at the beginning of the void loop(). HOWEVER:
//if a select of a menu leads to a menuLocation that is LATER in the SAME void loop(), the function is used to wipe the fact that a button was now pressed (during the same void loop). This is because without this,
//during the same void loop it would select through multiple menu's because since the button was pressed during that void loop, it could penetrate multiple selection checks which is not intended. If you are adding code
//and you come into circumstances that obey this rule, you must either halt the current void loop with a return, or use the fillButtonVars() function so multiple menus aren't navigated at once. But, it is okay to not use either 
//of these approaches if no more button presses are checked for in the current void loop. In summary, the overriding usage of the fillButtonVars() function is to avoid multiple button presses of the same button in
//a single void loop.

void loop() {
  /*Serial.println(((millis() - lastWake) % UNSIGNEDLONGINTARDUINO));
  Serial.print(" : ")
  Serial.print(  (sleepDelay * 1000))*/
  lcd.setCursor(0, 0);
  fillButtonVars();
  fillTimeVars();
  //if last wake up is eval(sleepDelay) seconds before current time, fall asleep.
  digitalWrite(LCD_MASTER, !(((millis() - lastWake) % UNSIGNEDLONGINTARDUINO) > (sleepDelay * 1000)));
  if (readRising(&currentLeft, &lastLeft) || readRising(&currentRight, &lastRight) || readRising(&currentSelect, &lastSelect) || readRising(&currentBack, &lastBack)) {
    wake();
  }
  if (readRising(&currentLeft, &lastLeft) && (menuLocation > 1) && (menuLocation < 16)) { //scroll menu options
    menuLocation--;
    fillButtonVars();
    resetLCD();
    wake();
    smallBeep(beepNote);
  }

  if (readRising(&currentRight, &lastRight) && (menuLocation < 15)) { //scroll menu options
    menuLocation++;
    fillButtonVars();
    resetLCD();
    wake();
    smallBeep(beepNote);
  }


  if (menuLocation == 1) { //Default Menu showing Time and Date, for menu for resetting time and date, goto menuLocation==16
    lcd.setCursor(0, 0);
    lcd.setCursor(4, 0);
    if (hour < 10) {
      lcd.print("0");
    }
    lcd.print(hour);
    lcd.setCursor(6, 0);
    lcd.print(":");
    lcd.setCursor(7, 0);
    if (minute < 10) {
      lcd.print("0");
    }
    lcd.print(minute);
    lcd.setCursor(9, 0);
    lcd.print(":");
    lcd.setCursor(10, 0);
    if (second < 10) {
      lcd.print("0");
    }
    lcd.print(second);
    lcd.setCursor(3, 1);
    if (month < 10) {
      lcd.print("0");
    }
    lcd.print(month);
    lcd.setCursor(5, 1);
    lcd.print("-");
    lcd.setCursor(6, 1);
    if (day < 10) {
      lcd.print("0");
    }
    lcd.print(day);
    lcd.setCursor(8, 1);
    lcd.print("-");
    lcd.setCursor(9, 1);
    if (year < 10) {
      lcd.print("0");
    }
    lcd.print(year);

    if (readRising(&currentSelect, &lastSelect)) {
      menuLocation = 16;
      subMenuLocation = 1;
      //fillButtonVars();
      resetLCD();
      wake();
      smallBeep(beepNote);
      setyear = year;
      setmonth = month;
      setday = day;
      sethour = hour;
      setminute = minute;
      setsecond = second;
      return;
    }
    delay(CLEARWRITESPEED);
  }

  if (menuLocation == 2) { //Header for option for changing amount of gate releases when it's feeding time, for the changing interface, goto menulocation==17
    lcd.print("NextCyclesToSkip");
    lcd.setCursor(0, 1);
    lcd.print(cyclesToSkip);
    if (readRising(&currentSelect, &lastSelect)) {
      menuLocation = 17;
      resetLCD();
      wake();
      smallBeep(beepNote);
      setCyclesToSkip = cyclesToSkip;
      return;
    }
  }

  if (menuLocation == 3) { //header for option for feeding time #1, for the changing interface, goto menuLocation==18
    lcd.print("Feed 1");
    uint8_t scoopDigits = scoopsPerFeed1 < 10 ? 1 : (scoopsPerFeed1 < 100 ? 2 : 3);
    lcd.setCursor(16 - 1 - scoopDigits, 0);
    lcd.print("x");
    lcd.print(scoopsPerFeed1);
    lcd.setCursor(0, 1);
    if (hourtimer1 < 10) {
      lcd.print("0");
    }
    lcd.print(hourtimer1);
    lcd.print(":");
    if (minutetimer1 < 10) {
      lcd.print("0");
    }
    lcd.print(minutetimer1);
    lcd.setCursor(13 + (timer1 == 1), 1);
    if (timer1) {
      lcd.print("ON");
    }
    else {
      lcd.print("OFF");
    }
    if (readRising(&currentSelect, &lastSelect)) {
      menuLocation = 18;
      //fillButtonVars();
      resetLCD();
      wake();
      smallBeep(beepNote);
      return;
    }
  }

  if (menuLocation == 4) { //header for option for feeding time #2, for the changing interface, goto menuLocation==19
    lcd.print("Feed 2");
    uint8_t scoopDigits = scoopsPerFeed2 < 10 ? 1 : (scoopsPerFeed2 < 100 ? 2 : 3);
    lcd.setCursor(16 - 1 - scoopDigits, 0);
    lcd.print("x");
    lcd.print(scoopsPerFeed2);
    lcd.setCursor(0, 1);
    if (hourtimer2 < 10) {
      lcd.print("0");
    }
    lcd.print(hourtimer2);
    lcd.print(":");
    if (minutetimer2 < 10) {
      lcd.print("0");
    }
    lcd.print(minutetimer2);
    lcd.setCursor(13 + (timer2 == 1), 1);
    if (timer2) {
      lcd.print("ON");
    }
    else {
      lcd.print("OFF");
    }
    if (readRising(&currentSelect, &lastSelect)) {
      menuLocation = 19;
      //fillButtonVars();
      resetLCD();
      wake();
      smallBeep(beepNote);
      return;
    }
  }

  if (menuLocation == 5) { //header for option for feeding time #3, for the changing interface, goto menuLocation==20
    lcd.print("Feed 3");
    uint8_t scoopDigits = scoopsPerFeed3 < 10 ? 1 : (scoopsPerFeed3 < 100 ? 2 : 3);
    lcd.setCursor(16 - 1 - scoopDigits, 0);
    lcd.print("x");
    lcd.print(scoopsPerFeed3);
    lcd.setCursor(0, 1);
    if (hourtimer3 < 10) {
      lcd.print("0");
    }
    lcd.print(hourtimer3);
    lcd.print(":");
    if (minutetimer3 < 10) {
      lcd.print("0");
    }
    lcd.print(minutetimer3);
    lcd.setCursor(13 + (timer3 == 1), 1);
    if (timer3) {
      lcd.print("ON");
    }
    else {
      lcd.print("OFF");
    }
    if (readRising(&currentSelect, &lastSelect)) {
      menuLocation = 20;
      //fillButtonVars();
      resetLCD();
      wake();
      smallBeep(beepNote);
      return;
    }
  }

  if (menuLocation == 6) { //option for turning off feed timer
    if (readRising(&currentSelect, &lastSelect)) {
      feedTimerOn++;
      if (feedTimerOn == 3){feedTimerOn = 0;}
      resetLCD();
      wake();
      smallBeep(beepNote);
    }
    lcd.setCursor(0,0);
    if (feedTimerOn == 0) { //yes I know that this could be if !feedTimerOn but I want to make it clear
      lcd.print("Feed Timer Off  ");
    } 
    if (feedTimerOn == 1){
      lcd.print("Feed Timer On   ");
    }
    if (feedTimerOn == 2){
      lcd.print("Alarm Mode On   ");
    }
  }

  if (menuLocation == 7) { //header for option for setting pitch of the beepNote, interface in menuLocation==22
    lcd.print("Set Beep Pitch");
    if (readRising(&currentSelect, &lastSelect)) {
      menuLocation = 22;
      resetLCD();
      wake();
      smallBeep(beepNote);
      return;
    }
  }

  if (menuLocation == 8) { //header for option for setting pitch of the badBeepNote, interface in menuLocation==23
    lcd.print("Set Beep Pitch 2");
    if (readRising(&currentSelect, &lastSelect)) {
      menuLocation = 23;
      resetLCD();
      wake();
      smallBeep(beepNote);
      return;
    }
  }

  if (menuLocation == 9) { //header for option for setting pitch of the ringNote, interface in menuLocation==24
    lcd.print("Set Ring Pitch");
    if (readRising(&currentSelect, &lastSelect)) {
      menuLocation = 24;
      resetLCD();
      wake();
      smallBeep(beepNote);
      return;
    }
  }
  
  if (menuLocation == 10) { //Header for option for changing amount of gate releases when it's feeding time, for the changing interface, goto menulocation==17
    lcd.print("Create SD log  ");
    if (readRising(&currentSelect, &lastSelect)) {
      menuLocation = 25;
      //fillButtonVars();
      resetLCD();
      wake();
      smallBeep(beepNote);
      logChoice = 0;
      return;
    }
  }

  if (menuLocation == 11) {//bridge skip from menu10 to menu13 because nothing there is coded yet.
    menuLocation = 13;
  }
  if (menuLocation == 12) {//bridge skip from menu12 to menu9 because nothing there is coded yet.
    menuLocation = 10;
  }

  if (menuLocation == 13) { //header option for starting feeder, onselect goes to menuLocation==28, the confirm
    lcd.print("Feed Now");
    if (readRising(&currentSelect, &lastSelect)) {
      menuLocation = 28;
      //fillButtonVars();
      resetLCD();
      wake();
      smallBeep(beepNote);
      return;
    }
  }

  if (menuLocation == 14) { //header option for pulling arduino reset pin low, onselect goes to menuLocation==29, the confirm
    lcd.print("Reset All");
    if (readRising(&currentSelect, &lastSelect)) {
      menuLocation = 29;
      //fillButtonVars();
      resetLCD();
      wake();
      smallBeep(beepNote);
      return;
    }
  }

  if (menuLocation == 15) { //header option for credits,  see menulocation==30 for credits
    lcd.print("Credits & Info");
    if (readRising(&currentSelect, &lastSelect)) {
      menuLocation = 30;
      subMenuLocation = 1;
      resetLCD();
      wake();
      smallBeep(beepNote);
      return;
    }
  }

  
//<=15 is the headers, from now on the interfaces to which the headers lead.


  if (menuLocation == 16) { //clock setting
    resetLCD();
    if (readRising(&currentSelect, &lastSelect)) {
      if (subMenuLocation < 6) {
        subMenuLocation++;
        wake();
        smallBeep(beepNote);
        fillButtonVars();
      } else {
        subMenuLocation = 1;
        menuLocation = 1;
        wake();
        smallBeep(beepNote);
        fillButtonVars();
        clockI2C.setDateTime(setyear, setmonth, setday, sethour, setminute, setsecond);
      }
    }
    if (readRising(&currentBack, &lastBack)) {
      subMenuLocation = 1;
      menuLocation = 1;
      wake();
      smallBeep(badBeepNote);
    }

    if (subMenuLocation == 1) {
      lcd.print("Year:");
      lcd.setCursor(0, 1);
      if (flashBool()) {
        lcd.print(setyear);
      }

      if (readRising(&currentLeft, &lastLeft) && (setyear > 2000)) {
        setyear--;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentRight, &lastRight) && (setyear < 2255)) {
        setyear++;
        wake();
        smallBeep(beepNote);
      }
    }

    if (subMenuLocation == 2) {
      lcd.print("Month:");
      lcd.setCursor(0, 1);
      if (flashBool()) {
        lcd.print(setmonth);
      }

      if (readRising(&currentLeft, &lastLeft) && (setmonth > 1)) {
        setmonth--;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentRight, &lastRight) && (setmonth < 12)) {
        setmonth++;
        wake();
        smallBeep(beepNote);
      }
    }

    if (subMenuLocation == 3) {
      lcd.print("Day:");
      lcd.setCursor(0, 1);
      if (flashBool()) {
        lcd.print(setday);
      }

      if (readRising(&currentLeft, &lastLeft) && (setday > 1)) {
        setday--;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentRight, &lastRight) && (setday < daysPerMonth[setmonth - 1])) {
        setday++;
        wake();
        smallBeep(beepNote);
      }
    }

    if (subMenuLocation == 4) {
      lcd.print("Military Hour:");
      lcd.setCursor(0, 1);
      if (flashBool()) {
        lcd.print(sethour);
      }

      if (readRising(&currentLeft, &lastLeft) && (sethour > 0)) {
        sethour--;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentRight, &lastRight) && (sethour < 23)) {
        sethour++;
        wake();
        smallBeep(beepNote);
      }
    }

    if (subMenuLocation == 5) {
      lcd.print("Minute:");
      lcd.setCursor(0, 1);
      if (flashBool()) {
        lcd.print(setminute);
      }

      if (readRising(&currentLeft, &lastLeft) && (setminute > 0)) {
        setminute--;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentRight, &lastRight) && (setminute < 59)) {
        setminute++;
        wake();
        smallBeep(beepNote);
      }
    }
    if (subMenuLocation == 6) {
      lcd.print("Second:");
      lcd.setCursor(0, 1);
      if (flashBool()) {
        lcd.print(setsecond);
      }

      if (readRising(&currentLeft, &lastLeft) && (setsecond > 0)) {
        setsecond--;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentRight, &lastRight) && (setsecond < 59)) {
        setsecond++;
        wake();
        smallBeep(beepNote);
      }
    }
    delay(CLEARWRITESPEED);
  }

  if (menuLocation == 17) {
    lcd.setCursor(0, 0);
    lcd.print("Set Skip Cycles");
    lcd.setCursor(0, 1);
    if (flashBool()) {
      lcd.print(setCyclesToSkip);
    } else {
      lcd.print("                 ");
    }
    if (readRising(&currentLeft, &lastLeft) && (setCyclesToSkip > 0)){
      setCyclesToSkip--;
      wake();
      smallBeep(beepNote);
    }
    
    if (readRising(&currentRight, &lastRight) && (setCyclesToSkip < 255)){
      setCyclesToSkip++;
      wake();
      smallBeep(beepNote);
    }

    if (readRising(&currentSelect, &lastSelect)){
      menuLocation = 2;
      subMenuLocation = 1;
      wake();
      smallBeep(beepNote);
      cyclesToSkip = setCyclesToSkip; //save new number
      resetLCD();
    }

    if (readRising(&currentBack, &lastBack)){
      menuLocation = 2;
      subMenuLocation = 1;
      wake();
      smallBeep(badBeepNote); //not saved so bad
      //notice absence of transfer, to not save new cycle number
      resetLCD();
    }
  }

  if (menuLocation == 18) {
    if (subMenuLocation == 1) {
      lcd.print("Feed Timer 1:   ");
      lcd.setCursor(0, 1);
      if (flashBool()) {
        if (hourtimer1 < 10) {
          lcd.print("0");
          lcd.setCursor(1, 1);
          lcd.print(hourtimer1);
        } else {
          lcd.print(hourtimer1);
        }
      } else {
        lcd.print("  ");
      }
      lcd.setCursor(2, 1);
      lcd.print(":");
      if (minutetimer1 < 10) {
        lcd.print("0");
        lcd.setCursor(4, 1);
        lcd.print(minutetimer1);
      } else {
        lcd.print(minutetimer1);
      }

      if (readRising(&currentSelect, &lastSelect)) {
        subMenuLocation++;
        wake();
        fillButtonVars();
        resetLCD();
        smallBeep(beepNote);
        return;
      }

      if (readRising(&currentLeft, &lastLeft) && (hourtimer1 > 0)) {
        hourtimer1--;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentRight, &lastRight) && (hourtimer1 < 23)) {
        hourtimer1++;
        wake();
        smallBeep(beepNote);
      }

    }

    if (subMenuLocation == 2) {
      lcd.print("Feed Timer 1:   ");
      lcd.setCursor(0, 1);
      if (hourtimer1 < 10) {
        lcd.print("0");
        lcd.setCursor(1, 1);
        lcd.print(hourtimer1);
      } else {
        lcd.print(hourtimer1);
      }
      lcd.setCursor(2, 1);
      lcd.print(":");
      if (flashBool()) {
        if (minutetimer1 < 10) {
          lcd.print("0");
          lcd.setCursor(4, 1);
          lcd.print(minutetimer1);
        } else {
          lcd.print(minutetimer1);
        }
      } else {
        lcd.print("  ");
      }

      if (readRising(&currentSelect, &lastSelect)) {
        subMenuLocation++;
        resetLCD();
        wake();
        smallBeep(beepNote);
        return;
      }

      if (readRising(&currentLeft, &lastLeft) && (minutetimer1 > 0)) {
        minutetimer1--;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentRight, &lastRight) && (minutetimer1 < 59)) {
        minutetimer1++;
        wake();
        smallBeep(beepNote);
      }


    }
    if (subMenuLocation == 3) {
      if (flashBool()) {
        if (timer1) {
          lcd.print("ON");
        } else {
          lcd.print("OFF");
        }
      } else {
        lcd.print("   ");
      }

      if (readRising(&currentRight, &lastRight) || readRising(&currentLeft, &lastLeft)) {
        timer1 = ! timer1;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentSelect, &lastSelect)) {
        resetLCD();
        wake();
        smallBeep(beepNote);
        subMenuLocation++;
        return;
      }
    }

    if (subMenuLocation == 4){
      lcd.setCursor(0, 0);
      lcd.print("Change #/Scoops");
      lcd.setCursor(0, 1);
      if (flashBool()) {
        lcd.print(scoopsPerFeed1);
      } else {
        lcd.print("                 ");
      }
      if (readRising(&currentLeft, &lastLeft) && (scoopsPerFeed1 > 1)){
        scoopsPerFeed1--;
        wake();
        smallBeep(beepNote);
      }
    
      if (readRising(&currentRight, &lastRight) && (scoopsPerFeed1 < 255)){
        scoopsPerFeed1++;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentSelect, &lastSelect)){
        menuLocation = 1;
        subMenuLocation = 1;
        wake();
        smallBeep(beepNote);
        resetLCD();
      }

      if (readRising(&currentBack, &lastBack)){
        menuLocation = 1;
        subMenuLocation = 1;
        wake();
        smallBeep(badBeepNote); //not saved so bad
        //notice absence of transfer from setscoopsperfeed to scoopsperfeed, to not save new scoop number
        resetLCD();  
      }
    }
    
    delay(CLEARWRITESPEED);
  }

  if (menuLocation == 19) {
    if (subMenuLocation == 1) {
      lcd.print("Feed Timer 2:   ");
      lcd.setCursor(0, 1);
      if (flashBool()) {
        if (hourtimer2 < 10) {
          lcd.print("0");
          lcd.setCursor(1, 1);
          lcd.print(hourtimer2);
        } else {
          lcd.print(hourtimer2);
        }
      } else {
        lcd.print("  ");
      }
      lcd.setCursor(2, 1);
      lcd.print(":");
      if (minutetimer2 < 10) {
        lcd.print("0");
        lcd.setCursor(4, 1);
        lcd.print(minutetimer2);
      } else {
        lcd.print(minutetimer2);
      }

      if (readRising(&currentSelect, &lastSelect)) {
        subMenuLocation++;
        wake();
        resetLCD();
        smallBeep(beepNote);
        return;
      }

      if (readRising(&currentLeft, &lastLeft) && (hourtimer2 > 0)) {
        hourtimer2--;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentRight, &lastRight) && (hourtimer2 < 23)) {
        hourtimer2++;
        wake();
        smallBeep(beepNote);
      }

    }

    if (subMenuLocation == 2) {
      lcd.print("Feed Timer 2:   ");
      lcd.setCursor(0, 1);
      if (hourtimer2 < 10) {
        lcd.print("0");
        lcd.setCursor(1, 1);
        lcd.print(hourtimer2);
      } else {
        lcd.print(hourtimer2);
      }
      lcd.setCursor(2, 1);
      lcd.print(":");
      if (flashBool()) {
        if (minutetimer2 < 10) {
          lcd.print("0");
          lcd.setCursor(4, 1);
          lcd.print(minutetimer2);
        } else {
          lcd.print(minutetimer2);
        }
      } else {
        lcd.print("  ");
      }

      if (readRising(&currentSelect, &lastSelect)) {
        subMenuLocation++;
        resetLCD();
        wake();
        smallBeep(beepNote);
        return;
      }

      if (readRising(&currentLeft, &lastLeft) && (minutetimer2 > 0)) {
        minutetimer2--;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentRight, &lastRight) && (minutetimer2 < 59)) {
        minutetimer2++;
        wake();
        smallBeep(beepNote);
      }


    }
    if (subMenuLocation == 3) {
      if (flashBool()) {
        if (timer2) {
          lcd.print("ON");
        } else {
          lcd.print("OFF");
        }
      } else {
        lcd.print("   ");
      }

      if (readRising(&currentRight, &lastRight) || readRising(&currentLeft, &lastLeft)) {
        timer2 = ! timer2;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentSelect, &lastSelect)) {
        resetLCD();
        wake();
        smallBeep(beepNote);
        subMenuLocation++;
        return;
      }
    }

    if (subMenuLocation == 4){
      lcd.setCursor(0, 0);
      lcd.print("Change #/Scoops");
      lcd.setCursor(0, 1);
      if (flashBool()) {
        lcd.print(scoopsPerFeed2);
      } else {
        lcd.print("                 ");
      }
      if (readRising(&currentLeft, &lastLeft) && (scoopsPerFeed2 > 1)){
        scoopsPerFeed2--;
        wake();
        smallBeep(beepNote);
      }
    
      if (readRising(&currentRight, &lastRight) && (scoopsPerFeed2 < 255)){
        scoopsPerFeed2++;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentSelect, &lastSelect)){
        menuLocation = 2;
        subMenuLocation = 1;
        wake();
        smallBeep(beepNote);
        resetLCD();
      }

      if (readRising(&currentBack, &lastBack)){
        menuLocation = 2;
        subMenuLocation = 1;
        wake();
        smallBeep(badBeepNote); //not saved so bad
        //notice absence of transfer from setscoopsperfeed to scoopsperfeed, to not save new scoop number
        resetLCD();  
      }
    }
    
    delay(CLEARWRITESPEED);
  }

  if (menuLocation == 20) {
    if (subMenuLocation == 1) {
      lcd.print("Feed Timer 3:   ");
      lcd.setCursor(0, 1);
      if (flashBool()) {
        if (hourtimer3 < 10) {
          lcd.print("0");
          lcd.setCursor(1, 1);
          lcd.print(hourtimer3);
        } else {
          lcd.print(hourtimer3);
        }
      } else {
        lcd.print("  ");
      }
      lcd.setCursor(2, 1);
      lcd.print(":");
      if (minutetimer3 < 10) {
        lcd.print("0");
        lcd.setCursor(4, 1);
        lcd.print(minutetimer3);
      } else {
        lcd.print(minutetimer3);
      }

      if (readRising(&currentSelect, &lastSelect)) {
        subMenuLocation++;
        wake();
        resetLCD();
        smallBeep(beepNote);
        return;
      }

      if (readRising(&currentLeft, &lastLeft) && (hourtimer3 > 0)) {
        hourtimer3--;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentRight, &lastRight) && (hourtimer3 < 23)) {
        hourtimer3++;
        wake();
        smallBeep(beepNote);
      }

    }

    if (subMenuLocation == 2) {
      lcd.print("Feed Timer 3:   ");
      lcd.setCursor(0, 1);
      if (hourtimer3 < 10) {
        lcd.print("0");
        lcd.setCursor(1, 1);
        lcd.print(hourtimer3);
      } else {
        lcd.print(hourtimer3);
      }
      lcd.setCursor(2, 1);
      lcd.print(":");
      if (flashBool()) {
        if (minutetimer3 < 10) {
          lcd.print("0");
          lcd.setCursor(4, 1);
          lcd.print(minutetimer3);
        } else {
          lcd.print(minutetimer3);
        }
      } else {
        lcd.print("  ");
      }

      if (readRising(&currentSelect, &lastSelect)) {
        subMenuLocation++;
        resetLCD();
        wake();
        smallBeep(beepNote);
        return;
      }

      if (readRising(&currentLeft, &lastLeft) && (minutetimer3 > 0)) {
        minutetimer3--;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentRight, &lastRight) && (minutetimer3 < 59)) {
        minutetimer3++;
        wake();
        smallBeep(beepNote);
      }


    }
    if (subMenuLocation == 3) {
      if (flashBool()) {
        if (timer3) {
          lcd.print("ON");
        } else {
          lcd.print("OFF");
        }
      } else {
        lcd.print("   ");
      }

      if (readRising(&currentRight, &lastRight) || readRising(&currentLeft, &lastLeft)) {
        timer3 = ! timer3;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentSelect, &lastSelect)) {
        resetLCD();
        wake();
        smallBeep(beepNote);
        subMenuLocation++;
        return;
      }
    }
    if (subMenuLocation == 4){
      lcd.setCursor(0, 0);
      lcd.print("Change #/Scoops");
      lcd.setCursor(0, 1);
      if (flashBool()) {
        lcd.print(scoopsPerFeed3);
      } else {
        lcd.print("                 ");
      }
      if (readRising(&currentLeft, &lastLeft) && (scoopsPerFeed3 > 1)){
        scoopsPerFeed3--;
        wake();
        smallBeep(beepNote);
      }
    
      if (readRising(&currentRight, &lastRight) && (scoopsPerFeed3 < 255)){
        scoopsPerFeed3++;
        wake();
        smallBeep(beepNote);
      }

      if (readRising(&currentSelect, &lastSelect)){
        menuLocation = 3;
        subMenuLocation = 1;
        wake();
        smallBeep(beepNote);
        resetLCD();
      }

      if (readRising(&currentBack, &lastBack)){
        menuLocation = 3;
        subMenuLocation = 1;
        wake();
        smallBeep(badBeepNote); //not saved so bad
        //notice absence of transfer from setscoopsperfeed to scoopsperfeed, to not save new scoop number
        resetLCD();  
      }
    }
    
    delay(CLEARWRITESPEED);
  }

  if (menuLocation == 22) {
    resetLCD();
    lcd.print("Beep Pitch:");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    if (flashBool()) {
      lcd.print(beepNote);
    }
    if (readRising(&currentLeft, &lastLeft) && (beepNoteIndex > 0)) {
      beepNoteIndex--;
      beepNote = notes[beepNoteIndex];
      smallBeep(beepNote);
      wake();
    }

    if (readRising(&currentRight, &lastRight) && (beepNoteIndex < 89)) {
      beepNoteIndex++;
      beepNote = notes[beepNoteIndex];
      smallBeep(beepNote);
      wake();
    }

    if (readRising(&currentSelect, &lastSelect) || readRising(&currentBack, &lastBack)) {
      menuLocation = 7;
      //fillButtonVars();
      wake();
      resetLCD();
      smallBeep(beepNote);
    }

    delay(CLEARWRITESPEED);

    /*lcd.print("Beep Pitch HIGH");
      smallBeep(beepNote);
      delay(500);
      lcd.print("Set Beep Pitch");
      if (readRising(&currentSelect, &lastSelect)) {
      beepNote=NOTE_E6;
      resetLCD();
      wake();
      lcd.print("Beep Pitch LOW");
      smallBeep(beepNote);
      delay(500);*/
  }

  if (menuLocation == 23) {
    resetLCD();
    lcd.print("Low Beep Pitch:");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    if (flashBool()) {
      lcd.print(badBeepNote);
    }
    if (readRising(&currentLeft, &lastLeft) && (badBeepNoteIndex > 0)) {
      badBeepNoteIndex--;
      badBeepNote = notes[badBeepNoteIndex];
      smallBeep(badBeepNote);
      wake();
    }

    if (readRising(&currentRight, &lastRight) && (badBeepNoteIndex < 89)) {
      badBeepNoteIndex++;
      badBeepNote = notes[badBeepNoteIndex];
      smallBeep(badBeepNote);
      wake();
    }

    if (readRising(&currentSelect, &lastSelect) || readRising(&currentBack, &lastBack)) {
      menuLocation = 8;
      //fillButtonVars();
      wake();
      resetLCD();
      smallBeep(beepNote);
    }

    delay(CLEARWRITESPEED);

  }

  if (menuLocation == 24) {
    resetLCD();
    lcd.print("Ring Pitch:");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    if (flashBool()) {
      lcd.print(ringNote);
    }
    if (readRising(&currentLeft, &lastLeft) && (ringNoteIndex > 0)) {
      ringNoteIndex--;
      ringNote = notes[ringNoteIndex];
      smallBeep(ringNote);
      wake();
    }

    if (readRising(&currentRight, &lastRight) && (ringNoteIndex < 89)) {
      ringNoteIndex++;
      ringNote = notes[ringNoteIndex];
      smallBeep(ringNote);
      wake();
    }

    if (readRising(&currentSelect, &lastSelect) || readRising(&currentBack, &lastBack)) {
      menuLocation = 9;
      //fillButtonVars();
      wake();
      resetLCD();
      smallBeep(beepNote);
    }

    delay(CLEARWRITESPEED);

  }

  if (menuLocation == 25) {
    String logOptions[LOGOPTIONSLENGTH] = LOGOPTIONS;
    lcd.setCursor(0, 0);
    lcd.print("Choose Text   ");
    lcd.setCursor(0, 1);
    if (flashBool()) {
      lcd.print(logOptions[logChoice]);
    } else {
      lcd.print("                 ");
    }
    if (readRising(&currentLeft, &lastLeft) && (logChoice > 0)){
      logChoice--;
      wake();
      smallBeep(beepNote);
    }
    
    if (readRising(&currentRight, &lastRight) && (logChoice < LOGOPTIONSLENGTH - 1)){
      logChoice++;
      wake();
      smallBeep(beepNote);
    }

    if (readRising(&currentSelect, &lastSelect)){
      menuLocation = 10;
      subMenuLocation = 1;
      wake();
      smallBeep(beepNote);
      logFile = SD.open(FILELOGPATH, FILE_WRITE);
      if (month < 10) {logFile.print("0");}
      logFile.print(month);
      logFile.print("/");
      if (day < 10) {logFile.print("0");}
      logFile.print(day);
      logFile.print("/");
      logFile.print(year);
      logFile.print(" ");
      if (hour < 10) {logFile.print("0");}
      logFile.print(hour);
      logFile.print(":");
      if (minute < 10) {logFile.print("0");}
      logFile.print(minute);
      logFile.print(":");
      if (second < 10) {logFile.print("0");}
      logFile.print(second);
      logFile.print(ARROW);
      logFile.println(logOptions[logChoice]);
      logFile.close();
      resetLCD();
    }

    if (readRising(&currentBack, &lastBack)){
      menuLocation = 10;
      subMenuLocation = 1;
      wake();
      smallBeep(badBeepNote);
      resetLCD();
    }
  }

  if (menuLocation == 28) {
    static uint8_t scoops = 2;
    lcd.print("Scoop Count?");
    lcd.setCursor(0, 1);
    if (flashBool()){
      lcd.print(scoops);
      lcd.print("   "); // clear last number
    } else {
      lcd.print("                ");
    }
    if (readRising(&currentLeft, &lastLeft) && (scoops > 1)){
      scoops--;
      resetLCD();
      wake();
      smallBeep(beepNote);
    }
    if (readRising(&currentRight, &lastRight) && (scoops < 255)){
      scoops++;
      resetLCD();
      wake();
      smallBeep(beepNote);
    }
    if (readRising(&currentSelect, &lastSelect)) {
      resetLCD();
      wake();
      smallBeep(beepNote);
      feed(scoops);
      menuLocation = 13;
    }
    if (readRising(&currentBack, &lastBack)) {
      menuLocation = 13;
      wake();
      resetLCD();
      smallBeep(badBeepNote);
    }
  }

  if (menuLocation == 29) {
    lcd.print("Rly reset all?");
    lcd.setCursor(0, 1);
    lcd.print("Yes           No");
    if (readRising(&currentSelect, &lastSelect)) {
      resetLCD();
      lcd.print("Bye bye");
      resetFunc();
    }
    if (readRising(&currentBack, &lastBack)) {
      menuLocation = 14;
      fillButtonVars();
      wake();
      resetLCD();
      smallBeep(badBeepNote);
    }
  }

  if (menuLocation == 30){
    lcd.setCursor(0,0);
    if (readRising(&currentSelect, &lastSelect)){
      if (subMenuLocation < 19){
        subMenuLocation++;
        smallBeep(beepNote);
        wake();
      } else {
        subMenuLocation = 1;
        menuLocation = 15;
        wake();
        smallBeep(beepNote);
        resetLCD();
        return;
        }
    }

    if (readRising(&currentBack, &lastBack)){
       subMenuLocation = 1;
       menuLocation = 15;
       smallBeep(badBeepNote);
       wake();
       resetLCD();
    }

    if (subMenuLocation == 1){
      lcd.print("ChickenDog Locks");
      lcd.setCursor(0,1);
      lcd.print("Auto Dog Feeder ");
    }

    if (subMenuLocation == 2){
      lcd.print("Software Version");
      lcd.setCursor(0,1);
      lcd.print("V1.4.0 7/26/2020");
    }
    
    if (subMenuLocation == 3){
      lcd.print("Designer        ");
      lcd.setCursor(0,1);
      lcd.print("Miles C.        ");
    }

    if (subMenuLocation == 4){
      lcd.print("Asst. Designer  ");
      lcd.setCursor(0,1);
      lcd.print("Matthew C.        ");
    }
    
    if (subMenuLocation == 5){
      lcd.print("3D Modeler      ");
      lcd.setCursor(0,1);
      lcd.print("Miles C.        ");
    }

    if (subMenuLocation == 6){
      lcd.print("Asst. 3D Modeler");
      lcd.setCursor(0,1);
      lcd.print("Matthew C.      ");
    }

    if (subMenuLocation == 7){
      lcd.print("CAD Software    ");
      lcd.setCursor(0,1);
      lcd.print("SketchUp Web    ");
    }

    if (subMenuLocation == 8){
      lcd.print("3D printer      ");
      lcd.setCursor(0,1);
      lcd.print("QIDI X-SMART    ");
    }

    if (subMenuLocation == 9){
      lcd.print("Printer Filament");
      lcd.setCursor(0,1);
      lcd.print("Enotepad        ");
    }

    if (subMenuLocation == 10){
      lcd.print("3D-Printer Owner ");
      lcd.setCursor(0,1);
      lcd.print("Jonas M.");
    }

    if (subMenuLocation == 11){
      lcd.print("Code Developer  ");
      lcd.setCursor(0,1);
      lcd.print("Miles C.        ");
    }

    if (subMenuLocation == 12){
      lcd.print("Microcontroller ");
      lcd.setCursor(0,1);
      lcd.print("Arduino Mega2560");
    }

    if (subMenuLocation == 13){
      lcd.print("Idea            ");
      lcd.setCursor(0,1);
      lcd.print("Miles C.        ");
    }

    if (subMenuLocation == 14){
      lcd.print("Source Code:    ");
      lcd.setCursor(0,1);
      lcd.print("Go to           ");
    }

    if (subMenuLocation == 15){
      lcd.print("www.tinyurl.com/");
      lcd.setCursor(0,1);
      lcd.print("chickendoglocks ");
    }

    if (subMenuLocation == 16){
      lcd.print("Copyright 2020  ");
      lcd.setCursor(0,1);
      lcd.print("By Miles C.     ");
    }

    if (subMenuLocation == 17){
      lcd.print("(Age 13)        ");
    }
    
    if (subMenuLocation == 18){
      lcd.print("Thanks To All   ");
      lcd.setCursor(0,1);
      lcd.print("Contributors    ");
    }

    if (subMenuLocation == 19){
      lcd.print("And to you for  ");
      lcd.setCursor(0,1);
      lcd.print("Making!!!       ");
    }
  }

  //autofeed timer
  if ((!coolDown) && (((millis() - lastTimerFeed) % UNSIGNEDLONGINTARDUINO) > (TIMERCOOLDOWNMS))){coolDown = 1;}
  bool isTimer1 = (hourtimer1 == hour) && (minutetimer1 == minute) && (timer1);
  bool isTimer2 = (hourtimer2 == hour) && (minutetimer2 == minute) && (timer2);
  bool isTimer3 = (hourtimer3 == hour) && (minutetimer3 == minute) && (timer3);
  uint8_t scoops = isTimer1 ? scoopsPerFeed1 : (isTimer2 ? scoopsPerFeed2 : (isTimer3 ? scoopsPerFeed3 : 0));
  if (
      (
       (isTimer1) ||
       (isTimer2) ||
       (isTimer3)
      ) 
      && 
      (coolDown)
     ){
    if (!cyclesToSkip) {
      if (feedTimerOn == 1){
        feed(scoops);
      }
      if (feedTimerOn == 2){
        ring();
      }
    } else {
      cyclesToSkip--;
    }
    //use ifelse loop etc here with feed and ring later to change each alarm/feedtime: maybe??
    coolDown = 0;
    lastTimerFeed = millis();
  }



  
  delay(LOOPSPEED);
}

void fillButtonVars() {
  lastLeft = currentLeft;
  lastRight = currentRight;
  lastSelect = currentSelect;
  lastBack = currentBack;
  currentLeft = digitalRead(LEFT_BUTTON);
  currentRight = digitalRead(RIGHT_BUTTON);
  currentSelect = digitalRead(SELECT_BUTTON);
  currentBack = digitalRead(BACK_BUTTON);
}

bool readRising(bool *current, bool *last) {
  if (*current && !*last) {
    delay(5); //debounce
    return true;
  } else {
    return false;
  }
}

void resetLCD() {
  //for menu location change
  lcd.setCursor(0, 0);
  lcd.clear();
}

void fillTimeVars() {
  dt = clockI2C.getDateTime();
  year = dt.year;
  month = dt.month;
  day = dt.day;
  hour = dt.hour;
  minute = dt.minute;
  second = dt.second;

  strcpy(timeString, clockI2C.dateFormat("d-m-Y H:i:s", dt));
  /*strcpy(temp, clockI2C.dateFormat("s", dt));
    second = atoi(temp);
    strcpy(temp, clockI2C.dateFormat("i", dt));
    minute = atoi(temp);
    strcpy(temp, clockI2C.dateFormat("H", dt));
    hour = atoi(temp);
    strcpy(temp, clockI2C.dateFormat("Y", dt));
    day = atoi(temp);
    strcpy(temp, clockI2C.dateFormat("m", dt));
    month = atoi(temp);
    strcpy(temp, clockI2C.dateFormat("d", dt));
    year = atoi(temp);*/
}

void wake() {
  lastWake = millis();
}

void smallBeep(uint16_t pitchHZ) {
  tone(BUZZER_PIN, pitchHZ, 100);
}

bool flashBool() {
  return (millis() % 1000 > 250);
}

void feed(uint8_t scoops) {
  servoPower(HIGH);
  ring();
  lcd.setCursor(0,0);
  lcd.print("Feeding!        ");
  for (uint8_t times = 0; times < scoops; times++){
    lcd.setCursor(0, 1);
    lcd.print("Scoop: ");
    lcd.print(times+1);
    lcd.print("/");
    lcd.print(scoops);
    delay(GATESWAPTIME);
    loadGate.write(SERVODOWNPOSE);
    delay(GATEFALLTIME);
    loadGate.write(SERVOUPPOSE);
    delay(GATESWAPTIME);
    releaseGate.write(SERVODOWNPOSE);
    delay(GATEFALLTIME);
    releaseGate.write(SERVOUPPOSE);
    tone(BUZZER_PIN, ringNote, 500);
    wake();
  }
  delay(1000);
  resetLCD();
  //Serial.println(SD.exists(FILELOGPATH));
  logFile = SD.open(FILELOGPATH, FILE_WRITE);
  if (month < 10) {logFile.print("0");}
  logFile.print(month);
  logFile.print("/");
  if (day < 10) {logFile.print("0");}
  logFile.print(day);
  logFile.print("/");
  logFile.print(year);
  logFile.print(" ");
  if (hour < 10) {logFile.print("0");}
  logFile.print(hour);
  logFile.print(":");
  if (minute < 10) {logFile.print("0");}
  logFile.print(minute);
  logFile.print(":");
  if (second < 10) {logFile.print("0");}
  logFile.print(second);
  logFile.print(ARROW);
  logFile.println(FEEDLOGTEXT);
  logFile.close();
  lcd.print("Done!");
  delay(DONEFLASH);
  resetLCD();
  servoPower(LOW);
}

void ring(){
  resetLCD();
  lcd.print("Ringing!        ");
  for (uint8_t times = 0; times < RINGS; times++) {
    tone(BUZZER_PIN, ringNote, 500);
    delay(1000);
  }
}

void resetFunc() {
  digitalWrite(RESET_TRANS, HIGH);
}

void servoPower(bool power){
  digitalWrite(SERVOA_POWER_TRANS, power);
  digitalWrite(SERVOB_POWER_TRANS, power);
}
