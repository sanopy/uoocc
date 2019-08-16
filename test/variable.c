/**
 * @file variable.c
 * @author uoo38
 */

int cnt;
int expect(int a, int b) {
  if (a != b) {
    printf("Test %d: Failed\n", cnt++);
    printf("  %d expected, but got %d\n", b, a);
    exit(1);
  } else
    printf("Test %d: Passed\n", cnt++);
  return 0;
}

int expect_char(char a, char b) {
  if (a != b) {
    printf("Test %d: Failed\n", cnt++);
    printf("  %d expected, but got %d\n", b, a);
    exit(1);
  } else
    printf("Test %d: Passed\n", cnt++);
  return 0;
}

int expect_ptr(int *a, int *b) {
  if (a != b) {
    printf("Test %d: Failed\n", cnt++);
    printf("  %p expected, but got %p\n", b, a);
    exit(1);
  } else
    printf("Test %d: Passed\n", cnt++);
  return 0;
}

void test_int_var() {
  int a;
  int b;
  int c;
  a = 5;
  b = a + 6;
  c = b * 2;
  expect(a, 5);
  expect(b, 11);
  expect(a + b, 16);
  expect(c, 22);
  expect(a * 2 + b + c, 43);
  return;
}

void test_char_var() {
  char x;
  char y;
  char z[3];
  x = 1;
  y = 2;
  z[0] = 1;
  expect_char(x, 1);
  expect_char(y, 2);
  expect_char(z[0], 1);
  expect_char(z[0] + y, 3);
  return;
}

int e[4];
void test_array() {
  int a[2];
  int b[2][2];

  *a = 1;
  *(a + 1) = 2;
  **b = 3;
  *(*(b + 0) + 1) = 4;
  *(*(b + 1) + 1) = 5;

  expect(*a, 1);
  expect(*(a + 1), 2);
  expect(**b, 3);
  expect(*(*(b + 0) + 1), 4);
  expect(*(*(b + 1) + 1), b[1][1]);
  expect(*(*(b + 1) + 1), 5);

  int *p;
  p = a;

  expect(*p + *(p + 1), 3);

  int c[2];
  int d[2][2];

  expect_ptr(d[0], *d);
  expect_ptr(d[1], *(d + 1));
  expect_ptr(&d[1][1], *(d + 1) + 1);

  c[0] = 1;
  c[1] = 2;
  d[0][0] = 1;
  d[0][1] = 2;
  d[1][0] = 3;

  expect(c != &d[1][0], 1);

  expect(c[0], 1);
  expect(c[1], 2);
  expect(c[0] + c[1], 3);
  expect(d[0][0] + d[0][1] * d[1][0], 7);
  expect(1 [c], 2);

  int f[4];
  p = f;
  expect_ptr(f, p);

  p = e;
  expect_ptr(e, p);

  return;
}

void test_string() {
  char *s;
  s = "abc";

  expect_char(s[0], 97);
  expect_char(s[0], 97);
  return;
}

int x;
int a[20];

int inc() { return ++x; }

void assign(void) {
  a[5] = 5;
  return;
}

void test_global_var() {
  x = 1;
  expect(x, 1);
  inc();
  expect(x, 2);
  assign();
  expect(a[5], 5);
  return;
}

enum { ENUM_A, ENUM_B, ENUM_C, ENUM_D, ENUM_E };

void test_enum() {
  enum {
    TYPE_A,
    TYPE_B,
    TYPE_C,
    TYPE_D,
  };

  expect(ENUM_A, 0);
  expect(ENUM_B, 1);
  expect(ENUM_C, 2);
  expect(ENUM_D, 3);
  expect(ENUM_E, 4);
  expect(TYPE_A, 0);
  expect(TYPE_B, 1);
  expect(TYPE_C, 2);
  expect(TYPE_D, 3);

  return;
}

void test_struct_sizeof() {
  expect(sizeof(struct {
           int a;
           char *b;
           int c;
           char d;
           char *e;
         }),
         32);
  expect(sizeof(struct {
           int a;
           char b;
           int c;
         }),
         12);
  expect(sizeof(struct {
           int a;
           int b[3];
           char *c;
         }),
         24);

  return;
}

struct _A {
  int a;
  char b;
  int c;
  char d;
} A;

void test_struct() {
  struct _B {
    int a;
    char b;
    int *p;
    int c;
    char d;
  } B;
  A.a = 0;
  A.b = 60;
  A.c = 1;
  A.d = 61;
  B.a = 2;
  B.b = 62;
  B.c = 3;
  B.d = 63;
  B.p = &(A.c);
  expect(A.a, 0);
  expect(A.c, 1);
  expect(B.a, 2);
  expect(B.c, 3);
  expect_char(A.b, 60);
  expect_char(A.d, 61);
  expect_char(B.b, 62);
  expect_char(B.d, 63);

  *(B.p) = 5;
  expect(A.c, 5);

  struct _A *p;

  p = &A;
  expect((*p).a, 0);
  (*p).a = 15;
  expect(A.a, 15);

  expect(p->a, 15);
  p->a = 16;
  expect(A.a, 16);
  expect(p->a, 16);
  p->b = 20;
  expect_char(p->b, 20);
  expect_char(A.b, 20);
  (*p).c = 25;
  expect(p->c, 25);
  expect(A.c, 25);
  A.d = 22;
  expect_char(A.d, 22);
  expect_char(p->d, 22);

  return;
}

void test_typedef() {
  typedef int T;
  T x;
  x = 99;
  expect(x, 99);

  typedef struct {
    int a;
    int b;
    int c;
  } S;

  S s;
  s.a = 77;
  s.b = 78;
  s.c = 79;
  expect(s.a, 77);
  expect(s.b, 78);
  expect(s.c, 79);

  return;
}

int main() {
  printf("Testing variable ...\n");

  test_int_var();
  test_char_var();
  test_array();
  test_string();
  test_global_var();
  test_enum();
  test_struct_sizeof();
  test_struct();
  test_typedef();

  printf("OK!\n");

  return 0;
}
