import 'dart:math';
import 'dart:ui';
import 'package:flutter/material.dart';
import 'item.dart';

int _maxalt(Iterable<TraceItem> data) {
    if (data.isEmpty)
        return 0;
    int m = data.first.alt;
    data.forEach((t) { if (m < t.alt) m = t.alt; });
    return m;
}

class TracePaint extends CustomPainter {
    final Iterable<TraceItem> _data;
    int _maxv;
    int _maxc;

    TracePaint(Iterable<TraceItem> data) :
        _data = data,
        _maxv = (_maxalt(data) / 100).ceil() * 100,
        _maxc = (data.length / 100).ceil() * 100;

    void _scale(int min, int max, int width, Function(int val, int x) item) {
        double dx = 40;
        if (width < dx/2)
            return;
        
        int dv = ((max-min) / ((width+dx-1) / dx)).round();
        
        for (int r = 10; (r < 1000000) && (dv > r); r *= 10)
            dv = (dv / r).round() * r;
        
        dx = width / ((max-min) / dv);
        int v = min;
        for (double x = 0; x <= (width-dx/2); x += dx, v += dv)
            item(v, x.round());
        
        item(max, width-1);
    }
    
    void _dashedLine(
        Canvas canvas,
        Offset p1,
        Offset p2,
        Iterable<double> pattern,
        Paint paint,
    ) {
        assert(pattern.length.isEven);
        final distance = (p2 - p1).distance;
        final normalizedPattern = pattern.map((width) => width / distance).toList();
        final points = <Offset>[];
        double t = 0;
        int i = 0;
        while (t < 1) {
            points.add(Offset.lerp(p1, p2, t)!);
            t += normalizedPattern[i++];  // dashWidth
            points.add(Offset.lerp(p1, p2, t.clamp(0, 1))!);
            t += normalizedPattern[i++];  // dashSpace
            i %= normalizedPattern.length;
        }
        canvas.drawPoints(PointMode.lines, points, paint);
    }

    @override
    void paint(Canvas canvas, Size size) {
        canvas.save();

        final paint = Paint()
            ..color = Colors.black
            ..strokeWidth = 1;
        canvas.drawLine(Offset(10, size.height-10.0), Offset(10, 10), paint);
        final y = (int alt) => size.height - 10 - (size.height-20) * alt / _maxv;
        _scale(
            0, _maxv, (size.height-20).round(),
            (v, y) {
                canvas.drawLine(Offset(0, size.height-y-10.0), Offset(10, size.height-y-10.0), paint);
                if (y <= 0) return;
                final text = TextPainter(
                    text: TextSpan(
                        text: v.toString(),
                        style: TextStyle(
                            color: Colors.black,
                            fontSize: 14,
                        ),
                    ),
                    textDirection: TextDirection.ltr,
                );
                text.layout();
                text.paint(canvas, Offset(15, size.height-y-20));
            }
        );


        canvas.drawLine(Offset(10, size.height-10.0), Offset(size.width-10, size.height-10.0), paint);
        final x = (int n) => (size.width-20) * n / _maxc;
        _scale(
            0, _maxc, (size.width-20).round(),
            (v, x) {
                canvas.drawLine(Offset(x+10.0, size.height-10.0), Offset(x+10.0, size.height), paint);
                if (x <= 0) return;
                final text =  TextPainter(
                    text: TextSpan(
                        text: (v/600).floor().toString() + ':' + ((v % 600) /10).floor().toString().padLeft(2,'0'),
                        style: TextStyle(
                            color: Colors.black,
                            fontSize: 14,
                        ),
                    ),
                    textDirection: TextDirection.ltr,
                );
                text.layout();
                canvas.translate(x-10, size.height-15);
                canvas.rotate(-pi/2);
                text.paint(canvas, Offset(0, 10));
                canvas.rotate(pi/2);
                canvas.translate(-1*(x-10), -1*(size.height-15));
            }
        );

        paint.color = Colors.deepOrangeAccent;
        paint.strokeWidth = 1;
        final pchg = Paint.from(paint);
        pchg.color = Colors.grey;
        for (int n = 1; n < _data.length; n++) {
            final el = _data.elementAt(n);
            final xn = x(n);
            final yn = y(el.alt);
            canvas.drawLine(
                Offset(x(n-1), y(_data.elementAt(n-1).alt)),
                Offset(xn, yn),
                paint
            );
            if (((el.clc ?? 0) > 0) || ((el.chg ?? 0) > 0)) {
                _dashedLine(canvas, Offset(xn, y(0)), Offset(xn, yn), [2, 5], pchg);
                _dashedLine(canvas, Offset(x(0), yn), Offset(xn, yn), [2, 5], pchg);
            }
        }

        canvas.restore();
    }

    @override
    bool shouldRepaint(TracePaint oldDelegate) => true;
}
