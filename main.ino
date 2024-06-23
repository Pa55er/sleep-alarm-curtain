#include <DS1302.h>
#include <NewTone.h>
#include <IRremote.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2); // LCD 설정 값

//ir센서와 리모컨1
int irPin = 12;
IRrecv irrecv(irPin);
decode_results results;

// 전원 여부
int power = 0;

//시간 설정 값
int alarm = 0;  // 0이면 알람 설정 미완료, 1이면 완료
int hour = -1;
int mn = -1;

//LED 설정 값
int ledGreen = 3;
int ledYellow = 2;
int ledOnOff = 1; // 1이면 LED OFF, 0이면 LED ON

// 피에조 버저 설정 값
int soundGo = 0; // 동작 여부
int buzzerPin = 10;
int songLength = 2;
char notes[] = "c ";
int beats[] = {1,1};
int tempo = 200;

// 초음파 거리센서 값
const char trigPin = 5;
const char echoPin = 6;
int pulseWidth;
int distance;

// DS1302 설정 값
const int CEPin = 7;
const int IOPin = 8;
const int CLKPin = 9;
DS1302 rtc(CEPin, IOPin, CLKPin);

void setup() {
  // 시리얼 통신
  Serial.begin(9600);
  //ir센서와 리모컨2
  irrecv.enableIRIn();
  
  //lcd 초기화
  lcd.init();
  
  //LED output 설정
  pinMode(ledGreen, OUTPUT);
  pinMode(ledYellow, OUTPUT);
  
  // 피에조 버저핀 설정
  pinMode(buzzerPin, OUTPUT);
  
  // 초음파 거리센서 트리거 핀 설정
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW);
  
  // DS1302 RTC 모듈 설정
  rtc.writeProtect(false);
  rtc.halt(false);
  Time t(2022,11,28,10,20,00,Time::kMonday);
  rtc.time(t);
}

void loop() {
  Time t = rtc.time();
  
  // 실시간으로 변화하는 현재시간과 설정한 알람 시간을 LCD에 반영
  if(power == 1) {
    delay(20);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("ALARM >> ");
    if(alarm == 0) lcd.print("NOT YET"); // 알람 설정이 안되어 있는 경우
    else {  // 알람 설정이 되어 있는 경우
      lcd.setCursor(8,0);
      lcd.print(hour);lcd.print(" : ");lcd.print(mn);
    }
    lcd.setCursor(0,1);   // 현재 시간
    lcd.print("  NOW >> ");lcd.print(t.hr);lcd.print(" : ");lcd.print(t.min);
  }
  
  // 시리얼 통신에 입력한 값이 있는지 확인하여 알람 시간으로 설정
  int a = 0;
  char b;
  if(Serial.available()) {
    while(Serial.available()) {
      b = Serial.read();
      if(isDigit(b)) {
        a *= 10;
        a += b - '0';
        Serial.println(a);
      }
    }
    hour = a / 100;
    mn = a % 100;
    alarm = 1;
    ledOnOff = 1;
    soundGo = 0;
    digitalWrite(ledGreen, LOW);
    digitalWrite(ledYellow, LOW);
  }
  
  // 리모컨 사용시 동작하는 코드
  if(irrecv.decode(&results)) {
    // 전원 버튼 (리모컨 0번)
    if(results.value == 0xFF6897) {
      delay(10);
      irrecv.resume();
      if(power == 0) {  // 전원을 켜고 싶을 때
        Serial.println("power ON");
        lcd.backlight();
        power = 1;
      }
      else {            // 전원을 끄고 싶을 때
        Serial.println("power OFF");
        lcd.noBacklight();
        power = 0;
        alarm = 0;
        ledOnOff = 1;
        soundGo = 0;
        hour = -1;
        mn = -1;
        digitalWrite(ledGreen, LOW);
        digitalWrite(ledYellow, LOW);
      }
    }
    // 알람 취소 버튼 (리모컨 2번)
    else if(results.value == 0xFF18E7 && power == 1) {
      Serial.println("alarm Cancel");
      delay(10);
      irrecv.resume();
      alarm = 0;
      ledOnOff = 1;
      soundGo = 0;
      hour = -1;
      mn = -1;
      digitalWrite(ledGreen, LOW);
      digitalWrite(ledYellow, LOW);
    }
    // 램프 버튼 (리모컨 3번)
    else if(results.value == 0xFF7A85 && power == 1) {
      delay(10);
      irrecv.resume();
      if(ledOnOff) {    // LED를 켜고 싶을 때
        Serial.println("led ON");
        // 알람이 설정되어 있으면 초록색 램프가 켜짐.
        if(alarm == 1) digitalWrite(ledGreen, HIGH);
        // 알람이 설정 안되어 있으면 노란색 램프가 켜짐.
        else digitalWrite(ledYellow, HIGH);
        ledOnOff = 0;
      }
      else {            // LED를 끄고 싶을 때
        Serial.println("led OFF");
        ledOnOff = 1;
        digitalWrite(ledGreen, LOW);
        digitalWrite(ledYellow, LOW);
      }
    }
    // 예외 입력 처리
    else {
      delay(10);
      irrecv.resume();
    }
  }

  // 알람 시간과 현재 시간이 일치하는지 검사하고 알람 동작 결정
  if(alarm == 1 && hour == t.hr && mn == t.min) {
    soundGo = 1;
  }

  // 알람이 작동하는 코드
  if(soundGo) {
    Serial.println("sound ON");
    NewTone(buzzerPin,262,200);
    delay(20);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    pulseWidth = pulseIn(echoPin,HIGH);
    distance = pulseWidth / 58;
    // 초음파거리센서와 커튼 사이의 거리가 5cm 이상이면 알람 종료
    if(distance >= 5) {
      Serial.println("sound OFF");
      soundGo = 0;
      alarm = 0;
      Serial.println("led OFF");
      ledOnOff = 1;
      digitalWrite(ledGreen, LOW);
      digitalWrite(ledYellow, LOW);
    }
  }
}
