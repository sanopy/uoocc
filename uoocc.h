// vector.c
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

// map.c
typedef struct {
  char *key;
  void *val;
} MapEntry;

typedef struct {
  Vector *vec;
  int size;
} Map;

extern Map *symbol_table;

MapEntry *allocate_MapEntry(char *, void *);
Map *map_new(void);
int map_put(Map *, MapEntry *);
MapEntry *map_get(Map *, char *);

// mylib.c
char *allocate_string(char *);
int *allocate_integer(int);
