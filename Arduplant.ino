//
// Arduplant - feeding plants by schedule
// by Andrey Zhelnin <andjel@gmail.com>
//

#include <ClickButton.h>
#include <Wire.h>
// Get the LCD I2C Library here:
// https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Wire.h>
#include <RealTimeClockDS1307.h>
#include <EEPROM.h>

// Defines
//#define PROD ; uncomment after debug
#define DEBUG

// Pins
const int pumpPin = 9; // pumpControl pin
// Button pins
const int selectButtonPin = 2;
const int downButtonPin = 3;
const int upButtonPin = 4;

// EEPROM locations
// ST1,HH1,MM1,ST2,HH2,MM2
const int schedulesEEPROM[] = {0,1,2,3,4,5};

// set the LCD address to 0x27 for a 20 chars 4 line display
// Set the pins on the I2C chip used for LCD connections:
//			addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);	// Set the LCD I2C address
// Set buttons
ClickButton selectButton(selectButtonPin, LOW, CLICKBTN_PULLUP);
ClickButton downButton(downButtonPin, LOW, CLICKBTN_PULLUP);
ClickButton upButton(upButtonPin, LOW, CLICKBTN_PULLUP);

// Arrays
char formatted[] = "00-00-00 00:00";
char formatted1[] = "00-00-00 00:00";
const char rotateLogo[] = {'-',0x92,'|','/','-',0x92,'|','/'}; // pump rotate logo
// Set time ranges
const int DTMaxs[5] = {99,12,31,24,59};//Maximums for setting date\time
const int DTMins[5] = {14, 1, 1, 0, 0};//Minimums for setting date\time
const int monthsMaxs[12] = {31,29,31,30,31,30,31,31,30,31,30,31}; // Maximums for months
int timeSet1[5] = {0,0,0,0,0}; // Initial time array
int timeSet2[5] = {0,0,0,0,0}; // New time array
// Schedules
// (0/1 (enabled/disabled), HH, MM)
int schedules[] = {0,0,0,0,0,0};
// Array for state displaying
char schedulesState [] = {'N','N'};
// Amount of schedules
const int schAmounts = 2;
// Blink cursor positions
// 0123456789012345
// N*00:00  N*00:00
const int schChgCursor[6] = {0,2,5,9,11,14};

// Menu actions
char action0[] = "Set Clock";
char action1[] = "Set Schedule";
char action2[] = "Status";
char* actions1[4];

// Menu actions
const int ROOTMENU = 0;
const int SECDTMENU = 10;
const int TRDDTMENU = 20;
const int SECSCHMENU = 11;
const int TRDSCHMENU = 30;
const int SECSTATMENU = 12;

// States
int action = ROOTMENU; // menu state
int timeSelector; // Define time changing state
int schSelector; // Define schedule changing state
int timeValue;
boolean pumpState = false; // Pump run state
int rotateStep = 0; // rotate logo show step

// Timers
unsigned long clockTimer; // Time display update timer
unsigned long clockTimer1; // Set time update timer
unsigned long rotateTimer; // RotateLogo timer
unsigned long pumpTimer; // PumpTimer

// Intervals
const long interval = 30000; // Time display update interval
const long interval1 = 500; // Set time update interval
const int pumpInterval = 6000; // Pump run interval
const int rotateInterval = 100; // rotate logo update interval


void setup()
{
	Serial.begin(57600);
	pinMode(pumpPin, OUTPUT);
	digitalWrite(pumpPin, LOW);

	// Init Menus
	actions1[0] = action0;
	actions1[1] = action1;
	actions1[2] = action2;

	// Init EEPROM values if 255
  for (int i = 0; i < schAmounts*3 ;i++) if ( EEPROM.read(schedulesEEPROM[i]) == 255) EEPROM.write(schedulesEEPROM[i],0);

	// init schedules from EEPROM
  for (int i = 0; i < schAmounts*3; i++) {
			schedules[i] = EEPROM.read(schedulesEEPROM[i]);
#ifdef DEBUG
      Serial.print(i);
      Serial.print(" = ");
      Serial.println(schedules[i]);
#endif
  }

	// Switch RTC to 24h mode
	RTC.switchTo24h();
	lcd.begin(16,2);	 // initialize the lcd for 16 chars 2 lines, turn on backlight

	// ------- Quick 3 blinks of backlight	-------------
	#ifdef PROD
	for(int i = 0; i< 3; i++)
	{
		lcd.backlight();
		delay(250);
		lcd.noBacklight();
		delay(250);
	}
	lcd.backlight(); // finish with backlight on

	lcd.setCursor(0,0);
	lcd.print("All your plant");
	lcd.setCursor(0,1);
	lcd.print("are belong to us");
	#endif

	// Write time first time
	RTC.readClock();
	RTC.getFormattedShort(formatted);
	lcdUpdateTime(formatted);
	#ifdef DEBUG
	Serial.println(formatted);
	#endif
	lcdUpdateMenu("SELECT for MENU");

	}/*--(end setup )---*/

	//
	boolean cmpArray(int *a, int *b, int len){
		int n;
		// test each element to be the same. if not, return false
		for (n=0;n<len;n++) if (a[n]!=b[n]) return false;
		//ok, if we have not returned yet, they are equal :)
		return true;
	}


	// fill current time into array
	void getRTCtoArray (int* timeArray) {
		RTC.readClock();
		timeArray[0] = RTC.getYear();
		timeArray[1] = RTC.getMonth();
		timeArray[2] = RTC.getDay();
		timeArray[3] = RTC.getHours();
		timeArray[4] = RTC.getMinutes();
	}

	//Set time from array
	void setRTCfromArray (int* timeArray) {
		RTC.setYear(timeArray[0]);
		RTC.setMonth(timeArray[1]);
		RTC.setDay(timeArray[2]);
		RTC.setHours(timeArray[3]);
		RTC.setMinutes(timeArray[4]);
		RTC.setClock();
	}


	// Copy one array to another
	void copyArray(int *arrayOriginal, int *arrayCopy, int arraySize){
		while(arraySize--) *arrayCopy++ = *arrayOriginal++;
	}

	// Update DateTime on screen
	void lcdUpdateTime (char* showString ) {
		lcd.setCursor(0,0);
		lcd.print("                ");//clear ROW
		lcd.setCursor(1,0);
		lcd.print(showString);
		//#ifdef DEBUG
		//        Serial.println(showString);
		//#endif
	}

	// Update Menu on screen
	void lcdUpdateMenu (char* showString) {
		lcd.setCursor(0,1);
		lcd.print("                ");//clear ROW
		lcd.setCursor(0,1);
		lcd.print(showString);
		#ifdef DEBUG
		Serial.print(action);
		Serial.print(".");
		Serial.println(showString);
		#endif
	}

// Update schedules on LCD
void lcdUpdateSchedule ( ) {
	for (int i = 0; i < schAmounts; i++) if (schedules[i*3] == 1 ) schedulesState[i] = 'Y';
			else schedulesState[i] = 'N';
	sprintf(formatted1, "%c*%02d:%02d  %c*%02d:%02d", schedulesState[0], schedules[1], schedules[2], schedulesState[1], schedules[4], schedules[5] );
	lcdUpdateMenu(formatted1);
	lcd.setCursor(schChgCursor[schSelector],1);
}
// Update time

void updateDT(int* timeSet, int position, int direction) {
	if (direction == 1) timeSet[position]++;
	else timeSet[position]--;

	// check boundaries
	if (timeSet[position] > DTMaxs[position] || (position == 2 && timeSet[position] > monthsMaxs[timeSet[1]-1]))
	{ timeSet[position] = DTMins[position]; }

	if (timeSet[position] < DTMins[position]) {
		if (position == 2) timeSet[2] = monthsMaxs[timeSet[1]-1];
		else timeSet[position] = DTMaxs[position];
	}

	  #ifdef DEBUG
		for (int i=0;i<5;i++) { Serial.print(timeSet[i]); Serial.print(".");}
		Serial.println();
		#endif
}


	void loop()	 /*----( LOOP: RUNS CONSTANTLY )----*/
	{
		unsigned long currentMillis = millis();
		selectButton.Update();
		upButton.Update();
		downButton.Update();

		// If Select presset 3 times short - run pump
		if (selectButton.clicks == 3 && !pumpState) {
			pumpState = true;
			pumpTimer = currentMillis;
			digitalWrite(pumpPin, HIGH);
			Serial.println("Select Button clicked 3 times");
		}

		// Run pump for interval
		if (pumpState) {
			if (currentMillis - pumpTimer > pumpInterval) {
				pumpTimer = currentMillis;
				digitalWrite(pumpPin, LOW);
				pumpState = false;
				Serial.println("Pump timer off");
				lcd.setCursor(15,1);
				lcd.print(" ");
				} else if (currentMillis - rotateTimer > rotateInterval) {
					rotateTimer = currentMillis;
					lcd.setCursor(15,1);
					lcd.print(rotateLogo[rotateStep]);
					rotateStep++;
					if (rotateStep > 7) { rotateStep = 0; }
				}
			}

			// Update time on screen
			if((currentMillis - clockTimer > interval) && action != 20) {
				RTC.readClock();
				RTC.getFormattedShort(formatted);
				clockTimer = currentMillis;
				lcdUpdateTime(formatted);
				} else if ((currentMillis - clockTimer1 > interval1) && action == 20 && !cmpArray(timeSet1, timeSet2, 5)) {
					// Update clocks based on changing value
					clockTimer1 = currentMillis;
					sprintf(formatted1, "%02d-%02d-%02d %02d:%02d", timeSet1[0], timeSet1[1], timeSet1[2], timeSet1[3], timeSet1[4]);
					lcdUpdateTime(formatted1);
					lcd.setCursor(1+timeSelector*3,0);
				}

				// Select button clicked
				if (selectButton.clicks == 1) {
					switch (action) {
						case ROOTMENU:
						// Go to second level menu
						action = SECDTMENU;
						lcdUpdateMenu(actions1[action-10]);
						break;
						case SECDTMENU:
						// Set clock
							action = TRDDTMENU;
							timeSelector = 0; // Starting with year
							lcdUpdateMenu("Correct time");
						// Fill time to array if empty and make copy in timeSet2
							if (timeSet1[0] == 0 ) {getRTCtoArray(timeSet1); copyArray(timeSet1,timeSet2,5);}
							lcd.setCursor(1+timeSelector*3,0);
							lcd.blink();
						break;
						case TRDDTMENU:
						// Itterating through DT items
							timeSelector++;
							if (timeSelector > 4) timeSelector = 0;
							lcd.setCursor(1+timeSelector*3,0);
						break;
						case SECSCHMENU:
						// Set Schedule
            	action = TRDSCHMENU;
							schSelector = 0;
							lcd.blink();
							lcdUpdateSchedule();
						break;
						case SECSTATMENU:
						// Show stats
						break;
						case TRDSCHMENU:
						// Itterating through SCH items
							schSelector++;
							if (schSelector > 5 )	schSelector = 0;
							lcdUpdateSchedule();
						break;
					}

				}

				// Select button pressed
				if (selectButton.clicks == 2) {
					// Do for Second level menu
					if (action<TRDDTMENU) {
						action = ROOTMENU;
						lcdUpdateMenu("SELECT for MENU");
						} else if (action == TRDDTMENU) {
							// For set time menu
							// Compare arrays and if differs setting time
							if (!cmpArray(timeSet1,timeSet2,5)) setRTCfromArray(timeSet1);
							lcd.noBlink();
							action = SECDTMENU;
							lcdUpdateMenu(actions1[action-10]);
						} else if (action == TRDSCHMENU) {
							// Schedule set menu
							lcd.noBlink();
							action = SECDTMENU;
							lcdUpdateMenu(actions1[action-10]);
                                                }

					}

					// Up button clicked
					if (upButton.clicks == 1) {
						// Do for Second level menu
						switch(action) {
							case ROOTMENU:
							break;
							case TRDDTMENU:
							updateDT(timeSet1, timeSelector, 1);
							break;
							case TRDSCHMENU:
							break;
							default:
							action--;
							if (action < SECDTMENU ) { action = SECSTATMENU; }
							lcdUpdateMenu(actions1[action-10]);
						}
					}

					// Down button clicked
				        if (downButton.clicks == 1) {
							// Do for Second level menu
							switch(action) {
								case ROOTMENU:
								break;
								case TRDDTMENU:
									updateDT(timeSet1, timeSelector, -1);
								break;
								case TRDSCHMENU:
								break;
								default:
									action++;
									if (action > SECSTATMENU) { action = SECDTMENU; }
									lcdUpdateMenu(actions1[action-10]);
							}
    }
}
