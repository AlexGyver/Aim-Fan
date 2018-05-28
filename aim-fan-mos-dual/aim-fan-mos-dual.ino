/*
   Как работает алгоритм:
   Серво крутится от MIN_ANGLE до MAX_ANGLE с шагом 1 градус
   Каждые 2 градуса стреляем дальномером. Перед началом работы
   калибруемся: сканируем пространство и составляем "карту"
   рабочей области, получаем массив расстояний.
   В режиме seek крутимся и сравниваем значения с измеренными.
   Если находим расхождение в DEADZONE, начинаем считать, сколько
   будет таких точек. Если их больше MIN_CATCH - считаем за "цель".
   Таким образом фильтруем случайные маленькие объекты и шумы. Также
   прощаем системе несколько ошибок (MISTAKES) во время сканирования цели.
   В итоге получаем угол начала "цели" и угол конца. Ищем средний угол
   и поворачиваемся на него, то есть в центр цели, это режим hold.
   Далее в режиме удержания цели измеряем расстояние, если выпадаем из
   DEADZONE - переходим к поиску цели.
   Если цель не была обнаружена за PWR_TIMEOUT миллисекунд - переходим в
   режим ожидания: выключаем вентилятор, стоим в центре и измеряем.
   Если обнаружили препятствие ближе 10 см в течение 2 секунд - поиск цели
*/

// ------------- НАСТРОЙКИ --------------
#define STEP_DELAY 15       // скорость серво (меньше 10 не ставить, сонар тупит)
#define TIMEOUT 1000        // таймаут на новый поиск цели из режима удержания
#define PWR_TIMEOUT 10000   // таймаут на выключение вентилятора
#define MAX_ANGLE 140       // максимальный угол поворота
#define MIN_ANGLE 0         // минимальный угол поворота
#define DIST_MAX 150        // максимальное расстояние (см). Датчик бьёт до 4 метров, но будут шумы

#define DEADZONE 20         // зона нечувствительности (мин. разность с калибровкой)
#define MIN_CATCH 15         // мин. количество точек подряд, чтобы  считать цель целью
#define MISTAKES 2          // допустимое количество пропусков при сканировании цели
// ------------- НАСТРОЙКИ --------------

// ---------- ПИНЫ ----------
#define TRIG 4
#define ECHO 5
#define TRIG2 6
#define ECHO2 7

#define SERVO 3
#define MOS 2
// ---------- ПИНЫ ----------

// ---------- ДЛЯ РАЗРАБОТЧИКОВ ----------
//#define RADAR_DEBUG 1
#include "Servo.h"
Servo servo;

#include "GyverHacks.h"
GTimer stepTimer(STEP_DELAY);
GTimer sonarTimer(50);
GTimer timeoutTimer(TIMEOUT);
GTimer powerTimer(PWR_TIMEOUT);
GTimer wakeTimer(1000);

#include <NewPing.h>
NewPing sonar1(TRIG, ECHO, DIST_MAX);
NewPing sonar2(TRIG2, ECHO2, DIST_MAX);

typedef enum {SEEK, HOLD, WAIT};
boolean direct;
boolean next;
const byte steps_num = (MAX_ANGLE - MIN_ANGLE);
int angle = MIN_ANGLE;
int distance1[steps_num + 1], distance2[steps_num + 1];
boolean catch_flag, catched_flag, hold_flag, wait_flag = false;
byte catch_num;
byte mistakes;
byte mode = 0;
byte catch_pos;
int hold_signal;
byte direct_seek = 2;
byte lost_flag = 0;
// ---------- ДЛЯ РАЗРАБОТЧИКОВ ----------

void setup() {
  Serial.begin(115200);
  servo.attach(SERVO);
  servo.write(MIN_ANGLE);
  pinMode(MOS, OUTPUT);
  digitalWrite(MOS, 0);
  delay(1000);            // ждём поворота в начальное положение
  calibration();          // забиваем калибровочный массив
  digitalWrite(MOS, 1);   // включили вентилятор
  powerTimer.reset();     // сброс таймера ждущего режима
}

void loop() {
  switch (mode) {
    case SEEK: seek();  // режим поиска цели
      break;
    case HOLD: hold();  // режим удержания позиции
      break;
    case WAIT: wait();  // режим ожидания
      break;
  }
}

void wait() {
  if (!wait_flag) {
    turn_to((MAX_ANGLE - MIN_ANGLE) / 2);     // крутим серво в середину
    digitalWrite(MOS, 0);                     // мосфет вырубили
  }
  if (sonarTimer.isReady()) {                 // опрос сонара по таймеру
    int curr_dist1, curr_dist2;
    curr_dist1 = sonar1.ping_cm();          // получаем сигнал с датчика
    delay(40);
    curr_dist2 = sonar2.ping_cm();
    if ((curr_dist1 > 1 && curr_dist1 < 10) ||
        (curr_dist2 > 1 && curr_dist2 < 10)) {    // если меньше 10 см
      if (wakeTimer.isReady()) {              // и сработал таймер
        // переходим в поиск цели, включаем вентилятор...
        mode = SEEK;
        wait_flag = false;
        digitalWrite(MOS, 1);
        delay(2000);    // чтобы успеть убрать руку
        powerTimer.reset();   // сбрасываем, чтобы сразу не сработал
      }
    } else {
      wakeTimer.reset();                      // сбросить таймер поднесения руки
    }
  }
}

void seek() {
  if (direct) {             // движемся в прямом направлении
    if (angle < MAX_ANGLE)
      turn_to(MAX_ANGLE);   // плавный поворот
    else {
      direct = false;       // смена направления
      delay(50);            // задержка в крайнем положении
    }
  }
  else {                    // движемся в обратном направлении
    if (angle > MIN_ANGLE)
      turn_to(MIN_ANGLE);   // плавный поворот
    else {
      direct = true;        // смена направления
      delay(50);            // задержка в крайнем положении
    }
  }
  search();                 // ищем цель дальномером
}

void hold() {
  if (!hold_flag) {           // движение к середине цели
    turn_to(catch_pos);     // крутим серво
  } else {                    // расчётная точка достигнута
    if (sonarTimer.isReady()) {                   // отдельный таймер сонара
      byte pos = (angle - MIN_ANGLE);         // перевод градусов в элемент массива
      int curr_dist1, curr_dist2;
      int diff1, diff2;

      curr_dist1 = sonar2.ping_cm(); // получаем сигнал с датчика
      if (curr_dist1 == 0) curr_dist1 = DIST_MAX;   // 0 это не только 0, но и максимум
      diff1 = distance1[pos] - curr_dist1;          // ищем разницу
      delay(50);
      curr_dist2 = sonar1.ping_cm();   // получаем сигнал с датчика
      if (curr_dist2 == 0) curr_dist2 = DIST_MAX;   // 0 это не только 0, но и максимум
      diff2 = distance2[pos] - curr_dist2;          // ищем разницу

#ifdef RADAR_DEBUG
      Serial.print(angle);
      Serial.print(",");
      Serial.print((diff1 > 0) ? diff1 : DIST_MAX);
      Serial.print(".");
#endif
      if (lost_flag == 0) {
        if (diff1 < DEADZONE) {
          direct_seek = 1;
          lost_flag = 1;
        }
        if (diff2 < DEADZONE) {
          direct_seek = 0;
          lost_flag = 1;
        }
      } else if (lost_flag == 1) {
        if (diff1 < DEADZONE && diff2 < DEADZONE) lost_flag = 2;
      }

      if (diff1 > DEADZONE && diff2 > DEADZONE) {
        lost_flag = 0;
      }

      if (lost_flag == 2) {   // если вышли из зоны ИЛИ поднесли руку на 1-10 см
        if (timeoutTimer.isReady()) {             // отработка таймаута
          mode = SEEK;                            // если цель потеряна и вышло время - переходим на поиск
          hold_flag = false;
          powerTimer.reset();                     // сброс таймера ждущего режима
          direct = direct_seek;
          lost_flag = 0;
        }
      } else {                                    // если цель всё ещё в зоне видимости
        timeoutTimer.reset();                     // сбросить таймер
      }
    }
  }
}

void search() {
  if (/*angle % 2 == 0 &&*/ next) {                 // каждые 2 градуса
    if (powerTimer.isReady()) {
      mode = WAIT;          // если давно не было цели (PWR_TIMER) - перейти в ожидание
      wait_flag = false;
    }
    next = false;

    byte pos = (angle - MIN_ANGLE);         // перевод градусов в элемент массива
    int curr_dist;
    int diff;

    if (direct) curr_dist = sonar2.ping_cm(); // получаем сигнал с датчика
    else curr_dist = sonar1.ping_cm();

    if (curr_dist == 0) curr_dist = DIST_MAX;   // 0 это не только 0, но и максимум

    if (direct) diff = distance1[pos] - curr_dist;          // ищем разницу
    else diff = distance2[pos] - curr_dist;

#ifdef RADAR_DEBUG
    Serial.print(angle);
    Serial.print(",");
    Serial.print((diff > 0) ? diff : DIST_MAX);
    Serial.print(".");
#endif
    if (diff > DEADZONE) {                      // разность показаний больше мертвой зоны
      if (!catch_flag) {
        catch_flag = true;        // флаг что поймали какое то значение
        catch_pos = angle;        // запомнили угол начала предполагаемой цели
      }
      catch_num++;                // увеличили счётчик значений
      if (catch_num > MIN_CATCH)  // если поймали подряд много значений
        catched_flag = true;      // считаем, что это цель
    } else {                                    // если "пусто"
      if (catch_flag) {               // если ловим цель
        if (mistakes > MISTAKES) {    // если число ошибок превысило допустимое
          // сбросить всё
          catch_flag = false;
          catched_flag = false;
          catch_num = 0;
          mistakes = 0;
        } else {
          mistakes++;                 // увеличить число ошибок
        }
      }

      if (catched_flag) {             // поймали цель!
        mode = HOLD;                  // переход в режим удержание
        // catch_pos - середина цели. Считаем по разному, если двигались вперёд или назад
        if (direct) catch_pos += (catch_num / 2);
        else catch_pos -= (catch_num / 2);

        // сбросить всё
        hold_flag = false;
        catch_flag = false;
        catched_flag = false;
        catch_num = 0;
        mistakes = 0;
      }
    }
  }
}

void calibration() {
  // пробегаемся по рабочему диапазону
  for (angle = MIN_ANGLE; angle <= MAX_ANGLE; angle++) {
    servo.write(angle);
    //if (angle % 2 == 0) {   // каждый второй градус
    byte pos = (angle - MIN_ANGLE);
    int curr_dist = sonar2.ping_cm();
    if (curr_dist == 0) distance1[pos] = DIST_MAX;
    else distance1[pos] = curr_dist;
    //}
    delay(STEP_DELAY);
  }
  for (angle = MAX_ANGLE; angle >= MIN_ANGLE; angle--) {
    servo.write(angle);
    //if (angle % 2 == 0) {   // каждый второй градус
    byte pos = (angle - MIN_ANGLE);
    int curr_dist = sonar1.ping_cm();
    if (curr_dist == 0) distance2[pos] = DIST_MAX;
    else distance2[pos] = curr_dist;
    //}
    delay(STEP_DELAY);
  }
}

// дополнительная функция плавного поворота серво на указанный угол
void turn_to(byte to_angle) {
  if (stepTimer.isReady()) {
    if (angle < to_angle) angle++;
    else if (angle > to_angle) angle--;
    else {
      hold_flag = true;
      wait_flag = true;
    }
    servo.write(angle);
    next = true;
    if (angle > MAX_ANGLE) direct = false; // костыль ебать
    if (angle < MIN_ANGLE) direct = true; // костыль ебать
  }
}
