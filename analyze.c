#include <stdlib.h>
#include "uoocc.h"

Map *symbol_table;

static SymbolTableEntry *make_SymbolTableEntry(CType *ctype, int offset) {
  SymbolTableEntry *p = (SymbolTableEntry *)malloc(sizeof(SymbolTableEntry));
  p->ctype = ctype;
  p->offset = offset;
  return p;
}

static int sizeof_ctype(CType *ctype) {
  if (ctype->type == TYPE_INT)
    return 4;
  else if (ctype->type == TYPE_PTR)
    return 8;
  else if (ctype->type == TYPE_ARRAY) {
    return sizeof_ctype(ctype->ptrof) * ctype->array_size;
  } else
    return 0;
}

int offset_from_bp;
static int get_offset_from_bp(CType *ctype) {
  int stack_size = sizeof_ctype(ctype);
  if (stack_size == 4)
    offset_from_bp += 4;
  else if (stack_size >= 8) {
    if (offset_from_bp % 8 == 0)
      offset_from_bp += stack_size;
    else
      offset_from_bp += 8 - offset_from_bp % 8 + stack_size;
  }
  return offset_from_bp;
}

static Ast *array_to_ptr(Ast *p) {
  CType *ctype = p->ctype;
  if (ctype->type == TYPE_ARRAY) {
    Ast *new = make_ast_op(AST_OP_REF, p, NULL, NULL);
    new->ctype = make_ctype(TYPE_PTR, ctype->ptrof);
    return new;
  } else {
    return p;
  }
}

void semantic_analysis(Ast *p) {
  if (p == NULL)
    return;

  switch (p->type) {
    case AST_OP_ADD:
      semantic_analysis(p->left);
      semantic_analysis(p->right);

      p->left = array_to_ptr(p->left);
      p->right = array_to_ptr(p->right);

      if (p->left->ctype->type == TYPE_PTR && p->right->ctype->type == TYPE_PTR)
        error_with_token(p->token, "invalid operands to binary expression");
      else if (p->right->ctype->type == TYPE_PTR)
        p->ctype = p->right->ctype;
      else
        p->ctype = p->left->ctype;
      break;
    case AST_OP_SUB:
      semantic_analysis(p->left);
      semantic_analysis(p->right);

      p->left = array_to_ptr(p->left);
      p->right = array_to_ptr(p->right);

      if (p->left->ctype->type == TYPE_PTR && p->right->ctype->type == TYPE_PTR)
        p->ctype = make_ctype(TYPE_INT, NULL);
      else if (p->right->ctype->type == TYPE_PTR)
        p->ctype = p->right->ctype;
      else
        p->ctype = p->left->ctype;
      break;
    case AST_OP_MUL:
    case AST_OP_DIV:
    case AST_OP_LT:
    case AST_OP_LE:
    case AST_OP_EQUAL:
    case AST_OP_NEQUAL:
      semantic_analysis(p->left);
      semantic_analysis(p->right);
      p->ctype = p->left->ctype;
      break;
    case AST_OP_POST_INC:
    case AST_OP_POST_DEC:
    case AST_OP_PRE_INC:
    case AST_OP_PRE_DEC:
      if (p->left == NULL || p->left->type != AST_VAR)
        error_with_token(p->token, "expression is not assignable");
      semantic_analysis(p->left);
      p->left = array_to_ptr(p->left);
      p->ctype = p->left->ctype;
      break;
    case AST_OP_REF:
      semantic_analysis(p->left);
      p->ctype = make_ctype(TYPE_PTR, p->left->ctype);
      break;
    case AST_OP_DEREF:
      semantic_analysis(p->left);
      p->left = array_to_ptr(p->left);
      if (p->left->ctype->type != TYPE_PTR || p->left->ctype->ptrof == NULL)
        error_with_token(p->token, "indirection requires pointer operand");
      p->ctype = p->left->ctype->ptrof;
      break;
    case AST_OP_ASSIGN:
      semantic_analysis(p->left);
      semantic_analysis(p->right);
      p->left = array_to_ptr(p->left);
      p->right = array_to_ptr(p->right);
      if (p->left->type != AST_VAR && p->left->type != AST_OP_DEREF)
        error_with_token(p->token, "expression is not assignable");
      else if (p->left->ctype->type != p->right->ctype->type)
        error_with_token(p->token, "expression is not assignable");
      p->ctype = p->left->ctype;
      break;
    case AST_VAR:
      if (map_get(symbol_table, p->ident) == NULL)  // undefined variable.
        error_with_token(
            p->token, allocate_concat_3string("use of undeclared identifier '",
                                              p->ident, "'"));
      else
        p->ctype =
            ((SymbolTableEntry *)map_get(symbol_table, p->ident)->val)->ctype;
      break;
    case AST_DECLARATION: {
      SymbolTableEntry *_e =
          make_SymbolTableEntry(p->ctype, get_offset_from_bp(p->ctype));
      MapEntry *e = allocate_MapEntry(p->ident, _e);
      if (map_get(symbol_table, e->key) != NULL)  // already defined variable.
        error_with_token(p->token, allocate_concat_3string("redifinition of '",
                                                           p->ident, "'"));
      map_put(symbol_table, e);
      break;
    }
    case AST_CALL_FUNC:
      for (int i = p->args->size - 1; i >= 0; i--)
        semantic_analysis(vector_at(p->args, i));
      break;
    case AST_DECL_FUNC:
      symbol_table = p->symbol_table;
      offset_from_bp = 0;
      if (p->args->size > 6)
        error_with_token(p->token, "too many arguments");
      for (int i = 0; i < p->args->size; i++)
        semantic_analysis(vector_at(p->args, i));
      semantic_analysis(p->statement);
      p->offset_from_bp = offset_from_bp;
      break;
    case AST_COMPOUND_STATEMENT:
      for (int i = 0; i < p->statements->size; i++)
        semantic_analysis(vector_at(p->statements, i));
      break;
    case AST_IF_STATEMENT:
      semantic_analysis(p->cond);
      semantic_analysis(p->left);
      semantic_analysis(p->right);
      break;
    case AST_FOR_STATEMENT:
      semantic_analysis(p->init);
      semantic_analysis(p->step);
    case AST_WHILE_STATEMENT:
      semantic_analysis(p->cond);
      semantic_analysis(p->statement);
      break;
  }

  return;
}
