import 'package:flutter/material.dart';
import 'package:flutter_libserialport/flutter_libserialport.dart';
import 'logbook.dart';

extension IntToString on int {
    String toTransport() {
        switch (this) {
        case SerialPortTransport.usb:
            return 'USB';
        case SerialPortTransport.bluetooth:
            return 'Bluetooth';
        case SerialPortTransport.native:
            return 'Native';
        default:
            return 'Unknown';
        }
    }
}

class PageDevSelect extends StatelessWidget {
    final List<String> all = [];
    final chg = ValueNotifier<int>(0);
    PageDevSelect({super.key}) { reload(); }

    void reload() {
        all.clear();
        all.addAll(SerialPort.availablePorts);
        all.sort();
        chg.value ++;
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
                    title: Text('Выбор подключения'),
                ),
            body: ValueListenableBuilder(
                valueListenable: chg,
                builder: (context, value, child) {
                    return ListView.separated(
                        separatorBuilder: (context, index) => const Divider(),
                        itemCount: all.length,
                        itemBuilder: (context, index) {
                            final address = all[index];
                            final port = SerialPort(address);
                            return ListTile(
                                title: Text(port.description ?? 'unknown'),
                                subtitle: Text('[${port.transport.toTransport()}] $address'),
                                onTap: () =>
                                    Navigator.push(
                                        context,
                                        MaterialPageRoute(builder: (context) => PageLogBook.byDev(address)),
                                    ),
                            );
                        },
                    );
                }
            ),
            floatingActionButton: FloatingActionButton(
                onPressed: reload,
                mini: true,
                heroTag: null,
                child: const Icon(Icons.refresh)
            )
        );
    }
}
