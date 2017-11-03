
#include <iostream>
#include <assert.h>
#include "protocol.h"

struct Msg : public MsgBase {
    int    a;
    Msg() : a(0) {}

    CLASS_ENCODE(s << a)
    CLASS_DECODE(s >> a)
};

struct MyMsg : public MsgBase {
    Msg     m[10];
    double  d;
    MyMsg() : d(0.0) {}

    CLASS_ENCODE(s << m << d)
    CLASS_DECODE(s >> m >> d)
};

enum E1 {
    E1_OK  = 1,
    E1_ERR
};

enum class E2 {
    E2_OK  = 10,
    E2_ERR
};

int main() {
    Stream    stream;
    string    s;
    stream << 0xfeeffeef;
    s = stream.str();
    assert(s.size() == 5);
    assert((s[4] & 0xff) == 0x6f && (s[3] & 0xff) == 0xfd && (s[2] & 0xff) == 0xbf && (s[1] & 0xff) == 0xf7 && (s[0] & 0xff) == 0x8f);
    uint32_t  i = 0;
    stream >> i;
    assert(i == 0xfeeffeef);
    //stream.Clear();

    stream << 0;
    s = stream.str();
    assert(s.size() == 1);
    assert((s[0] & 0xff) == 0);
    stream >> i;
    assert(i == 0);
    //stream.Clear();
    
    stream << 0xfeeffeeffeeffeef;
    s = stream.str();
    assert(s.size() == 10);
    assert((s[9] & 0xff) == 0x6f && (s[8] & 0xff) == 0xfd && (s[7] & 0xff) == 0xbf && (s[6] & 0xff) == 0xf7 && (s[5] & 0xff) == 0xff
        && (s[4] & 0xff) == 0xdd && (s[3] & 0xff) == 0xff && (s[2] & 0xff) == 0xf7 && (s[1] & 0xff) == 0xfe && (s[0] & 0xff) == 0x81);
    uint64_t   u = 0;
    stream >> u;
    assert(u == 0xfeeffeeffeeffeef);
    //stream.Clear();

    stream << 0.11;
    s = stream.str();
    assert(s.size() == sizeof(double));
    assert(*(double *)s.c_str() == 0.11);
    double d = 0.0;
    stream >> d;
    assert(d == 0.11);
    //stream.Clear();

    stream << "aaabbccc";
    s = stream.str();
    assert((int)s[0] == 8);
    assert(s.substr(1) == "aaabbccc");
    string   ss;
    stream >> ss;
    assert(ss == "aaabbccc");
    //stream.Clear();

    string  in = "12345555";
    stream << in;
    s = stream.str();
    assert((int)s[0] == 8);
    assert(s.substr(1) == in);
    string   out;
    stream >> out;
    assert(out == in);

    stream << E1_ERR;
    E1    e1;
    stream >> e1;
    assert(e1 == E1_ERR);

    stream << E2::E2_OK;
    E2    e2;
    stream >> e2;
    assert(e2 == E2::E2_OK);

    int   a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
    stream << a;
    int   b[10] = {0};
    stream >> b;
    assert(memcmp(a, b, sizeof(a)) == 0);

    int   aa[10][3] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0,11, 12, 13, 14, 15, 16, 17, 18, 19, 10,21, 22, 23, 24, 25, 26, 27, 28, 29, 20};
    stream << aa;
    int   bb[10][3] = {0};
    stream >> bb;
    assert(memcmp(aa, bb, sizeof(aa)) == 0);

    
    MyMsg    m, t;
    for (int i = 0; i < 10; i++) {
        m.m[i].a = i;
    }
    m.d = 7.88;
    stream << m;
    stream >> t;
    assert(m.d == t.d);
    for (int i = 0; i < 10; i++) {
        assert(m.m[i].a == t.m[i].a);
    }

    vector<string>  v = {"123", "456", "7890"};
    vector<string>  r;
    stream << v;
    stream >> r;
    assert(v == r);

    map<string, int>  mm;
    mm.insert(make_pair("123", 1));
    map<string, int>  mr;
    stream << mm;
    stream >> mr;
    assert(mm == mr);
    
    return 0;
}

