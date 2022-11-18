#define func0(b, c, d, e, f) b() { return c(d, e, f) }
#define func1(a, b, c) 1 + a(b, c)
#define func2(a, b) a * b;
int func0(MyFuncName, func1, func2, 3, 5)