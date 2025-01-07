class TraceItem {
    final int alt;
    final int ? clc;
    final int ? chg;
    const TraceItem({ required this.alt, int ? clc, int ? chg }) :
        clc = (clc != null) && (clc > 0x20) ? clc : 0,
        chg = (chg != null) && (chg > 0x20) ? chg : 0;
}
