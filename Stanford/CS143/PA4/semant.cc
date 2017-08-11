#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <cassert>
#include "semant.h"
#include "utilities.h"

#include <map>    // not c++03, no std::unordered_map
#include <set>

extern int semant_debug;
extern char *curr_filename;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
static Symbol 
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;
//
// Initializing the predefined symbols.
//
static void initialize_constants(void)
{
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy        = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any 
    //   user-defined class.
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& SemantErrorLogger::semant_error()                
//
//    ostream& SemantErrorLogger::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& SemantErrorLogger::semant_error(Symbol filename, tree_node *t)  
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

static SemantErrorLogger err_logger;
static ostream& error_stream = cerr;

ostream& SemantErrorLogger::semant_error(Class_ c)
{                                                             
    semant_error(c->get_filename(),c) << c->get_name()->get_string();
    return error_stream;
}    

ostream& SemantErrorLogger::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& SemantErrorLogger::semant_error()                  
{                                                 
    semant_errors++;                            
    return error_stream;
}

ostream& semant_error(Class_ c) {
    return err_logger.semant_error(c);
}

ostream& semant_error(Symbol filename, tree_node* t) {
    return err_logger.semant_error(filename, t);
}

ostream& semant_error() {
    return err_logger.semant_error();
}

////////////////////////////////////////////////////////////////////
//
///////////////////// My Code Below ///////////////////
//
////////////////////////////////////////////////////////////////////

#define THROW_ERR_CLASS_RELATIONSHIP (throw 1)
// #define THROW_ERR_SCOPE_TYPE (throw 2)
// #define THROW_ERR_CLASS_RELATIONSHIP return
#define THROW_ERR_SCOPE_TYPE return

// Type checking environments
typedef SymbolTable<Symbol, Entry> symtab_type;
static symtab_type* cur_symtab;
static Class_ cur_class;
static std::set<Symbol> seen_func;

class _ClassMap {
private:
    typedef std::map<Symbol, Class_> map_type;
    typedef map_type::iterator iter_type;
    typedef map_type::const_iterator citer_type;
    
    map_type _map;

public:
    bool add(const Symbol& s, const Class_& c) {
        using namespace std;
        pair<iter_type, bool> res = _map.insert(make_pair(s, c));
        return res.second;
        // _map[s] = c;
    }

    Class_ lookup(const Symbol& s) const {
        citer_type iter = _map.find(s);

        if (iter != _map.end()) {
            return iter->second;
        }
        return (Class_)NULL;
    }
};

// Attribute/Method should be retrievable from @class_map
static _ClassMap class_map;

class _InheritenceGraph {
private:
    typedef std::map<Symbol, Symbol> map_type;
    typedef map_type::const_iterator map_iter_type;

    map_type _graph;

public:

    void add_edge(const Symbol child, const Symbol parent) {
        _graph[child] = parent;
    }

    Symbol lookup_parent(const Symbol c) const {
        map_iter_type iter = _graph.find(c);
        return (iter != _graph.end()) ? iter->second : NULL;
    }

    // Check if @_graph forms a DAG.
    void validate() const {
        typedef std::set<Symbol> set_type;

        set_type seen;
        Symbol parent;
        for (map_iter_type iter = _graph.begin(); iter != _graph.end(); ++iter) {
            seen.clear();

            parent = iter->first;
            // All user declared classes will ultimately inherit from Object
            while (strcmp(parent->get_string(), Object->get_string()) != 0) {
                if (seen.count(parent)) {
                    cerr << "Loop inheritence detected in class "
                        << iter->first->get_string() << endl;
                    THROW_ERR_CLASS_RELATIONSHIP;
                }

                seen.insert(parent);
                parent = lookup_parent(parent);
            }
        }
    }
};

// Class inheritence relationship should be retrievable from @inh_graph
static _InheritenceGraph inh_graph;

bool is_subclass(Symbol a, Symbol b) {
    // Check NULL
    if (!a || !b) return false;

    if (a == b) {
        // includes the situation where a == b == SELF_TYPE
        return true;
    }
    else if (b == SELF_TYPE) {
        return false;
    }

    if (a == SELF_TYPE) {
        a = cur_class->get_name();
    }
    
    while (a != No_class) {
        if (a == b) {
            return true;
        }
        a = inh_graph.lookup_parent(a);
    }
    
    return false;
}

Symbol lub(Symbol a, Symbol b) {
    // Check NULL
    if (!a || !b) return false;

    if (a == b) {
        return a;
    }
    else if (a == SELF_TYPE) {
        a = cur_class->get_name();
    }
    else if (b == SELF_TYPE) {
        b = cur_class->get_name();
    }

    Symbol a_copy(a); Symbol b_copy(b);
    unsigned ha(0), hb(0);

    while (a != No_class) {
        a = inh_graph.lookup_parent(a);
        ++ha;
    }
    while (b != No_class) {
        b = inh_graph.lookup_parent(b);
        ++hb;
    }

    unsigned delta;
    if (ha < hb) {
        // lub() has reflectivity, always make @a the deeper node
        a = b_copy;
        b = a_copy;
        delta = hb - ha;
    } else {
        a = a_copy;
        b = b_copy;
        delta = ha - hb;
    }

    for (unsigned i = 0; i < delta; ++i) {
        a = inh_graph.lookup_parent(a);
    }

    while (a != b) {
        a = inh_graph.lookup_parent(a);
        b = inh_graph.lookup_parent(b);
    }

    a = (a == No_class) ? NULL : a;
    return a;
}

void install_basic_classes() {

    // The tree package uses these globals to annotate the classes built below.
   // curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");
    
    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.
    
    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    Class_ Object_class =
    class_(Object, 
           No_class,
           append_Features(
                   append_Features(
                           single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
                           single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
                   single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
           filename);

    class_map.add(Object, Object_class);
    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ IO_class = 
    class_(IO, 
           Object,
           append_Features(
                   append_Features(
                           append_Features(
                                   single_Features(method(out_string, single_Formals(formal(arg, Str)),
                                              SELF_TYPE, no_expr())),
                                   single_Features(method(out_int, single_Formals(formal(arg, Int)),
                                              SELF_TYPE, no_expr()))),
                           single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
                   single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
           filename);  

    class_map.add(IO, IO_class);
    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
    class_(Int, 
           Object,
           single_Features(attr(val, prim_slot, no_expr())),
           filename);

    class_map.add(Int, Int_class);
    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
    class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);

    class_map.add(Bool, Bool_class);
    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //       
    Class_ Str_class =
    class_(Str, 
           Object,
           append_Features(
                   append_Features(
                           append_Features(
                                   append_Features(
                                           single_Features(attr(val, Int, no_expr())),
                                           single_Features(attr(str_field, prim_slot, no_expr()))),
                                   single_Features(method(length, nil_Formals(), Int, no_expr()))),
                           single_Features(method(concat, 
                                      single_Formals(formal(arg, Str)),
                                      Str, 
                                      no_expr()))),
                   single_Features(method(substr, 
                              append_Formals(single_Formals(formal(arg, Int)), 
                                     single_Formals(formal(arg2, Int))),
                              Str, 
                              no_expr()))),
           filename);

    class_map.add(Str, Str_class);
}

// This function will add all the declared classes in the source COOL program 
// into @class_map.
// \Return: 0 on success, 1 on faiure
void fill_class_map(Classes classes) {
    unsigned char has_main_class = 0;

    for (unsigned i = classes->first(); classes->more(i); i = classes->next(i)) {
        cur_class = classes->nth(i);
        char* class_name_str = cur_class->get_name()->get_string();

        // class name cannot be SELF_TYPE
        if (strcmp(class_name_str, SELF_TYPE->get_string()) == 0) {
            semant_error(cur_class) << "Class name cannot be SELF_TYPE" << endl;
            THROW_ERR_CLASS_RELATIONSHIP;
        }

        // class namec cannot be builtin types
        if (strcmp(class_name_str, Object->get_string()) == 0 ||
            strcmp(class_name_str, Int->get_string()) == 0 ||
            strcmp(class_name_str, Bool->get_string()) == 0 ||
            strcmp(class_name_str, Str->get_string()) == 0) {
            // strcmp(class_name_str, IO->get_string()) == 0) {
            semant_error(cur_class) << "Class name cannot be any of the builtin classes" << endl;
            THROW_ERR_CLASS_RELATIONSHIP;
        }

        bool ok = class_map.add(cur_class->get_name(), cur_class);
        if (!ok) {
            semant_error(cur_class) << "Class name redefined" << endl;
            THROW_ERR_CLASS_RELATIONSHIP;
        }

        if (strcmp(class_name_str, Main->get_string()) == 0) {
            has_main_class = 1;
        }
    }

    if (!has_main_class) {
        semant_error() << "Class Main is not defined." << endl;
        THROW_ERR_CLASS_RELATIONSHIP;
    }
}

// This function creates the inheritence relationship of all the declared
// classes in source COOL program. It only does simple check, like inheriting
// from an non-existing class or a builtin class. However, it leaves the detection
// of circular inheritence to @_InheritenceGraph::validate_inheritence_graph().
// \Note: This function should be called after @fill_class_map()
void fill_inheritence_graph(Classes classes) {
    // initialize the inheritence relationship for builtin classes
    inh_graph.add_edge(Object, No_class);
    inh_graph.add_edge(Int, Object);
    inh_graph.add_edge(Bool, Object);
    inh_graph.add_edge(Str, Object);
    inh_graph.add_edge(IO, Object);

    for (unsigned i = classes->first(); classes->more(i); i = classes->next(i)) {
        cur_class = classes->nth(i);
        
        Symbol cur_symbol = cur_class->get_name();
        Symbol parent_symbol = cur_class->get_parent();
        char* parent_name_str = parent_symbol->get_string();

        // Check inheriting from a non-existing class 
        if (!class_map.lookup(parent_symbol)) {
            semant_error(cur_class) << "Trying to inherit from a non-existing class" << endl;
            THROW_ERR_CLASS_RELATIONSHIP;
        }

        // Check inheriting from a builtin class
        if (strcmp(parent_name_str, Int->get_string()) == 0 ||
            strcmp(parent_name_str, Bool->get_string()) == 0 ||
            strcmp(parent_name_str, Str->get_string()) == 0) {
            // || strcmp(parent_name_str, IO->get_string()) == 0) {
            semant_error(cur_class) << "Cannot inherit from a builtin class" << endl;
            THROW_ERR_CLASS_RELATIONSHIP;
        }

        // Check inheriting from SELF_TYPE
        if (strcmp(parent_name_str, SELF_TYPE->get_string()) == 0) {
            semant_error(cur_class) << "Cannot inherit SELF_TYPE" << endl;
            THROW_ERR_CLASS_RELATIONSHIP;
        }

        inh_graph.add_edge(cur_symbol, parent_symbol);
    }
}


inline Symbol eval_self_type(const Symbol type) {
    return type == SELF_TYPE ? cur_class->get_name() : type;
}

void program_class::analyze() {
    for (unsigned i = classes->first(); classes->more(i); i = classes->next(i)) {
        cur_class = classes->nth(i);
        cur_class->analyze();
    }
}

Attribute class__class::get_attr(Symbol s) const {
    for (unsigned i = features->first(); features->more(i); i = features->next(i)) {
        Feature f = features->nth(i);
        if ((f->kind() == Feature_Kind::ATTR_) && 
            (strcmp(f->get_name()->get_string(), s->get_string()) == 0)) {
            return (Attribute)f;
        }
    }

    if (parent != No_class) {
        Class_ parent_class = class_map.lookup(parent);
        // @parent_class must not be null
        assert(parent_class);
        return parent_class->get_attr(s);
    }

    return NULL;
}

Method class__class::get_method(Symbol s) const {
    for (unsigned i = features->first(); features->more(i); i = features->next(i)) {
        Feature f = features->nth(i);
        if ((f->kind() == Feature_Kind::METH_) &&
            (strcmp(f->get_name()->get_string(), s->get_string()) == 0)) {
            return (Method)f;
        }
    }

    if (parent != No_class) {
        Class_ parent_class = class_map.lookup(parent);
        // @parent_class must not be null
        assert(parent_class);
        return parent_class->get_method(s);
    }
    return NULL;
}

void class__class::analyze() {
    cur_symtab->enterscope();
    
    for (unsigned i = features->first(); features->more(i); i = features->next(i)) {
        Feature f = features->nth(i);
        f->analyze();
    }

    cur_symtab->exitscope();
}

bool type_exists(const Symbol s, bool allow_self_type=true) {
    if (allow_self_type && (s == SELF_TYPE)) {
        return true;
    }
    
    // if (s == No_type) return true;

    return (class_map.lookup(s) != NULL);
}


void attr_class::analyze() {
    // Check duplicate attribute name
    if (cur_symtab->probe(name)) {
        semant_error(cur_class) << "::" << name->get_string()
            << ": attribute has been declared already" << endl;
        THROW_ERR_SCOPE_TYPE;
    }
    
    // Check attribute name is `self`
    if (strcmp(name->get_string(), self->get_string()) == 0) {
        semant_error(cur_class) << ": Cannot have `self` as attribute name" << endl;
        THROW_ERR_SCOPE_TYPE;
    }

    // Check type exists
    if (!type_exists(type_decl)) {
        THROW_ERR_SCOPE_TYPE;
    }

    // Check parent has not declared attribute @name already
    Class_ base_class = class_map.lookup(cur_class->get_parent());
    // @base_class cannot be No_class, otherwise @cur_class is Object
    Attribute base_attr = base_class->get_attr(name);
    if (base_attr) {
        semant_error(cur_class) << ": Cannot override attribute" << endl;
        THROW_ERR_SCOPE_TYPE;
    }

    // Check @init_type <= @type_decl, if @init_type != @No_type
    // \Note: @init_type == @No_type iff. @init is an instance of no_expr_class
    init->analyze();
    Symbol init_type = init->get_type();
    if ((init_type != No_type) && (!is_subclass(init_type, type_decl))) {
        semant_error(cur_class) << ": Attribute init type does not match" << endl;
        THROW_ERR_SCOPE_TYPE;
    }

    // This is necessary to check duplicate attribute name!
    cur_symtab->addid(name, type_decl);
}

void method_class::analyze() {
    // TODO: Missing duplicate method name checking

    cur_symtab->enterscope();

    // check each formal
    for (unsigned i = formals->first(); formals->more(i); i = formals->next(i)) {
        Formal f = formals->nth(i);
        f->analyze();
    }

    // Check if @return_type exists
    if (!type_exists(return_type)) {
        semant_error(cur_class) << "::" << name->get_string()
            << ": Unknown return type " << endl;
        THROW_ERR_SCOPE_TYPE;
    }

    // If this method is overriden in @cur_class, then find the 
    // base version, @base_method, in @base_class.
    // Check if the signatures match.
    Class_ base_class = class_map.lookup(cur_class->get_parent());
    Method base_method = base_class->get_method(name);

    if (base_method) {
        Formals base_formals = base_method->get_formals();

        // Check #formal params match
        if (formals->len() != base_formals->len()) {
            semant_error(cur_class) << "::" << name->get_string() 
                << ": Number of formal parameters does not match "
                << base_class->get_name()->get_string() << endl;
            THROW_ERR_SCOPE_TYPE;
        }

        // Check formal param type match
        Formal formal; 
        Formal base_formal;
        
        for (unsigned i = formals->first(); formals->more(i); i = formals->next(i)) {
            formal = formals->nth(i);
            base_formal = base_formals->nth(i);

            if (formal->get_type_decl() != base_formal->get_type_decl()) {
                semant_error(cur_class) << "::" << name->get_string() 
                  << "::" << formal->get_name()->get_string()
                  << ":  Declared type does not match "
                  << base_class->get_name()->get_string() << endl;
                THROW_ERR_SCOPE_TYPE;
            }
        }

        // Check return type;
        if (return_type != base_method->get_return_type()) {
            semant_error(cur_class) << "::" << name->get_string()
                << ":  Return type does not match "
                << base_class->get_name()->get_string() << endl;
            THROW_ERR_SCOPE_TYPE;
        }
    }
    
    // type checking
    expr->analyze();

    // Type of method body @expr <= @return_type
    // \Note: If @return_type == @SELF_TYPE, then @expr->get_type()
    //    should also be @SELF_TYPE, and that the last expression
    //    in @expr should always be 'self'.
    if (!is_subclass(expr->get_type(), return_type)) {
        semant_error(cur_class) << "::" << name->get_string()
            << ": Method body type does not match" 
            << ", expected: " << return_type->get_string()  << endl;
        THROW_ERR_SCOPE_TYPE;
    }

    cur_symtab->exitscope();
}

void formal_class::analyze() {
    // Check duplicate 
    if (cur_symtab->probe(name)) {
        semant_error(cur_class) << ": Duplicate formal parameter: " << name->get_string() << endl;
        THROW_ERR_SCOPE_TYPE;
    }

    if (name == self) {
        semant_error(cur_class) << ": formal parameter cannot be self"
            << endl;
        THROW_ERR_SCOPE_TYPE;
    }
    // Type of formal parameter cannot be `SELF_TYPE`
    if (type_decl == SELF_TYPE) {
        semant_error(cur_class) << "Formal parameter: " << name->get_string() 
            << " cannot be SELF_TYPE" << endl;
        THROW_ERR_SCOPE_TYPE;
    }
    // Check type exists
    if (!type_exists(type_decl, false)) {
        semant_error(cur_class) << " Formal parameter: " << name->get_string()
            << " has unknown type" << endl;
        THROW_ERR_SCOPE_TYPE;
    }

    cur_symtab->addid(name, type_decl);
}

void branch_class::analyze() {

    cur_symtab->enterscope();

    if (!type_exists(type_decl)) {
        THROW_ERR_SCOPE_TYPE;
    }

    cur_symtab->addid(name, type_decl);

    expr->analyze();
    cur_symtab->exitscope();
}

void assign_class::analyze() {
    // Cannot assign to `self`
    if (name == self) {
        semant_error(cur_class) << ": Cannot assign value to `self`" << endl;
        THROW_ERR_SCOPE_TYPE;
    }

    // First find in @cur_symtab
    Symbol type_decl = cur_symtab->lookup(name);
    // If not found, try find in the attributes of current class
    // and all its parent classes.
    if (!type_decl) {
        Attribute attr = cur_class->get_attr(name);

        if (!attr) {
            semant_error(cur_class) << ": identifier " << name->get_string() 
                << " not found" << endl;
            THROW_ERR_SCOPE_TYPE;
        }
        type_decl = attr->get_type_decl();
    }

    expr->analyze();
    // @type_decl and @expr->get_type() can be SELF_TYPE
    if (!is_subclass(expr->get_type(), type_decl)) {
        semant_error(cur_class) << ": initializer type does not match "
            << " identifier " << name->get_string() << " type: "
            << type_decl->get_string() << endl;
        THROW_ERR_SCOPE_TYPE;
    }

    type = expr->get_type();
}

void static_dispatch_class::analyze() {
    expr->analyze();
    
    Class_ target_class = class_map.lookup(type_name);
    // check @type_name actually exists and is not `SELF_TYPE`
    // \Note: @type_name is the static type `T` in `expr@T.f(...)`
    //      Therefore it cannot be `SELF_TYPE`
    if (!target_class || (type_name == SELF_TYPE)) {
        semant_error(cur_class) << ": static dispatch type: "
            << type_name->get_string() << " error" << endl;
        THROW_ERR_SCOPE_TYPE;
    }
    // check @expr is a subclass of @type_name
    if (!is_subclass(expr->get_type(), type_name)) {
        semant_error(cur_class) << ": caller type does not match static dispatch type: " << type_name->get_string() << endl;
        THROW_ERR_SCOPE_TYPE;
    }

    Method method = target_class->get_method(name);
    // check @method specified by @name actually exists
    if (!method) {
        semant_error(target_class) << ": Dispatch to undefined method "
            << name << "." << endl;
        THROW_ERR_SCOPE_TYPE;
    }

    Formals formals = method->get_formals();
    // formal params and actual args should be of the same length
    if (formals->len() != actual->len()) {
        semant_error(target_class) << ": Method: "
            << name << " number of arguments mismatch" << endl;
        THROW_ERR_SCOPE_TYPE;
    }

    // type of the @arg_expr should <= that of the corresponding formal param
    for (unsigned i = actual->first(); actual->more(i); i = actual->next(i)) {
        Expression arg_expr = actual->nth(i);
        arg_expr->analyze();

        Formal formal = formals->nth(i);
        if (!is_subclass(arg_expr->get_type(), formal->get_type_decl())) {
            semant_error(target_class) << ": Method: "
                << name << ": " << formal->get_name()->get_string()
                << " argument type does not match" << endl;
            THROW_ERR_SCOPE_TYPE;
        }
    }

    Symbol return_type = method->get_return_type();
    if (return_type == SELF_TYPE) {
        // CANNOT use eval_self_type() here! The fact that @return_type
        // is `SELF_TYPE` here has nothing to do with @cur_class. Instead,
        // the @return_type should bind to @expr that calls this method.
        return_type = expr->get_type();
    }

    type = return_type;
}

void dispatch_class::analyze() {
    expr->analyze();

    Symbol evaled_expr_type = expr->get_type();
    // - type_exists(@evaled_expr_type) should always be true because this 
    //    variable is obtained from expr->get_type(). If the type does not 
    //    exist, an error should be thrown at the @expr->analyze() stage.
    // - Need to evaluate @evaled_expr_type to an actual type here.
    evaled_expr_type = (evaled_expr_type == SELF_TYPE) ? 
                        cur_class->get_name() : evaled_expr_type;
    
    Class_ target_class = class_map.lookup(evaled_expr_type);
    Method method = target_class->get_method(name);
    
    // check @method specified by @name actually exists
    if (!method) {
        semant_error(target_class) << ": Dispatch to undefined method "
            << name << "." << endl;
        THROW_ERR_SCOPE_TYPE;
    }

    Formals formals = method->get_formals();
    // formal params and actual args should be of the same length
    if (formals->len() != actual->len()) {
        semant_error(target_class) << ": Method: "
            << name << " number of arguments mismatch" << endl;
        THROW_ERR_SCOPE_TYPE;
    }

    // type of the @arg_expr should <= that of the corresponding formal param
    for (unsigned i = actual->first(); actual->more(i); i = actual->next(i)) {
        Expression arg_expr = actual->nth(i);
        arg_expr->analyze();

        Formal formal = formals->nth(i);
        if (!is_subclass(arg_expr->get_type(), formal->get_type_decl())) {
            semant_error(target_class) << ": Method: "
                << name << ": " << formal->get_name()->get_string()
                << " argument type does not match" << endl;
            THROW_ERR_SCOPE_TYPE;
        }
    }

    Symbol return_type = method->get_return_type();
    if (return_type == SELF_TYPE) {
        // CANNOT use eval_self_type() here! The fact that @return_type
        // is `SELF_TYPE` here has nothing to do with @cur_class. Instead,
        // the @return_type should bind to @expr that calls this method.
        return_type = expr->get_type();
    }

    type = return_type;
}

void cond_class::analyze() {
    pred->analyze();
    if (pred->get_type() != Bool) {
        THROW_ERR_SCOPE_TYPE;
    }

    then_exp->analyze();
    else_exp->analyze();

    type = lub(then_exp->get_type(), else_exp->get_type());
    // cout << type->get_string() << endl;
    if (!type) {
        semant_error(cur_class) << ": No common ancestor type found for condition expression" << endl;
        THROW_ERR_SCOPE_TYPE;
    }
}

void loop_class::analyze() {
    pred->analyze();
    if (pred->get_type() != Bool) {
        THROW_ERR_SCOPE_TYPE;
    }

    body->analyze();
    type = Object;
}

void typcase_class::analyze() {
    expr->analyze();

    type = NULL;
    cur_symtab->enterscope();
    std::set<Symbol> seen_types;
    for (unsigned i = cases->first(); cases->more(i); i = cases->next(i)) {
        Case _case = cases->nth(i);
        Symbol case_type = _case->get_type_decl();
        if (seen_types.count(case_type)) {
            semant_error(cur_class) << ": duplicate branch binding type: "
                << case_type->get_string() << endl;
            THROW_ERR_SCOPE_TYPE;
        }
        seen_types.insert(case_type);

        _case->analyze();

        type = type ? lub(_case->get_expr_type(), type) : _case->get_expr_type();
        if (!type) {
            semant_error(cur_class) << ": No common ancestor type found for type cases expression" << endl;

            THROW_ERR_SCOPE_TYPE;
        }
    }
    cur_symtab->exitscope();
}

void block_class::analyze() {
    Symbol final_type = NULL;
    for (unsigned i = body->first(); body->more(i); i = body->next(i)) {
        Expression expr = body->nth(i);

        expr->analyze();
        final_type = expr->get_type();
    }
    // For atomicity
    type = final_type;
}

void let_class::analyze() {
    // Check @identifier name is not `self`
    if (strcmp(identifier->get_string(), self->get_string()) == 0) {
        semant_error(cur_class) << ": Let cannot have `self` as identifier" << endl;
        THROW_ERR_SCOPE_TYPE;
    }

    // Check type exists
    if (!type_exists(type_decl)) {
        semant_error(cur_class) << ": Let: id type: " 
            << type_decl->get_string() << " does not exist" << endl;
        THROW_ERR_SCOPE_TYPE;
    }
    // Check @init_type <= @type_decl, if @init_type != @No_type
    // \Note: @init_type == @No_type iff. @init is an instance of no_expr_class
    init->analyze();
    Symbol init_type = init->get_type();
    if ((init_type != No_type) && (!is_subclass(init_type, type_decl))) {
        semant_error(cur_class) << ": init type does not match" 
            << ", id: " << identifier->get_string() 
            << ", expect: " << type_decl->get_string() << endl;
        THROW_ERR_SCOPE_TYPE;
    }

    cur_symtab->enterscope();
    cur_symtab->addid(identifier, type_decl);

    body->analyze();
    type = body->get_type();

    cur_symtab->exitscope();
}

void _analyze_binary_arithmetic_expr(Expression e1, Expression e2, Symbol& type) {
    e1->analyze();
    e2->analyze();

    if ((e1->get_type() != Int) || (e2->get_type() != Int)) {
        THROW_ERR_SCOPE_TYPE;
    }

    type = Int;
}

void plus_class::analyze() {
    _analyze_binary_arithmetic_expr(e1, e2, type);
}

void sub_class::analyze() {
    _analyze_binary_arithmetic_expr(e1, e2, type);
}

void mul_class::analyze() {
    _analyze_binary_arithmetic_expr(e1, e2, type);
}

void divide_class::analyze() {
    _analyze_binary_arithmetic_expr(e1, e2, type);
}

void neg_class::analyze() {
    e1->analyze();

    if (e1->get_type() != Int) {
        semant_error(cur_class) << ": '-' must be applied on Int object"
            << endl;
        THROW_ERR_SCOPE_TYPE;
    }

    type = Int;
}

void _analyze_binary_cmp_expr(Expression e1, Expression e2, Symbol& type) {
    e1->analyze();
    e2->analyze();

    if ((e1->get_type() != Int) || (e2->get_type() != Int)) {
        THROW_ERR_SCOPE_TYPE;
    }

    type = Bool;
}

void lt_class::analyze() {
    _analyze_binary_cmp_expr(e1, e2, type);
}

void eq_class::analyze() {
    // If either <expr1> or <expr2> has static type Int, Bool, or String,
    // then the other must have the same static type. Any other types, 
    // including SELF TYPE, may be freely compared. On non-basic objects, 
    // equality simply checks for pointer equality (i.e., whether the 
    // memory addresses of the objects are the same). Equality is defined
    // for void.
    e1->analyze();
    e2->analyze();
    if ((e1->get_type() == Int) || 
        (e1->get_type() == Bool) ||
        (e1->get_type() == Str)) {
        if (e1->get_type() != e2->get_type()) {
            THROW_ERR_SCOPE_TYPE;
        }
    }

    type = Bool;
    
    // _analyze_binary_cmp_expr(e1, e2, type);
}

void leq_class::analyze() {
    _analyze_binary_cmp_expr(e1, e2, type);
}

void comp_class::analyze() {
    e1->analyze();

    if (e1->get_type() != Bool) {
        THROW_ERR_SCOPE_TYPE;
    }

    type = Bool;
}

void int_const_class::analyze() {
    type = Int;
}

void bool_const_class::analyze() {
    type = Bool;
}

void string_const_class::analyze() {
    type = Str;
}

void new__class::analyze() {
    type = type_name;
}

void isvoid_class::analyze() {
    e1->analyze();
    type = Bool;
}

void no_expr_class::analyze() {
    type = No_type;
}

void object_class::analyze() {
    type = cur_symtab->lookup(name);

    if (!type) {
        Attribute attr = cur_class->get_attr(name);

        if (!attr) {
            semant_error(cur_class) << ": identifier " << name->get_string() 
                << " not found" << endl;
            THROW_ERR_SCOPE_TYPE;
        }
        type = attr->get_type_decl();
    }
}
/*   This is the entry point to the semantic checker.

     Your checker should do the following two things:

     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')

     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */
void program_class::semant()
{
    initialize_constants();
    install_basic_classes();

    cur_symtab = new symtab_type();
    cur_symtab->enterscope();
    cur_symtab->addid(self, SELF_TYPE);

    try {
        fill_class_map(classes);
        fill_inheritence_graph(classes);
        inh_graph.validate();
    
    }
    catch (int) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);

    }

    try {
        analyze();
    }
    catch (int) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }
    cur_symtab->exitscope();
    delete cur_symtab;
    
    /* some semantic analysis code may go here */

    if (err_logger.errors()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }
}
