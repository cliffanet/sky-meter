import 'package:flutter/material.dart';
import 'package:file_picker/file_picker.dart';
import 'trace.dart';

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
                                child: Text('btn2'),
                                onPressed: () {},
                            )
                        ]
                    )
            )
        );
    }
}
