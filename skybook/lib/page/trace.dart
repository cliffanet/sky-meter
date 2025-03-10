import 'dart:io';
import 'package:flutter/material.dart';
import 'package:flutter/gestures.dart';

import '../trace/paint.dart';
import '../trace/csv.dart';
import '../trace/item.dart';
import '../trace/viewdata.dart';
import '../jump/call.dart';

class PageTrace extends StatelessWidget {
    final String _name;
    final _data = TraceViewData();

    PageTrace.byFile({ super.key, required String fname }) :
        _name = File(fname).uri.pathSegments.last
    {
        loadfile(fname);
    }

    void loadfile(String fname) async {
        _data.clear();
        final lnall = await File(fname).readAsLines();
        lnall.removeAt(0); // заголовок

        jump.clear();

        for (final l in lnall) {
            final r = bycsv(l);
            final alt = int.parse(r[0]);
            jump.add(alt * 1.0);
            try {
                _data.add(TraceItem(
                    alt: alt,
                    clc: (r.length > 1) && r[1].isNotEmpty ? r[1].codeUnitAt(0) : 0,
                    chg: (r.length > 2) && r[2].isNotEmpty ? r[2].codeUnitAt(0) : 0,
                ));
            }
            catch(ex) {}
        }
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
                onScaleStart:   _data.scaleStart,
                onScaleUpdate:  _data.scaleUpdate,
                child: ValueListenableBuilder(
                    valueListenable: _data.notify,
                    builder: (BuildContext context, _, Widget? child) {
                        return Listener(
                            onPointerSignal: (pointerSignal) {
                                //pointerSignal.
                                if (pointerSignal is PointerScrollEvent) {
                                    _data.scaleScroll(pointerSignal.scrollDelta.dy);
                                }
                            },
                            child: CustomPaint(
                                painter: TracePaint(_data),
                                size: Size.infinite
                            )
                        );
                    }
                )
            )
        );
    }
}
