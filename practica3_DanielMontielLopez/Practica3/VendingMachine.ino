#include <LiquidCrystal.h>
#include <string.h>
#include <DHT.h>
#include <avr/wdt.h> //library for watchdog timer

enum {
  BOOT = 0,
  WAITING_CLIENT,
  SHOWING_DATA,
  SERVICE_STATE,
  PREPARING,
  PRODUCT_READY,
  ADMIN
};

//list for admin options
String options[] = {"Ver temperatura",
                    "Ver distancia",
                    "Ver contador",
                    "Modificar precios"
                   };
int index_admin = 0;
bool admin = false;

//variables for admin/reset button
const int button = 2;
int state_button = LOW;
unsigned long press_start = 0;
unsigned long press_duration = 0;
bool button_pressed = false;

//LCD screen
const int rs = 9, en = 8, d4 = 7, d5 = 6, d6 = 5, d7 = 4;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//ultrasounds sensor
const int Trigger = 10;
const int Echo = 12;
long t, distancia;

//joystick
const int VRx = 1; //A0
const int VRy = 0; //A1
const int joystick_button = 3;
int last_button_state = LOW;
int joystick_Y, joystick_X;
bool joystick_H, joystick_L = false;
bool pulse_joistick = false;
 
//declare boot variables
const int boot_led = A2;
bool boot_state = false;
int state_led;

//temperature/ humidity sensor
const int temperature_sensor = 13;
#define DHTTYPE DHT11
DHT dht(temperature_sensor, DHTTYPE);
float temperature;
float humidity;

//global variables 
int index_admin_coffe = 0;
int index = 0;
const int coffe_led = 11;
bool back = false;
int menu_control = 0;

//states for admin menu
bool change_price = false;
bool confirm_price = false;
bool see_coffes = false;
unsigned long start_state_time = 0;
int state;
bool first_time_state;
int times_led_changes;
int preparing_time;



//class cafe with cafe name and price
class Cafe{
private:
  String cafe_type;
  float price;

public:
  //constructor
  Cafe(String type = "Sin nombre", float p = 0.0f) : cafe_type(type), price(p){}

  void set_price(float new_price) {
    price = new_price;
  }

  String get_cafe() { 
    return cafe_type; 
  }
  float get_price() { 
    return price; 
  }
};

Cafe cafes[5] = {

  Cafe("Cafe Solo", 1.00),
  Cafe("Cafe Cortado", 1.10),
  Cafe("Cafe Doble", 1.25),
  Cafe("Cafe Premium", 1.50),
  Cafe("Chocolate", 2.00),
};

//to know if joistick button is pressed
void get_pulse_joistick() {

  int read_j_button = digitalRead(joystick_button);        
  if (read_j_button == LOW && last_button_state == HIGH) {
    pulse_joistick = true;                                
    delay(50);                                             
  }                                                       
  last_button_state = read_j_button;                       
}

//to get person distance
long get_distance() {

  digitalWrite(Trigger, HIGH);
  delayMicroseconds(10); //send pulse of 10us
  digitalWrite(Trigger, LOW);
  t = pulseIn(Echo, HIGH);
  distancia = (t * 0.0343) / 2;

  return distancia;
}

//to now if joistick move to left
void get_back() {

  joystick_X = analogRead(VRx);

  if (joystick_X < 450 && !joystick_H) {
    if (see_coffes) {//if it is in coffe menu return to menu
      if (change_price) {//if it is chage price return to coffe menu
        change_price = false;
      } else {
        see_coffes = false;
      }
    } else {//if it is read a sensor return to menu
      menu_control = 0;
    }
    joystick_L = true;
  } else if (joystick_X >= 450) {
    joystick_H = false;
  }
}

//to show coffes with prices
void coffe_show(int index) {

  lcd.clear();

//when its in last coffe, LCD shows the 2 last coffes
  if (index >= 4) {
    lcd.setCursor(0, 0);
    lcd.print(cafes[3].get_cafe());
    lcd.setCursor(12, 0);
    lcd.print(cafes[3].get_price());
    lcd.setCursor(0, 1);
    lcd.print(cafes[4].get_cafe());
    lcd.setCursor(12, 1);
    lcd.print(cafes[4].get_price());
  } else {
    lcd.setCursor(0, 0);//show cafe 1 in line 0
    lcd.print(cafes[index].get_cafe());
    lcd.setCursor(12, 0);
    lcd.print(cafes[index].get_price());
    lcd.setCursor(0, 1);//show cafe 2 in line 1
    lcd.print(cafes[index +1].get_cafe());
    lcd.setCursor(12, 1);
    lcd.print(cafes[index +1].get_price());
  }
}

//to change coffe price
void show_cafe_change(int i) {
  //get coffe price
  float new_price = cafes[i].get_price();

  while (change_price) {
    get_back();

    get_pulse_joistick();//for detect joystick button

    if (pulse_joistick) {
      confirm_price = true;
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(cafes[i].get_cafe());
    lcd.setCursor(12, 0);
    lcd.print(new_price);

    //if joistick is pressed chenge the coffe price
    if (confirm_price) {
      cafes[i].set_price(new_price);
      confirm_price = false;
    }

    joystick_Y = analogRead(VRy);

    //increment price 0.05
    if (joystick_Y > 600 && !joystick_H) {
      new_price += 0.05;
      joystick_H = true;
    } else if (joystick_Y <= 600) {
      joystick_H = false;
    }
    //decrease price 0.05
    if (joystick_Y < 450 && !joystick_L) {
      new_price -= 0.05;
      joystick_L = true;
    } else if (joystick_Y >= 450) {
      joystick_L = false;
    }
  }
}

//show the admin menu
void admin_options(int index_admin) {

  lcd.clear();

  //when its in last coffe, LCD shows the 2 last coffes
  if (index_admin >= 2) {
    lcd.setCursor(0, 0);
    lcd.print(options[2]);
    lcd.setCursor(0, 1);
    lcd.print(options[3]);
  } else {
    lcd.setCursor(0, 0);//show option 1 in line 0
    lcd.print(options[index_admin]);
    lcd.setCursor(0, 1);//show option 2 in line 1
    lcd.print(options[index_admin +1]);
  }
}

//to detect time press button
void handle_button() {
  noInterrupts();//block others interrutions

  byte button_state = digitalRead(button);

  //if button begin to press
  if (button_state == 0) {
    press_start = millis();
  } else {
    //if button quit to press
    press_duration = millis() - press_start;

    if (press_duration >= 2000 && press_duration <= 3000) {   //reset the system
      state = WAITING_CLIENT;
      first_time_state = true;
    } else if (press_duration >= 5000) {  //admin mode ON
      if (!admin) {
        state = ADMIN;
        menu_control = 0;
        admin = true;
      } else {
        state = SERVICE_STATE;
        admin = false;
      }
    }
  }
  interrupts();
}

void setup() {
  
  lcd.begin(16, 2);//size LCD screen

  Serial.begin(9600);//initialize comunication

  dht.begin();//initialize dht sensor

  pinMode(button, INPUT_PULLUP);//initialize joystick button at PULLUP

  pinMode(boot_led, OUTPUT);//initialize led boot
  digitalWrite(boot_led, LOW);

  pinMode(coffe_led, OUTPUT);//initialize led coffe

  pinMode(joystick_button, INPUT_PULLUP);//initialize joystick button

  pinMode(Trigger, OUTPUT); //pin as Ultrasonic output
  pinMode(Echo, INPUT);  //pin as input Ultrasounds
  digitalWrite(Trigger, LOW);//initialize the pin with 0

  state = BOOT;
  first_time_state = true;
  admin = false;

  //interruption mode CHANGE for get time press button
  attachInterrupt(digitalPinToInterrupt(button), handle_button, CHANGE);
}

void loop() {
  switch (state) {
    case BOOT:
      if(first_time_state) {
        first_time_state = false;
        times_led_changes = 0;
        state_led = HIGH;
        lcd.setCursor(0, 0);
        lcd.clear();
        lcd.print("Cargando...");
        start_state_time = millis();
      }
      digitalWrite(boot_led, state_led);

      if (millis() - start_state_time >= 1000) {
        state_led = !state_led;
        digitalWrite(boot_led, state_led);
        times_led_changes += 1;
        start_state_time = millis();
      }
      if (times_led_changes >= 5) {
        state = WAITING_CLIENT;
        first_time_state = true;
      }
      break;

    case WAITING_CLIENT:
      if (first_time_state) {
        first_time_state = false;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ESPERANDO CLIENTE");
      }
      if (get_distance() <= 100) {
        state = SHOWING_DATA;
        first_time_state = true;
      }
      break;
      
    case SHOWING_DATA:
      if (first_time_state) {
        first_time_state = false;
        lcd.clear();
        start_state_time = millis();
      }
      temperature = dht.readTemperature();
      humidity = dht.readHumidity();

      lcd.setCursor(0, 0);
      lcd.print(temperature);
      lcd.setCursor(4, 0);
      lcd.print("C");
      lcd.setCursor(0, 1);
      lcd.print(humidity);
      lcd.setCursor(4, 1);
      lcd.print("%");

      if (millis() - start_state_time >= 5000) {
        state = SERVICE_STATE;
        first_time_state = true;
      }
      break;

    case SERVICE_STATE:
      if (first_time_state) {
        first_time_state = false;
        digitalWrite(coffe_led, LOW);  //off both leds
        digitalWrite(boot_led, LOW);
        coffe_show(index);
      }
      get_pulse_joistick();

      //if joistick press, show coffe preparation
      if (pulse_joistick) {
        state = PREPARING;
        pulse_joistick = false;
        first_time_state = true;
      }

      joystick_Y = analogRead(VRy);

      if (joystick_Y > 600 && !joystick_H) {
        if(index > 0){//upper bumper index
          index -= 1;
          joystick_H = true;
          first_time_state = true;
        }
      } else if (joystick_Y <= 600) {
        joystick_H = false;
        first_time_state = true;
      }
      if (joystick_Y < 450 && !joystick_L) {
        if (index < 4) {//lower bumper index
          index += 1;
          joystick_L = true;
          first_time_state = true;
        }
      } else if(joystick_Y >= 450){
        joystick_L = false;
        first_time_state = true;
      }
      break;

    case PREPARING:
       if (first_time_state) {
        first_time_state = false;
        lcd.clear();
        lcd.print("Preparando Cafe...");
        preparing_time = random(4, 8) * 1000;//random in miliseconds
        start_state_time = millis();
      }
      if (millis() - start_state_time <= preparing_time) {
        float progress = (float)(millis() - start_state_time) / preparing_time;
        int led_intensity = (int)(progress * 255);
        analogWrite(coffe_led, led_intensity);
      } else {
        digitalWrite(coffe_led, LOW);
        state = PRODUCT_READY;
        first_time_state = true;
      }
      break;

    case PRODUCT_READY:
      if(first_time_state) {
        first_time_state = false;
        digitalWrite(coffe_led, LOW);
        lcd.clear();
        lcd.print("RETIRE BEBIDA");
        start_state_time = millis();
      }
      if (millis() - start_state_time >= 3000) {
        state = SERVICE_STATE;
        first_time_state = true;
      }
      break;

    case ADMIN:                                      //mode ADMIN
      digitalWrite(coffe_led, HIGH); //ON both leds
      digitalWrite(boot_led, HIGH);

      get_pulse_joistick();//for detect joystick button

      get_back();

      //select option
      if (pulse_joistick) {
        menu_control = index_admin +1;

        //if it is in admin menu enter in see coffes mode
        if (menu_control == 4) {
          //if it is in coffe menu enter in change price mode
          if (see_coffes) {
            change_price = true;
          }
          see_coffes = true;
        }
        pulse_joistick = false;
      }

      lcd.clear();

      if (menu_control == 0) {
        //show options
        admin_options(index_admin);
      } else if (menu_control == 1) {//show temperature and humidity
        temperature = dht.readTemperature();
        humidity = dht.readHumidity();

        lcd.setCursor(0, 0);
        lcd.print(temperature);
        lcd.setCursor(4, 0);
        lcd.print("C");
        lcd.setCursor(0, 1);
        lcd.print(humidity);
        lcd.setCursor(4, 1);
        lcd.print("%");
      } else if (menu_control == 2) {//show person distance
        lcd.setCursor(0, 0);
        lcd.print(get_distance());
        lcd.setCursor(3, 0);
        lcd.print("cm");
      } else if (menu_control == 3) {//show timer
        lcd.setCursor(0, 0);
        lcd.print(millis() /1000);
      } else if (menu_control == 4) {//show coffe menu
        coffe_show(index_admin_coffe);

        if (change_price) {
          show_cafe_change(index_admin_coffe);
        }

        //for move in coffe menu
        joystick_Y = analogRead(VRy);

        if (joystick_Y > 600 && !joystick_H) {
          if (index_admin_coffe > 0) {//upper bumper index
            index_admin_coffe -= 1;
            joystick_H = true;
          }
        } else if (joystick_Y <= 600){
          joystick_H = false;
        }
        if (joystick_Y < 450 && !joystick_L) {
          if (index_admin_coffe < 4) {//lower bumper index
            index_admin_coffe += 1;
            joystick_L = true;
          }
        } else if (joystick_Y >= 450) {
          joystick_L = false;
        }
      }
      //for move in admin menu
      joystick_Y = analogRead(VRy);

      if (joystick_Y > 600 && !joystick_H) {
        if(index_admin > 0){//upper bumper index
          index_admin -= 1;
          joystick_H = true;
        }
      } else if(joystick_Y <= 600){
        joystick_H = false;
      }
      if (joystick_Y < 450 && !joystick_L){
        if (index_admin < 3){//lower bumper index
          index_admin += 1;
          joystick_L = true;
        }
      } else if(joystick_Y >= 450){
        joystick_L = false;
      }
      break;
  }
}
