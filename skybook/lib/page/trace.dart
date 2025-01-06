import 'package:flutter/material.dart';

class PageTrace extends StatelessWidget {
    const PageTrace({ super.key });
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
                    title: const Text('Трассировка'),
                ),
            body: Center(
                child: Text('test')
            )
        );
    }
}
