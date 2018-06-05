![PROJECT_PHOTO](https://github.com/AlexGyver/Aim-Fan/blob/master/proj_img.jpg)
# Самонаводящийся вентилятор своми руками
* [Описание проекта](#chapter-0)
* [Папки проекта](#chapter-1)
* [Схемы подключения](#chapter-2)
* [Материалы и компоненты](#chapter-3)
* [Как скачать и прошить](#chapter-4)
* [FAQ](#chapter-5)
* [Полезная информация](#chapter-6)
[![AlexGyver YouTube](http://alexgyver.ru/git_banner.jpg)](https://www.youtube.com/channel/UCgtAOyEQdAyjvm9ATCi_Aig?sub_confirmation=1)

<a id="chapter-0"></a>
## Описание проекта
Самонаводящийся вентилятор
Особенности:
- Простейшая схема: сервопривод и ультразвуковой датчик расстояния
- Построения карты рабочей области
- Определение цели, её захват и удержание
- Три версии прошивки: обычная, с мосфетом и с двумя датчиками
- Подробности в видео: https://youtu.be/_fh2nlUJePI
<a id="chapter-1"></a>
## Папки
**ВНИМАНИЕ! Если это твой первый опыт работы с Arduino, читай [инструкцию](#chapter-4)**
- **libraries** - библиотеки проекта. Заменить имеющиеся версии
- **radar** - прошивка и программа для проекта Arduino Radar
- **aim-fan** - обычная версия (первая в видео)
- **aim-fan-mos** - версия с транзистором и выключением питания (вторая в видео)
- **aim-fan-mos-dual** - версия с транзистором (можно не подключать) и двумя датчиками(третья в видео)
- **schemes** - схемы подключения

<a id="chapter-2"></a>
## Схемы
![SCHEME](https://github.com/AlexGyver/Aim-Fan/blob/master/schemes/scheme2.jpg)
![SCHEME](https://github.com/AlexGyver/Aim-Fan/blob/master/schemes/scheme3.jpg)
![SCHEME](https://github.com/AlexGyver/Aim-Fan/blob/master/schemes/scheme4.jpg)

<a id="chapter-3"></a>
## Материалы и компоненты
### Ссылки оставлены на магазины, с которых я закупаюсь уже не один год
### Почти все компоненты можно взять в магазине WAVGAT по первым ссылкам
* Arduino NANO с ногами http://ali.pub/2jivem http://ali.pub/2jivf0
* Arduino NANO без ног http://ali.pub/2jivfs http://ali.pub/2jivfz
* Arduino Pro Mini http://ali.pub/2jm2p8
* Программатор http://ali.pub/2jm2js
* Программатор WAVGAT, без пина DTR (во время загрузки нужно самому нажать reset на Arduino) http://ali.pub/2jm2no
* Программатор как на видео http://ali.pub/2jm2o2
* Макетная плата http://ali.pub/2jivgr
* Джамперы папа-мама http://ali.pub/2jivhc
* Джамперы макетные http://ali.pub/2jivhq
* Блок питания 5V - любой зарядник для смартфона
* Серво большая http://ali.pub/2jivjo
* Серво маленькая http://ali.pub/2jivn6
* Датчик расстояния http://ali.pub/2jivl8
* Мосфет модуль http://ali.pub/2jivtl
* Транзисторы, конденсаторы и резисторы - в ЛЮБОМ магазине радиодеталей
* Вентилятор из видео! http://ali.pub/2jivru	http://ali.pub/2jivst

## Вам скорее всего пригодится
* [Всё для пайки (паяльники и примочки)](http://alexgyver.ru/all-for-soldering/)
* [Недорогие инструменты](http://alexgyver.ru/my_instruments/)
* [Все существующие модули и сенсоры Arduino](http://alexgyver.ru/arduino_shop/)
* [Электронные компоненты](http://alexgyver.ru/electronics/)
* [Аккумуляторы и зарядные модули](http://alexgyver.ru/18650/)

<a id="chapter-4"></a>
## Как скачать и прошить
* [Первые шаги с Arduino](http://alexgyver.ru/arduino-first/) - ультра подробная статья по началу работы с Ардуино, ознакомиться первым делом!
* Скачать архив с проектом
> На главной странице проекта (где ты читаешь этот текст) вверху справа зелёная кнопка **Clone or download**, вот её жми, там будет **Download ZIP**
* Установить библиотеки в  
`C:\Program Files (x86)\Arduino\libraries\` (Windows x64)  
`C:\Program Files\Arduino\libraries\` (Windows x86)
* Подключить Ардуино к компьютеру
* Запустить файл прошивки (который имеет расширение .ino)
* Настроить IDE (COM порт, модель Arduino, как в статье выше)
* Настроить что нужно по проекту
* Нажать загрузить
* Пользоваться  

## Настройки в коде
    #define STEP_DELAY 20       // скорость серво (меньше 5 не ставить, сонар тупит)
    #define TIMEOUT 2000        // таймаут на новый поиск цели из режима удержания    
    #define PWR_TIMEOUT 10000   // таймаут на выключение вентилятора
    #define MAX_ANGLE 150       // максимальный угол поворота
    #define MIN_ANGLE 10         // минимальный угол поворота
    #define DIST_MAX 150         // максимальное расстояние (см). Датчик бьёт до 4 метров, но будут шумы
    
    #define DEADZONE 20         // зона нечувствительности (мин. разность с калибровкой)
    #define MIN_CATCH 5         // мин. количество точек подряд, чтобы  считать цель целью
    #define MISTAKES 2          // допустимое количество пропусков при сканировании цели  
<a id="chapter-5"></a>
## FAQ
### Основные вопросы
В: Как скачать с этого грёбаного сайта?  
О: На главной странице проекта (где ты читаешь этот текст) вверху справа зелёная кнопка **Clone or download**, вот её жми, там будет **Download ZIP**

В: Скачался какой то файл .zip, куда его теперь?  
О: Это архив. Можно открыть стандартными средствами Windows, но думаю у всех на компьютере установлен WinRAR, архив нужно правой кнопкой и извлечь.

В: Я совсем новичок! Что мне делать с Ардуиной, где взять все программы?  
О: Читай и смотри видос http://alexgyver.ru/arduino-first/

В: Вылетает ошибка загрузки / компиляции!
О: Читай тут: https://alexgyver.ru/arduino-first/#step-5

В: Сколько стоит?  
О: Ничего не продаю.

### Вопросы по этому проекту

<a id="chapter-6"></a>
## Полезная информация
* [Мой сайт](http://alexgyver.ru/)
* [Основной YouTube канал](https://www.youtube.com/channel/UCgtAOyEQdAyjvm9ATCi_Aig?sub_confirmation=1)
* [YouTube канал про Arduino](https://www.youtube.com/channel/UC4axiS76D784-ofoTdo5zOA?sub_confirmation=1)
* [Мои видеоуроки по пайке](https://www.youtube.com/playlist?list=PLOT_HeyBraBuMIwfSYu7kCKXxQGsUKcqR)
* [Мои видеоуроки по Arduino](http://alexgyver.ru/arduino_lessons/)