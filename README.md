# Высотомер Sky-Meter

Инструмент для парашютиста - простой высотомер на базе барометра.

![](doc/img/main.jpg)

***
## Что умеет

* измерять высоту;
* определять начала подъёма и начало прыжка;
* писать в логбук историю прыжков: время взлёта, высота отделения, высота раскрытия;

***
## Внутри

[![](doc/img/inside1.jpg)](doc/img/inside1.full.jpg)
[![](doc/img/inside2.jpg)](doc/img/inside2.full.jpg)


***
## Документация

* [Исходники](doc/code.md) - где, что лежит, права на использование, помощь при компиляции.
* [Где скачать](doc/download.md)


***
## Известные проблемы

- повышенное энергопотребление в спящем режиме (850 мкА)

    В первой аппаратной версии применялся чип stm32f473 и энергопотребление всего устройства
    на его основе удалось снизить до 125 мкА, а в выключенном - до 85-90 мкА. И это
    при заявленных в документации 56 мкА (что-то дополнительно подребляет и сама плата).

    А вот на чипе stm32f411 при заявленных в документации 10 мкА получить потребление ниже
    60 мкА так и не удалось - но это на пустой прошивке. На боевой же прошивке почему-то
    не отключаются какие-то узлы. С этим ещё предстоит разобраться.

- нестабильная работа с SD-картой

    Есть мысль отказаться от работы с SD-картой совсем и перейти на отдельную flash-память
    объёмом до 64 МБайт.

- отсутствие приложения для импорта логбука

    Разработка отложена на неопределённый срок.

***
