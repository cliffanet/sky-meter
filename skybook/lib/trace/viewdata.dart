import 'item.dart';
import 'package:flutter/material.dart';

class ViewTransform {
    final Offset point;
    final double scale;

    ViewTransform(this.point, this.scale);
    static ViewTransform get zero => ViewTransform(Offset.zero, 1);

    ViewTransform operator+(ViewTransform t) =>
        ViewTransform(point + t.point, scale * t.scale);
    ViewTransform operator-(ViewTransform t) =>
        ViewTransform(point - t.point, scale / t.scale);
}

class AxisItem {
    double point;
    int value;
    AxisItem(this.point, this.value);
}

class TraceViewArea {
    final double _xmin = 10, _ymin = 10;
    final double _xmax, _ymax;
    final int _rmaxalt, _rcount;
    final ViewTransform _trans;

    TraceViewArea(TraceViewData data, Size size) :
        _xmax = size.width-10,
        _ymax = size.height-10,
        _rmaxalt = data._rmaxalt,
        _rcount  = data._rcount,
        _trans   = data.transform;
    
    double get xmin => _xmin;
    double get xmax => _xmax;
    double get ymin => _ymin;
    double get ymax => _ymax;
    double get width    => _xmax-_xmin;
    double get height   => _ymax-_ymin;
    // точки углов графика
    Offset get ltop => Offset(_xmin, _ymin);
    Offset get rtop => Offset(_xmax, _ymin);
    Offset get lbot => Offset(_xmin, _ymax);
    Offset get rbot => Offset(_xmax, _ymax);

    bool isin(double x, double y) => 
        (x >= _xmin) && (x <= _xmax) &&
        (y >= _ymin) && (y <= _ymax);

    // получение координат без учёта сдвигов пальцами
    double bx(int n)    => _xmin + width * n / _rcount;
    double by(int alt)  => _ymax - height * alt / _rmaxalt;
    Offset base(int n, int alt)     => Offset(bx(n), by(alt));

    double x(int n)     => bx(n)    + _trans.point.dx;
    double y(int alt)   => by(alt)  + _trans.point.dy;
    Offset point(int n, int alt)    => Offset(x(n), y(alt));

    List<AxisItem> get axisx {
        if ((_rcount <= 0) || (width <= 0))
            return [];
        
        int dv = 1;
        for (dv in [1, 5, 10, 50, 100, 200, 300, 600, 1200])
            if (x(dv)-x(0) >= 50)
                break;
        
        final r = <AxisItem>[];
        final frst = (- _trans.point.dx) * _rcount / width;
        int val = (frst / dv).floor() * dv;
        while (true) {
            final px = x(val);
            if (px > _xmax)
                break;
            if (px > _xmin + 40)
                r.add(AxisItem(px, val));
            val += dv;
        }
        
        return r;
    }
    List<AxisItem> get axisy {
        if ((_rmaxalt <= 0) || (height <= 0))
            return [];
        
        int dv = 1;
        for (dv in [1, 5, 10, 20, 25, 50, 100, 200, 500, 1000])
            if (y(0)-y(dv) >= 50)
                break;
        
        final r = <AxisItem>[];
        final frst = _trans.point.dy * _rmaxalt / height;
        int val = (frst / dv).floor() * dv;
        while (true) {
            final py = y(val);
            if (py < _ymin)
                break;
            if (py < _ymax - 40)
                r.add(AxisItem(py, val));
            val += dv;
        }
        
        return r;
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

    ViewTransform _tbeg = ViewTransform.zero;
    ViewTransform _tupd = ViewTransform.zero;
    ViewTransform _tprv = ViewTransform.zero;
    void scaleStart(ScaleStartDetails details) {
        _tprv += _tupd - _tbeg;
        _tbeg = ViewTransform(details.focalPoint, 1);
    }
    void scaleUpdate(ScaleUpdateDetails details) {
        _tupd = ViewTransform(details.focalPoint, details.scale);
        _notify.value ++;
    }
    ViewTransform get transform => _tprv + (_tupd - _tbeg);

    TraceViewArea area(Size size) => TraceViewArea(this, size);
}
