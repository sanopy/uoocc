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

int test_int_var() {
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
  return 0;
}

int test_char_var() {
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
  return 0;
}

int test_array() {
  int a[2];
  int b[2][2];

  *a = 1;
  *(a + 1) = 2;
  **b = 3;
  *(*(b + 0) + 1) = 4;

  expect(*a, 1);
  expect(*(a + 1), 2);
  expect(**b, 3);
  expect(*(*(b + 0) + 1), 4);

  int *p;
  p = a;

  expect(*p + *(p + 1), 3);

  int c[2];
  int d[2][2];

  c[0] = 1;
  c[1] = 2;
  d[0][0] = 1;
  d[0][1] = 2;
  d[1][0] = 3;

  expect(c[0], 1);
  expect(c[1], 2);
  expect(c[0] + c[1], 3);
  expect(d[0][0] + d[0][1] * d[1][0], 7);
  expect(1 [c], 2);
  return 0;
}

int test_string() {
  char *s;
  s = "abc";

  expect_char(s[0], 97);
  expect_char(s[0], 97);
  return 0;
}

int x;
int a[20];

int inc() { return ++x; }

int assign() {
  a[5] = 5;
  return 0;
}

int test_global_var() {
  x = 1;
  expect(x, 1);
  inc();
  expect(x, 2);
  assign();
  expect(a[5], 5);
  return 0;
}

int main() {
  printf("Testing variable ...\n");

  test_int_var();
  test_char_var();
  test_array();
  test_string();
  test_global_var();

  printf("OK!\n");

  return 0;
}
