
class CsvParser {
    String _s;
    int _skip;

    CsvParser(String s) : _s = s, _skip = 0;

    int get skipped => _skip;
    String get s => _s;
    int get first => at(0);
    int at(int index) => (index >= 0) && (index < _s.length) ? _s.codeUnitAt(index) : 0;
    int operator[](int index) => at(index);
    bool get isNotEmpty => _s.isNotEmpty;

    void cut(int count) {
        _s = _s.substring(count);
        _skip += count;
    }

    bool _isSpace(int i) {
        final f = at(i);
        return (f == 0x20) || (f == 0x09);
    }
    int skipSpace() {
        int n = 0;
        while (_isSpace(n))
            n++;
        if (n > 0)
            cut(n);
        return n;
    }

    bool _isDelim(int i) {
        final d = at(i);
        return (d == 0x2c) || (d == 0x3a) || (d == 0x3b);
    }
    bool skipDelim() {
        final ok = _isDelim(0);
        if (ok) cut(1);
        return ok;
    }

    String ? strQuoted() {
        final q = first;
        int n = 1;
        List<int> r = [];

        while (n < _s.length) {
            final c = at(n);
            if (c == q) {
                n ++;
                break;
            }
            else
            if (c == 0x5c) {
                n++;
                if (n >= _s.length)
                    return null;
                r.add(at(n));
                n++;
            }
            else {
                r.add(c);
                n++;
            }
        }

        if ((n < 2) || (at(n-1) != q))
            return null;
        
        cut(n);
        skipDelim();
        
        return String.fromCharCodes(r);
    }

    String ? strNative() {
        if (_s.isEmpty)
            return null;
        
        int n = 0;
        while ((n < _s.length) && !_isDelim(n))
            n++;
        
        final r = _s.substring(0, n);

        cut(n);
        skipDelim();

        return r;
    }

    String ? fetch() {
        skipSpace();
        switch (first) {
            case 0x22:
            case 0x27:
                return strQuoted();
        }
        return strNative();
    }
}

List<String> bycsv(String src) {
    final s = CsvParser(src);
    final r = <String>[];

    while (s.isNotEmpty) {
        final a = s.fetch();
        if (a == null)
            break;
        r.add(a);
    }

    return r;
}
