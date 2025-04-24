import 'package:flutter/material.dart';
import 'package:file_picker/file_picker.dart';
import 'logbook.dart';
import 'trace.dart';
import 'devselect.dart';

//import 'dart:developer' as developer;

class PageMain extends StatelessWidget {
    const PageMain({ super.key });
    @override
    Widget build(BuildContext context) {
        final st = ElevatedButton.styleFrom(
            minimumSize: const Size(250, 40)
        );
        return Scaffold(
            body: Center(
                child:  // this row has full width
                    Column(
                        spacing: 10,
                        mainAxisAlignment: MainAxisAlignment.center,
                        crossAxisAlignment: CrossAxisAlignment.center,
                        children: [
                            ElevatedButton(
                                style: st,
                                child: Text('Трассировка из файла'),
                                onPressed: () async {
                                    final r = await FilePicker.platform.pickFiles(
                                        dialogTitle: 'Открыть файл трейса',
                                        type: FileType.custom,
                                        allowedExtensions: ['csv']
                                    );
                                    if ((r == null) || (r.count != 1))
                                        return;
                                    final fname = r.paths[0] ?? '';
                                    Navigator.push(
                                        context,
                                        MaterialPageRoute(builder: (context) => PageTrace.byFile(fname: fname)),
                                    );
                                },
                            ),
                            ElevatedButton(
                                style: st,
                                child: Text('Логбук из папки'),
                                onPressed: () async {
                                    final dir = await FilePicker.platform.getDirectoryPath(
                                        dialogTitle: 'Открыть папку с логбуком',
                                    );
                                    if ((dir == null) || dir.isEmpty)
                                        return;
                                    Navigator.push(
                                        context,
                                        MaterialPageRoute(builder: (context) => PageLogBook.byDir(dir)),
                                    );
                                },
                            ),
                            ElevatedButton(
                                style: st,
                                child: Text('Логбук с устройства'),
                                onPressed: () {
                                    Navigator.push(
                                        context,
                                        MaterialPageRoute(builder: (context) => PageDevSelect()),
                                    );
                                },
                            )
                        ]
                    )
            )
        );
    }
}
