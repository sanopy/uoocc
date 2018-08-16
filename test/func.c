int cnt;
int expect(int a, int b) {
  if (a != b) {
    printf("Test %d: Failed\n", cnt++);
    printf("  %d expected, but got %d\n", b, a);
    exit(1);
  } else
    printf("Test %d: Passed\n", cnt++);
}

int return_seven() { 7; }

int add_three_args(int x, int y, int z) { x + y + z; }

int test_func() {
  expect(return_seven(), 7);
  expect(return_seven() * 2 + 5, 19);
  expect(add_three_args(1, 2, 3), 6);
  expect(3 * 6 / 2, 9);
  expect(add_three_args(1, 2, 3), 6);
  expect(3 * add_three_args(1, 2, 3), 18);
  expect(add_three_args(1, 2, 3) / 2, 3);
  expect(3 * add_three_args(1, 2, 3) / 2, 9);
}

int main() {
  printf("Testing function ...\n");

  test_func();

  printf("OK!\n");

  0;
}
