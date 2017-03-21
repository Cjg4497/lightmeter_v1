
/* This is a light meter using BH1750 light sensor


pin connections:

BH1750
        vcc -> 3.3v
        gnd -> gnd
        scl -> a5
        sda -> a4
        addr-> a3

LCD 1602
        gnd -> gnd
        vcc -> 5v
        sda -> a4
        scl -> a5

Rotary encoder
        gnd -> gnd
        vcc -> 5v
        sw  -> a2
        dt  -> a1
        clk -> a0




 */



 
 // First define the library :
 #include <BH1750FVI.h> // Sensor Library
 #include <Wire.h> // I2C Library
 #include <LiquidCrystal_I2C.h>
 #include <math.h>

#include <ClickEncoder.h>
#include <TimerOne.h>

#define num_bones 20

ClickEncoder *encoder;
int encoder_value = 0; 
int iso_index=0; 
int shutter_index=0;
int fstop_index=0;

void timerIsr() {
  encoder->service();
}


int led;
enum Turn {
  pos,
  neg,
  still,
};



float fstop = 2.8;
int shutter = 125;
int iso = 100;

float last_fstop;
int last_shutter;

const unsigned int shutterspeedArray[] = {
  125, 250, 500, 1000, 2000, 4000, 1, 2, 4, 8, 15, 30, 60
};

const unsigned int isoArray[] = {
  200, 400, 800, 1600, 3200, 6400, 50, 100
};

const float fstopArray[] = {
  2.8, 4, 5.6, 8, 11, 16, 22, 1, 1.4, 2
};



 LiquidCrystal_I2C lcd(0x3F,2,1,0,4,5,6,7,3,POSITIVE); 

 BH1750FVI LightSensor;
 double ev = 0;
 double lux= 0;

enum Modes {
  lux_mode,
  s_mode,
  f_mode,
  iso_mode,
};

Modes current_mode;
Modes next_mode;

int bones = num_bones;
 
void watchdog(){
  bones --;

  if(bones <=0){
    bones = 0;
    lcd.noBacklight();

  }
  else{
    lcd.backlight();

  }

}

void feeddog(){
  bones = num_bones;
}

// functions for s-mode
void set_shutter(){
  encoder_value = encoder->getValue();
  shutter_index += encoder_value;
  if (shutter_index < 0){
    shutter_index += 13;
  }
  shutter_index = shutter_index % 13;  
  shutter = shutterspeedArray[shutter_index];
}

void calculate_fstop(){
  last_fstop = fstop;
  lux = LightSensor.GetLightIntensity();
  fstop = sqrt((lux * iso/250) / shutter);
  if (last_fstop != fstop){
    lcd.clear();
  }
}

//set iso
void set_iso(){
  encoder_value = encoder->getValue();
  iso_index += encoder_value;
  if (iso_index < 0){
    iso_index += 8;
  }
  iso_index = iso_index % 8;
  iso = isoArray[iso_index];
}

//functions for f-mode
void calculate_shutter(){
  last_shutter = shutter;
  lux = LightSensor.GetLightIntensity();
  shutter = lux / (fstop * fstop) * iso / 250;
  if (last_shutter != shutter){
    lcd.clear();
  }
}

void set_fstop(){
  encoder_value = encoder->getValue();
  fstop_index += encoder_value;
  if (fstop_index < 0){
    fstop_index += 10;
  }
  fstop_index = fstop_index % 10;  
  fstop = fstopArray[fstop_index];
}

//general display function
void set_display(){
      lcd.setCursor(1, 0);
      lcd.print("ISO");
      lcd.setCursor(1, 1);
      lcd.print(iso);
      lcd.setCursor(7, 0);
      lcd.print("S 1/");
      lcd.print(shutter);
      lcd.setCursor(7, 1);
      lcd.print("f ");
      lcd.print(fstop);
}

void set_mode(){
  switch(current_mode){
    case lux_mode:
      next_mode = s_mode;
      get_ev();
      lcd.setCursor(0, 0);
      lcd.print("LUX");
      lcd.setCursor(5, 0);
      lcd.print("ev ");
      lcd.print(ev);
      lcd.setCursor(0, 1);
      lcd.print("MODE");
      lcd.setCursor(5, 1);
      lcd.print("lux ");
      lcd.print(lux);
      delay(500);

      break;

    case s_mode:
      next_mode = f_mode;
      set_shutter();
      calculate_fstop();
      lcd.setCursor(6, 0);
      lcd.print(">");
      lcd.setCursor(15, 0);
      lcd.print("<");
      set_display();
      delay(500);
      break;

    case f_mode:
      next_mode = iso_mode;
      set_fstop();
      calculate_shutter();
      lcd.setCursor(6, 1);
      lcd.print(">");
      lcd.setCursor(15, 1);
      lcd.print("<");
      set_display();
      delay(500);
      break;

    case iso_mode:
      next_mode = s_mode;
      calculate_fstop();
      lcd.setCursor(0, 0);
      lcd.print(">");
      lcd.setCursor(4, 0);
      lcd.print("<");
      set_iso();
      set_display();
      delay(500);
      break;

  }
}

void get_ev(){
   lux = LightSensor.GetLightIntensity();
   ev = log10(lux/2.5)/log10(2);
}


void setup() {


  Serial.begin(9600);
  encoder = new ClickEncoder(A1, A0, A2);
  encoder->setAccelerationEnabled(true);

  encoder_value = 0;

  lcd.begin(16, 2);
 //  call begin Function so turn the sensor On .
  LightSensor.begin();
  LightSensor.SetAddress(Device_Address_H); //Address 0x5C
  LightSensor.SetMode(Continuous_H_resolution_Mode);

  
  Timer1.initialize(500);
  Timer1.attachInterrupt(timerIsr); 
  

  lcd.setCursor(0, 0);
  lcd.print("Lightmeter");
  lcd.setCursor(1, 1);
  lcd.print("Please wait..."); 
  delay(1000);
  lcd.clear();
  
  current_mode = f_mode;
}


 
void loop() {
  
  watchdog();

  if(encoder_value != 0){
    feeddog();
    lcd.backlight();
  }
   
  set_mode();

   
  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    Serial.print("Button: ");
    #define VERBOSECASE(label) case label: Serial.println(#label); break;
    switch (b) {
      VERBOSECASE(ClickEncoder::Pressed);
      VERBOSECASE(ClickEncoder::Released)
      case ClickEncoder::Clicked:
          current_mode = next_mode;
          lcd.clear();
          feeddog();
          lcd.backlight();

        break;
      case ClickEncoder::Held:
        current_mode = lux_mode;
        lcd.clear();
        feeddog();
        lcd.backlight();
      break;

    }
  }
  Serial.print("iso ");
  Serial.println(iso);
  Serial.print("shutter ");
  Serial.println(shutter);
  Serial.print("f ");
  Serial.println(fstop);

  Serial.println("________________________");

  
}


