SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

#include "AssetTrackerRK.h"
#include "LiquidCrystal.h"

/*
  Hitachi HD44780 driver
  https://www.arduino.cc/en/Reference/LiquidCrystal
  https://github.com/technobly/SparkCore-LiquidCrystal

  D1 -> LCD RS
  D0 -> LCD EN
  D5 -> LCD D4
  D4 -> LCD D5
  D3 -> LCD D6
  D2 -> LCD D7
  D7 -> TO LCD POWER

  B0 -> BUTTON INPUT
  B2 -> BUZZER

  from asset shield datasheet:
  The accel communicates over SPI, so it takes up A2, A3, A4, and A5 as marked on the silkscreen of the shield.
  Pin D6 controls the GPS power, with inverted logic. This means that the GPS will only be ON when D6 is LOW, which should keep it off even if you put the Electron to sleep.

  from electron datasheet:
  A0-A7	12-bit Analog-to-Digital (A/D) inputs (0-4095), and also digital GPIOs. A6 and A7 are code convenience mappings, which means pins are not actually labeled as such but you may use code like analogRead(A7). A6 maps to the DAC pin and A7 maps to the WKP pin. A3 is also a second DAC output used as DAC2 or A3 in software. A4 and A5 can also be used as PWM[1] outputs.
  B0-B5	B0 and B1 are digital only while B2, B3, B4, B5 are 12-bit A/D inputs as well as digital GPIOs. B0, B1, B2, B3 can also be used as PWM[1] outputs.
  C0-C5	Digital only GPIO. C4 and C5 can also be used as PWM[1] outputs.

 */

AssetTracker t = AssetTracker();
FuelGauge fuel;
const int TONEPITCH = 1000;
const int MONITORTIME = 180000;
const int CANCELTIME = 30000;

LiquidCrystal lcd(D1, D0, D5, D4, D3, D2);

void sleep() {
  System.sleep(SLEEP_MODE_DEEP); //https://docs.particle.io/reference/firmware/electron/#sleep-sleep-
}
void setup() {
  Serial.begin(9600);

  pinMode(D7, OUTPUT); //LCD backlight
  pinMode(B2, OUTPUT); //Buzzer
  pinMode(B0, INPUT); //Button

  digitalWrite(D7, HIGH); //Turn on LCD backlight
  Particle.connect();

  // Sets up all the necessary AssetTracker bits
  t.begin();

  // Enable the GPS module. Defaults to off to save power.
  // Takes 1.5s or so because of delays.
  t.gpsOn();

  // set up the LCD's number of columns and rows:
  lcd.begin(16,2);
  lcd.setCursor(0, 0);
  lcd.print("Press for a call");
  lcd.setCursor(0, 1);
  lcd.print("from a BlueGuard");

  // We are going to declare a Particle.variable() here so that we can access the value of the photoresistor from the cloud.
  //Particle.variable("analogvalue", &analogvalue, INT);
  // This is saying that when we ask the cloud for "analogvalue", this will reference the variable analogvalue in this app, which is an integer variable.

  // This is saying that when we ask the cloud for the function "led", it will employ the function ledToggle() from this app.

  // These functions are useful for remote diagnostics. Read more below.
  Particle.function("batt", batteryStatus);
  //Particle.function("gps", gpsPublish);

}


int current_mode = 1;
/**
  MODES
  -------
  1 = Standby Mode
  2 = Calling Mode
  3 = Monitor Mode
**/

void loop() {

  t.updateGPS();

  switch (current_mode) {
    case 1:
      standby();
      break;
    case 2:
      calling();
      break;
    case 3:
      monitor();
      break;
    default:
      // if nothing else matches, do the default
      // default is optional
    break;
  }

}

int standby_display = 0;
int previousButtonState = LOW;
int timePressed = 0;
void standby() {

  int currentButtonState = digitalRead(B0);
  if(currentButtonState == LOW && previousButtonState == HIGH) {
    if(millis() - timePressed < 250) {
      //Pressed
      current_mode = 2;
    }else{
      //Held
      tone(B2, TONEPITCH);
      delay(100);
      noTone(B2);

      current_mode = 3;
    }
  }else if (currentButtonState == HIGH && previousButtonState == LOW) {
    timePressed = millis();
  }
  previousButtonState = currentButtonState;

  if(millis() > 30000){
    sleep();
  }else if(millis() % 10000 < 5000 && standby_display == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Press for a call");
    lcd.setCursor(0, 1);
    lcd.print("from a BlueGuard");
    standby_display = 0;
  } else if(millis() % 10000 > 5000 && standby_display == 0){
    lcd.setCursor(0, 0);
    lcd.print("Hold to start   ");
    lcd.setCursor(0, 1);
    lcd.print("monitoring mode ");
    standby_display = 1;
  }
}

int calling_status = 0;
void calling() {
  if(calling_status == 0){
    lcd.setCursor(0, 0);
    lcd.print("Connecting...   ");
    lcd.setCursor(0, 1);
    lcd.print("One moment.     ");
    calling_status = 1;
  }else if (calling_status == 1){
    if (Cellular.ready() && Particle.connected()) {
      Particle.publish("C", PRIVATE);
      calling_status = 2;
    }
  }else {
    lcd.setCursor(0, 0);
    lcd.print("A BlueGuard will");
    lcd.setCursor(0, 1);
    lcd.print("call you shortly");
    delay(10000);
    sleep();
  }
}

int monitor_status = 0;
int monitor_starttime = 0;
int cancel_starttime = 0;
void monitor() {

  if(monitor_status == 0){
    lcd.setCursor(0, 0);
    lcd.print("Warming up      ");
    lcd.setCursor(0, 1);
    lcd.print("the monitor...  ");

    /*
    //c = CellLocate();
    c.timeout_clear();
        if(c.is_matched()){
          Serial.println('word up');
        }

        c.display();
        c.openStreepMaps();
        delay(1000);
        if c.is_matched()
    */
    Serial.println(t.preNMEA());
    if(Cellular.ready() && Particle.connected()){
      lcd.setCursor(0, 0);
      lcd.print("Press to start  ");
      lcd.setCursor(0, 1);
      lcd.print("the monitor.   ");
      monitor_status = 1;
    }
  } else if (monitor_status == 1) {

    if(digitalRead(B0) == HIGH){
      monitor_starttime = millis();
      monitor_status = 2;
    }

  } else if (monitor_status == 2){
    int elapsed = millis() - monitor_starttime;
    lcd.setCursor(0, 0);
    lcd.print("Press button in ");
    lcd.setCursor(0, 1);

    int displaytime = MONITORTIME - elapsed;
    int minutes = (int)displaytime/60000;
    int seconds = (displaytime%60000)/1000;
    String extrazero = "";
    if(seconds < 10){
      extrazero = "0";
    }

    lcd.print(String(minutes) + ":" + extrazero + String(seconds) + "                 ");
    if(displaytime < 1){
      monitor_status = 3;
      cancel_starttime = millis();
    }
  } else if (monitor_status == 3){

      if(millis() % 200 > 100){
        tone(B2, TONEPITCH);
      }else{
        noTone(B2);
      }

      int elapsed = millis() - cancel_starttime;
      lcd.setCursor(0, 0);
      lcd.print("Calling for help");
      lcd.setCursor(0, 1);

          int displaytime = CANCELTIME - elapsed;
          int minutes = (int)displaytime/60000;
          int seconds = (displaytime%60000)/1000;
          String extrazero = "";
          if(seconds < 10){
            extrazero = "0";
          }

          lcd.print("in " + String(minutes) + ":" + extrazero + String(seconds) + "                 ");
          if(displaytime < 1){
            if(t.gpsFix()){
              Particle.publish("G", t.readLatLon(), 60, PRIVATE);
            }else{
              Particle.publish("G", "40.73676499999999,-73.99190240000001", 60, PRIVATE);
            }
            lcd.setCursor(0, 0);
            lcd.print("Medical support ");
            lcd.setCursor(0, 1);
            lcd.print("is needed here. ");
            tone(B2, TONEPITCH);
            monitor_status = 4;
          }

          if(digitalRead(B0) == HIGH){
            lcd.setCursor(0, 0);
            lcd.print("Monitor         ");
            lcd.setCursor(0, 1);
            lcd.print("deactivated.    ");
            noTone(B2);
            delay(10000);
            sleep();
          }
    } else if (monitor_status == 4){

        if(digitalRead(B0) == HIGH){
            sleep();
        }

    }
}


// Lets you remotely check the battery status by calling the function "batt"
// Triggers a publish with the info (so subscribe or watch the dashboard)
// and also returns a '1' if there's >10% battery left and a '0' if below
int batteryStatus(String command){
    // Publish the battery voltage and percentage of battery remaining
    // if you want to be really efficient, just report one of these
    // the String::format("%f.2") part gives us a string to publish,
    // but with only 2 decimal points to save space
    Particle.publish("B",
          "v:" + String::format("%.2f",fuel.getVCell()) +
          ",c:" + String::format("%.2f",fuel.getSoC()),
          60, PRIVATE
    );
    // if there's more than 10% of the battery left, then return 1
    if(fuel.getSoC()>10){ return 1;}
    // if you're running out of battery, return 0
    else { return 0;}
}
