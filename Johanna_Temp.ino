/*
// with this sketch the user can also press both buttons at once, and the LCD is updated explicitly,
// not via an external function, which removes the need for clearing the LCD, and only updates when a state changes
// however this causes problems for single digit temperatures
*/

#include <LiquidCrystal.h>

LiquidCrystal lcd(10, 9, 5, 4, 3, 2);

// define input and output pin variables
const int tempSensPin = A0;
const int heaterPin = 13; // use the built-in LED to simulate Heater activity
const int upSwitchPin = 12;
const int downSwitchPin = 11;
const int lcdTogglePin = 7;
const int lcdBacklightPin = 8;

boolean upSwitchState = 0;
boolean downSwitchState = 0;
boolean heaterState = 0; // we need this variable to decide if we need to display "ON" or "OFF" on the LCD

boolean lcdTogglePress = 0;
boolean lcdToggle = 1;

// we could get floating point values for temperature, but that would be unnecessarily precise
int istTemp;
int sollTemp;

// we don't need this thing to be switching the heater on and off at 16MHz, maybe once every minute
unsigned long previousTime = 0;
const int interval = 60000;

// we sample the temperature continuously and take the average over the above interval
int sensorMin = 1023;
int sensorMax = 0;
int sensorValue;

void setup(){	
	Serial.begin(9600);
	
	// initialize input and output pins
	pinMode(tempSensPin, INPUT);
	pinMode(upSwitchPin, INPUT);
	pinMode(downSwitchPin, INPUT);
	pinMode(lcdTogglePin, INPUT);
	pinMode(heaterPin, OUTPUT);
	
	pinMode(lcdBacklightPin, OUTPUT);
	digitalWrite(lcdBacklightPin, 1);
	
	//initialize LCD with 16 columns and 2 rows
	lcd.begin(16, 2);
	
	istTemp = calibrate();
	sollTemp = istTemp; // initially we don't want to activate the heater, so we set the requested temperature equal to the ambient temperature
	
	
	// explicit LCD setup. without a menu system, etc. we only need to update the temperature and heater state
	lcd.setCursor(0,0);
	lcd.print("Soll: ");
	lcd.print(sollTemp);
	lcd.print((char)223);
	lcd.print("C");
	
	lcd.setCursor(13, 0);
	lcd.print("OFF");
	
	lcd.setCursor(0,1);
	lcd.print("Ist : ");
	lcd.print(istTemp);
	lcd.print((char)223);
	lcd.print("C");

}

/*
// check for user input, adjust sollTemp accordingly
// we want to avoid switching the heater state too often, but we do want to allow the user
// the satisfaction of immediate feedback when changing the temperature
// therefore we only check the current temperature every INTERVAL (which limits automatic switching), but if the sollTemp
// is more than 2 degrees greater than istTemp, then we can immediately switch on the heater
*/
void loop(){
	UserInput();
	
	// measure temperature continuously
	sensorValue = analogRead(tempSensPin);
	if(sensorValue > sensorMax) {
		sensorMax = sensorValue;
	}
	if(sensorValue < sensorMin) {
		sensorMin = sensorValue;
	}
	
	
	if((millis() - previousTime) > interval){
		
		// calculate the temperature by scaling the sensor value
		sensorValue = (sensorMax + sensorMin) / 2;
		float voltage = (sensorValue/1024.0) * 5.0;
		istTemp = (voltage - .5) * 100;
		
		
		lcd.setCursor(6,1);
		lcd.print("    ");
		lcd.setCursor(6,1);
		lcd.print(istTemp);
		lcd.print((char)223);
		lcd.print("C");
		
		// reset min and max
		sensorMin = 1023;
		sensorMax = 0;
		
		previousTime = millis();
	}
	
	if(istTemp < sollTemp - 1){
		heaterState = 1;
		digitalWrite(heaterPin, heaterState);
		
		// in order to overwrite all 3 chars of "OFF", we use the same offset and write a space before "ON"
		lcd.setCursor(13, 0);
		lcd.print(" ON");
	}
	if(istTemp >= sollTemp){
		heaterState = 0;
		digitalWrite(heaterPin, heaterState);
		
		lcd.setCursor(13, 0);
		lcd.print("OFF");
	}
}


/*
the following are subroutines
*/

void UserInput(){
	unsigned long waitBegin = millis();
	
	if(digitalRead(upSwitchPin) == 1 | digitalRead(downSwitchPin) == 1 | digitalRead(lcdTogglePin) == 1){ // start sample only if any or all buttons are already pressed, otherwise the user might start input at the end of a sample, which is not the point
		while(millis() - waitBegin < 200){ // sample buttons for milliseconds
			// if either button is pressed during interval, then this positive value is retained
			if(digitalRead(upSwitchPin) == 1){
				upSwitchState = 1;
			}
			if(digitalRead(downSwitchPin) == 1){
				downSwitchState = 1;
			}
			
			if(digitalRead(lcdTogglePin) == 1){
				lcdTogglePress = 1;
			}
		}
	}
	
	if(upSwitchState == 1 && downSwitchState == 0){
		sollTemp = constrain(sollTemp+1, 0, 40);
		
		lcd.setCursor(6,0);
		lcd.print("    ");
		lcd.setCursor(6,0);
		lcd.print(sollTemp);
		lcd.print((char)223);
		lcd.print("C");
	}
	
	if(upSwitchState == 0 && downSwitchState == 1){
		sollTemp = constrain(sollTemp-1, 0, 40);
		
		lcd.setCursor(6,0);
		lcd.print("    ");
		lcd.setCursor(6,0);
		lcd.print(sollTemp);
		lcd.print((char)223);
		lcd.print("C");
	}
	
	if(upSwitchState == 1 && downSwitchState == 1){
		sollTemp = istTemp;
		
		lcd.setCursor(6,0);
		lcd.print("    ");
		lcd.setCursor(6,0);
		lcd.print(sollTemp);
		lcd.print((char)223);
		lcd.print("C");
	}
	if(lcdTogglePress == 1){
		if(lcdToggle == 1){ // if already on, turn off
			lcdToggle = 0;
			lcd.noDisplay();
			digitalWrite(lcdBacklightPin, 0);
		}
		else{
			lcdToggle = 1;
			lcd.display();
			digitalWrite(lcdBacklightPin, 1);

		}
	}
	
	// reset button states to 0
	upSwitchState = 0;
	downSwitchState = 0;
	lcdTogglePress = 0;
}

int calibrate(){
	// we sample the temperature for 15 seconds and retain the min and max values
	
	unsigned long sampleBegin = millis();
	
	lcd.setCursor(2,0);
	lcd.print("Calibrating");
	lcd.setCursor(2,1);
	lcd.print("Please Wait");
	lcd.setCursor(15,1);
	lcd.blink();
	
	// we sample the temperature sensor over an interval to get an average value
	while(millis() - sampleBegin < 15000){
		sensorValue = analogRead(tempSensPin);
		
		if (sensorValue > sensorMax) {
			sensorMax = sensorValue;
    	}

		// record the minimum sensor value
		if (sensorValue < sensorMin) {
			sensorMin = sensorValue;
		}
	}
	sensorValue = (sensorMax + sensorMin) / 2;
	
	// calculate the temperature by scaling the sensor value
	float voltage = (sensorValue/1024.0) * 5.0;
	int temp = (voltage - .5) * 100;
	
	lcd.noBlink();
	lcd.clear();
	
	Serial.println(temp);
	return temp;
}