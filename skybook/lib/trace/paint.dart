import 'dart:math';
import 'dart:ui';
import 'package:flutter/material.dart';
import 'viewdata.dart';

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
    
    static void _dashedLine(
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

        paint.color = Colors.deepOrangeAccent;
        paint.strokeWidth = 1;
        final pchg = Paint.from(paint);
        pchg.color = Colors.grey;
        for (int n = 1; n < _data.count; n++) {
            final el = _data[n];
            final l = a.point(n-1, _data[n-1].alt);
            if (!a.isin(l.dx, l.dy))
                continue;
            final p = a.point(n, el.alt);
            if (!a.isin(p.dx, p.dy))
                continue;
            
            canvas.drawLine(l, p, paint);
            if (((el.clc ?? 0) > 0) || ((el.chg ?? 0) > 0)) {
                _dashedLine(canvas, Offset(p.dx, a.ymax), p, [2, 5], pchg);
                _dashedLine(canvas, Offset(a.xmin, p.dy), p, [2, 5], pchg);
            }
        }

        canvas.restore();
    }

    @override
    bool shouldRepaint(TracePaint oldDelegate) => true;
}
