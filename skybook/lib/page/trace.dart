import 'dart:io';
import 'package:flutter/material.dart';
import '../trace/paint.dart';
import '../trace/csv.dart';
import '../trace/item.dart';

class PageTrace extends StatelessWidget {
    final String _name;
    final _data = <TraceItem>[];
    final _notify = ValueNotifier(0);

    PageTrace.byFile({ super.key, required String fname }) :
        _name = File(fname).uri.pathSegments.last
    {
        loadfile(fname);
    }

    void loadfile(String fname) async {
        _data.clear();
        _notify.value ++;
        final lnall = await File(fname).readAsLines();
        lnall.removeAt(0); // заголовок

        lnall.forEach((l) {
            final r = bycsv(l);
            try {
                _data.add(TraceItem(
                    alt: int.parse(r[0]),
                    clc: (r.length > 1) && r[1].isNotEmpty ? r[1].codeUnitAt(0) : 0,
                    chg: (r.length > 2) && r[2].isNotEmpty ? r[2].codeUnitAt(0) : 0,
                ));
                _notify.value ++;
            }
            catch(ex) {};
        });
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
            body: ValueListenableBuilder(
                valueListenable: _notify,
                builder: (BuildContext context, _, Widget? child) {
                    return CustomPaint(
                        painter: TracePaint(_data),
                        size: Size.infinite
                    );
                }
            )
        );
    }
}
