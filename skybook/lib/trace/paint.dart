import 'dart:math';
import 'dart:ui';
import 'package:flutter/material.dart';
import 'item.dart';
import 'viewdata.dart';

class TracePaint extends CustomPainter {
    final TraceViewData _data;
    final ViewMatrix _view;

    TracePaint(this._data, this._view);

    void _drawAlt(Canvas canvas, double ? Function(TraceItem i) alt, Color c) {
        final pnt = Paint()
            ..color = c
            ..strokeWidth = 1;
        
        if (_data.count <= 0)
            return;

        final a = alt(_data[0]);
        if (a == null)
            return;
        
        Offset p0 = _view.pnt(0, a);
        for (int n = 1; n < _data.count; n++) {
            final a = alt(_data[n]);
            if (a == null)
                continue;
            Offset p1 = _view.pnt(n, a);
            if (_view.pok(p0) || _view.pok(p1))
                canvas.drawLine(p0, p1, pnt);
            p0 = p1;
        }
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
    void _drawCursorV(Canvas canvas, int n, Color c) {
        final pnt = Paint()
            ..color = c
            ..strokeWidth = 1;

        final p = Offset(_view.pnt(n, 0).dx, _view.yu+1);
        if (_view.pok(p))
            _dashedLine(canvas, p, Offset(p.dx, _view.yd), [2, 5], pnt);
    }

    void _drawCursorH(Canvas canvas, int n, double alt, Color c) {
        final pnt = Paint()
            ..color = c
            ..strokeWidth = 1;

        final p = _view.pnt(n, alt);
        if (_view.pok(p))
            _dashedLine(canvas, Offset(_view.xl, p.dy), p, [2, 5], pnt);
    }

    static String sectm(double v, [bool ms=false]) =>
            (v < 0 ? '-' : '') +
            (v/600).abs().floor().toString() +
            ':' +
            ((v.abs() % 600) /10).floor().toString().padLeft(2,'0') +
            (ms ? '.${(v.abs().toInt() % 10)}' : '');

    @override
    void paint(Canvas canvas, Size size) {
        _view.size = size;
        canvas.save();

        final paint = Paint()
            ..color = Colors.black
            ..strokeWidth = 1;
        
        canvas.drawLine(Offset(_view.xl, _view.yd), Offset(_view.xr, _view.yd), paint);
        for (final a in _view.axisx) {
            canvas.drawLine(a.pnt, a.pnt+Offset(0,5), paint);
            final text = TextPainter(
                text: TextSpan(
                    text: sectm(a.val),
                    style: TextStyle(
                        color: Colors.black,
                        fontSize: 14,
                    ),
                ),
                textDirection: TextDirection.ltr,
            );
            text.layout();
            canvas.translate(a.pnt.dx-20, a.pnt.dy-5);
            canvas.rotate(-pi/2);
            text.paint(canvas, Offset(0, 10));
            canvas.rotate(pi/2);
            canvas.translate(-1*(a.pnt.dx-20), -1*(a.pnt.dy-5));
        }

        canvas.drawLine(Offset(_view.xl, _view.yd), Offset(_view.xl, _view.yu), paint);
        for (final a in _view.axisy) {
            canvas.drawLine(a.pnt, a.pnt - Offset(5, 0), paint);
            final text = TextPainter(
                text: TextSpan(
                    text: a.val.toInt().toString(),
                    style: TextStyle(
                        color: Colors.black,
                        fontSize: 14,
                    ),
                ),
                textDirection: TextDirection.ltr,
            );
            text.layout();
            text.paint(canvas, a.pnt + Offset(5, -10));
        }

        _drawAlt(canvas, (e) => e.inf?.avg.alt, Colors.green);
        _drawAlt(canvas, (e) => e.inf?.a05.alt, Colors.blue);
        _drawAlt(canvas, (e) => e.inf?.a10.alt, Colors.lightBlue);
        _drawAlt(canvas, (e) => e.inf?.app.alt, Colors.brown);
        _drawAlt(canvas, (e) => e.alt.toDouble(), Colors.deepOrangeAccent);

        for (int n = 0; n < _data.count; n++) {
            final d = _data[n];
            if (((d.clc ?? 0) > 0) || ((d.chg ?? 0) > 0)) {
                _drawCursorV(canvas, n, Colors.black12);
                _drawCursorH(canvas, n, d.inf != null ? d.inf!.a10.alt : d.alt.toDouble(), Colors.black12);
            }

            if (d.inf == null)
                continue;
            
            final j = d.inf!.jmp;
            if ((n > 0) && (j.mode > 0) && (j.mode != _data[n-1].inf!.jmp.mode)) {
                _drawCursorV(canvas, n, Colors.blue);
                if ((j.cnt > 0) && (n > j.cnt))
                    _drawCursorV(canvas, n - j.cnt, Colors.deepPurple);
            }
        }

        if (_view.cursor != null) {
            final n = _view.cursor!.dx.toInt();
            final d = _data[n];
            final i = d.inf;
            _drawCursorV(canvas, n, Colors.red);

            final m = [
                'INIT',
                'GROUND',
                'TAKEOFF',
                'FREEFALL',
                'CANOPY',
            ];

            final text = TextPainter(
                text: TextSpan(
                    text:
                        'time: ${sectm(_view.cursor!.dx, true)}\n' +
                        ( i != null ?
                            'alt: ${d.alt} (sqdiff: ${i.sqdiff.toStringAsFixed(1)})\n' +
                            'mode: ${m[ i.jmp.mode + 1 ]}${i.jmp.newcnt > 0 ? '(${i.jmp.newcnt} / ${sectm(i.jmp.newtm / 100, true)})' : ''}\n' +
                            'avg (alt / speed): ${i.avg.alt.toStringAsFixed(0)} / ${i.avg.speed.toStringAsFixed(1)}\n' +
                            'a05 (alt / speed): ${i.a05.alt.toStringAsFixed(0)} / ${i.a05.speed.toStringAsFixed(1)}\n' +
                            'a10 (alt / speed): ${i.a10.alt.toStringAsFixed(0)} / ${i.a10.speed.toStringAsFixed(1)}\n' +
                            'app (alt / speed): ${i.app.alt.toStringAsFixed(0)} / ${i.app.speed.toStringAsFixed(1)}\n' +
                            'sq: ${i.sq.val.toStringAsFixed(1)}${i.sq.isbig > 0 ? ' (big)' : ''}'
                            : ''
                        ),
                    style: TextStyle(
                        color: Colors.black,
                        fontSize: 14,
                    ),
                ),
                textDirection: TextDirection.ltr,
            );
            text.layout();
            final p = _view.pnt(n, _view.cursor!.dy)+Offset(15,0);
            canvas.drawRRect(
                RRect.fromRectAndRadius(
                    Rect.fromPoints(p - Offset(5, 5), p + Offset(text.width, text.height) + Offset(5, 5)),
                    Radius.circular(10)
                ),
                Paint()..color = Colors.red
            );
            text.paint(canvas, p);
        }

        canvas.restore();
    }

    @override
    bool shouldRepaint(TracePaint oldDelegate) => true;
}
