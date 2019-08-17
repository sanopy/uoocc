#define NULL (0)

typedef struct _Node {
  int data;
  struct _Node *next;
} Node;

void *malloc(int);

Node *new_node(int data) {
  Node *p;
  p = malloc(sizeof(Node));
  p->data = data;
  p->next = NULL;
  return p;
}

void print_list(Node *p) {
  printf(" %d ", p->data);
  if (p->next == NULL)
    printf("\n");
  else {
    printf("->");
    print_list(p->next);
  }

  return;
}

int main(void) {
  int i;
  Node *list;
  Node *p;

  p = list = new_node(0);
  for (i = 1; i <= 10; i++) {
    p->next = new_node(i);
    p = p->next;
  }

  print_list(list);

  return 0;
}
