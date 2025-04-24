import 'dart:io';
import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:csv/csv.dart';
import 'package:path/path.dart' as p;
import 'package:intl/intl.dart';

import 'dart:developer' as developer;

import 'trace.dart';
import '../device.dart';


extension on String {
    int get tmToMs {
        final vv = split(':').reversed.toList();
        final vs = (vv.isNotEmpty ? vv[0] : '0').split('.');

        return
            (
                int.parse(vv.length > 2 ? vv[2] : '0') * 3600 +
                int.parse(vv.length > 1 ? vv[1] : '0') * 60 +
                int.parse(vs.isNotEmpty ? vs[0] : '0')
            ) * 1000 +
            int.parse((vs.length > 1 ? vs[1] : '0').padRight(3, '0'));
    }
    DateTime get toDT {
        try {
            final d = DateFormat("d.MM.yyyy H:mm:ss").parse(this);
            return d;
        }
        catch (e) {
            return DateTime(0);
        }
    }
}

class jmpitem {
    final DateTime dt;
    final int num, msToff, altBeg, msFF, altCnp, msCnp;
    jmpitem({
        required this.dt,
        required this.num,
        required this.msToff,
        required this.altBeg,
        required this.msFF,
        required this.altCnp,
        required this.msCnp
    });

    jmpitem.byCSV(List<dynamic> r) :
        dt      = (r[0] ?? '').toString().toDT,
        num     = r[1] is int ? r[1] : int.parse(r[1] ?? ''),
        msToff  = (r[2] ?? '').toString().tmToMs,
        altBeg  = r[3] is int ? r[3] : int.parse(r[3] ?? ''),
        msFF    = (r[4] ?? '').toString().tmToMs,
        altCnp  = r[5] is int ? r[1] : int.parse(r[5] ?? ''),
        msCnp   = (r[6] ?? '').toString().tmToMs;
    
    static String _tm(int v) {
        if (v <= 0)
            return '-';
        int sec = v ~/ 1000;
        int min = sec ~/ 60;
        sec -= min*60;

        return '$min:${sec.toString().padLeft(2,'0')}';
    }

    String get date     => '${dt.day}.${dt.month.toString().padLeft(2,'0')}.${dt.year}';
    String get time     => '${dt.hour}:${dt.minute.toString().padLeft(2,'0')}';
    String get tmToff   => _tm(msToff);
    String get tmFF     => _tm(msFF);
    String get tmCnp    => _tm(msCnp);

    List<String> get trace => _trace[num] ?? [];
}

class jmpdate {
    final String date;
    final list = <jmpitem>[];
    jmpdate(this.date);
}

final _logbook = <jmpdate>[];
final _byDate = <String, jmpdate>{};
final _notify = ValueNotifier(0);

final _trace = <int, List<String>>{};
Function (BuildContext context, String name) _tropen = (_, __) {};

class LogBook {
    static Future<bool> byDir(String dir) async {
        clear();
        List<List<dynamic>> txt = [];
        
        try {
            txt =  await File(p.join(dir, 'logbook.csv'))
                            .openRead()
                            .transform(utf8.decoder)
                            .transform(CsvToListConverter(eol: '\n', fieldDelimiter: ','))
                            .toList();
        }
        catch (e) {
            developer.log('Can\'t read logbook.csv: ${e.toString()}');
            return false;
        }

        txt.removeAt(0);

        for (final r in txt)
            add(jmpitem.byCSV(r));
        

        final files =
            (await Directory(dir).list().toList())
                .whereType<File>()
                .map((f) => p.basename(f.path));
        
        for (final fname in files) {
            final m = RegExp(r'^jump_(\d+)_').firstMatch(fname);
            if (m == null)
                continue;
            final num = int.parse(m[1]!);
            if (!_trace.containsKey(num))
                _trace[num] = [];
            
            _trace[num]!.add(fname);
        }

        _tropen = (context, fname) {
            Navigator.push(
                context,
                MaterialPageRoute(builder: (context) => PageTrace.byFile(fname: p.join(dir, fname))),
            );
        };
        
        _notify.value++;
        
        return true;
    }

    static Future<bool> byDev(String address) async {
        clear();
        if (!devConnect(address))
            return false;

        return true;
    }


    static add(jmpitem j) => LogBook.date(j.date).list.add(j);

    static void clear() {
        _logbook.clear();
        _byDate.clear();
        _trace.clear();
        _tropen = (_, __) {};
        _notify.value++;
    }

    static List<jmpdate> get all => _logbook;
    static jmpdate date(String date) {
        final d = _byDate[date];
        if (d != null)
            return d;
        
        final dd = jmpdate(date);
        _logbook.add(dd);
        _byDate[date] = dd;
        return dd;
    }
}

TableRow _Param(String name, String value) {
    return TableRow(
        children: [
            Text('$name:'),
            Text(value)
        ],
    );
}

class PageLogBook extends StatelessWidget {
    final String _from;
    final bool _bydev;
    PageLogBook.byDir(String dir, {super.key}) :
        _from = ' из папки',
        _bydev= false
    {
        LogBook.byDir(dir);
    }
    PageLogBook.byDev(String address, {super.key}) :
        _from = ' с устройства',
        _bydev=true
    {
        LogBook.byDev(address);
    }

    @override
    Widget build(BuildContext context) {
        final ScrollController scroll = ScrollController();
        void scrollend() {
            scroll.animateTo(
                scroll.position.maxScrollExtent,
                duration: Duration(seconds: 2),
                curve: Curves.fastOutSlowIn,
            );
        }

        return Scaffold(
            appBar: 
                AppBar(
                    leading: Navigator.canPop(context) ?
                        IconButton(
                            icon: const Icon(Icons.navigate_before),
                            onPressed: () {
                                Navigator.pop(context);
                                if (_bydev)
                                    devStop();
                            },
                        ) : null,
                    title: Text('Логбук$_from'),
                ),
            body: ValueListenableBuilder(
                valueListenable: _notify,
                builder: (context, value, child) {

                    return ListView.separated(
                        controller: scroll,
                        separatorBuilder: (context, index) => const Divider(),
                        itemCount: _logbook.length,
                        itemBuilder: (context, index) {
                            final d = _logbook[index];

                            return ExpansionTile(
                                initiallyExpanded: (_logbook.length - index) <= 2,
                                title: Text(
                                    d.date,
                                    style: TextStyle(fontWeight: FontWeight.bold),
                                ),
                                childrenPadding: EdgeInsets.only(left: 30),
                                children: d.list.map((j) {
                                    final title = <Widget>[
                                        SizedBox(
                                            width: 80,
                                            child: Text(
                                                j.num.toString(),
                                                style: TextStyle(fontWeight: FontWeight.bold),
                                            ),
                                        ),
                                    ];

                                    for (final f in j.trace)
                                        title.add(
                                            ElevatedButton(
                                                style: ElevatedButton.styleFrom(
                                                    minimumSize: const Size(120, 24),
                                                    maximumSize: const Size(120, 24),
                                                    textStyle: TextStyle(fontSize: 9),
                                                    
                                                ),
                                                onPressed: () => _tropen(context, f),
                                                child: Text('трассировка'),
                                            )
                                        );

                                    return Card(
                                        child: ListTile(
                                            leading: Text(j.time),
                                            title: Row(children: title),
                                            subtitle: Table(
                                                columnWidths: {
                                                    0: IntrinsicColumnWidth(),
                                                    1: FixedColumnWidth(80.0),
                                                },
                                                children: [
                                                    _Param('Длительность набора', j.tmToff),
                                                    _Param('Высота отделения', '${j.altBeg} m'),
                                                    _Param('Время своб. падения', j.tmFF),
                                                    _Param('Высота раскрытия', '${j.altCnp} m'),
                                                    _Param('Время пилотирования', j.tmCnp),
                                                ],
                                            ),
                                        ),
                                    );
                                }).toList(),
                            );
                        }
                    );
                }
            ),
            floatingActionButton: FloatingActionButton(
                onPressed: scrollend,
                mini: true,
                child: const Icon(Icons.arrow_downward)
            )
        );
    }
}
