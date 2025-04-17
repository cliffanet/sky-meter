import 'dart:math';
import 'dart:ui';
import 'package:flutter/material.dart';
import 'item.dart';
import 'viewdata.dart';

class TracePaint extends CustomPainter {
    final TraceViewData _data;
    final ViewMatrix _view;

    TracePaint(this._data, this._view);

    void _drawAlt(Canvas canvas, double Function(TraceItem i) alt, Color c) {
        final pnt = Paint()
            ..color = c
            ..strokeWidth = 1;
        
        if (_data.count <= 0)
            return;

        Offset p0 = _view.pnt(0, alt(_data[0]));
        for (int n = 1; n < _data.count; n++) {
            Offset p1 = _view.pnt(n, alt(_data[n]));
            if (_view.prng(p0) || _view.prng(p1))
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

        final p = _view.pnt(n, _view.datamin.dy);
        if (_view.prng(p))
            _dashedLine(canvas, Offset(p.dx, _view.viewLT.dy), Offset(p.dx, _view.viewLB.dy), [2, 5], pnt);
    }

    void _drawCursorH(Canvas canvas, int n, double alt, Color c) {
        final pnt = Paint()
            ..color = c
            ..strokeWidth = 1;

        final p = _view.pnt(n, alt);
        if (_view.prng(p))
            _dashedLine(canvas, Offset(_view.viewLT.dx, p.dy), p, [2, 5], pnt);
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
        
        canvas.drawLine(_view.viewLB, _view.viewRB, paint);
        for (final v in _view.axisx) {
            final p = _view.pnt(v.toInt(), 0);
            canvas.drawLine(Offset(p.dx, _view.viewRB.dy), Offset(p.dx, size.height), paint);
            final text = TextPainter(
                text: TextSpan(
                    text: sectm(v),
                    style: TextStyle(
                        color: Colors.black,
                        fontSize: 14,
                    ),
                ),
                textDirection: TextDirection.ltr,
            );
            text.layout();
            final ymax = _view.viewLB.dy - 5;
            canvas.translate(p.dx-20, ymax);
            canvas.rotate(-pi/2);
            text.paint(canvas, Offset(0, 10));
            canvas.rotate(pi/2);
            canvas.translate(-1*(p.dx-20), -1*ymax);
        }

        canvas.drawLine(_view.viewLT, _view.viewLB, paint);
        for (final v in _view.axisy) {
            final p = _view.pnt(0, v);
            canvas.drawLine(Offset(0, p.dy), Offset(_view.viewLT.dx, p.dy), paint);
            final text = TextPainter(
                text: TextSpan(
                    text: v.toInt().toString(),
                    style: TextStyle(
                        color: Colors.black,
                        fontSize: 14,
                    ),
                ),
                textDirection: TextDirection.ltr,
            );
            text.layout();
            text.paint(canvas, Offset(15, p.dy-10));
        }

        _drawAlt(canvas, (e) => e.inf.avg.alt, Colors.green);
        _drawAlt(canvas, (e) => e.inf.sav.alt, Colors.blue);
        _drawAlt(canvas, (e) => e.inf.app.alt, Colors.brown);
        _drawAlt(canvas, (e) => e.alt.toDouble(), Colors.deepOrangeAccent);

        for (int n = 0; n < _data.count; n++) {
            final d = _data[n];
            if (((d.clc ?? 0) > 0) || ((d.chg ?? 0) > 0)) {
                _drawCursorV(canvas, n, Colors.black12);
                _drawCursorH(canvas, n, d.inf.sav.alt, Colors.black12);
            }

            final j = d.inf.jmp;
            if ((n > 0) && (j.mode > 0) && (j.mode != _data[n-1].inf.jmp.mode)) {
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
                        'alt: ${d.alt} (sqdiff: ${i.sqdiff.toStringAsFixed(1)})\n' +
                        'mode: ${m[ i.jmp.mode + 1 ]}${i.jmp.newcnt > 0 ? '(${sectm(i.jmp.newtm / 100, true)})' : ''}\n' +
                        'avg (alt / speed): ${i.avg.alt.toStringAsFixed(0)} / ${i.avg.speed.toStringAsFixed(1)}\n' +
                        'sav (alt / speed): ${i.sav.alt.toStringAsFixed(0)} / ${i.sav.speed.toStringAsFixed(1)}\n' +
                        'app (alt / speed): ${i.app.alt.toStringAsFixed(0)} / ${i.app.speed.toStringAsFixed(1)}\n' +
                        'sq: ${i.sq.val.toStringAsFixed(1)}${i.sq.isbig > 0 ? ' (big)' : ''}',
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
