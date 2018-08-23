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

int test_number() {
  expect(0, 0);
  expect(2, 2);
  expect(9, 9);
  return 0;
}

int test_additive_expr() {
  expect(1 + 2, 3);
  expect(2 - 1, 1);
  expect(1 + 2 + 3, 6);
  expect(3 - 2 - 1, 0);
  expect(5 + 6 - 3, 8);
  expect(3 + 5 - 3 + 6, 11);

  int x;
  x = 0 - 999;
  expect(1000 + x, 1);
  return 0;
}

int test_multiplicative_expr() {
  expect(2 * 3, 6);
  expect(4 / 2, 2);
  expect(1 * 2 + 3, 5);
  expect(1 + 2 * 3, 7);
  expect(1 / 2 + 3, 3);
  expect(1 + 2 / 3, 1);
  expect(1 * 2 + 3 + 4 * 5 * 6 / 7, 22);
  return 0;
}

int test_primary_expr() {
  expect(6 - (5 - 4), 5);
  expect((1 + 2) * 3, 9);
  expect((1 + 2) * 3 + (4 + 5 + 6) * 2, 39);
  return 0;
}

int test_unary_expr() {
  int a;
  a = 1;
  expect(++a, 2);
  ++a;
  expect(a, 3);
  expect(--a, 2);
  --a;
  expect(a, 1);

  a = 0;
  expect(~a, 0 - 1);
  expect(~~a, 0);

  expect(!0, 1);
  expect(!1, 0);

  expect(sizeof(int), 4);
  expect(sizeof(char), 1);
  expect(sizeof(int *), 8);
  expect(sizeof(char *), 8);

  int arr[25][30][5];
  int *p;

  p = &arr[5][9][2];

  expect(sizeof(arr), 15000);
  expect(sizeof arr[10], 600);
  expect(sizeof arr[5][20], 20);
  expect(sizeof arr[5][20][1], 4);
  expect(sizeof p, 8);
  expect(sizeof *p, 4);

  return 0;
}

int test_postfix_expr() {
  int a;
  a = 1;
  expect(a++, 1);
  expect(a, 2);
  expect(a--, 2);
  expect(a, 1);
  return 0;
}

int test_relational_expr() {
  expect(3 < 5, 1);
  expect(5 < 5, 0);
  expect(3 <= 5, 1);
  expect(5 <= 5, 1);
  expect(5 <= 3, 0);
  expect(5 > 3, 1);
  expect(5 > 5, 0);
  expect(5 >= 3, 1);
  expect(5 >= 5, 1);
  expect(3 >= 5, 0);
  return 0;
}

int test_equality_expr() {
  expect(25 == 25, 1);
  expect(10 + 5 == 3 * 5, 1);
  expect(23 == 39, 0);
  return 0;
}

int test_bitwise_expr() {
  expect(11 & 11, 11);
  expect(11 | 11, 11);
  expect(11 ^ 11, 0);
  expect(5 & 10, 0);
  expect(5 | 10, 15);
  expect(5 ^ 10, 15);
  return 0;
}

int test_logical_expr() {
  expect(1 && 1, 1);
  expect(1 && 0, 0);
  expect(0 && 0, 0);
  expect(1 || 1, 1);
  expect(1 || 0, 1);
  expect(0 || 0, 0);
  return 0;
}

int test_additive_ptr() {
  int a[4];
  int *p;
  p = a;
  a[0] = 1;
  a[1] = 2;
  a[2] = 4;
  a[3] = 8;

  expect(*p, 1);
  expect(*(p + 2), 4);
  expect(*(p + 3), 8);
  p = p + 3;
  expect(*(p - 2), 2);
  expect(p - a, 3);
  return 0;
}

int swap(int *p, int *q) {
  int tmp;
  tmp = *p;
  *p = *q;
  *q = tmp;
  return 0;
}

int test_unary_ptr() {
  int a[4];
  int *p;
  p = a;
  a[0] = 1;
  a[1] = 2;
  a[2] = 4;
  a[3] = 8;

  expect(*(++p), 2);
  p = a + 3;
  expect(*(--p), 4);

  int x;
  int y;
  x = 5;
  y = 9;

  p = &x;
  expect(*p, 5);
  *p = 15;
  expect(*p, 15);
  expect(x, 15);

  swap(&x, &y);
  expect(x, 9);
  expect(y, 15);
  return 0;
}

int test_postfix_ptr() {
  int a[4];
  int *p;
  p = a;
  a[0] = 1;
  a[1] = 2;
  a[2] = 4;
  a[3] = 8;

  expect(*(p++), 1);
  expect(*p, 2);
  p = a + 3;
  expect(*(p--), 8);
  expect(*p, 4);
  return 0;
}

int main() {
  printf("Testing expression ...\n");

  test_number();
  test_additive_expr();
  test_multiplicative_expr();
  test_primary_expr();
  test_unary_expr();
  test_postfix_expr();
  test_relational_expr();
  test_equality_expr();
  test_bitwise_expr();
  test_logical_expr();
  test_additive_ptr();
  test_unary_ptr();

  printf("OK!\n");

  return 0;
}
