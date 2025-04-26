import 'item.dart';
import 'package:flutter/material.dart';
import 'dart:developer' as developer;

/*
    Когда начал пытаться сделать масштабирование с привязкой к определённой
    точке, понял, что окончательно запутался, что во что преобразовывается.
    Поэтому ViewMatrix разбил на дополнительные классы, чтобы разбить
    преобразование на более простые локаничные этапы.

    Преобразования координат будем делать в следующей последовательности:

        Исходные значения (время/высота)
                    |
                ViewPosition
                    |
        Координаты, отмасштабированные
        в выделенный прямоугольник
        под отрисовку графика.
                    |
                ViewModify
                    |
        Координаты, смёщенные и увеличенные
        пользователем.
                    |
                ViewPort
                    |
        Координаты, преобразованные из
        декартовых (OY - направлена вверх)
        в дисплейные (OY - направлена вниз),
        а так же перенесенные, с учётом отступов
        от края (под оси координат). Эти
        координаты уже готовы для отрисовки
        в canvas.

    ViewPort
    Этот класс:
        - преобразует координаты из дисплейного представления в декартовы
        - создаёт дополнительный padding под оси координат
    Декартовы координаты - это входные координаты, которые нам нужно преобразовать в отображаемые
    Дисплейные координаты - это точка, которую надо отрисовать на canvas
*/

class ViewPort {
    final Size size;
    final Offset padtl, padbr;
    ViewPort (this.size, [this.padtl = const Offset(10,10), this.padbr = const Offset(10,10)]);
    static ViewPort get zero => ViewPort(Size(20,20));

    // максимальные значения декартовых координат, которые будут видны на canvas (минимальные = [0,0])
    double get maxx => size.width   - padtl.dx - padbr.dx;
    double get maxy => size.height  - padtl.dy - padbr.dy;
    Offset get max  => Offset(maxx, maxy);

    double get xl => padtl.dx;
    double get xr => size.width - padbr.dx;
    double get yu => padtl.dy;
    double get yd => size.height - padbr.dy;

    // проверка на видимость декартовых координат
    bool xok(double x) => (x >= padtl.dx) && (x < size.width  - padbr.dx);
    bool yok(double y) => (y >= padtl.dy) && (y < size.height - padbr.dy);
    bool pok(Offset p) => xok(p.dx) && yok(p.dy);
    
    // Прямое преобразование из декартовых координат в точку на canvas
    Offset pnt(Offset val) =>
        Offset(
            val.dx + padtl.dx,
            size.height - padbr.dy - val.dy
        );
    // Обратное преобразование из точки на canvas в декартову координату
    Offset inv(Offset pnt) =>
        Offset(
            pnt.dx - padtl.dx,
            size.height - padbr.dy - pnt.dy
        );
}

/*
    ViewPosition
    Класс масштабирует значения, вписывая их
    на полную высоту и ширину в выделенный
    прямоугольник. Значения (время и высота)
    теперь становятся декартовыми координатами
    (ось OY направлена ввех).
*/

class ViewPosition {
    // Эти min/max - это абсолютный диапазон всего графика,
    // который через матрицу _port масштабируется
    // на всю ширину/высоту в нужный ViewPort
    final Offset _min, _max, _k;

    ViewPosition(Offset sz, Offset max, [Offset min = Offset.zero]) :
        _min = min,
        _max = max,
        _k = Offset(
            max.dx > min.dx ? sz.dx / (max.dx - min.dx) : 1.0,
            max.dy > min.dy ? sz.dy / (max.dy - min.dy) : 1.0,
        );
    static ViewPosition get zero => ViewPosition(Offset.zero, Offset.zero);

    Offset get min => _min;
    Offset get max => _max;
    
    // Прямое преобразование из исходного значения в декартову координату
    Offset pnt(Offset val) =>
        Offset(
            (val.dx - _min.dx) * _k.dx,
            (val.dy - _min.dy) * _k.dy,
        );
    // Обратное преобразование из декартовых координат в исходное значение
    Offset inv(Offset pnt) =>
        Offset(
            pnt.dx / _k.dx + _min.dx,
            pnt.dy / _k.dy + _min.dy,
        );
}

/*
    ViewModify
    Класс преобразователь декартовых координат по матрице:
        - масштабирует
        - смещает
*/
class ViewModify {
    Offset _pi = Offset.zero, _po = Offset.zero;
    double _scal = 1.0, _scbeg = 1.0;

    // прямое преобразование
    Offset pnt(Offset val) => (val - _pi) * _scal + _po;
    // обратное преобразование
    Offset inv(Offset pnt) => (pnt - _po) / _scal + _pi;

    set pfoc(Offset pnt) {
        // в _pi указываем точку в декартовых координатах
        // до преобразований, в _pi - после преобразований.
        // т.е. в итоге эти точки визуально должны совпасть
        _pi = inv(pnt);
        _po = pnt;
        _scbeg = _scal;
    }

    void move(Offset pnt, double scale) {
        _po = pnt;
        _scal = _scbeg*scale;
    }
    void scale(double scale, Offset center) {
        pfoc = center;
        _scal *= scale;
    } 
}

class AxisItem {
    final Offset pnt;
    final double val;
    AxisItem(this.pnt, this.val);
}

class ViewMatrix {
    final _notify = ValueNotifier<int>(0);
    get notify => _notify;

    ViewPosition _pos = ViewPosition.zero;
    final _mod = ViewModify();

    // _dmin / _dmax - это минимальные и максимальные
    // исходные значения, отображаемые в видимой
    // области. Нужно только для отрисовки осей.
    Offset _dmin = Offset.zero, _dmax = Offset.zero;
    void _dupd() {
        _dmin = inv(Offset(_port.xl, _port.yd));
        _dmax = inv(Offset(_port.xr, _port.yu));
    }

    void _viewupd() {
        _dupd();
        _notify.value ++;
    }

    // размер viewport
    ViewPort _port = ViewPort.zero;
    double get xl => _port.xl;
    double get xr => _port.xr;
    double get yu => _port.yu;
    double get yd => _port.yd;
    bool pok(Offset p) => _port.pok(p);

    Size _size = Size.zero;
    Size get size => _size;
    set size(Size sz) {
        _port = ViewPort(sz);
        _pos = ViewPosition(_port.max, _pos.max, _pos.min);

        _size = sz;
        _dupd();
    }

    // scale
    void scaleStart(ScaleStartDetails details) {
        // Сохранение начальных значений
        _mod.pfoc = _port.inv(details.localFocalPoint);
    }
    void scaleUpdate(ScaleUpdateDetails details) {
        _mod.move(_port.inv(details.localFocalPoint), details.scale);
        _viewupd();
    }
    void scaleChange(double scroll, Offset cur) {
        final scale = scroll > 0 ? 0.87 : scroll < 0 ? 1.15 : 1.0;
        _mod.scale(scale, _port.inv(cur) );
        _viewupd();
    }

    Offset ? _cur;
    Offset ? get cursor => _cur;
    void tapDown(TapDownDetails d) {
        _cur = inv(d.localPosition);
        if (!_port.pok(d.localPosition) || (_cur!.dx < 0) || (_cur!.dx >= _omax.dx))
            _cur = null;
        developer.log("down: $_cur");
        _notify.value ++;
    }
    void tapEnd() {
        _cur = null;
        _notify.value ++;
    }

    Offset pnt(int n, double alt) => _port.pnt( _mod.pnt( _pos.pnt(Offset(n.toDouble(), alt)) ) );
    Offset inv(Offset pnt) => _pos.inv( _mod.inv( _port.inv(pnt) ) );

    // максимальный размер исходных данных
    // этот диапазон нужен, чтобы:
    //  - отрисовать корректно оси координат
    //  - при зуме и перетаскивании не уйти за пределы
    //  этого диапазона, иначе потом ловить обратно
    //  актуальную область тяжело без сброса
    Offset _omax = Offset.zero;
    Offset get origmax => _omax;
    set origmax(Offset omax) {
        _pos = ViewPosition(Offset.zero, omax);
        _omax = omax;
        _viewupd();
    }

    // Оси по X и Y
    List<double> _axis(double min, double max, int count, List<double> dlist) {
        if ((max <= min) || dlist.isEmpty || (dlist[0] > max-min))
            return [];
        final n = (max-min) / count;
        double dv = 1;
        for (dv in [1, 5, 10, 50, 100, 200, 300, 600, 1200])
            if (dv >= n)
                break;
        double val = (min / dv).toInt() * dv;
        while (val < min+dv*2/3)
            val += dv;

        final r = <double>[];
        for (; val <= max; val += dv)
            r.add(val);
        return r;
    }
    List<AxisItem> get axisx => 
        _axis(_dmin.dx, _dmax.dx, (_size.width/50) .toInt(), [1, 5, 10, 50, 100, 200, 300, 600, 1200])
        .map((v) => AxisItem(Offset( pnt(v.toInt(),0).dx, _port.yd ), v))
        .toList();
    List<AxisItem> get axisy =>
        _axis(_dmin.dy, _dmax.dy, (_size.height/50).toInt(), [1, 5, 10, 20, 25, 50, 100, 200, 500, 1000])
        .map((v) => AxisItem(Offset( xl, pnt(0,v).dy ), v))
        .toList();
    
    bool _loading = false;
    bool get loading => _loading;
    set loading(bool _l) {
        _loading = _l;
        _notify.value ++;
    }
}

class TraceViewData {
    final _notify = ValueNotifier(0);
    get notify => _notify;

    final _data = <TraceItem>[];
    TraceItem operator[](int index) => _data.elementAt(index);
    int get count => _data.length;
    int _minalt = 0, _maxalt = 0, _rminalt = 0, _rmaxalt = 0, _rcount = 0;
    int get minalt => _minalt;
    int get maxalt => _maxalt;
    int get rminalt => _rminalt;
    int get rmaxalt => _rmaxalt;
    int get rcount => _rcount;
    void add(TraceItem value) {
        if (_data.isEmpty) {
            _minalt = value.alt;
            _maxalt = value.alt;
            _rminalt = (value.alt / 100).floor() * 100;
            _rmaxalt = (value.alt / 100).ceil() * 100;
        }
        else {
            if (_minalt > value.alt)
                _minalt = value.alt;
            if (_maxalt < value.alt)
                _maxalt = value.alt;
            while (_rminalt > value.alt)
                _rminalt -= 100;
            while (_rmaxalt < value.alt)
                _rmaxalt += 100;
        }
        _data.add(value);
        while (_rcount < _data.length)
            _rcount += 100;
        _notify.value ++;
    }
    void clear() {
        _data.clear();
        _minalt = 0;
        _maxalt = 0;
        _notify.value ++;
    }
}
