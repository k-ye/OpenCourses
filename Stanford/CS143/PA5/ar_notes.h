#ifndef DRAFT_H
#define DRAFT_H
/*

No temporary

- $s0 is used to store self object
- $a0 is used to store the result of any expression
- $a0 is used to pass in the self object at a function call

AR of 'func'

            ------------------------    \
            arg_n                       |
            ------------------------    | 
            ... ...                      > created by the caller
            ------------------------    |
            arg_1                       |
    $fp ->  ------------------------   < 
            old $fp                     | 
            ------------------------    |
            old self object $s0          > created by the 'func' 
            ------------------------    |
            return address $ra          |
    $sp ->  ------------------------    /


func(x, y) { x + y }:
    // $fp, $so, $ra take a total of 3 entries
    - addiu $sp $sp -12
    -   sw $fp 12($sp)
        sw $s0 8($sp)
        sw $ra 4($sp)
        addiu $fp $sp 12
        move $s0 $a0
    // this is a completely new env, which should be created by the callee.
    // @cur_class should be updated by the called, too.
    - cur_env->enterscope();
    - map all @cur_class' attributes into @cur_cur: ($s0, offset)
    - update @cur_env by mapping all formal params of 'func' into @cur_class ($fp, offset)
    -   cur_temp_offset = -12
        code(@body, -12)

expr.func(x, y):
    @id will be in $a0
    - code(y, cur_temp_offset)
    -   push $a0
        cur_temp_offset -= 4
    - code(x, cur_temp_offset)
    -   push $a0
        cur_temp_offset -= 4
    - code(expr, cur_temp_offset)

x + y:

case expr of id1 : T1 => e1 ; ...; idn : Tn => en; esac:
    - $a0 <- expr->code(...)
    - normal_end_label = get_next_label()
    -   bne $a0 $zer0 @next_label
        la $a0 str_const0
        li $t1 1
        jal _case_abort2

    - label##get_next_label():
        lw      $t1     0($a0)
        blt     $t1     #tag        @next_label
        bgt     $t1     #tag_end    @next_label
    - push $a0 to the stack, map @branch->get_name() to current stack top
    - e1->code(...)
    - pop the stack
    

*/


/*
With temporary variables

- $s0 is used to store self object
- $a0 is used to store the result of any expression
- $a0 is used to pass in the self object in $s0 at a function call

AR of 'func'

            ------------------------    \
            arg_n                       |
            ------------------------    | 
            ... ...                      > created by the caller
            ------------------------    |
            arg_1                       |
            ------------------------   <
                                        |
            #NT of 'func' body          |
                                        |
    $fp ->  ------------------------    | 
            old $fp                      > created by the 'func' 
            ------------------------    |
            old self object $s0         |
            ------------------------    |
            return address $ra          |
    $sp ->  ------------------------    /


func(x, y) { x + y }:
    - #NT = body->nt();
    // $fp, $so, $ra take a total of 3 entries
    - addiu $sp $sp -4*(#NT + 3)
    -   sw $fp 12($sp)
        sw $s0 8($sp)
        sw $ra 4($sp)
        addiu $fp $sp 12
        move $s0 $a0
    // this is a completely new env, which should be created by the callee.
    // @cur_class should be updated by the called, too.
    - cur_env->enterscope();
    - map all @cur_class' attributes into @cur_cur: ($s0, offset)
    - update @cur_env by mapping all formal params of 'func' into @cur_class ($fp, #NT + offset)
    - code(@body, 0)

id.func(x, y):
    @id will be in $a0
    @x will be in 
*/
#endif