/*
  Скетч к проекту "DaenLight"
  Версия 1.0
  Группа Вконтакте: https://vk.com/daenshow
  Исходники на GitHub: https://github.com/DaenShow/DaenLight
  Нравится проект? Поддержи автора! https://vk.me/moneysend/to/1fxiW
  Автор: DaenShow, 2021
*/

/*
   УПРАВЛЕНИЕ:
   Нажатие верхней кнопки (по кругу смена яркости и сон):
   - включение на 1 уровень яркости
   - переключение на 2 уровень яркости
   - переключение на 3 уровень яркости
   - сон
   Удержание верхней кнопки:
   - переключение на белый цвет и сон
   Нажатие нижней кнопки (по кругу смена режимов):
   - белый цвет
   - радуга
   - уголек
   - разные цвета
   Удержание нижней кнопки:
   - проверка уровня заряда (зеленый - полный заряд, красный - низкий заряд)
*/

//****************** НАСТРАИВАЕМЫЕ ПАРАМЕТРЫ ************************
#define NUMLEDS 10                        // кол-во светодиодов
#define BAT_TIMER 120000                  // период измерения напряжения в миллисекундах
#define SLEEP_TIMER 600000                // период автоматического отключения в миллисекундах
#define BAT_LOW 3300                      // минимальное напряжение работы в милливольтах 
#define LED_TIMER 30                      // период обновления ленты в миллисекундах (влияет на скорость затухания и работы динамических режимов)
#define FIRST_LEVEL 75                    // первый уровень яркости
#define SECOND_LEVEL 100                  // второй уровень яркости
#define THIRD_LEVEL 125                   // третий уровень яркости

//**************************** ПИНЫ *********************************
#define STRIP_PIN PB4                     // пин ленты
#define BTN_PIN_UP PB1                    // пин верхней кнопки
#define BTN_PIN_DOWN PB3                  // пин нижней кнопки
#define MOSFET_PIN PB2                    // пин мосфета

//********************** ПРОЧИЕ НАСТРОЙКИ **************************
#define COLOR_DEBTH 3
#include <microLED.h>                     // подключение ленты
microLED <NUMLEDS, STRIP_PIN, MLED_NO_CLOCK, LED_WS2818, ORDER_GRB, CLI_AVER> strip;

#include <GyverButton.h>
GButton btn_up (BTN_PIN_UP);              // объявление верхней кнопки
GButton btn_down (BTN_PIN_DOWN);          // объявление нижней кнопки

#include <TimerMs.h>
TimerMs timerLed(LED_TIMER, 1, 0);        // объявление таймера обновления ленты
TimerMs timerBat(BAT_TIMER, 1, 0);        // объявление таймера проверки уровня заряда батареи
TimerMs timerSleep(SLEEP_TIMER, 1, 0);    // объявление таймера перехода в режим сна

#include <GyverPower.h>                   // подключение библиотеки сна

//************************ ПЕРЕМЕННЫЕ *****************************

byte counter_up = 0;
byte counter_down = 0;
byte brightness = 0;
int newBrightness = 0;
byte lastBrightness = 0;
byte r, g;
int VCC;
bool powerON, sleepON, batOK, batSHOW, firstloop, brightnessDir, batshowOK;

//************************ ПЕРВОЕ ВКЛЮЧЕНИЕ ************************
void setup() {
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, HIGH);
  btn_up.setDebounce(100);
  btn_up.setTimeout(500);
  btn_down.setDebounce(100);
  btn_down.setTimeout(500);
  power.setSleepMode(POWERDOWN_SLEEP);
  PCMSK |= 1 << PCINT1;
  GIMSK |= 1 << PCIE;
  if (readVCC() <= BAT_LOW) {
    batOK = false;
    batSHOW = true;
  }
  else {
    batOK = true;
    batSHOW = true;
  }
  powerON = true;
  firstloop = true;
  brightnessDir = true;
}

//*********************** ОСНОВНОЙ ЦИКЛ РАБОТЫ ПРОГРАММЫ *****************
void loop() {
  btn_up.tick();
  btn_down.tick();

  if (btn_up.isClick() && newBrightness == brightness) {
    counter_up++;
    if (counter_up > 3)
      counter_up = 0;
  }

  if (timerLed.tick() && powerON == true && batSHOW == false) {
    switch (counter_up) {
      case 0:
        lastBrightness = 0;
        brightness = FIRST_LEVEL;
        break;
      case 1:
        lastBrightness = FIRST_LEVEL;
        brightness = SECOND_LEVEL;
        break;
      case 2:
        lastBrightness = SECOND_LEVEL;
        brightness = THIRD_LEVEL;
        break;
      case 3:
        lastBrightness = THIRD_LEVEL;
        brightness = 0;
        break;
    }

    if (lastBrightness < brightness) {
      newBrightness += 3;
      if (newBrightness >= brightness)
        newBrightness = brightness;
    }
    if (lastBrightness > brightness) {
      newBrightness -= 8;
      if (newBrightness <= brightness)
        newBrightness = brightness;
    }

    strip.setBrightness(newBrightness);

    if (btn_down.isClick()) {
      counter_down++;
      if (counter_down > 12)
        counter_down = 0;
    }

    if (counter_down == 0)
      strip.fill(mWhite);
    else if (counter_down == 1)
      rainbow();
    else if (counter_down == 2)
      fire();
    else
      strip.fill(mWheel8((counter_down - 2) * 25));

    strip.show();
  }

  if (btn_down.isHolded()) {
    batSHOW = true;
  }

  if (timerBat.tick()) {
    if (readVCC() <= BAT_LOW) {
      batOK = false;
      batSHOW = true;
    }
    else {
      batOK = true;
      batSHOW = false;
    }
  }

  if (timerLed.tick() && powerON == true && batSHOW == true) {
    VCC = mapVCC();
    if (firstloop == true) {
      newBrightness -= 10;
      if (newBrightness <= 0) {
        newBrightness = 0;
        firstloop = false;
      }
    }
    else {
      if (brightnessDir == true) {
        newBrightness += 10;
        if (newBrightness >= 100) {
          newBrightness = 100;
          brightnessDir = false;
        }
      }
      else {
        newBrightness -= 10;
        if (newBrightness <= 0) {
          newBrightness = 0;
          batshowOK = true;
        }
      }
      if (batOK == true) {
        r = 255 - VCC;
        g = VCC;
        strip.fill(mRGB(r, g, 0));
      }
      else
        strip.fill(mRed);

    }

    strip.setBrightness(newBrightness);
    strip.show();

    if (batshowOK == true && batOK == false) {
      firstloop = true;
      brightnessDir = true;
      batshowOK = false;
      batSHOW = false;
      powerON = false;
    }
    if (batshowOK == true && batOK == true) {
      firstloop = true;
      brightnessDir = true;
      batshowOK = false;
      batSHOW = false;
      powerON = true;
    }
  }

  if (timerSleep.tick())
    counter_up = 3;

  if (btn_up.isHolded()) {
    counter_up = 3;
    counter_down = 0;
  }

  if (newBrightness == 0 && counter_up == 3)
    powerON = false;

  if (powerON == false) {
    strip.clear();
    digitalWrite(MOSFET_PIN, LOW);
    delay(100);
    power.sleep(SLEEP_FOREVER);
    powerON = true;
    timerBat.start();
    timerSleep.start();
    digitalWrite(MOSFET_PIN, HIGH);

    if (readVCC() <= BAT_LOW) {
      batOK = false;
      batSHOW = true;
    }
    else {
      batOK = true;
      batSHOW = false;
      counter_up++;
    }

  }
}

//**************************** ФУНКЦИЯ ПРЕРЫВАНИЯ ***************************************
ISR(PCINT0_vect) {}

//********************** ФУНКЦИЯ ИЗМЕРЕНИЯ НАПРЯЖЕНИЯ ***********************************
long readVCC() {
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both
  long result = (high << 8) | low;
  result = 1.05 * 1023 * 1000 / result; // расчёт реального VCC
  return result; // возвращает VCC
}

long mapVCC() {
  long a = readVCC();
  long result = constrain (a, 3200, 4200);
  result = map (result, 3200, 4200, 0, 255);
  return result;
}

//********************** РЕЖИМ РАДУГИ ************************************
void rainbow() {
  static byte counter = 0;
  for (int i = 0; i < NUMLEDS; i++) {
    strip.set(i, mWheel8(counter + i * 255 / NUMLEDS));   // counter смещает цвет
  }
  counter += 1;   // counter имеет тип byte и при достижении 255 сбросится в 0
}

//********************** РЕЖИМ УГОЛЬКА ***********************************

void fire() {
#define FLASH  1      // 1-10
#define COOLING 105   // 100-125
#define PAUSE 86   // 50-100  

  static byte heat[10];
  static byte lastheat[10];
  randomSeed(millis());

  for (int i = 0; i < 10; i++) {
    if (i == 0)
      heat[i] = (lastheat[i] * PAUSE + lastheat[9] * random(20) + lastheat[i + 1] * random(20)) / (COOLING);
    else if (i == 9)
      heat[i] = (lastheat[i] * PAUSE + lastheat[i - 1] * random(20) + lastheat[0] * random(20)) / (COOLING);
    else
      heat[i] = (lastheat[i] * PAUSE + lastheat[i - 1] * random(20) + lastheat[i + 1] * random(20)) / (COOLING);
  }

  for (int i = 0; i < 10; i++) {
    if (heat[i] < 10)
      heat[i] = 10;
    if (heat[i] > 100)
      heat[i] = 100;
  }
  for (int i = 0; i < 10; i++) {
    if (random(300) <= FLASH)
      heat[i] = 100;
  }
  for (int i = 0; i < 10; i++) {
    lastheat[i] = heat[i];
  }

  for (int i = 0; i < 10; i++) {
    if (heat[i] < 33)
      strip.set(i, mRGB(255 / 33 * heat[i], 0, 0));
    else if (heat[i] == 33)
      strip.set(i, mRGB(255, 0, 0));
    else if (heat[i] < 67)
      strip.set(i, mRGB(255, 64 / 33 * (heat[i] - 34), 0));
    else if (heat[i] == 67)
      strip.set(i, mRGB(255, 64, 0));
    else if (heat[i] < 100)
      strip.set(i, mRGB(255, 64 + (128 / 33 * (heat[i] - 67)), 0));
    else
      strip.set(i, mRGB(255, 128, 0));
  }
}
