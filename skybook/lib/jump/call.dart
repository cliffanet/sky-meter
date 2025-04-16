import 'bind.dart';

import 'dart:io';
import 'dart:ffi' as ffi;

const String _libName = 'jump';

final ffi.DynamicLibrary _dylib = () {
  if (Platform.isMacOS || Platform.isIOS) {
    //return ffi.DynamicLibrary.open('$_libName.framework/$_libName');
    return ffi.DynamicLibrary.process();
  }
  if (Platform.isAndroid || Platform.isLinux) {
    return ffi.DynamicLibrary.open('lib$_libName.so');
  }
  if (Platform.isWindows) {
    return ffi.DynamicLibrary.open('$_libName.dll');
  }
  throw UnsupportedError('Unknown platform: ${Platform.operatingSystem}');
}();

/// The bindings to the native functions in [_dylib].
final jump = JumpCaller(_dylib);
