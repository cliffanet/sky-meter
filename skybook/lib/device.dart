import 'package:flutter/foundation.dart';
import 'package:flutter_libserialport/flutter_libserialport.dart';
import 'dart:async';
import 'dart:math';

import 'dart:developer' as developer;

/////////////////////////////////////////////////////////
///
///     Общая обработка подключения к порту
///
/////////////////////////////////////////////////////////

SerialPort ?_port;
SerialPortReader ?_reader;
StreamSubscription<Uint8List> ?_subscr;
Timer ?_echo;

bool devConnect(String addr)  {
    if (_port != null)
        devStop();
    
    _rcv.clear();
    _port = SerialPort(addr);
    if (_port!.isOpen) {
        _port!.close();
        _port!.dispose();
        _port = SerialPort(addr);
    }

    try {
        final cfg = _port!.config;
        cfg.baudRate = 115200;
        cfg.parity = 0;
        cfg.bits = 8;
        cfg.cts = 0;
        cfg.rts = 0;
        cfg.stopBits = 1;
        cfg.xonXoff = 0;

        _reader = SerialPortReader(_port!);
        if (!_port!.openReadWrite()) {
            _port = null;
            return false;
        }

        // только в такой странной последовательности всё работает
        _subscr = _reader!.stream.listen(_read);
        _port!.config = cfg;

        int _efail = 0;
        _echo = Timer.periodic(
            Duration(seconds: 1),
            (_) async {
                if (_port == null) {
                    _.cancel();
                    _echo = null;
                }
                _efail ++; // защита от утечки памяти, если запросы будут подвисать
                final s = await devCmd('echo');
                
                if (s.ok)
                    _efail = 0;
                else {
                    _efail++;
                    if (_efail > 10) {
                        _.cancel();
                        _echo = null;
                    }
                }
            }
        );
    }
    catch (e) {
        _port!.close();
        _port!.dispose();
        _port = null;
        if (_subscr != null)
            _subscr!.cancel();
        if (_echo != null) {
            _echo!.cancel();
            _echo = null;
        }

        developer.log('connect to: $addr - fail');
        
        return false;
    }

    developer.log('connected to: $addr');

    return true;
}

void devStop() async {
    if (_echo != null) {
        _echo!.cancel();
        _echo = null;
        devCmd('term');
    }

    if (_subscr != null) {
        await _subscr!.cancel();
        _subscr = null;
    }
    if (_reader != null) {
        _reader!.close();
        _reader = null;
    }

    if (_port != null) {
        _port!.close();
        _port!.dispose();
        _port = null;
    }

    for (final addr in SerialPort.availablePorts) {
        final p = SerialPort(addr);
        p.close();
        p.dispose();
    }
    
    _rcv.clear();
    _rcvstr = '';
}


/////////////////////////////////////////////////////////
///
///     Отправка команды
///
/////////////////////////////////////////////////////////

class SerialReply {
    final List<String> data = [];
    String err = '';
    bool operator () => err.isEmpty;
    bool get ok => err.isEmpty;
    SerialReply();
    SerialReply.e(this.err);
}

class SerialReplyCmpl {
    final reply = SerialReply();
    final cmpl = Completer<SerialReply>();
}


final Map<String, SerialReplyCmpl> _rcv = {};
String _rcvstr = '';
void _read(Uint8List data) {
    _rcvstr += String.fromCharCodes(data);
    for (;;) {
        int n = _rcvstr.indexOf('\n');
        if (n < 0)
            break;
        String s = _rcvstr.substring(0, n);
        _rcvstr = _rcvstr.substring(n+1);
        while (s.codeUnitAt(s.length - 1) < 32)
            s = s.substring(0, s.length-1);
        
        if (s.substring(0, 4) == '### ') {
            //_event(s.substring(4));
            continue;
        }

        if (s.isEmpty || (s[0] != ':'))
            continue;
        s = s.substring(1);
        n = s.indexOf(':');

        if (n < 0) {
            final r = _rcv[s];
            if (r != null) {
                r.cmpl.complete(r.reply);
                _rcv.remove(s);
            }
            continue;
        }

        final code = s.substring(0, n);
        final r = _rcv[code];
        if (r == null)
            continue;
        
        s = s.substring(n+1);
        if (s.isEmpty) {
            r.cmpl.complete(r.reply);
            _rcv.remove(code);
        }
        else
        if (s[0] == ':') {
            r.reply.data.add(s.substring(1));
        }
        else {
            r.reply.err = s;
            r.cmpl.complete(r.reply);
            _rcv.remove(code);
        }
    }
}

Future<SerialReply> devCmd(String scmd, [List<dynamic> ?args]) async {
    if (_port == null)
        return SerialReply.e('Serial not connected');
    
    String code = Random().nextInt(0xffff).toRadixString(16);
    while (_rcv.containsKey(code))
        code = Random().nextInt(0xffff).toRadixString(16);

    final c = SerialReplyCmpl();
    _rcv[code] = c;
    
    final String str =
        ':$code:$scmd:${(args ?? []).map((a) => a.toString()).join(';')}\n';
    _port!.write(Uint8List.fromList(str.codeUnits));

    return c.cmpl.future;
}
