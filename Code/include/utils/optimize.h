#ifndef __OPTIMIZE_H__
#define __OPTIMIZE_H__

#define DELETE_LINE(to) {                                                                   \
        ICList to_line = line->to;                                                          \
        if(line->prev != NULL)                                                              \
            line->prev->next = line->next;                                                  \
        else                                                                                \
            icode = line->next;                                                             \
        if(line->next != NULL)                                                              \
            line->next->prev = line->prev;                                                  \
        free(line);                                                                         \
        line = to_line;                                                                     \
        break;                                                                              \
    }

#define FIND_LAST_ASSIGN(src, tar, max_kind_index)                                          \
    for(ICList prev_line = line->prev; prev_line != NULL; prev_line = prev_line->prev) {    \
        if(prev_line->code.kind == IC_ASSIGN &&                                             \
           prev_line->code.u.assign.left->kind == OP_VAR                                    \
        ) {                                                                                 \
            if(!strcmp(tar->u.var_name, prev_line->code.u.assign.left->u.var_name)) {       \
                if(prev_line->code.u.assign.right->kind > max_kind_index) {                 \
                    break;                                                                  \
                }                                                                           \
                line->code.src = copy_operand(prev_line->code.u.assign.right);              \
                free(tar);                                                                  \
                tar = line->code.src;                                                       \
                if(tar->kind != OP_VAR) {                                                   \
                    break;                                                                  \
                }                                                                           \
            }                                                                               \
        }                                                                                   \
        else {                                                                              \
            if(prev_line->code.kind < 4) {                                                  \
                Operand prev_line_res = prev_line->code.u.binop.res;                        \
                if(0 != strcmp(tar->u.var_name, prev_line_res->u.var_name) &&               \
                   prev_line_res->kind == OP_VAR                                            \
                ) {                                                                         \
                    continue;                                                               \
                }                                                                           \
            }                                                                               \
            break;                                                                          \
        }                                                                                   \
    }

#define COUNT_USED_VARIABLE(var)                                                            \
    if(var->kind == OP_VAR || var->kind == OP_ADDR || var->kind == OP_DEREF) {              \
        (*insert(var->u.var_name))++;                                                       \
    }

#endif