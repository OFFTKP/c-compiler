#define func(a, b, str) int a() { b; printf(str); }
func(main, printf("Hello world"), "Hello again")