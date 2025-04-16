import 'dart:math';
import 'dart:ui';
import 'package:flutter/material.dart';
import 'item.dart';
import 'viewdata.dart';


class _drawSect {
    final Canvas canvas;
    final int n;
    final TraceViewArea a;
    final TraceItem e, p;

    _drawSect(this.canvas, this.n, this.a, TraceViewData data) :
        e = data[n],
        p = data[n-1]
    {
        _line((e) => e.inf.avg.alt, Colors.green);
        _line((e) => e.inf.app.alt, Colors.brown);
        _line((e) => e.alt.toDouble(), Colors.deepOrangeAccent);

        if (((e.clc ?? 0) > 0) || ((e.chg ?? 0) > 0)) {
            _cursorv(Colors.grey);
            _cursorh( e.inf.app.alt, Colors.grey);
        }
    }

    void _line(double Function(TraceItem i) alt, Color c) {
        final pnt = Paint()
            ..color = c
            ..strokeWidth = 1;
        final pp = a.point(n-1, alt(p));
        final pe = a.point(n.toDouble(), alt(e));
        if (a.isin(pp) || a.isin(pe))
            canvas.drawLine(pp, pe, pnt);
    }

    void _cursorv(Color c) {
        final pnt = Paint()
            ..color = c
            ..strokeWidth = 1;

        final x = a.x(n.toDouble());
        if (a.isinx(x))
            _dashedLine(Offset(x, a.ymax), Offset(x, a.ymin), [2, 5], pnt);
    }

    void _cursorh(double alt, Color c) {
        final pnt = Paint()
            ..color = c
            ..strokeWidth = 1;

        final p = a.point(n.toDouble(), alt);
        if (a.isin(p))
            _dashedLine(Offset(a.xmin, p.dy), p, [2, 5], pnt);
    }

    void _dashedLine(
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
}



class TracePaint extends CustomPainter {
    final TraceViewData _data;

    TracePaint(TraceViewData data) :
        _data = data;

    static void _scale(int min, int max, int width, Function(int val, int x) item) {
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

    @override
    void paint(Canvas canvas, Size size) {
        final a = _data.area(size);
        canvas.save();

        final paint = Paint()
            ..color = Colors.black
            ..strokeWidth = 1;
        
        canvas.drawLine(a.lbot, a.rbot, paint);
        for ( final e in a.axisx) {
            canvas.drawLine(Offset(e.point, a.ymax), Offset(e.point, size.height), paint);
            final text = TextPainter(
                text: TextSpan(
                    text:
                            (e.value < 0 ? '-' : '') +
                            (e.value/600).abs().floor().toString() +
                            ':' +
                            ((e.value.abs() % 600) /10).floor().toString().padLeft(2,'0'),
                    style: TextStyle(
                        color: Colors.black,
                        fontSize: 14,
                    ),
                ),
                textDirection: TextDirection.ltr,
            );
            text.layout();
            canvas.translate(e.point-20, a.ymax-5);
            canvas.rotate(-pi/2);
            text.paint(canvas, Offset(0, 10));
            canvas.rotate(pi/2);
            canvas.translate(-1*(e.point-20), -1*(a.ymax-5));
        }

        canvas.drawLine(a.ltop, a.lbot, paint);
        for (final e in a.axisy) {
            canvas.drawLine(Offset(0, e.point), Offset(a.xmin, e.point), paint);
            final text = TextPainter(
                text: TextSpan(
                    text: e.value.toString(),
                    style: TextStyle(
                        color: Colors.black,
                        fontSize: 14,
                    ),
                ),
                textDirection: TextDirection.ltr,
            );
            text.layout();
            text.paint(canvas, Offset(15, e.point-10));
        }

        for (int n = 1; n < _data.count; n++)
            _drawSect(canvas, n, a, _data);

        canvas.restore();
    }

    @override
    bool shouldRepaint(TracePaint oldDelegate) => true;
}
