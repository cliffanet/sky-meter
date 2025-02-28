# sky-meter - Исходники

## Открытость проекта

Проект полностью открыт. Закрывать - не планирую. На коммерческую основу ставить - не планирую.

[Репозиторий на github.com](https://github.com/cliffanet/sky-meter)


## Можно ли использовать исходники в своих проектах

Да, никаких возражений против использования исходного кода в своих проектах. Однако, будет лучше, если вы будете ссылаться на использование данного проекта. Это один из важных мотивационных факторов к дальнейшему развитию.

Это относится и к любительским проектам, и к крупным бесплатным, и к коммерческим.


## Где и что находится

* Исходный код прошивки

    Директория [`cube`](https://github.com/cliffanet/sky-meter/tree/master/cube).
    Оптимизировано для среды [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html).

    - [Как прошить](download.md#обновление-прошивки)
    - [Самостоятельно собрать прошивку](#компиляция-сборка-прошивки-с-помощью-cubemx)

* Исходники электрической схемы и печатной платы

    Всё находится в директории [`hw`](https://github.com/cliffanet/sky-meter/tree/master/hw).
    Среда разработки: [KiCad EDA](https://www.kicad.org/)

    - [Архив с gerber-файлами](../hw/v2.0/sky-meter.v2.0.zip)
    - [Схема принципиальная](../hw/v2.0/sky-meter.v2.0.png)

![](doc/pcb.top.png) ![](doc/pcb.bottom.png)

* Исходники 3D-моделей для печати корпуса

    Директория [`box`](https://github.com/cliffanet/sky-meter/tree/master/box).
    Среда: [FreeCAD](https://www.freecad.org/)
    
    Тут же лежат экспортированные в `stl` файлы моделей. В имени файлов зашифровано:
    
    * `top` / `bottom` - верхняя и нижняя часть корпуса.


## Электронные компоненты:

* Дисплей JLX19292G: COG 192x96 на чипе ST75256 c 24-pin шлейфом ([например](https://aliexpress.ru/item/1005002157371258.html))
* Аккумулятор: 503445 900 mAh ([например](https://aliexpress.ru/item/4000288981646.html))
* Микроконтроллер: STM32F411CEU6


## Компиляция (сборка) прошивки с помощью CubeMX

Необходимые пакеты и программы:

- [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html)

- [arm-none-eabi-gcc](https://developer.arm.com/downloads/-/gnu-rm)

    Для MacOS используйте:

    ```
    brew install --cask gcc-arm-embedded
    ```

- утилита make

Исходный код находится в директории [cube](../cube).

1. Переходим в директорию [cube](../cube).

2. Копируем файл Makefile.stm32f411 в Makefile

3. Открываем файл [sky-meter.stm32f411ceu.ioc](../cube/sky-meter.stm32f411ceu.ioc) в CubeMX.

4. Жмём кнопку "Generate Code".

5. Скачиваем [драйвер SD-карты](http://elm-chan.org/fsw/ff/arc/ff15a.zip) с сайта [elm-chan.org](http://elm-chan.org).

    - берём оттуда только директорию: source
    - переименовываем её в: ff
    - копируем в нашу директорию: [cube/source](../cube/source)
    - удаляем файл cube/source/ff/diskio.c

6. Скачиваем [драйвер дисплея](https://github.com/olikraus/u8g2)

    - берём оттуда только директорию: csrc
    - переименовываем её в: u8g2
    - копируем в нашу директорию: [cube/source](../cube/source)

7. Запускаем командную строку (терминал) в этой же директории.

8. Выполняем:

        make

9. Если всё скомпилировалось успешно, во вложенной директории build будет лежать файл sky-meter.stm32f411ceu.bin

[Инструкция по обновлению прошивки](download.md#обновление-прошивки)

Чтобы изменить язык меню:

    make VLANG=R

или:

    make VLANG=E


## Предыдущая аппаратная версия

- [плата v1.0](../hw/v1.0/sky-meter.v1.0.zip)
- CubeMX файлы:
    - [sky-meter.stm32g431cbu.ioc](../cube/sky-meter.stm32g431cbu.ioc) и [Makefile.stm32g431](../cube/Makefile.stm32g431)
    - [sky-meter.stm32g473ceu.ioc](../cube/sky-meter.stm32g473ceu.ioc) и [Makefile.stm32g473](../cube/Makefile.stm32g473)
