
// FULL DEMO
#include <Arduino.h>

#define LED_PIN 2
#define MOT1_PIN 27
#define MOT2_PIN 26
#define PWMCHAN1 0
#define PWMCHAN2 1
#define LEDPWM 3
#define BUT 0
#define STARTSPEED 52
#define MINSPEED 35


//#define CHECKMOTOR // в начале программы крутанет колесами туда сюда

#include <EncButton.h>
EncButton eb(21, 19, 18);


void setup()
{
  pinMode(BUT, INPUT);
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  //  motor.setMinDuty(70); // мин. ШИМ

  //  ledcSetup(LEDPWM, 5000, 8);        // chanel, freq, res
  //  ledcAttachPin(LED_PIN, LEDPWM);    // Pin, Channel
  //  ledcWrite(LEDPWM, 0);              // 0..255

  // config PWM esp32
  ledcSetup(PWMCHAN1, 5000, 8);      // chanel, freq, res
  ledcSetup(PWMCHAN2, 5000, 8);      // chanel, freq, res
  ledcAttachPin(MOT1_PIN, PWMCHAN1); // Pin, hannel
  ledcAttachPin(MOT2_PIN, PWMCHAN2); // Pin, Channel

  // пробный прокрут колесами
#ifdef CHECKMOTOR
  ledcWrite(PWMCHAN1, 180);
  ledcWrite(PWMCHAN2, 0);
  delay(500);
  ledcWrite(PWMCHAN1, 0);
  ledcWrite(PWMCHAN2, 180);
  delay(500);
#endif

  ledcWrite(PWMCHAN1, 0);            // 0..255
  ledcWrite(PWMCHAN2, 0);            // 0..255


}//setup

byte pwm = 0;
bool state = 0;
int speed = 0;



void setSpeed(int spd)
{
  Serial.print(" Speed = ");
  Serial.print(spd);
  byte pwm = spd;
  if (spd < 0) {  // reverse speeds
    pwm = spd;
    Serial.print("\t pwm = ");
    Serial.println(pwm);
    ledcWrite(PWMCHAN1, pwm);
    ledcWrite(PWMCHAN2, 255);

  } else { // stop or forward
    pwm = 255 - spd;
    Serial.print("\t pwm = ");
    Serial.println(pwm);
    ledcWrite(PWMCHAN1, 255);
    ledcWrite(PWMCHAN2, pwm);
  }
}


void loop()
{
  eb.tick();
  if (eb.click()) {
    Serial.println(state);
    state = !state;
  }
  if (eb.left()) {
        if((speed < 35) && (speed > -35)) speed = -35;
        else if (speed > -254) speed --;
    Serial.print("Speed ");
    Serial.println(speed);
  }
  if (eb.right()) {
    if ((speed > -35) && (speed < 35)) speed = 35;
    else if (speed < 254) speed ++;
    Serial.print("Speed ");
    Serial.println(speed);
  }



  if (state) {
    digitalWrite(LED_PIN, 1);
    setSpeed(speed);
    //    motor.setSpeed(speed);
  } else {
    digitalWrite(LED_PIN, 0);
    ledcWrite(PWMCHAN1, 0);
    ledcWrite(PWMCHAN2, 0);
    // тормоз
    //    ledcWrite(PWMCHAN1, 255);
    //    ledcWrite(PWMCHAN2, 255);

  }



} // loop