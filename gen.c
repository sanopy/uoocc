#include "uoocc.h"

static void emit_lvalue(Ast *p) {
  if (p->type == AST_OP_DEREF) {
    codegen(p->left);
  } else if (p->type == AST_VAR) {
    if (p->symbol_table_entry->is_global) {
      printf("\tleaq %s(%%rip), %%rax\n", p->ident);
      printf("\tpushq %%rax\n");
    } else {
      printf("\tleaq %d(%%rbp), %%rax\n", -p->symbol_table_entry->offset);
      printf("\tpushq %%rax\n");
    }
  }
}

static void emit_expr(Ast *p) {
  CType *ltype = p->left == NULL ? NULL : p->left->ctype;
  CType *rtype = p->right == NULL ? NULL : p->right->ctype;

  switch (p->type) {
    case AST_INT:
      printf("\tpushq $%d\n", p->ival);
      break;
    case AST_OP_ADD:
    case AST_OP_SUB:
      codegen(p->left);
      codegen(p->right);
      printf("\tpopq %%rdx\n");
      printf("\tpopq %%rax\n");
      if ((ltype->type == TYPE_INT || ltype->type == TYPE_CHAR) &&
          (rtype->type == TYPE_INT || rtype->type == TYPE_CHAR))
        printf("\t%s %%rdx, %%rax\n", p->type == AST_OP_ADD ? "addq" : "subq");
      else if (ltype->type == TYPE_PTR && rtype->type == TYPE_PTR) {
        printf("\tsubq %%rdx, %%rax\n");
        printf("\tsarq $%d, %%rax\n", ltype->ptrof->type == TYPE_INT ? 2 : 3);
      } else {
        if (ltype->type == TYPE_INT)
          printf("\tsalq $%d, %%rax\n", rtype->ptrof->type == TYPE_INT ? 2 : 3);
        else
          printf("\tsalq $%d, %%rdx\n", ltype->ptrof->type == TYPE_INT ? 2 : 3);
        printf("\t%s %%rdx, %%rax\n", p->type == AST_OP_ADD ? "addq" : "subq");
      }
      printf("\tpushq %%rax\n");
      break;
    case AST_OP_MUL:
    case AST_OP_DIV:
      codegen(p->left);
      codegen(p->right);
      printf("\tpopq %%rbx\n");
      printf("\tpopq %%rax\n");
      if (p->type == AST_OP_MUL)
        printf("\tmul %%rbx\n");
      else {
        printf("\txor %%rdx, %%rdx\n");
        printf("\tdiv %%rbx\n");
      }
      printf("\tpushq %%rax\n");
      break;
    case AST_OP_ASSIGN:
      emit_lvalue(p->left);
      codegen(p->right);
      printf("\tpopq %%rdi\n");
      printf("\tpopq %%rax\n");
      if (p->left->ctype->type == TYPE_PTR)
        printf("\tmovq %%rdi, (%%rax)\n");
      else if (p->left->ctype->type == TYPE_CHAR) {
        printf("\tmovb %%dil, (%%rax)\n");
      } else
        printf("\tmovl %%edi, (%%rax)\n");
      printf("\tpushq %%rdi\n");
      break;
  }
}

void codegen(Ast *p) {
  if (p == NULL)
    return;

  CType *ltype = p->left == NULL ? NULL : p->left->ctype;

  switch (p->type) {
    case AST_INT:
      printf("\tpushq $%d\n", p->ival);
      break;
    case AST_OP_ADD:
    case AST_OP_SUB:
    case AST_OP_MUL:
    case AST_OP_DIV:
    case AST_OP_ASSIGN:
      emit_expr(p);
      break;
    case AST_OP_POST_INC:
    case AST_OP_POST_DEC:
      emit_lvalue(p->left);
      printf("\tpopq %%rax\n");
      printf("\tpushq (%%rax)\n");
      if (ltype->type == TYPE_INT) {
        printf("\t%s (%%rax)\n", p->type == AST_OP_POST_INC ? "incl" : "decl");
      } else {
        Ast *right =
            make_ast_op(p->type == AST_OP_POST_INC ? AST_OP_ADD : AST_OP_SUB,
                        p->left, make_ast_int(1), p->token);
        Ast *node = make_ast_op(AST_OP_ASSIGN, p->left, right, p->token);
        semantic_analysis(node);
        emit_expr(node);
        printf("\tpopq %%rax\n");
      }
      break;
    case AST_OP_PRE_INC:
    case AST_OP_PRE_DEC:
      if (ltype->type == TYPE_INT) {
        emit_lvalue(p->left);
        printf("\tpopq %%rax\n");
        printf("\t%s (%%rax)\n", p->type == AST_OP_PRE_INC ? "incl" : "decl");
        printf("\tpushq (%%rax)\n");
      } else {
        Ast *right =
            make_ast_op(p->type == AST_OP_PRE_INC ? AST_OP_ADD : AST_OP_SUB,
                        p->left, make_ast_int(1), p->token);
        Ast *node = make_ast_op(AST_OP_ASSIGN, p->left, right, p->token);
        semantic_analysis(node);
        emit_expr(node);
      }
      break;
    case AST_OP_REF:
      emit_lvalue(p->left);
      break;
    case AST_OP_DEREF:
      codegen(p->left);
      printf("\tpopq %%rax\n");
      printf("\tpushq (%%rax)\n");
      break;
    case AST_OP_LT:
    case AST_OP_LE:
    case AST_OP_EQUAL:
    case AST_OP_NEQUAL:
      codegen(p->left);
      codegen(p->right);
      printf("\tpopq %%rdx\n");
      printf("\tpopq %%rax\n");
      printf("\tcmpl %%edx, %%eax\n");
      char *s;
      if (p->type == AST_OP_LT)
        s = allocate_string("setl");
      else if (p->type == AST_OP_LE)
        s = allocate_string("setle");
      else if (p->type == AST_OP_EQUAL)
        s = allocate_string("sete");
      else
        s = allocate_string("setne");
      printf("\t%s %%al\n", s);
      printf("\tmovzbl %%al, %%eax\n");
      printf("\tpushq %%rax\n");
      break;
    case AST_VAR:
      if (p->symbol_table_entry->is_global) {
        printf("\tpushq %s(%%rip)\n", p->symbol_table_entry->ident);
      } else {
        if (p->ctype->type == TYPE_CHAR) {
          printf("\tmovzbl %d(%%rbp), %%eax\n", -p->symbol_table_entry->offset);
          printf("\tpushq %%rax\n");
        } else
          printf("\tpushq %d(%%rbp)\n", -p->symbol_table_entry->offset);
      }
      break;
    case AST_DECL_GLOBAL_VAR:
      printf(".data\n");
      printf("%s:\n", p->ident);
      printf("\t.zero %d\n", sizeof_ctype(p->ctype));
      break;
    case AST_CALL_FUNC:
      for (int i = p->args->size - 1; i >= 0; i--)
        codegen(vector_at(p->args, i));

      for (int i = 1; i <= (p->args->size > 6 ? 6 : p->args->size); i++) {
        char *reg[] = {"", "rdi", "rsi", "rdx", "rcx", "r8", "r9"};
        printf("\tpopq %%%s\n", reg[i]);
      }
      printf("\tcall %s\n", p->ident);
      printf("\tpushq %%rax\n");
      break;
    case AST_DECL_FUNC:
      symbol_table = p->symbol_table;
      printf(".text\n");
      printf("%s:\n", p->ident);
      printf("\tpushq %%rbp\n");
      printf("\tmovq %%rsp, %%rbp\n");
      if (p->offset_from_bp > 0 && (p->offset_from_bp) % 16 == 0)
        printf("\tsub $%d, %%rsp\n", p->offset_from_bp);
      else if (p->offset_from_bp > 0)
        printf("\tsub $%d, %%rsp\n",
               (p->offset_from_bp) + (16 - p->offset_from_bp % 16));

      for (int i = 0; i < (p->args->size > 6 ? 6 : p->args->size); i++) {
        Ast *node = vector_at(p->args, i);
        char *s = node->ident;
        char *reg32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
        char *reg64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
        if (node->ctype->type == TYPE_INT)
          printf("\tmovl %%%s, %d(%%rbp)\n", reg32[i],
                 -((SymbolTableEntry *)map_get(symbol_table, s)->val)->offset);
        else
          printf("\tmovq %%%s, %d(%%rbp)\n", reg64[i],
                 -((SymbolTableEntry *)map_get(symbol_table, s)->val)->offset);
      }

      codegen(p->statement);

      printf("\tpopq %%rax\n");
      printf("\tmovq %%rbp, %%rsp\n");
      printf("\tpopq %%rbp\n");
      printf("\tret\n");
      break;
    case AST_COMPOUND_STATEMENT:
      for (int i = 0; i < p->statements->size; i++)
        codegen(vector_at(p->statements, i));
      break;
    case AST_IF_STATEMENT:
      codegen(p->cond);
      printf("\tpopq %%rax\n");
      printf("\ttest %%rax, %%rax\n");
      int seq1 = get_sequence_num();
      printf("\tjz .L%d\n", seq1);
      codegen(p->left);
      if (p->right != NULL) {
        int seq2 = get_sequence_num();
        printf("\tjmp .L%d\n", seq2);
        printf(".L%d:\n", seq1);
        codegen(p->right);
        printf(".L%d:\n", seq2);
      } else
        printf(".L%d:\n", seq1);
      break;
    case AST_WHILE_STATEMENT: {
      int seq1 = get_sequence_num(), seq2 = get_sequence_num();
      printf(".L%d:\n", seq1);
      codegen(p->cond);
      printf("\tpopq %%rax\n");
      printf("\ttest %%rax, %%rax\n");
      printf("\tjz .L%d\n", seq2);
      codegen(p->statement);
      printf("\tjmp .L%d\n", seq1);
      printf(".L%d:\n", seq2);
      break;
    }
    case AST_FOR_STATEMENT: {
      int seq1 = get_sequence_num(), seq2 = get_sequence_num();
      codegen(p->init);
      printf(".L%d:\n", seq1);
      codegen(p->cond);
      printf("\tpopq %%rax\n");
      printf("\ttest %%rax, %%rax\n");
      printf("\tjz .L%d\n", seq2);
      codegen(p->statement);
      codegen(p->step);
      printf("\tjmp .L%d\n", seq1);
      printf(".L%d:\n", seq2);
      break;
    }
  }
}
