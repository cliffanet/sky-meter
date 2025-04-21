import 'item.dart';
import 'package:flutter/material.dart';
import 'package:vector_math/vector_math_64.dart';
import 'dart:developer' as developer;

class ViewMatrix {
    final _notify = ValueNotifier<int>(0);
    get notify => _notify;

    // общая матрица преобразования
    final Matrix4 _matr = Matrix4.identity();
    Matrix4 get matr => _matr;
    void _matrupd() => _matr.setFrom( _trbeg * _scal * _move * _trend );
    void _viewupd() {
        _matrupd();

        // Ограничиваем перемещение
        Offset d = Offset.zero;
        Offset p = pnt(0, 0);
        if (p.dx > _size.width)
            d -= Offset(p.dx - _size.width, 0);
        if (p.dy < 0)
            d -= Offset(0, p.dy);

        p = pnt(_omax.dx.toInt(), _omax.dy);
        if (p.dx < 0)
            d -= Offset(p.dx, 0);
        if (p.dy > _size.height)
            d -= Offset(0, p.dy - _size.height);
        
        if ((d.dx != 0) || (d.dy != 0)) {
            _move.translate(d.dx, d.dy);
            _matrupd();
        }

        _dataupd();

        _notify.value ++;
    }

    // размер viewport
    Size _size = Size.zero;
    Size get size => _size;
    // _trbeg и _trend нужны, чтобы все преобразования выполнялись
    // из центра рисуемого окна, а не из начала координат
    final _trbeg = Matrix4.identity();
    final _trend = Matrix4.identity();
    // масштаб к оригинальным данным
    Offset _dmap = Offset(1, 1);
    set size(Size sz) {
        _size = sz;
        _trbeg
            ..setIdentity()
            ..translate(_size.width/2, _size.height/2);
        _trend
            ..setIdentity()
            ..translate(-_size.width/2, -_size.height/2);
        // тут не нужно оповещать о необходимости перерисовать,
        // т.к. изменение size мы делаем уже из метода перерисовки
        //_notify.value ++;

        // однако, коэфициенты и границы данных надо обновить
        _dmap = Offset(
            _omax.dx > 0 ? (_size.width-20)  / _omax.dx : 1,
            _omax.dy > 0 ? (_size.height-20) / _omax.dy : 1
        );
        _dataupd();
    }
    Offset get viewLT => Offset(10,                 10);
    Offset get viewRT => Offset(_size.width - 10,   10);
    Offset get viewLB => Offset(10,                 _size.height - 10);
    Offset get viewRB => Offset(_size.width - 10,   _size.height - 10);
    bool prng(Offset p) => 
        (p.dx >= 10) && (p.dx <= _size.width - 10) &&
        (p.dy >= 10) && (p.dy <= _size.height - 10);

    // move
    Offset _pnt = Offset.zero;
    final _move = Matrix4.identity();
    final _mbeg = Matrix4.identity();
    
    // scale
    final _scal = Matrix4.identity();
    final _sbeg = Matrix4.identity();
    void scaleStart(ScaleStartDetails details) {
        // Сохранение начальных значений
        _pnt = details.focalPoint;
        _mbeg.setFrom(_move);
        _sbeg.setFrom(_scal);
    }
    void scaleUpdate(ScaleUpdateDetails details) {
        final delta = details.focalPoint - _pnt;
        final dtr = Matrix4.inverted(_sbeg).transform3( Vector3(delta.dx, delta.dy, 0) );
        _move.setFrom(
            Matrix4.copy(_mbeg)
            ..translate(dtr.x, dtr.y)
        );

        _scal.setFrom(
            Matrix4.copy(_sbeg)
            ..scale(details.scale)
        );

        _viewupd();
    }
    void scaleChange(double scroll) {
        final scale = 1 / (scroll > 0 ? (100 + scroll) / 100 : -3.5 / scroll);
        //developer.log('scale: $scale ($scroll)');
        _scal.scale(scale);
        _viewupd();
    }

    Offset ? _cur;
    Offset ? get cursor => _cur;
    void tapDown(TapDownDetails d) {
        _cur = invert(d.localPosition);
        if (!drng(_cur!.dx.toInt(), _cur!.dy) || (_cur!.dx < 0) || (_cur!.dx.toInt() >= _omax.dy))
            _cur = null;
        developer.log("down: $_cur");
        _notify.value ++;
    }
    void tapEnd() {
        _cur = null;
        _notify.value ++;
    }
    
    void reset() {
        _move.setIdentity();
        _scal.setIdentity();
        _matr.setIdentity();

        _viewupd();
    }

    Offset pnt(int n, double val) {
        // метод .transform3 модифицирует Vector3, который мы ему передаём,
        // поэтому надо передавать копию.
        final t = _matr.transform3( Vector3(n * _dmap.dx, _size.height - 20 - val * _dmap.dy, 0) );

        return Offset(
            t.x + 10,
            t.y + 10
        );
    }
    Offset invert(Offset pnt) {
        final m = Matrix4.inverted(_matr).transform3( Vector3(pnt.dx-10, pnt.dy-10, 0) );
        return Offset( m.x / _dmap.dx, (_size.height - 20 - m.y) / _dmap.dy );
    }

    // максимальный размер исходных данных
    // этот диапазон нужен, чтобы:
    //  - отрисовать корректно оси координат
    //  - при зуме и перетаскивании не уйти за пределы
    //  этого диапазона, иначе потом ловить обратно
    //  актуальную область тяжело без сброса
    Offset _omax = Offset.zero;
    Offset get origmax => _omax;
    set origmax(Offset omax) {
        _omax = omax;
        _viewupd();
    }

    // диапазон данных, видимых на экране
    Offset _dmin = Offset.zero;
    Offset get datamin => _dmin;
    Offset _dmax = Offset.zero;
    Offset get datamax => _dmax;
    void _dataupd() {
        _dmin = invert(Offset( 0, _size.height - 20 ) + Offset(10,10));
        _dmax = invert(Offset( _size.width - 20, 0  ) + Offset(10,10));
    }
    bool drng(int n, double alt) => 
        (n >= _dmin.dx)     && (n <= _dmax.dx) &&
        (alt >= _dmin.dy)   && (alt <= _dmax.dy);

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
    List<double> get axisx => _axis(_dmin.dx, _dmax.dx, (_size.width/50) .toInt(), [1, 5, 10, 50, 100, 200, 300, 600, 1200]);
    List<double> get axisy => _axis(_dmin.dy, _dmax.dy, (_size.height/50).toInt(), [1, 5, 10, 20, 25, 50, 100, 200, 500, 1000]);
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
