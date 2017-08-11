/*
*  cool.y
*              Parser definition for the COOL language.
*
*/
%{
  #include <iostream>
  #include "cool-tree.h"
  #include "stringtab.h"
  #include "utilities.h"
  
  extern char *curr_filename;
  
  
  /* Locations */
  #define YYLTYPE int              /* the type of locations */
  #define cool_yylloc curr_lineno  /* use the curr_lineno from the lexer
  for the location of tokens */
    
    extern int node_lineno;          /* set before constructing a tree node
    to whatever you want the line number
    for the tree node to be */
      
      
      #define YYLLOC_DEFAULT(Current, Rhs, N)         \
      Current = Rhs[1];                             \
      node_lineno = Current;
    
    
    #define SET_NODELOC(Current)  \
    node_lineno = Current;
    
    /* IMPORTANT NOTE ON LINE NUMBERS
    *********************************
    * The above definitions and macros cause every terminal in your grammar to 
    * have the line number supplied by the lexer. The only task you have to
    * implement for line numbers to work correctly, is to use SET_NODELOC()
    * before constructing any constructs from non-terminals in your grammar.
    * Example: Consider you are matching on the following very restrictive 
    * (fictional) construct that matches a plus between two integer constants. 
    * (SUCH A RULE SHOULD NOT BE  PART OF YOUR PARSER):
    
    plus_consts : INT_CONST '+' INT_CONST 
    
    * where INT_CONST is a terminal for an integer constant. Now, a correct
    * action for this rule that attaches the correct line number to plus_const
    * would look like the following:
    
    plus_consts : INT_CONST '+' INT_CONST 
    {
      // Set the line number of the current non-terminal:
      // ***********************************************
      // You can access the line numbers of the i'th item with @i, just
      // like you acess the value of the i'th exporession with $i.
      //
      // Here, we choose the line number of the last INT_CONST (@3) as the
      // line number of the resulting expression (@$). You are free to pick
      // any reasonable line as the line number of non-terminals. If you 
      // omit the statement @$=..., bison has default rules for deciding which 
      // line number to use. Check the manual for details if you are interested.
      @$ = @3;
      
      
      // Observe that we call SET_NODELOC(@3); this will set the global variable
      // node_lineno to @3. Since the constructor call "plus" uses the value of 
      // this global, the plus node will now have the correct line number.
      SET_NODELOC(@3);
      
      // construct the result node:
      $$ = plus(int_const($1), int_const($3));
    }
    
    */
    
    
    
    void yyerror(char *s);        /*  defined below; called for each parse error */
    extern int yylex();           /*  the entry point to the lexer  */
    
    /************************************************************************/
    /*                DONT CHANGE ANYTHING IN THIS SECTION                  */
    
    Program ast_root;         /* the result of the parse  */
    Classes parse_results;        /* for use in semantic analysis */
    int omerrs = 0;               /* number of errors in lexing and parsing */
   #define YYMAXDEPTH 40000
   %}
    
    /* A union of all the types that can be the result of parsing actions. */
    %union {
      Boolean boolean;
      Symbol symbol;
      Program program;
      Class_ class_;
      Classes classes;
      Feature feature;
      Features features;
      Formal formal;
      Formals formals;
      Case case_;
      Cases cases;
      Expression expression;
      Expressions expressions;
      char *error_msg;
    }
    
    /* 
    Declare the terminals; a few have types for associated lexemes.
    The token ERROR is never used in the parser; thus, it is a parse
    error when the lexer returns it.
    
    The integer following token declaration is the numeric constant used
    to represent that token internally.  Typically, Bison generates these
    on its own, but we give explicit numbers to prevent version parity
    problems (bison 1.25 and earlier start at 258, later versions -- at
    257)
    */
    %token CLASS 258 ELSE 259 FI 260 IF 261 IN 262 
    %token INHERITS 263 LET 264 LOOP 265 POOL 266 THEN 267 WHILE 268
    %token CASE 269 ESAC 270 OF 271 DARROW 272 NEW 273 ISVOID 274
    %token <symbol>  STR_CONST 275 INT_CONST 276 
    %token <boolean> BOOL_CONST 277
    %token <symbol>  TYPEID 278 OBJECTID 279 
    %token ASSIGN 280 NOT 281 LE 282 ERROR 283
    
    /*  DON'T CHANGE ANYTHING ABOVE THIS LINE, OR YOUR PARSER WONT WORK       */
    /**************************************************************************/
    
    /* Complete the nonterminal list below, giving a type for the semantic
    value of each non terminal. (See section 3.6 in the bison 
    documentation for details). */
    
    /* Declare types for the grammar's non-terminals. */
    %type <program> program
    %type <classes> class_list
    %type <class_> class
    
    /* You will want to change the following line. */
    %type <features> dummy_feature_list let_id_list
    %type <feature> feature attribute
    %type <formals> maybe_formal_list filled_formal_list
    %type <formal> formal
    %type <cases> case_branch_list
    %type <case_> case_branch
    %type <expressions> maybe_expr_as_args filled_expr_as_args expr_as_blocks
    %type <expression> expr dispatch_expr let_expr arith_expr cmp_expr
    
    /* Precedence declarations go here. */
    %right ASSIGN
    %nonassoc NOT
    %nonassoc '<' '=' LE
    %left '+' '-'
    %left '*' '/'
    %nonassoc ISVOID
    %nonassoc '~'
    %nonassoc '@'
    %nonassoc '.'
    
    %%
    /* 
    Save the root of the abstract syntax tree in a global variable.
    */
    program : 
        class_list { @$ = @1; ast_root = program($1); }
    ;
    
    class_list : 
        class         /* single class */
        { 
            $$ = single_Classes($1);
            parse_results = $$; 
        }
    |   class_list class  /* several classes */
        { 
            $$ = append_Classes($1, single_Classes($2)); 
            parse_results = $$; 
        }
    ;

    /* If no parent is specified, the class inherits from the Object class. */
    class : 
        CLASS TYPEID '{' dummy_feature_list '}' ';' 
        { 
            $$ = class_($2, idtable.add_string("Object"), $4,
                    stringtable.add_string(curr_filename)); 
        }
    |   CLASS TYPEID INHERITS TYPEID '{' dummy_feature_list '}' ';' 
        { 
            $$ = class_($2, $4, $6, stringtable.add_string(curr_filename)); 
        }
    ;

    /* Feature list may be empty, but no empty features in list. */
    dummy_feature_list : 
      /* empty */ {  $$ = nil_Features(); }
    |   feature ';' dummy_feature_list
        {
            $$ = append_Features(single_Features($1), $3);
        }
    ;

    feature :
        /* method definition */
        OBJECTID '(' maybe_formal_list ')' ':' TYPEID '{' expr '}'
        {
            $$ = method($1, $3, $6, $8);
        }
    |   attribute { $$ = $1; }
    ;

    attribute :
        /* OBJECTID[attr_name] ':' TYPEID[attr_type] */
        OBJECTID ':' TYPEID
        {
            $$ = attr($1, $3, no_expr());
            /* $$ = attr($attr_name, $attr_type, no_expr()); */
        }
        /* attribute declaration with initlization */
    |   /* OBJECTID[attr_name] ':' TYPEID[attr_type] ASSIGN expr[init_val] */
        OBJECTID ':' TYPEID ASSIGN expr
        {
            $$ = attr($1, $3, $5);
            /* $$ = attr($attr_name, $attr_type, $init_val); */
        }
    ;

    maybe_formal_list :
        /* empty */ { $$ = nil_Formals(); }
    |   filled_formal_list { $$ = $1; }
    ;

    filled_formal_list :
        formal { $$ = single_Formals($1); }
    |   formal ',' filled_formal_list
        { $$ = append_Formals(single_Formals($1), $3); }
    ;

    formal :
        OBJECTID ':' TYPEID { $$ = formal($1, $3); }
    ;

    case_branch_list :
        case_branch
        {
            $$ = single_Cases($1);
        }
    |   case_branch case_branch_list
        {
            $$ = append_Cases(single_Cases($1), $2);
        }
    ;

    case_branch :
        /* OBJECTID[var_name] ':' TYPEID[var_type] DARROW expr[branch_body] ';' */
        OBJECTID ':' TYPEID DARROW expr ';'
        {
            $$ = branch($1, $3, $5);
            /* $$ = branch($var_name, $var_type, $branch_body); */
        }
    ;

    maybe_expr_as_args :
        /* empty */ { $$ = nil_Expressions(); }
    |   filled_expr_as_args { $$ = $1; }
    ;
    
    filled_expr_as_args :
        expr { $$ = single_Expressions($1); }
    |   expr ',' filled_expr_as_args 
        { $$ = append_Expressions(single_Expressions($1), $3); }
    ;

    expr_as_blocks :
        expr ';' { $$ = single_Expressions($1); }
    |   expr ';' expr_as_blocks
        { $$ = append_Expressions(single_Expressions($1), $3); }
    ;

    expr :
        OBJECTID ASSIGN expr 
        {   
            $$ = assign($1, $3); 
        }
    |   dispatch_expr { $$ = $1; }
    |   /* IF expr[pred] THEN expr[true_branch] ELSE expr[false_branch] FI */
        IF expr THEN expr ELSE expr FI
        {
            $$ = cond($2, $4, $6);
            /* $$ = cond($pred, $true_branch, $false_branch); */
        }
    |   /* WHILE expr[loop_pred] LOOP expr[loop_body] POOL */
        WHILE expr LOOP expr POOL
        {
            $$ = loop($2, $4);
            /* $$ = loop($loop_pred, $loop_body); */
        }
    |   /* CASE expr[var] OF case_branch_list[body] ESAC */
        CASE expr OF case_branch_list ESAC
        {
            $$ = typcase($2, $4);
            /* $$ = typcase($var, $body); */
        }
    |   '{' expr_as_blocks '}'
        {   
            $$ = block($2);
        }
    |   let_expr { $$ = $1; } 
    |   arith_expr { $$ = $1; }
    |   cmp_expr { $$ = $1; }
    |   NOT expr { $$ = comp($2); }             /* boolean NOT */
    |   '(' expr ')' { $$ = $2; }
    |   INT_CONST { $$ = int_const($1); }
    |   BOOL_CONST { $$ = bool_const($1); }
    |   STR_CONST { $$ = string_const($1); }
    |   NEW TYPEID { $$ = new_($2); }
    |   ISVOID expr { $$ = isvoid($2); }
    |   OBJECTID { $$ = object($1); }
    ;

    dispatch_expr :
        /* expr[caller] '.' OBJECTID[func_name] '(' maybe_expr_as_args[func_args] ')' */
        expr '.' OBJECTID '(' maybe_expr_as_args ')'
        {
            $$ = dispatch($1, $3, $5);
            /* $$ = dispatch($caller, $func_name, $func_args); */
        }
    |   /* OBJECTID[func_name] '(' maybe_expr_as_args[func_args] ')' */
        OBJECTID '(' maybe_expr_as_args ')'
        {
            $$ = dispatch(object(idtable.add_string("self")), $1, $3);
            /* $$ = dispatch(object(idtable.add_string("self")), $func_name, $func_args); */
        }
    |   /* expr[caller] '@' TYPEID[func_t] '.' OBJECTID[func_name] '(' maybe_expr_as_args[func_args] ')' */
        expr '@' TYPEID '.' OBJECTID '(' maybe_expr_as_args ')'
        {
            $$ = static_dispatch($1, $3, $5, $7);
            /* $$ = static_dispatch($caller, $func_t, $func_name, $func_args); */
        }
    ;

    let_expr :
        LET let_id_list IN expr
        {
            Features lst = $2;
            attr_class* ft;
            Expression res_expr = $4;

            for (unsigned i = lst->first(); lst->more(i); i = lst->next(i))
            {
                ft = (attr_class*)(lst->nth(i));
                res_expr = let(ft->get_name(), ft->get_type(), ft->get_init(), res_expr);
            }
            
            $$ = res_expr;
        }
    ;

    let_id_list :
        attribute { $$ = single_Features($1); }
    |   attribute ',' let_id_list
        {
            /* REVERSE order is important! */
            $$ = append_Features($3, single_Features($1)); 
        }
    ;

    arith_expr :
        expr '+' expr { $$ = plus($1, $3); }
    |   expr '-' expr { $$ = sub($1, $3); }
    |   expr '*' expr { $$ = mul($1, $3); }
    |   expr '/' expr { $$ = divide($1, $3); }
    |   '~' expr { $$ = neg($2); }
    ;

    cmp_expr :
        expr '<' expr { $$ = lt($1, $3); } 
    |   expr '=' expr { $$ = eq($1, $3); }
    |   expr LE expr  { $$ = leq($1, $3); }
    ;

    /* end of grammar */
    %%
    
    /* This function is called automatically when Bison detects a parse error. */
    void yyerror(char *s)
    {
      extern int curr_lineno;
      
      cerr << "\"" << curr_filename << "\", line " << curr_lineno << ": " \
      << s << " at or near ";
      print_cool_token(yychar);
      cerr << endl;
      omerrs++;
      
      if(omerrs>50) {fprintf(stdout, "More than 50 errors\n"); exit(1);}
    }
