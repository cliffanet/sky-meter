import 'package:flutter/material.dart';
import './page/main.dart';

void main() {
    runApp(MaterialApp(
        title: 'SkyBook',
        home: PageMain(),
        debugShowCheckedModeBanner: false,
        theme: ThemeData(
            useMaterial3: false,
        ),
    ));
}
