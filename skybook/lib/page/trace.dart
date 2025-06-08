import 'dart:io';
import 'package:flutter/material.dart';
import 'package:flutter/gestures.dart';

import '../trace/paint.dart';
import '../trace/csv.dart';
import '../trace/item.dart';
import '../trace/viewdata.dart';
import '../jump/call.dart';
import '../jump/bind.dart';
import '../device.dart';

import 'dart:developer' as developer;

class PageTrace extends StatelessWidget {
    final String _name;
    final _data = TraceViewData();
    final _view = ViewMatrix();

    PageTrace.byFile({ super.key, required String fname }) :
        _name = File(fname).uri.pathSegments.last
    {
        loadfile(fname);
    }

    PageTrace.byDev({ super.key, required String fname }) :
        _name = fname
    {
        loaddev(fname);
    }

    void loadfile(String fname) async {
        _data.clear();
        _view.loading = true;
        final lnall = await File(fname).readAsLines();
        loadcsv(lnall);
        _view.loading = false;
    }

    void loaddev(String fname) async {
        _data.clear();
        _view.loading = true;
        final s = await devCmd('traceget', [fname]);
        loadcsv(s.data);
        _view.loading = false;
    }

    void loadcsv(List<String> lnall) {
        if (lnall.isNotEmpty)
            lnall.removeAt(0); // заголовок

        try {
            jump.clear();
        } catch (e) {
            developer.log('jump(altcalc) fail: $e');
        }

        for (final l in lnall) {
            final r = bycsv(l);
            final alt = int.parse(r[0]);
            JumpInf ?inf;
            try {
                jump.add(alt * 1.0);
                inf = jump.info();
            } catch (_) {}
            _data.add(TraceItem(
                alt: alt,
                inf: inf,
                clc: (r.length > 1) && r[1].isNotEmpty ? r[1].codeUnitAt(0) : 0,
                chg: (r.length > 2) && r[2].isNotEmpty ? r[2].codeUnitAt(0) : 0,
            ));
        }

        _view.origmax = Offset(_data.rcount.toDouble(), _data.rmaxalt.toDouble());
    }

    @override
    Widget build(BuildContext context) {
        return Scaffold(
            appBar: 
                AppBar(
                    leading: Navigator.canPop(context) ?
                        IconButton(
                            icon: const Icon(Icons.navigate_before),
                            onPressed: () => Navigator.pop(context),
                        ) : null,
                    title: Text('Трассировка: $_name'),
                ),
            body: GestureDetector(
                onScaleStart:   _view.scaleStart,
                onScaleUpdate:  _view.scaleUpdate,
                onTapDown: _view.tapDown,
                onTapUp: (_) => _view.tapEnd(),
                onTapCancel: _view.tapEnd,

                child: ValueListenableBuilder(
                    valueListenable: _view.notify,
                    builder: (BuildContext context, _, Widget? child) {
                        if (_view.loading)
                            return Center(
                                child: CircularProgressIndicator()
                            );
                        return Listener(
                            onPointerSignal: (pointerSignal) {
                                //pointerSignal.
                                if (pointerSignal is PointerScrollEvent) {
                                    _view.scaleChange(pointerSignal.scrollDelta.dy, pointerSignal.localPosition);
                                }
                            },
                            child: CustomPaint(
                                painter: TracePaint(_data, _view),
                                size: Size.infinite
                            )
                        );
                    }
                )
            )
        );
    }
}
