int cnt;
int expect(int a, int b) {
  if (a != b) {
    printf("Test %d: Failed\n", cnt++);
    printf("  %d expected, but got %d\n", b, a);
    exit(1);
  } else
    printf("Test %d: Passed\n", cnt++);
}

int f1() {
  if (1 == 2)
    1;
  else
    2;
}

int fact(int n) {
  if (n <= 1)
    1;
  else
    n* fact(n - 1);
}

int fib(int n) {
  if (n <= 0)
    0;
  else if (n == 1)
    1;
  else
    fib(n - 1) + fib(n - 2);
}

int test_if() {
  expect(f1(), 2);
  expect(fact(5), 120);
  expect(fib(8), 21);
}

int f2() {
  int i;
  i = 1;
  while (i < 10)
    i++;
  i;
}

int sum1() {
  int i;
  int sum;
  sum = 0;
  i = 1;
  while (i <= 10)
    sum = sum + i++;
  sum;
}

int test_while() {
  expect(f2(), 10);
  expect(sum1(), 55);
}

int f3() {
  int i;
  for (i = 0; i <= 10; i++)
    0;
  i;
}

int sum2() {
  int i;
  int sum;
  sum = 0;
  for (i = 1; i <= 10; i++)
    sum = sum + i;
  sum;
}

int test_for() {
  expect(f3(), 11);
  expect(sum2(), 55);
}

int main() {
  printf("Testing statement ...\n");

  test_if();
  test_while();
  test_for();

  printf("OK!\n");

  0;
}
