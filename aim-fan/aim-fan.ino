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
*/

// ------------- НАСТРОЙКИ --------------
#define STEP_DELAY 15   // скорость серво (меньше 5 не ставить, сонар тупит)
#define TIMEOUT 3000    // таймаут на новый поиск цели из режима удержания
#define MAX_ANGLE 140   // максимальный угол поворота
#define MIN_ANGLE 0    	// минимальный угол поворота
#define DIST_MAX 50    	// максимальное расстояние (см). Датчик бьёт до 4 метров, но будут шумы

#define DEADZONE 10     // зона нечувствительности (мин. разность с калибровкой)
#define MIN_CATCH 8    // мин. количество точек подряд, чтобы  считать цель целью
#define MISTAKES 4      // допустимое количество пропусков при сканировании цели
// ------------- НАСТРОЙКИ --------------

// ---------- ПИНЫ ----------
#define ECHO 5
#define TRIG 4
#define SERVO 3
#define MOS 2
// ---------- ПИНЫ ----------

// ---------- ДЛЯ РАЗРАБОТЧИКОВ ----------
#include "Servo.h"
Servo servo;

#include "GyverHacks.h"
GTimer stepTimer(STEP_DELAY);
GTimer sonarTimer(100);
GTimer timeoutTimer(TIMEOUT);

#include <NewPing.h>
NewPing sonar(TRIG, ECHO, DIST_MAX);

boolean direct;
boolean next;
const byte steps_num = (MAX_ANGLE - MIN_ANGLE) / 2;
int angle = MIN_ANGLE;
int distance[steps_num + 1];
boolean catch_flag, catched_flag, hold_flag;
byte catch_num;
byte mistakes;
byte mode = true;
byte catch_pos;
int hold_signal;
// ---------- ДЛЯ РАЗРАБОТЧИКОВ ----------

void setup() {
  Serial.begin(9600);
  servo.attach(SERVO);
  servo.write(MIN_ANGLE);
  delay(1000);            // ждём поворота в начальное положение
  calibration();          // забиваем калибровочный массив
}

void loop() {
  if (mode) seek();       // режим поиска цели
  else hold();            // режим удержания позиции
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
  if (!hold_flag)           // движение к середине цели
    turn_to(catch_pos);     // крутим серво
  else {                    // расчётная точка достигнута

    if (sonarTimer.isReady()) {                   // отдельный таймер сонара
      byte pos = (angle - MIN_ANGLE) / 2;         // перевод градусов в элемент массива
      int curr_dist = sonar.ping_cm();            // получаем сигнал с датчика
      if (curr_dist == 0) curr_dist = DIST_MAX;   // 0 это не только 0, но и максимум
      int diff = distance[pos] - curr_dist;       // ищем разницу
      if ((diff < DEADZONE) || (curr_dist > 1 && curr_dist < 10)) {   // если вышли из зоны ИЛИ поднесли руку на 1-10 см
        if (timeoutTimer.isReady())               // отработка таймаута
          mode = true;                            // если цель потеряна и вышло время - переходим на поиск
        hold_flag = false;
      } else {                                    // если цель всё ещё в зоне видимости
        timeoutTimer.reset();                     // сбросить таймер
      }
    }
  }
}

void search() {
  if (angle % 2 == 0 && next) {                 // каждые 2 градуса
    next = false;
    byte pos = (angle - MIN_ANGLE) / 2;         // перевод градусов в элемент массива
    int curr_dist = sonar.ping_cm();
    if (curr_dist == 0) curr_dist = DIST_MAX;
    int diff = distance[pos] - curr_dist;
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
        mode = false;                 // переход в режим удержание
        // catch_pos - середина цели. Считаем по разному, если двигались вперёд или назад
        if (direct) catch_pos += catch_num;
        else catch_pos -= catch_num;
        
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
    if (angle % 2 == 0) {   // каждый второй градус
      byte pos = (angle - MIN_ANGLE) / 2;
      int curr_dist = sonar.ping_cm();
      if (curr_dist == 0) distance[pos] = DIST_MAX;
      else distance[pos] = sonar.ping_cm();
    }
    delay(STEP_DELAY * 1.5);
  }
}

// дополнительная функция плавного поворота серво на указанный угол
void turn_to(byte to_angle) {
  if (stepTimer.isReady()) {
    if (angle < to_angle) angle++;
    else if (angle > to_angle) angle--;
    else hold_flag = true;
    servo.write(angle);
    next = true;
  }
}
