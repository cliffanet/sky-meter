import '../jump/bind.dart';

class TraceItem {
    final int alt;
    final int ? clc;
    final int ? chg;
    final JumpInf ?inf;
    const TraceItem({ required this.alt, required this.inf, int ? clc, int ? chg }) :
        clc = (clc != null) && (clc > 0x20) ? clc : 0,
        chg = (chg != null) && (chg > 0x20) ? chg : 0;
}

