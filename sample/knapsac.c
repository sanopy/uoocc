int N;
int W;
int v[128];
int w[128];
int memo[128][10001];

int solve(int i, int weight) {
  if (i == N && weight <= W)
    return 0;
  else if (i >= N || weight > W)
    return 0 - 99999;
  else if (memo[i][weight] != 0)
    return memo[i][weight];
  else {
    int a;
    int b;
    a = solve(i + 1, weight);
    b = solve(i + 1, weight + w[i]) + v[i];
    if (a < b)
      return memo[i][weight] = b;
    else
      return memo[i][weight] = a;
  }
}

int main() {
  int i;
  scanf("%d %d", &N, &W);
  for (i = 0; i < N; i++)
    scanf("%d %d", v + i, w + i);
  printf("%d\n", solve(0, 0));

  return 0;
}
