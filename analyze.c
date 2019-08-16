#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "uoocc.h"

Map *symbol_table;

static SymbolTableEntry *make_SymbolTableEntry(CType *ctype, int is_global) {
  SymbolTableEntry *p = malloc(sizeof(SymbolTableEntry));
  p->ctype = ctype;
  p->is_global = is_global;
  p->is_constant = 0;
  return p;
}

static SymbolTableEntry *symboltable_get(Map *table, char *key) {
  MapEntry *e = map_get(table, key);
  if (e == NULL) {
    if (table->next == NULL)
      return NULL;
    else
      return symboltable_get(table->next, key);
  } else {
    return e->val;
  }
}

static int max_size(CType *ctype) {
  int ret = 0;
  if (ctype->type == TYPE_CHAR)
    ret = 1;
  else if (ctype->type == TYPE_INT)
    ret = 4;
  else if (ctype->type == TYPE_PTR)
    ret = 8;
  else if (ctype->type == TYPE_ARRAY)
    ret = max_size(ctype->ptrof);
  else if (ctype->type == TYPE_STRUCT) {
    for (int i = 0; i < ctype->struct_decl->size; i++) {
      int s =
          max_size(((StructMember *)vector_at(ctype->struct_decl, i))->ctype);
      if (ret < s)
        ret = s;
    }
  }
  return ret;
}

static int calc_offset(CType *ctype, int now_offset) {
  int max = max_size(ctype);
  int mod = now_offset % max;
  return mod == 0 ? now_offset : now_offset + max - mod;
}

int sizeof_ctype(CType *ctype) {
  if (ctype->type == TYPE_INT)
    return 4;
  else if (ctype->type == TYPE_CHAR)
    return 1;
  else if (ctype->type == TYPE_PTR)
    return 8;
  else if (ctype->type == TYPE_ARRAY)
    return sizeof_ctype(ctype->ptrof) * ctype->array_size;
  else if (ctype->type == TYPE_STRUCT) {
    Vector *list = ctype->struct_decl;
    int now_offset = 0;
    for (int i = 0; i < list->size; i++) {
      StructMember *p = vector_at(list, i);
      p->offset = calc_offset(p->ctype, now_offset);
      now_offset = p->offset + sizeof_ctype(p->ctype);
    }
    int mod = now_offset % max_size(ctype);
    return mod == 0 ? now_offset : now_offset + max_size(ctype) - mod;
  } else
    assert(0);
}

int offset_from_bp;
static int get_offset_from_bp(CType *ctype) {
  int stack_size = sizeof_ctype(ctype);
  if (stack_size == 1)
    offset_from_bp += 1;
  else if (stack_size <= 4 && offset_from_bp % 4 == 0)
    offset_from_bp += stack_size;
  else if (stack_size <= 4 &&
           offset_from_bp / 4 == (offset_from_bp + stack_size) / 4)
    offset_from_bp += stack_size;
  else if (stack_size <= 4)
    offset_from_bp += 4 - (offset_from_bp % 4) + stack_size;
  else if (offset_from_bp % 8 == 0)
    offset_from_bp += stack_size;
  else
    offset_from_bp += 8 - (offset_from_bp % 8) + stack_size;
  return offset_from_bp;
}

static Ast *array_to_ptr(Ast *p) {
  CType *ctype = p->ctype;
  if (ctype->type == TYPE_ARRAY) {
    Ast *new = make_ast_op(AST_OP_REF, p, NULL, NULL);
    return semantic_analysis(new);
  } else {
    return p;
  }
}

static CType *char_to_int(CType *ctype) {
  if (ctype->type == TYPE_CHAR)
    return make_ctype(TYPE_INT, NULL);
  else
    return ctype;
}

static CType *update_ctype(CType *ctype, Token *token) {
  if (ctype->type == TYPE_ARRAY || ctype->type == TYPE_PTR) {
    ctype->ptrof = update_ctype(ctype->ptrof, token);
    return ctype;
  }

  if (ctype->struct_tag != NULL) {
    if (ctype->struct_decl == NULL) {
      // load ctype from tag name
      SymbolTableEntry *e = symboltable_get(symbol_table, ctype->struct_tag);
      return e->ctype;
    } else {
      // register ctype with tag name
      SymbolTableEntry *_e = make_SymbolTableEntry(ctype, 0);
      _e->is_struct_tag = 1;
      _e->ident = ctype->struct_tag;
      MapEntry *e = allocate_MapEntry(_e->ident, _e);
      if (map_get(symbol_table, e->key) !=
          NULL)  // already defined tag or variable.
        error_with_token(token,
                         allocate_concat_3string("redefinition of '",
                                                 ctype->struct_tag, "'"));
      map_put(symbol_table, e);
    }
  }
  return ctype;
}

Ast *semantic_analysis(Ast *p) {
  if (p == NULL)
    return NULL;

  switch (p->type) {
    case AST_OP_ADD:
      p->left = semantic_analysis(p->left);
      p->right = semantic_analysis(p->right);

      p->left = array_to_ptr(p->left);
      p->right = array_to_ptr(p->right);

      if (p->left->ctype->type == TYPE_PTR && p->right->ctype->type == TYPE_PTR)
        error_with_token(p->token, "invalid operands to binary expression");
      else if (p->right->ctype->type == TYPE_PTR)
        p->ctype = p->right->ctype;
      else
        p->ctype = char_to_int(p->left->ctype);
      break;
    case AST_OP_SUB:
      p->left = semantic_analysis(p->left);
      p->right = semantic_analysis(p->right);

      p->left = array_to_ptr(p->left);
      p->right = array_to_ptr(p->right);

      if (p->left->ctype->type == TYPE_PTR && p->right->ctype->type == TYPE_PTR)
        p->ctype = make_ctype(TYPE_INT, NULL);
      else if (p->right->ctype->type == TYPE_PTR)
        p->ctype = p->right->ctype;
      else
        p->ctype = char_to_int(p->left->ctype);
      break;
    case AST_OP_MUL:
    case AST_OP_DIV:
    case AST_OP_LT:
    case AST_OP_LE:
    case AST_OP_EQUAL:
    case AST_OP_NEQUAL:
    case AST_OP_B_AND:
    case AST_OP_B_XOR:
    case AST_OP_B_OR:
    case AST_OP_LSHIFT:
    case AST_OP_RSHIFT:
    case AST_OP_L_AND:
    case AST_OP_L_OR:
      p->left = semantic_analysis(p->left);
      p->right = semantic_analysis(p->right);
      p->left = array_to_ptr(p->left);
      p->right = array_to_ptr(p->right);
      p->ctype = char_to_int(p->left->ctype);
      break;
    case AST_OP_B_NOT:
    case AST_OP_L_NOT:
      p->left = semantic_analysis(p->left);
      p->left = array_to_ptr(p->left);
      p->ctype = char_to_int(p->left->ctype);
      break;
    case AST_OP_POST_INC:
    case AST_OP_POST_DEC:
    case AST_OP_PRE_INC:
    case AST_OP_PRE_DEC:
      if (p->left == NULL || p->left->type != AST_VAR)
        error_with_token(p->token, "expression is not assignable");
      p->left = semantic_analysis(p->left);
      p->left = array_to_ptr(p->left);
      p->ctype = p->left->ctype;
      break;
    case AST_OP_REF:
      p->left = semantic_analysis(p->left);
      if (p->left->ctype->type == TYPE_ARRAY)
        p->ctype = make_ctype(TYPE_PTR, p->left->ctype->ptrof);
      else
        p->ctype = make_ctype(TYPE_PTR, p->left->ctype);
      break;
    case AST_OP_DEREF:
      p->left = semantic_analysis(p->left);
      p->left = array_to_ptr(p->left);
      if (p->left->ctype->type != TYPE_PTR || p->left->ctype->ptrof == NULL)
        error_with_token(p->token, "indirection requires pointer operand");
      p->ctype = p->left->ctype->ptrof;
      sizeof_ctype(p->ctype);  // to calc struct offset
      break;
    case AST_OP_ASSIGN:
      p->left = semantic_analysis(p->left);
      p->right = semantic_analysis(p->right);
      p->left = array_to_ptr(p->left);
      p->right = array_to_ptr(p->right);
      if (p->left->ctype->type == TYPE_CHAR &&
          p->right->ctype->type == TYPE_INT)
        ;
      else if (p->left->type != AST_VAR && p->left->type != AST_OP_DEREF &&
               p->left->type != AST_OP_DOT)
        error_with_token(p->token, "expression is not assignable");
      else if (p->left->ctype->type != p->right->ctype->type)
        error_with_token(p->token, "expression is not assignable");
      p->ctype = p->left->ctype;
      break;
    case AST_OP_SIZEOF:
      p->left = semantic_analysis(p->left);
      return make_ast_int(sizeof_ctype(p->left->ctype));
      break;
    case AST_OP_DOT:
      p->left = semantic_analysis(p->left);
      if ((p->left->type != AST_OP_DEREF && p->left->type != AST_VAR) ||
          p->right->type != AST_VAR || p->left->ctype->type != TYPE_STRUCT) {
        error_with_token(p->token, "cannot apply operator");
      }
      Vector *list = p->left->ctype->struct_decl;
      char *target = p->right->ident;
      int is_exist_target = 0;
      for (int i = 0; i < list->size; i++) {
        StructMember *sm = vector_at(list, i);
        if (strcmp(sm->name, target) == 0) {
          is_exist_target = 1;
          p->ctype = sm->ctype;
          p->offset_from_bp = sm->offset;
          break;
        }
      }
      if (is_exist_target == 0) {
        error_with_token(p->right->token, "not exist such member");
      }
      break;
    case AST_OP_ARROW: {
      Ast *deref = make_ast_op(AST_OP_DEREF, p->left, NULL, p->token);
      Ast *dot = make_ast_op(AST_OP_DOT, deref, p->right, p->token);
      p = semantic_analysis(dot);
      break;
    }
    case AST_VAR:
      if (symboltable_get(symbol_table, p->ident) ==
          NULL)  // undefined variable.
        error_with_token(
            p->token, allocate_concat_3string("use of undeclared identifier '",
                                              p->ident, "'"));
      else {
        SymbolTableEntry *e = symboltable_get(symbol_table, p->ident);
        if (e->is_constant == 1) {
          p = make_ast_int(e->constant_value);
        } else {
          p->symbol_table_entry = e;
          p->ctype = p->symbol_table_entry->ctype;
          sizeof_ctype(p->ctype);  // to calc struct offset
        }
      }
      break;
    case AST_ENUM: {
      Vector *v;
      v = p->ctype->enumerator_list;
      for (int i = 0; i < v->size; i++) {
        CType *ctype = make_ctype(TYPE_INT, NULL);
        SymbolTableEntry *_e = make_SymbolTableEntry(ctype, 0);
        _e->is_constant = 1;
        _e->constant_value = i;
        MapEntry *e = allocate_MapEntry(vector_at(v, i), _e);
        if (map_get(symbol_table, e->key) != NULL)  // already defined table.
          error_with_token(p->token, allocate_concat_3string(
                                         "redefinition of '", e->key, "'"));
        map_put(symbol_table, e);
      }
      break;
    }
    case AST_SUBSCRIPT:
      p->left = semantic_analysis(p->left);
      p->right = semantic_analysis(p->right);
      Ast *add = make_ast_op(AST_OP_ADD, p->left, p->right, p->token);
      Ast *deref = make_ast_op(AST_OP_DEREF, add, NULL, p->token);
      return semantic_analysis(deref);
      break;
    case AST_DECL_LOCAL_VAR: {
      p->ctype = update_ctype(p->ctype, p->token);

      // register variable
      SymbolTableEntry *_e = make_SymbolTableEntry(p->ctype, 0);
      _e->offset = get_offset_from_bp(p->ctype);
      MapEntry *e = allocate_MapEntry(p->ident, _e);
      if (map_get(symbol_table, e->key) != NULL)  // already defined variable.
        error_with_token(p->token, allocate_concat_3string("redefinition of '",
                                                           p->ident, "'"));
      map_put(symbol_table, e);
      break;
    }
    case AST_DECL_GLOBAL_VAR: {
      p->ctype = update_ctype(p->ctype, p->token);

      // register variable
      SymbolTableEntry *_e = make_SymbolTableEntry(p->ctype, 1);
      _e->ident = p->ident;
      MapEntry *e = allocate_MapEntry(p->ident, _e);
      if (map_get(symbol_table, e->key) != NULL)  // already defined variable.
        error_with_token(p->token, allocate_concat_3string("redefinition of '",
                                                           p->ident, "'"));
      map_put(symbol_table, e);
      break;
    }
    case AST_CALL_FUNC:
      for (int i = p->args->size - 1; i >= 0; i--) {
        Ast **q = (Ast **)p->args->data + i;
        *q = semantic_analysis(*q);
        *q = array_to_ptr(*q);
      }
      break;
    case AST_DECL_FUNC:
      symbol_table = p->symbol_table;
      offset_from_bp = 0;
      if (p->args->size > 6)
        error_with_token(p->token, "too many arguments");
      for (int i = 0; i < p->args->size; i++)
        p->args->data[i] = semantic_analysis(vector_at(p->args, i));
      p->statement = semantic_analysis(p->statement);
      symbol_table = symbol_table->next;
      p->offset_from_bp = offset_from_bp;
      break;
    case AST_COMPOUND_STATEMENT:
      for (int i = 0; i < p->statements->size; i++)
        p->statements->data[i] = semantic_analysis(vector_at(p->statements, i));
      break;
    case AST_EXPR_STATEMENT:
      p->expr = semantic_analysis(p->expr);
      break;
    case AST_IF_STATEMENT:
      p->cond = semantic_analysis(p->cond);
      p->left = semantic_analysis(p->left);
      p->right = semantic_analysis(p->right);
      break;
    case AST_FOR_STATEMENT:
      p->init = semantic_analysis(p->init);
      p->step = semantic_analysis(p->step);
    case AST_WHILE_STATEMENT:
      p->cond = semantic_analysis(p->cond);
      p->statement = semantic_analysis(p->statement);
      break;
    case AST_RETURN_STATEMENT:
      p->expr = semantic_analysis(p->expr);
      break;
  }

  return p;
}
