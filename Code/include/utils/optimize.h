#ifndef __OPTIMIZE_H__
#define __OPTIMIZE_H__

#define DELETE_LINE {                                                                       \
        ICList prev_line = line->prev;                                                      \
        if(line->prev != NULL)                                                              \
            line->prev->next = line->next;                                                  \
        if(line->next != NULL)                                                              \
            line->next->prev = line->prev;                                                  \
        free(line);                                                                         \
        line = prev_line;                                                                   \
        break;                                                                              \
    }

#define FIND_LAST_ASSIGN(src, tar)                                                          \
    for(ICList prev_line = line->prev; prev_line != NULL; prev_line = prev_line->prev) {    \
        if(prev_line->code.kind == IC_ASSIGN) {                                             \
            if(0 == strcmp(tar->u.var_name, prev_line->code.u.assign.left->u.var_name)) {   \
                if(prev_line->code.u.assign.right->kind != OP_VAR &&                        \
                    prev_line->code.u.assign.right->kind != OP_CONST) {                     \
                    break;                                                                  \
                }                                                                           \
                line->code.src = copy_operand(prev_line->code.u.assign.right);               \
                free(tar);                                                                  \
                tar = line->code.src;                                                       \
                if(tar->kind != OP_VAR) {                                                   \
                    break;                                                                  \
                }                                                                           \
            }                                                                               \
        }                                                                                   \
        else {                                                                              \
            break;                                                                          \
        }                                                                                   \
    }

#define COUNT_USED_VARIABLE(var)                                                            \
    if(var->kind == OP_VAR || var->kind == OP_ADDR || var->kind == OP_DEREF) {              \
        update(var->u.var_name, get_value(var->u.var_name) + 1);                            \
    }

#endif