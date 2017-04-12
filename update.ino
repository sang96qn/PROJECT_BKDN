/***********DANANG UNIVERSITY OF TECHNOLOGY AND SCIENCIES*****************/
#include <Wire.h> /*i2c interface*/
#include <SimpleTimer.h>  
SimpleTimer timer;

#include <idDHT11.h> /* DHT11 SENSOR*/
int idDHT11pin = 2;
int idDHT11intNumber = 0;
void dht11_wrapper();
idDHT11 DHT11(idDHT11pin, idDHT11intNumber, dht11_wrapper);

#include <AnalogMultiButton.h> /*Debounce button*/
const int BUTTONS_PIN = A0;
const int BUTTONS_TOTAL = 5;
const int BUTTONS_VALUES[BUTTONS_TOTAL] = {31, 174, 356, 516, 751};
const int RIGHT = 0;
const int UP = 1;
const int DOWN = 2;
const int LEFT = 3;
const int SELECT = 4;
AnalogMultiButton buttons(BUTTONS_PIN, BUTTONS_TOTAL, BUTTONS_VALUES);
#define _menu btn
#define NO_SELECT 5
#define ON true
#define OFF false

enum {Main_Screen, AGRICULTURE, Device_1,Timer_Mode, Change_Hour, Change_Minute, Change_Second, Change_Day, Change_Date, Change_Month, Change_Year, Fix_Time,Auto_Mode};

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

byte t, h;//temp and huminity
int _second, _minute, _hour, _day, _date, _month, _year;// read time
byte degree[8] = {
  B11110,
  B10010,
  B10010,
  B11110,
  B00000,
  B00000,
  B00000,
  B00000
};  // Create degree Celcius character

// button var
int menu_count = 0;
int btn_check;// btn_check: check button
int btn;
static int change = 0;
int gio, phut, giay, thu, ngay, thang, nam; //set timer
int hour1_Set, Min1_Set; //set timer motor
int End_hour1_Set , End_Min1_Set ;
bool Motor_Status = OFF; // in order to set timer
#define virtual_minute 100
#define virtual_hour 30
//devices 
#define Motor 12// device 1 
#define MASS A2
#define RAIN A1
int MASS_VALUE =0;
int RAIN_VALUE =0;
int Mass_Percent =0;
#define alert 11 // warning it's rain
#define Dry_Soil_Threshold 80
#define Wet_Soil_Threshold 40
#define No_Rain_Threshold 900
#define Rain_Threshold 250
static bool auto_check = true;
void setup() {
 // Serial.begin(9600);
  lcd.begin(16, 2);  // initialize the lcd for 16 chars 2 lines, turn on backlight
  lcd.backlight();
   Wire.begin();
  lcd.createChar(1, degree); // custom character
  timer.setInterval(500L, _update);// update data sensor after each 0,5s
  
  pinMode(Motor, OUTPUT);
  digitalWrite(Motor, 1); // 1 off-io pin -connect to relay
  pinMode(alert,OUTPUT);
  digitalWrite(alert, 0); // gpio arduino
  pinMode(MASS,INPUT);
  pinMode(RAIN,INPUT);
}
void loop()
{
  timer.run();
  check_button();
  timer1();
  argriculture_(); 
}

int btn_read()
{
  if (buttons.onRelease(UP))
    return UP;
  else if (buttons.onRelease(DOWN))
    return DOWN;
  else if (buttons.onRelease(LEFT))
    return LEFT;
  else if (buttons.onRelease(RIGHT))
    return RIGHT;
  else if (buttons.onRelease(SELECT))
    return SELECT;
  else return NO_SELECT;
}
int menu()
{
  if (buttons.onRelease(UP))
  {
    lcd.clear();
    menu_count++;
    if (menu_count > 12) menu_count = 0;
  }
  else if (buttons.onRelease(DOWN))
  {
    lcd.clear();
    menu_count--;
    if (menu_count <= 0) menu_count = 12;
  }
  else if (buttons.onRelease(LEFT))
  {
    lcd.clear();
    menu_count = 0;
  }
  return menu_count;
}
int dec2bcd(byte num)
{
  return ((num / 10) * 16 + (num % 10));
}
int bcd2dec(byte num)
{
  return ((num / 16) * 10 + (num % 16));
}
//-----------------
void settime(byte _hour_, byte _minute_, byte _second_, byte _day_, byte _date_, byte _month_, byte _year_)
{
  Wire.beginTransmission(0x68);// 0x68 :  ds3231 Slave'address
  Wire.write(0);// set pointer at second register
  Wire.write(dec2bcd(_second_));
  Wire.write(dec2bcd(_minute_));
  Wire.write(dec2bcd(_hour_));
  Wire.write(dec2bcd(_day_));
  Wire.write(dec2bcd(_date_));
  Wire.write(dec2bcd(_month_));
  Wire.write(dec2bcd(_year_));
  Wire.endTransmission();
}
//----------------------
void readTime()
{
  Wire.beginTransmission(0x68);
  Wire.write(0);// set pointer at second register
  Wire.endTransmission();
  Wire.requestFrom(0x68, 7);
  _second = bcd2dec(Wire.read() & 0x7f); //remove bits not used
  _minute = bcd2dec(Wire.read() & 0x7f);
  _hour = bcd2dec(Wire.read() & 0x3f); // 24h mode
  _day = bcd2dec(Wire.read() & 0x07);
  _date = bcd2dec(Wire.read() & 0x3f);
  _month = bcd2dec(Wire.read() & 0x1f);
  _year = bcd2dec(Wire.read());
  _year = _year + 2000;
}
void display_lcd()
{
  if ((_minute == 0) && (_second == 0)) //after 1 hour - clear cld
  {
    lcd.clear();
  }

  lcd.setCursor(0, 0);
  switch (_day)
  {
    case 1: lcd.print("Sun"); break;
    case 2: lcd.print("Mon"); break;
    case 3: lcd.print("Tue"); break;
    case 4: lcd.print("Wed"); break;
    case 5: lcd.print("Thur"); break;
    case 6: lcd.print("Fri"); break;
    case 7: lcd.print("Sat"); break;
  }

  lcd.setCursor(6, 0);
  lcd.print(_date);
  lcd.print("/");
  lcd.print(_month);
  lcd.print("/");
  lcd.print(_year);
  lcd.setCursor(0, 1);
  lcd.print(_hour);
  lcd.print(":");
  lcd.print(_minute);

  //-----dht
  lcd.setCursor(9, 1);
  lcd.print(t);
  lcd.setCursor(11, 1);
  lcd.write(1);
  lcd.setCursor(13, 1);
  lcd.print(h);
  lcd.print("%");
  /*------SOIL_SENSOR-------*/
  /* lcd.setCursor(13, 1);
  lcd.print(Mass_Percent);
  lcd.print("%");*/

}
void led_status1()
{

  int x = digitalRead(Motor);
  if (x == 0)
  {
    lcd.setCursor(0, 0);
    lcd.print("Motor : ON ");
  }
  else if (x == 1)
  {
    lcd.setCursor(0, 0);
    lcd.print("Motor : OFF");
  }
}

void change_day()
{
  switch (change)
  {
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("Sunday"); break;
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("Monday"); break;
    case 3:
      lcd.setCursor(0, 0);
      lcd.print("Tuesday"); break;
    case 4:
      lcd.setCursor(0, 0);
      lcd.print("Wednesday"); break;
    case 5:
      lcd.setCursor(0, 0);
      lcd.print("Thursday"); break;
    case 6:
      lcd.setCursor(0, 0);
      lcd.print("Friday"); break;
    case 7:
      lcd.setCursor(0, 0);
      lcd.print("Saturday"); break;
  }
}
void change_month()
{
  switch (change)
  {
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("January");
      lcd.setCursor(13, 1);
      lcd.print(change);
      break;
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("February");
      lcd.setCursor(13, 1);
      lcd.print(change);
      break;
    case 3:
      lcd.setCursor(0, 0);
      lcd.print("March");
      lcd.setCursor(13, 1);
      lcd.print(change);
      break;
    case 4:
      lcd.setCursor(0, 0);
      lcd.print("April");
      lcd.setCursor(13, 1);
      lcd.print(change);
      break;
    case 5:
      lcd.setCursor(0, 0);
      lcd.print("May");
      lcd.setCursor(13, 1);
      lcd.print(change);
      break;
    case 6:
      lcd.setCursor(0, 0);
      lcd.print("June");
      lcd.setCursor(13, 1);
      lcd.print(change);
      break;
    case 7:
      lcd.setCursor(0, 0);
      lcd.print("July");
      lcd.setCursor(13, 1);
      lcd.print(change);
      break;
    case 8:
      // lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("August");
      lcd.setCursor(13, 1);
      lcd.print(change);
      break;
    case 9:
      lcd.setCursor(0, 0);
      lcd.print("September");
      lcd.setCursor(13, 1);
      lcd.print(change);
      break;
    case 10:
      lcd.setCursor(0, 0);
      lcd.print("October");
      lcd.setCursor(13, 1);
      lcd.print(change);
      break;
    case 11:
      lcd.setCursor(0, 0);
      lcd.print("November");
      lcd.setCursor(13, 1);
      lcd.print(change);
      break;
    case 12:
      lcd.setCursor(0, 0);
      lcd.print("December");
      lcd.setCursor(13, 1);
      lcd.print(change);
      break;
  }
}
void timer1()
{
  if ((hour1_Set == _hour) && (Min1_Set == _minute))
  {
    digitalWrite(Motor, !Motor_Status);
    hour1_Set = virtual_hour; 
    Min1_Set = virtual_minute;
  }
   if ((End_hour1_Set == _hour) && (End_Min1_Set == _minute))
  {
    digitalWrite(Motor, !Motor_Status);
    End_hour1_Set = virtual_hour; 
    End_Min1_Set = virtual_minute;
  }
}

void _update()
{
  DHT11.acquire();
  while (DHT11.acquiring());

  h = (int)DHT11.getHumidity();
  t = (int)DHT11.getCelsius();
  RAIN_VALUE =analogRead(RAIN);
  MASS_VALUE =analogRead(MASS);
  Mass_Percent= map(MASS_VALUE,0,1023,0,100);// convert from analog value to % value
  /* update timer */
  readTime();
}
void check_button()
{
  static byte isPress = false;
  static bool isFirst = true;
  static bool smart = false;
  static bool isMenuChild = false;
  static byte healer = 0;
  buttons.update();
  btn_check = btn_read();
  if (isMenuChild == false) {
    _menu = menu(); //_menu <=> btn
  }
  switch (_menu)
  {
    case Main_Screen:
      display_lcd();
      break;

    case AGRICULTURE:
        lcd.setCursor(0, 0);
        lcd.print("Level:");
        lcd.setCursor(7, 0);
        lcd.print(Mass_Percent);
        lcd.print("%");
        if(Mass_Percent >=  Dry_Soil_Threshold)
        {
           lcd.setCursor(0, 1);
        lcd.print("Dry Soil !");
          }
          else {lcd.setCursor(0, 1);
        lcd.print("Wet Soil !");
            }
      break;

    case Device_1:
      if (smart == false) {
        if (btn_check == SELECT) {
          smart = true;
          healer = btn_check;
        }
        lcd.setCursor(0, 0);
        lcd.print("Device 1");
      }
      else {
        if (btn_check != NO_SELECT) {
          healer = btn_check;
        }
        switch (healer) {
          case UP:
          if(auto_check == false){
            digitalWrite(Motor,OFF); // on motor
            led_status1();}
            else  {lcd.setCursor(0, 1);
        lcd.print("Please Off Auto");}
            break;
          case DOWN:
        if(auto_check == false){
            digitalWrite(Motor,ON); // On: off motor
            led_status1();}
            else  {lcd.setCursor(0, 1);
        lcd.print("Please Off Auto");}
            break;
          case LEFT:
            break;
          case RIGHT:
            lcd.clear();
            isMenuChild = false;
            smart = false;
            break;
          case SELECT:
            led_status1();
            isMenuChild = true;
            break;
        }//switch child case 2
      }//else case 2
      break;
    
    /*-----------gap---------------*/
    case :
      if (smart == false) {
        if (btn_check == SELECT) {
          smart = true;
          healer = btn_check;
        }
        lcd.setCursor(0, 0);
        lcd.print("Timer Mode");
      }
      else {
        if (btn_check != NO_SELECT) {
          healer = btn_check;
        }
        switch (healer) {
          case UP:
          if(auto_check == false){
            hour1_Set = gio;
            Min1_Set = phut;
            Motor_Status = digitalRead(Motor); //check device status
            lcd.setCursor(0, 1);
            lcd.print("Set Time Start");}
            else {lcd.setCursor(0, 1);
        lcd.print("Please Off Auto");}
            break;
          case DOWN:
          if(auto_check ==false){
            End_hour1_Set = gio;
            End_Min1_Set = phut;
            Motor_Status = digitalRead(Motor); //check device status
            lcd.setCursor(0, 1);
            lcd.print("Set Time End  ");}
            else{lcd.setCursor(0, 1);
        lcd.print("Please Off Auto");}
            break;
          case LEFT:
            break;
          case RIGHT:
            lcd.clear();
            isMenuChild = false;
            smart = false;
            break;
          case SELECT:
            led_status1();
            isMenuChild = true;
            break;
        }//switch child case 2
      }//else case 2
      break;
    
    /*-----------gap---------------*/
    case Change_Hour:
      if (smart == false) {
        if (btn_check == SELECT) {
          smart = true;
          healer = btn_check;
        }
        lcd.setCursor(0, 0);
        lcd.print("Change Hour");
      }
      else {
        if (btn_check != NO_SELECT) {
          healer = btn_check;
          isPress = true;
        }
        switch (healer) {
          case UP:
            if (isPress) {
              change++;
              lcd.clear();
              isPress = false;
            }
            if (change > 23) {
              lcd.clear();
              change = 0;
            }
            lcd.setCursor(0, 0);
            lcd.print("Hour :");
            lcd.setCursor(7, 0);
            lcd.print(change);
            break;
          case DOWN:
            if (isPress) {
              change--;
              lcd.clear();
              isPress = false;
            }
            if (change < 0) {
              lcd.clear();
              change = 23;
            }
            lcd.setCursor(0, 0);
            lcd.print("Hour :");
            lcd.setCursor(7, 0);
            lcd.print(change);
            break;
          case LEFT:
            gio = change;
            lcd.setCursor(0, 1);
            lcd.print("Completed");
            break;
          case RIGHT:
            lcd.clear();
            isMenuChild = false;
            isFirst = true;
            smart = false;
            break;
          case SELECT:
            if (isFirst) {
              lcd.clear();
              isFirst = false;
            }
            change = _hour;
            lcd.setCursor(0, 0);
            lcd.print("Hour :");
            lcd.setCursor(7, 0);
            lcd.print(change);
            isMenuChild = true;
            break;
        }//switch child case 3
      }//else case 3
      break;
    /*------------gap---------------*/
    case Change_Minute:
      if (smart == false) {
        if (btn_check == SELECT) {
          smart = true;
          healer = btn_check;
        }
        lcd.setCursor(0, 0);
        lcd.print("Change Minute");
      }
      else {
        if (btn_check != NO_SELECT) {
          healer = btn_check;
          isPress = true;
        }
        switch (healer) {
          case UP:
            if (isPress) {
              change++;
              lcd.clear();
              isPress = false;
            }
            if (change > 59) {
              lcd.clear();
              change = 0;
            }
            lcd.setCursor(0, 0);
            lcd.print("Minute :");
            lcd.setCursor(8, 0);
            lcd.print(change);
            break;
          case DOWN:
            if (isPress) {
              change--;
              lcd.clear();
              isPress = false;
            }
            if (change < 0) {
              lcd.clear();
              change = 59;
            }
            lcd.setCursor(0, 0);
            lcd.print("Minute :");
            lcd.setCursor(8, 0);
            lcd.print(change);
            break;
          case LEFT:
            phut = change;
            lcd.setCursor(0, 1);
            lcd.print("Completed");
            break;
          case RIGHT:
            lcd.clear();
            isMenuChild = false;
            isFirst = true;
            smart = false;
            break;
          case SELECT:
            if (isFirst) {
              lcd.clear();
              isFirst = false;
            }
            change = _minute;
            lcd.setCursor(0, 0);
            lcd.print("Minute :");
            lcd.setCursor(8, 0);
            lcd.print(change);
            isMenuChild = true;
            break;
        }//switch child case 3
      }//else case 3
      break;
    /*------------gap---------------*/
    case Change_Second:
      if (smart == false) {
        if (btn_check == SELECT) {
          smart = true;
          healer = btn_check;
        }
        lcd.setCursor(0, 0);
        lcd.print("Change Second");
      }
      else {
        if (btn_check != NO_SELECT) {
          healer = btn_check;
          isPress = true;
        }
        switch (healer) {
          case UP:
            if (isPress) {
              change++;
              lcd.clear();
              isPress = false;
            }
            if (change > 59) {
              lcd.clear();
              change = 0;
            }
            lcd.setCursor(0, 0);
            lcd.print("Second :");
            lcd.setCursor(8, 0);
            lcd.print(change);
            break;
          case DOWN:
            if (isPress) {
              change--;
              lcd.clear();
              isPress = false;
            }
            if (change < 0) {
              lcd.clear();
              change = 59;
            }
            lcd.setCursor(0, 0);
            lcd.print("Second :");
            lcd.setCursor(8, 0);
            lcd.print(change);
            break;
          case LEFT:
            giay = change;
            lcd.setCursor(0, 1);
            lcd.print("Completed");
            break;
          case RIGHT:
            lcd.clear();
            isMenuChild = false;
            smart = false;
            isFirst = true;
            break;
          case SELECT:
            if (isFirst) {
              lcd.clear();
              isFirst = false;
            }
            change = 0;
            lcd.setCursor(0, 0);
            lcd.print("Second :");
            lcd.setCursor(8, 0);
            lcd.print(change);
            isMenuChild = true;
            break;
        }//switch child case 3
      }//else case 3
      break;
    /*------------gap---------------*/
    case Change_Day:
      if (smart == false) {
        if (btn_check == SELECT) {
          smart = true;
          healer = btn_check;
        }
        lcd.setCursor(0, 0);
        lcd.print("Change Day");
      }
      else {
        if (btn_check != NO_SELECT) {
          healer = btn_check;
          isPress = true;
        }
        switch (healer) {
          case UP:
            if (isPress) {
              change++;
              lcd.clear();
              isPress = false;
            }
            if (change > 7) {
              lcd.clear();
              change = 1;
            }
            change_day();
            break;
          case DOWN:
            if (isPress) {
              change--;
              isPress = false;
            }
            if (change < 1) {
              lcd.clear();
              change = 7;
            }
            change_day();
            break;
          case LEFT:
            thu = change;
            lcd.setCursor(0, 1);
            lcd.print("Completed");
            break;
          case RIGHT:
            lcd.clear();
            isMenuChild = false;
            smart = false;
            isFirst = true;
            break;
          case SELECT:
            if (isFirst) {
              lcd.clear();
              isFirst = false;
            }
            change = _day;
            change_day();
            isMenuChild = true;
            break;
        }//switch child case 3
      }//else case 3
      break;
    /*------------gap---------------*/
    case Change_Date:
      if (smart == false) {
        if (btn_check == SELECT) {
          smart = true;
          healer = btn_check;
        }
        lcd.setCursor(0, 0);
        lcd.print("Change Date");
      }
      else {
        if (btn_check != NO_SELECT) {
          healer = btn_check;
          isPress = true;
        }
        switch (healer) {
          case UP:
            if (isPress) {
              change++;
              lcd.clear();
              isPress = false;
            }
            if (change > 31) {
              lcd.clear();
              change = 0;
            }
            lcd.setCursor(0, 0);
            lcd.print("Date :");
            lcd.setCursor(7, 0);
            lcd.print(change);
            break;
          case DOWN:
            if (isPress) {
              change--;
              lcd.clear();
              isPress = false;
            }
            if (change < 0) {
              lcd.clear();
              change = 31;
            }
            lcd.setCursor(0, 0);
            lcd.print("Date :");
            lcd.setCursor(7, 0);
            lcd.print(change);
            break;
          case LEFT:
            ngay = change;
            lcd.setCursor(0, 1);
            lcd.print("Completed");
            break;
          case RIGHT:
            lcd.clear();
            isMenuChild = false;
            smart = false;
            isFirst = true;
            break;
          case SELECT:
            if (isFirst) {
              lcd.clear();
              isFirst = false;
            }
            change = _date;
            lcd.setCursor(0, 0);
            lcd.print("Date :");
            lcd.setCursor(7, 0);
            lcd.print(change);
            isMenuChild = true;
            break;
        }//switch child case 3
      }//else case 3
      break;
    /*------------gap---------------*/
    case Change_Month:
      if (smart == false) {
        if (btn_check == SELECT) {
          smart = true;
          healer = btn_check;
        }
        lcd.setCursor(0, 0);
        lcd.print("Change Month");
      }
      else {
        if (btn_check != NO_SELECT) {
          healer = btn_check;
          isPress = true;
        }
        switch (healer) {
          case UP:
            if (isPress) {
              change++;
              lcd.clear();
              isPress = false;
            }
            if (change > 12) {
              lcd.clear();
              change = 1;
            }
            change_month();
            break;
          case DOWN:
            if (isPress) {
              change--;
              lcd.clear();
              isPress = false;
            }
            if (change < 1) {
              lcd.clear();
              change = 12;
            }
            change_month();
            break;
          case LEFT:
            thang = change;
            lcd.setCursor(0, 1);
            lcd.print("Completed");
            break;
          case RIGHT:
            lcd.clear();
            isMenuChild = false;
            smart = false;
            isFirst = true;
            break;
          case SELECT:
            if (isFirst) {
              lcd.clear();
              isFirst = false;
            }
            change = _month;
            change_month();
            isMenuChild = true;
            break;
        }//switch child case 3
      }//else case 3
      break;
    /*------------gap---------------*/
    case Change_Year:
      if (smart == false) {
        if (btn_check == SELECT) {
          smart = true;
          healer = btn_check;
        }
        lcd.setCursor(0, 0);
        lcd.print("Change Year");
      }
      else {
        if (btn_check != NO_SELECT) {
          healer = btn_check;
          isPress = true;
        }
        switch (healer) {
          case UP:
            if (isPress) {
              change++;
              lcd.clear();
              isPress = false;
            }
            lcd.setCursor(0, 0);
            lcd.print("Year :");
            lcd.setCursor(7, 0);
            lcd.print(change);
            break;
          case DOWN:
            if (isPress) {
              change--;
              lcd.clear();
              isPress = false;
            }
            if (change < 0) {
              lcd.clear();
              change = _year;
            }
            lcd.setCursor(0, 0);
            lcd.print("Year :");
            lcd.setCursor(7, 0);
            lcd.print(change);
            break;
          case LEFT:
            nam = change;
            lcd.setCursor(0, 1);
            lcd.print("Completed");
            break;
          case RIGHT:
            lcd.clear();
            isMenuChild = false;
            smart = false;
            isFirst = true;
            break;
          case SELECT:
            if (isFirst) {
              lcd.clear();
              isFirst = false;
            }
            change = _year;
            lcd.setCursor(0, 0);
            lcd.print("Year :");
            lcd.setCursor(7, 0);
            lcd.print(change);
            isMenuChild = true;
            break;
        }//switch child case 3
      }//else case 3
      break;
    /*------------gap---------------*/
    case Fix_Time:
      if (smart == false) {
        if (btn_check == SELECT)
        {
          smart = true;
          healer = btn_check;
        }
        lcd.setCursor(0, 0);
        lcd.print("Fix Time");
      }
      else {
        if (btn_check != NO_SELECT)
        {
          healer = btn_check;
        }
        switch (healer)
        {
          case RIGHT:
            lcd.clear();
            isMenuChild = false;
            smart = false;
            break;
          case SELECT:
            //set time
            settime(gio, phut, giay, thu, ngay, thang, nam - 48);
            lcd.setCursor(0, 1);
            lcd.print("Completed");
            isMenuChild = true;
            break;
        }
      } break;
      /*-------------------------------------*/
      case Auto_Mode :
      if (smart == false) {
        if (btn_check == SELECT) {
          smart = true;
          healer = btn_check;
        }
        lcd.setCursor(0, 0);
        lcd.print("Auto Mode");
      }
      else {
        if (btn_check != NO_SELECT) {
          healer = btn_check;
          isPress = true;
        }
        switch (healer) {
          case UP:
           if (isPress) {
              lcd.clear();
              isPress = false;
            }
            auto_check =true;
             lcd.setCursor(0, 0);
            lcd.print("Mode :");
            if(auto_check == true)
            {
            lcd.setCursor(7, 0);
            lcd.print("ON");}
            else {  lcd.setCursor(7, 0);
            lcd.print("OFF");}
            break;
          case DOWN:
           if (isPress) {
              lcd.clear();
              isPress = false;
            }
            auto_check =false;
            lcd.setCursor(0, 0);
            lcd.print("Mode :");
            if(auto_check == true)
            {
            lcd.setCursor(7, 0);
            lcd.print("ON");}
            else {  lcd.setCursor(7, 0);
            lcd.print("OFF");}           
            break;
          case LEFT:
            break;
          case RIGHT:
            lcd.clear();
            isMenuChild = false;
            smart = false;
            isFirst = true;
            break;
          case SELECT:
            if (isFirst) {
              lcd.clear();
              isFirst = false;
            }
            
            lcd.setCursor(0, 0);
            lcd.print("Mode :");
            if(auto_check == true)
            {
            lcd.setCursor(7, 0);
            lcd.print("ON");}
            else {  lcd.setCursor(7, 0);
            lcd.print("OFF");}
            isMenuChild = true;
      break;
        }//switch child case 13
      }//else case 13
      break;
      /*-----------------gap_end----------------*/
  }//switch _menu
}//void
void dht11_wrapper() {
  DHT11.isrCallback();
}
/*void AM()
{    lcd.setCursor(5, 1);
            lcd.print("AM");
  }*/
void argriculture_()
{
  /*Serial.print("Rain : ");
  Serial.print(RAIN_VALUE);
  Serial.println();
    Serial.print("Mass : ");
  Serial.print(MASS_VALUE);
   Serial.println();
    Serial.print("Mass Percent : ");
  Serial.print(Mass_Percent);
   Serial.println();*/
   /*------------------------------*/
   // Dry Soil -- > 5V --> 100 %
   // Wet Soil --> 0v -- > 0%
 if(auto_check == true){
   
   if((Mass_Percent >= Dry_Soil_Threshold )&&(RAIN_VALUE>=No_Rain_Threshold)) // Dry soil & It's not rain
   digitalWrite(Motor,0); // On Motor
  else if(Mass_Percent<Wet_Soil_Threshold){ digitalWrite(Motor,1); // Off Motor
   }
   /*----------------------*/
     if(RAIN_VALUE <Rain_Threshold)
     { digitalWrite(Motor,1); // Off Motor
     digitalWrite(alert,1); }// it's raining
   else digitalWrite(alert,0); // turn off warning ,it's not rain                      
  }
 }

