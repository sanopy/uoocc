typedef struct {
  void **data;
  int size;
  int reserved_size;
} Vector;

#define VECTOR_BLOCK_SIZE 4

Vector *vector_new(void);
int vector_realloc(Vector *, int);
int vector_push_back(Vector *, void *);
void *vector_at(Vector *, int);
