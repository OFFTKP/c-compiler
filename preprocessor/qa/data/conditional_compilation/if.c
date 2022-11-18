#define __TEST
#define __TEST2
#if defined(__TEST) || (defined __NOTEST && (defined __TEST2 && defined(TEST)))
#define __TEST_PASSED
#endif