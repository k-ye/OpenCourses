
//**************************************************************
//
// Code generator SKELETON
//
// Read the comments carefully. Make sure to
//    initialize the base class tags in
//       `CgenClassTable::CgenClassTable'
//
//    Add the label for the dispatch tables to
//       `IntEntry::code_def'
//       `StringEntry::code_def'
//       `BoolConst::code_def'
//
//    Add code to emit everyting else that is needed
//       in `CgenClassTable::code'
//
//
// The files as provided will produce code to begin the code
// segments, declare globals, and emit constants.  You must
// fill in the rest.
//
//**************************************************************

#include "cgen.h"
#include "cgen_gc.h"

#include <algorithm>      // std::max
#include <utility>        // std::pair
#include <vector>

extern void emit_string_constant(ostream& str, char *s);
extern int cgen_debug;

//
// Three symbols from the semantic analyzer (semant.cc) are used.
// If e : No_type, then no code is generated for e.
// Special code is generated for new SELF_TYPE.
// The name "self" also generates code different from other references.
//
//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
Symbol 
       arg,
       arg2,
       Bool,
       concat,
       cool_abort,
       copy_,
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
  copy_        = idtable.add_string("copy");
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

static char *gc_init_names[] =
  { "_NoGC_Init", "_GenGC_Init", "_ScnGC_Init" };
static char *gc_collect_names[] =
  { "_NoGC_Collect", "_GenGC_Collect", "_ScnGC_Collect" };


//  BoolConst is a class that implements code generation for operations
//  on the two booleans, which are given global names here.
BoolConst falsebool(FALSE);
BoolConst truebool(TRUE);

//*********************************************************
//
// Define method for code generation
//
// This is the method called by the compiler driver
// `cgtest.cc'. cgen takes an `ostream' to which the assembly will be
// emmitted, and it passes this and the class list of the
// code generator tree to the constructor for `CgenClassTable'.
// That constructor performs all of the work of the code
// generator.
//
//*********************************************************

CgenClassTableP class_table;

void program_class::cgen(ostream &os) 
{
  // spim wants comments to start with '#'
  os << "# start of generated code\n";

  initialize_constants();
  // CgenClassTable *codegen_classtable = new CgenClassTable(classes,os);
  class_table = new CgenClassTable(os);
  
  class_table->enterscope();
  class_table->work(classes);
  class_table->exitscope();

  delete class_table;
  os << "\n# end of generated code\n";
}


//////////////////////////////////////////////////////////////////////////////
//
//  emit_* procedures
//
//  emit_X  writes code for operation "X" to the output stream.
//  There is an emit_X for each opcode X, as well as emit_ functions
//  for generating names according to the naming conventions (see emit.h)
//  and calls to support functions defined in the trap handler.
//
//  Register names and addresses are passed as strings.  See `emit.h'
//  for symbolic names you can use to refer to the strings.
//
//////////////////////////////////////////////////////////////////////////////

static void emit_load(char *dest_reg, int offset, char *source_reg, ostream& s)
{
  s << LW << dest_reg << " " << offset * WORD_SIZE << "(" << source_reg << ")" 
    << endl;
}

static void emit_store(char *source_reg, int offset, char *dest_reg, ostream& s)
{
  s << SW << source_reg << " " << offset * WORD_SIZE << "(" << dest_reg << ")"
      << endl;
}

static void emit_load_imm(char *dest_reg, int val, ostream& s)
{ s << LI << dest_reg << " " << val << endl; }

static void emit_load_address(char *dest_reg, char *address, ostream& s)
{ s << LA << dest_reg << " " << address << endl; }

static void emit_partial_load_address(char *dest_reg, ostream& s)
{ s << LA << dest_reg << " "; }

static void emit_load_bool(char *dest, const BoolConst& b, ostream& s)
{
  emit_partial_load_address(dest,s);
  b.code_ref(s);
  s << endl;
}

static void emit_load_string(char *dest, StringEntry *str, ostream& s)
{
  emit_partial_load_address(dest,s);
  str->code_ref(s);
  s << endl;
}

static void emit_load_int(char *dest, IntEntry *i, ostream& s)
{
  emit_partial_load_address(dest,s);
  i->code_ref(s);
  s << endl;
}

static void emit_move(char *dest_reg, char *source_reg, ostream& s)
{ s << MOVE << dest_reg << " " << source_reg << endl; }

static void emit_neg(char *dest, char *src1, ostream& s)
{ s << NEG << dest << " " << src1 << endl; }

static void emit_add(char *dest, char *src1, char *src2, ostream& s)
{ s << ADD << dest << " " << src1 << " " << src2 << endl; }

static void emit_addu(char *dest, char *src1, char *src2, ostream& s)
{ s << ADDU << dest << " " << src1 << " " << src2 << endl; }

static void emit_addiu(char *dest, char *src1, int imm, ostream& s)
{ s << ADDIU << dest << " " << src1 << " " << imm << endl; }

static void emit_div(char *dest, char *src1, char *src2, ostream& s)
{ s << DIV << dest << " " << src1 << " " << src2 << endl; }

static void emit_mul(char *dest, char *src1, char *src2, ostream& s)
{ s << MUL << dest << " " << src1 << " " << src2 << endl; }

static void emit_sub(char *dest, char *src1, char *src2, ostream& s)
{ s << SUB << dest << " " << src1 << " " << src2 << endl; }

static void emit_sll(char *dest, char *src1, int num, ostream& s)
{ s << SLL << dest << " " << src1 << " " << num << endl; }

static void emit_jalr(char *dest, ostream& s)
{ s << JALR << "\t" << dest << endl; }

static void emit_jal(char *address,ostream &s)
{ s << JAL << address << endl; }

static void emit_return(ostream& s)
{ s << RET << endl; }

static void emit_gc_assign(ostream& s)
{ s << JAL << "_GenGC_Assign" << endl; }

static void emit_disptable_ref(Symbol sym, ostream& s)
{  s << sym << DISPTAB_SUFFIX; }

static void emit_init_ref(Symbol sym, ostream& s)
{ s << sym << CLASSINIT_SUFFIX; }

static void emit_label_ref(int l, ostream &s)
{ s << "label" << l; }

static void emit_protobj_ref(Symbol sym, ostream& s)
{ s << sym << PROTOBJ_SUFFIX; }

static void emit_method_ref(Symbol classname, Symbol methodname, ostream& s)
{ s << classname << METHOD_SEP << methodname; }

static void emit_label_def(int l, ostream &s)
{
  emit_label_ref(l,s);
  s << ":" << endl;
}

static void emit_jal(Symbol classname, Symbol methodname, ostream& s)
{
  s << JAL; emit_method_ref(classname, methodname, s); s << endl;
}

static void emit_jal_init(Symbol classname, ostream& s)
{
  s << JAL; emit_init_ref(classname, s); s << endl;
}

static void emit_beqz(char *source, int label, ostream &s)
{
  s << BEQZ << source << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_beq(char *src1, char *src2, int label, ostream &s)
{
  s << BEQ << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_bne(char *src1, char *src2, int label, ostream &s)
{
  s << BNE << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_bleq(char *src1, char *src2, int label, ostream &s)
{
  s << BLEQ << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_blt(char *src1, char *src2, int label, ostream &s)
{
  s << BLT << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_blti(char *src1, int imm, int label, ostream &s)
{
  s << BLT << src1 << " " << imm << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_bgti(char *src1, int imm, int label, ostream &s)
{
  s << BGT << src1 << " " << imm << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_branch(int l, ostream& s)
{
  s << BRANCH;
  emit_label_ref(l,s);
  s << endl;
}

//
// Push a register on the stack. The stack grows towards smaller addresses.
//
static void emit_push(char *reg, ostream& str)
{
  emit_store(reg,0,SP,str);
  emit_addiu(SP,SP,-4,str);
}

static void emit_push(char *reg, ostream& str, int& tmp_offs)
{
  emit_push(reg, str);
  --tmp_offs;
}

//
// Pop the stack
//
static void emit_pop(ostream& str)
{
  emit_addiu(SP, SP, 4, str);
}

static void emit_pop(ostream& str, int& tmp_offs)
{
  emit_pop(str);
  ++tmp_offs;
}
//
// Fetch the integer value in an Int object.
// Emits code to fetch the integer value of the Integer object pointed
// to by register source into the register dest
//
static void emit_fetch_int(char *dest, char *source, ostream& s)
{ emit_load(dest, DEFAULT_OBJFIELDS, source, s); }

//
// Emits code to store the integer value contained in register source
// into the Integer object pointed to by dest.
//
static void emit_store_int(char *source, char *dest, ostream& s)
{ emit_store(source, DEFAULT_OBJFIELDS, dest, s); }

//
// Fetch the integer value (0/1) in an Bool object.
// Emits code to fetch the integer value of the Bool object pointed
// to by register source into the register dest
//
static void emit_fetch_bool(char *dest, char *source, ostream& s)
{ emit_load(dest, DEFAULT_OBJFIELDS, source, s); }

//
// Emits code to store the integer value contained in register source
// into the Integer object pointed to by dest.
//
static void emit_store_bool(char *source, char *dest, ostream& s)
{ emit_store(source, DEFAULT_OBJFIELDS, dest, s); }


static void emit_test_collector(ostream &s)
{
  emit_push(ACC, s);
  emit_move(ACC, SP, s); // stack end
  emit_move(A1, ZERO, s); // allocate nothing
  s << JAL << gc_collect_names[cgen_Memmgr] << endl;
  emit_addiu(SP,SP,4,s);
  emit_load(ACC,0,SP,s);
}

static void emit_gc_check(char *source, ostream &s)
{
  if (source != (char*)A1) emit_move(A1, source, s);
  s << JAL << "_gc_check" << endl;
}


///////////////////////////////////////////////////////////////////////////////
//
// coding strings, ints, and booleans
//
// Cool has three kinds of constants: strings, ints, and booleans.
// This section defines code generation for each type.
//
// All string constants are listed in the global "stringtable" and have
// type StringEntry.  StringEntry methods are defined both for String
// constant definitions and references.
//
// All integer constants are listed in the global "inttable" and have
// type IntEntry.  IntEntry methods are defined for Int
// constant definitions and references.
//
// Since there are only two Bool values, there is no need for a table.
// The two booleans are represented by instances of the class BoolConst,
// which defines the definition and reference methods for Bools.
//
///////////////////////////////////////////////////////////////////////////////

//
// Strings
//
void StringEntry::code_ref(ostream& s)
{
  s << STRCONST_PREFIX << index;
}

//
// Emit code for a constant String.
// You should fill in the code naming the dispatch table.
//

void str_code_def_body(ostream& os, class_tag_t tag, int len, const char* str)
{
  IntEntryP lensym = inttable.add_int(len);

  // class tag
  os << WORD << tag << endl;
  // size
  os << WORD << (DEFAULT_OBJFIELDS + STRING_SLOTS + (len+4)/4) << endl;
  // dispatch table
  os << WORD; emit_disptable_ref(Str, os); os << endl;
  // string length
  os << WORD;  lensym->code_ref(os);  os << endl; 
  // string literal
  emit_string_constant(os, (char *)str);
  // align to word
  os << ALIGN;
}

void StringEntry::code_def(ostream& s, int stringclasstag)
{
  // IntEntryP lensym = inttable.add_int(len);

  // Add -1 eye catcher
  s << WORD << "-1" << endl;

  code_ref(s);  s  << LABEL;                                             // label
  str_code_def_body(s, stringclasstag, len, str);
  //     << WORD << stringclasstag << endl                                 // tag
  //     << WORD << (DEFAULT_OBJFIELDS + STRING_SLOTS + (len+4)/4) << endl // size
  //     << WORD;


  /***** Add dispatch information for class String ******/

  //     s << endl;                                              // dispatch table
  //     s << WORD;  lensym->code_ref(s);  s << endl;            // string length
  // emit_string_constant(s,str);                                // ascii string
  // s << ALIGN;                                                 // align to word
}

//
// StrTable::code_string
// Generate a string object definition for every string constant in the 
// stringtable.
//
void StrTable::code_string_table(ostream& s, int stringclasstag)
{  
  for (List<StringEntry> *l = tbl; l; l = l->tl())
    l->hd()->code_def(s,stringclasstag);
}

//
// Ints
//
void IntEntry::code_ref(ostream &s)
{
  s << INTCONST_PREFIX << index;
}

//
// Emit code for a constant Integer.
// You should fill in the code naming the dispatch table.
//

void int_code_def_body(ostream& os, class_tag_t tag, const char* val)
{
  // class tag
  os << WORD << tag << endl;
  // size
  os << WORD << (DEFAULT_OBJFIELDS + INT_SLOTS) << endl;
  // dispatch table 
  os << WORD; emit_disptable_ref(Int, os); os << endl;
  // value
  os << WORD << val << endl;
}

void IntEntry::code_def(ostream &s, int intclasstag)
{
  // Add -1 eye catcher
  s << WORD << "-1" << endl;

  code_ref(s);  s << LABEL;                                // label
  int_code_def_body(s, intclasstag, str);
  //     << WORD << intclasstag << endl                      // class tag
  //     << WORD << (DEFAULT_OBJFIELDS + INT_SLOTS) << endl  // object size
  //     << WORD; 
  
  /***** Add dispatch information for class Int ******/
  //     s << 0;
  //     s << endl;                                          // dispatch table
  //     s << WORD << str << endl;                           // integer value
}


//
// IntTable::code_string_table
// Generate an Int object definition for every Int constant in the
// inttable.
//
void IntTable::code_string_table(ostream &s, int intclasstag)
{
  for (List<IntEntry> *l = tbl; l; l = l->tl())
    l->hd()->code_def(s, intclasstag);
}


//
// Bools
//
BoolConst::BoolConst(int i) : val(i) { assert(i == 0 || i == 1); }

void BoolConst::code_ref(ostream& s) const
{
  s << BOOLCONST_PREFIX << val;
}
  
//
// Emit code for a constant Bool.
// You should fill in the code naming the dispatch table.
//

void bool_code_def_body(ostream& os, class_tag_t tag, int val)
{
  // class tag
  os << WORD << tag << endl;
  // size
  os << WORD << (DEFAULT_OBJFIELDS + BOOL_SLOTS) << endl;
  // dispatch table (empty)
  os << WORD; emit_disptable_ref(Bool, os); os << endl;
  // value
  os << WORD << val << endl;
}
void BoolConst::code_def(ostream& s, int boolclasstag)
{
  // Add -1 eye catcher
  s << WORD << "-1" << endl;

  code_ref(s);  s << LABEL;                                  // label
  bool_code_def_body(s, boolclasstag, val);
  //     << WORD << boolclasstag << endl                       // class tag
  //     << WORD << (DEFAULT_OBJFIELDS + BOOL_SLOTS) << endl   // object size
  //     << WORD;

  /***** Add dispatch information for class Bool ******/
  //   s << 0;
  //     s << endl;                                            // dispatch table
  //     s << WORD << val << endl;                             // value (0 or 1)
}

//////////////////////////////////////////////////////////////////////////////
//
//  CgenClassTable methods
//
//////////////////////////////////////////////////////////////////////////////

typedef std::pair<int, char*> offs_reg_t;
typedef offs_reg_t * offs_reg_ptr_t;

typedef SymbolTable<Symbol, offs_reg_t> env_t;
typedef env_t* env_ptr_t;

typedef SymbolTable<Symbol, Entry> symtab_t;

static env_ptr_t cur_env;
static symtab_t* cur_symtab;
static CgenNodeP cur_class;

//***************************************************
//
//  Emit code to start the .data segment and to
//  declare the global names.
//
//***************************************************


static std::map<class_tag_t, StringEntryP> class_nametab;

namespace 
{

class_tag_t next_classtag = 0;
static class_tag_t get_next_classtag() 
{
  return next_classtag++;
}

int next_label = 0;
static int get_next_label()
{
  return next_label++;
}

static bool is_real_class(CgenNode* nd)
{
  Symbol name = nd->get_name();

  return (name != No_class) && (name != SELF_TYPE) && (name != prim_slot);
}

}; // namespace anonymous

CgenClassTable::CgenClassTable(ostream& s) : nds(NULL) , str(s)
{
}

void CgenClassTable::work(Classes classes)
{
   // intclasstag    = get_next_classtag() /* Change to your Int class tag here */;
   // boolclasstag   = get_next_classtag() /* Change to your Bool class tag here */;
   // stringclasstag = get_next_classtag() /* Change to your String class tag here */;

   if (cgen_debug) cout << "Building CgenClassTable" << endl;
   install_basic_classes();
   install_classes(classes);
   build_inheritance_tree();

   if (cgen_debug) cout << "Assigning class tags" << endl;
   assign_class_tags();

   // Need to calculate class layout first before analyze(),
   // because the latter uses the _attrs_layout and _meths_layout
   if (cgen_debug) cout << "Calculating classes layout" << endl;
   calc_classes_layout();
   
   if (cgen_debug) cout << "Analyzing the types" << endl;
   analyze();   // generate types

   code();
   
}


void CgenClassTable::install_basic_classes()
{

// The tree package uses these globals to annotate the classes built below.
  //curr_lineno  = 0;
  Symbol filename = stringtable.add_string("<basic class>");

//
// A few special class names are installed in the lookup table but not
// the class list.  Thus, these classes exist, but are not part of the
// inheritance hierarchy.
// No_class serves as the parent of Object and the other special classes.
// SELF_TYPE is the self class; it cannot be redefined or inherited.
// prim_slot is a class known to the code generator.
//
  addid(No_class,
    new CgenNode(class_(No_class,No_class,nil_Features(),filename),
                Basic,this));
  addid(SELF_TYPE,
    new CgenNode(class_(SELF_TYPE,No_class,nil_Features(),filename),
                Basic,this));
  addid(prim_slot,
    new CgenNode(class_(prim_slot,No_class,nil_Features(),filename),
                Basic,this));

// 
// The Object class has no parent class. Its methods are
//        cool_abort() : Object    aborts the program
//        type_name() : Str        returns a string representation of class name
//        copy() : SELF_TYPE       returns a copy of the object
//
// There is no need for method bodies in the basic classes---these
// are already built in to the runtime system.
//
  install_class(
   new CgenNode(
    class_(Object, 
       No_class,
       append_Features(
           append_Features(
           single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
           single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
           single_Features(method(copy_, nil_Formals(), SELF_TYPE, no_expr()))),
       filename),
    Basic,this));

// 
// The IO class inherits from Object. Its methods are
//        out_string(Str) : SELF_TYPE          writes a string to the output
//        out_int(Int) : SELF_TYPE               "    an int    "  "     "
//        in_string() : Str                    reads a string from the input
//        in_int() : Int                         "   an int     "  "     "
//
   install_class(
    new CgenNode(
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
       filename),       
    Basic,this));

//
// The Int class has no methods and only a single attribute, the
// "val" for the integer. 
//
   install_class(
    new CgenNode(
     class_(Int, 
        Object,
            single_Features(attr(val, prim_slot, no_expr())),
        filename),
     Basic,this));

//
// Bool also has only the "val" slot.
//
    install_class(
     new CgenNode(
      class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename),
      Basic,this));

//
// The class Str has a number of slots and operations:
//       val                                  ???
//       str_field                            the string itself
//       length() : Int                       length of the string
//       concat(arg: Str) : Str               string concatenation
//       substr(arg: Int, arg2: Int): Str     substring
//       
   install_class(
    new CgenNode(
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
         filename),
        Basic,this));

}

// CgenClassTable::install_class
// CgenClassTable::install_classes
//
// install_classes enters a list of classes in the symbol table.
//
void CgenClassTable::install_class(CgenNodeP nd)
{
  Symbol name = nd->get_name();

  if (probe(name))
    {
      return;
    }

  // The class name is legal, so add it to the list of classes
  // and the symbol table.
  nds = new List<CgenNode>(nd,nds);
  addid(name,nd);
}

void CgenClassTable::install_classes(Classes cs)
{
  for(int i = cs->first(); cs->more(i); i = cs->next(i))
    install_class(new CgenNode(cs->nth(i),NotBasic,this));
}

//
// CgenClassTable::build_inheritance_tree
//
void CgenClassTable::build_inheritance_tree()
{
  for(List<CgenNode> *l = nds; l; l = l->tl())
      set_relations(l->hd());
}

//
// CgenClassTable::set_relations
//
// Takes a CgenNode and locates its, and its parent's, inheritance nodes
// via the class table.  Parent and child pointers are added as appropriate.
//
void CgenClassTable::set_relations(CgenNodeP nd)
{
  CgenNode *parent_node = probe(nd->get_parent());
  nd->set_parentnd(parent_node);
  parent_node->add_child(nd);
}

void CgenNode::add_child(CgenNodeP n)
{
  children = new List<CgenNode>(n,children);
}

void CgenNode::set_parentnd(CgenNodeP p)
{
  assert(parentnd == NULL);
  assert(p != NULL);
  parentnd = p;
}

// Find the LUB of two classes

CgenNodeP CgenClassTable::lub(CgenNodeP a, CgenNodeP b) 
{
  if (!a || !b) return NULL;
  
  if (a == b) return a;
  else if (a->get_name() == SELF_TYPE) a = cur_class;
  else if (b->get_name() == SELF_TYPE) b = cur_class;

  CgenNodeP a_copy(a); CgenNodeP b_copy(b);
  unsigned ha(0), hb(0);
  
  while (a->get_name() != No_class) {
    a = a->get_parentnd();
    ++ha;
  }

  while (b->get_name() != No_class) {
    b = b->get_parentnd();
    ++hb;
  }

  unsigned delta;
  if (ha < hb) {
    a = b_copy;
    b = a_copy;
    delta = hb - ha;
  }
  else {
    a = a_copy;
    b = b_copy;
    delta = ha - hb;
  }

  for (unsigned i = 0; i < delta; ++i) {
    a = a->get_parentnd();
  }

  while (a != b) {
    a = a->get_parentnd();
    b = b->get_parentnd();
  }

  a = (a->get_name() == No_class) ? NULL : a;
  return a;
}

Symbol CgenClassTable::lub(Symbol a, Symbol b)
{
  if (!a || !b) return NULL;

  CgenNodeP a_nd = probe(a);
  CgenNodeP b_nd = probe(b);
  CgenNodeP res_nd = lub(a_nd, b_nd);
  return res_nd->get_name();
}

//
// Assign class tag and tag end to each class
// The effort is for cases testing
//

static void assign_class_tags_impl(CgenNodeP nd, std::map<CgenNodeP, bool>& visited)
{
  assert(visited.count(nd) == 0);

  visited.insert(std::make_pair(nd, true));

  nd->set_tag(get_next_classtag());
  // recursively set class tag for each subclass
  List<CgenNode> *l;
  CgenNodeP child_nd;
  
  for (l = nd->get_children(); l; l = l->tl()) {
    child_nd = l->hd();
    assign_class_tags_impl(child_nd, visited);
  }

  // Finally, obtain the class tag end
  nd->set_tag_end((next_classtag - 1));

  // cout << "  class: " << nd->get_name() 
  //     << ", tag: " << nd->tag() 
  //     << ", tag end: " << nd->tag_end() << endl;
  assert(nd->tag() <= nd->tag_end());
}

void CgenClassTable::assign_class_tags()
{
  std::map<CgenNodeP, bool> visited;
  
  assign_class_tags_impl(root(), visited);
 
  List<CgenNode> *l;
  CgenNodeP nd;
  unsigned num_classes = 0;

  for (l = nds; l; l = l->tl()) {
    nd = l->hd();

    if (is_real_class(nd)) {
      ++num_classes;
      // assign_class_tags_impl(nd, visited);
      class_nametab[nd->tag()] = stringtable.lookup_string(nd->get_name()->get_string());
    }
  }
  // 3 classes are dummy: No_class, SELF_TYPE, prim_slot
  assert(visited.size() == num_classes); 

  intclasstag = probe(Int)->tag();
  boolclasstag = probe(Bool)->tag();
  stringclasstag = probe(Str)->tag();
}

// 
// Calculate the memory layout of each class
//

void CgenClassTable::calc_classes_layout()
{
  List<CgenNode> *l;
  CgenNode* nd;
  
  for (l = nds; l; l = l->tl()) {
    nd = l->hd();

    if (is_real_class(nd)) {
      nd->calc_layout();
    }
  }
}

//
// Re-generate the type information of each expression 
// in the AST.
//
void CgenClassTable::analyze()
{
  cur_symtab = new symtab_t();
  cur_symtab->enterscope();
  cur_symtab->addid(self, SELF_TYPE);

  List<CgenNode> *l;
  CgenNodeP nd;

  for (l = nds; l; l = l->tl()) {
    nd = l->hd();

    if (is_real_class(nd)) {
      cur_class = nd;
      nd->analyze();
    }
  }

  cur_symtab->exitscope();
  delete cur_symtab;
}

//
// Emit all the assembly codes
//

void CgenClassTable::code()
{
  if (cgen_debug) cout << "coding global data" << endl;
  code_global_data();

  if (cgen_debug) cout << "choosing gc" << endl;
  code_select_gc();

  if (cgen_debug) cout << "coding constants" << endl;
  code_constants();

//                 Add your code to emit
//                   - prototype objects
//                   - class_nameTab
//                   - dispatch tables
//
  if (cgen_debug) cout << "coding " << CLASSNAMETAB << endl;
  code_class_nametab();

  if (cgen_debug) cout << "coding " << CLASSOBJTAB << endl;
  code_class_objtab();

  if (cgen_debug) cout << "coding dispatch tables" << endl;
  code_disptabs();
  
  if (cgen_debug) cout << "coding prototype objects" << endl;
  code_protobjs();
  

  if (cgen_debug) cout << "coding global text" << endl;
  code_global_text();

//                 Add your code to emit
//                   - object initializer
//                   - the class methods
//                   - etc...
  if (cgen_debug) cout << "coding class initializer and methods" << endl;
  code_classes();

}

void CgenClassTable::code_global_data()
{
  Symbol main    = idtable.lookup_string(MAINNAME);
  Symbol string  = idtable.lookup_string(STRINGNAME);
  Symbol integer = idtable.lookup_string(INTNAME);
  Symbol boolc   = idtable.lookup_string(BOOLNAME);

  str << "\t.data\n" << ALIGN;
  //
  // The following global names must be defined first.
  //
  str << GLOBAL << CLASSNAMETAB << endl;
  str << GLOBAL; emit_protobj_ref(main,str);    str << endl;
  str << GLOBAL; emit_protobj_ref(integer,str); str << endl;
  str << GLOBAL; emit_protobj_ref(string,str);  str << endl;
  str << GLOBAL; falsebool.code_ref(str);  str << endl;
  str << GLOBAL; truebool.code_ref(str);   str << endl;
  str << GLOBAL << INTTAG << endl;
  str << GLOBAL << BOOLTAG << endl;
  str << GLOBAL << STRINGTAG << endl;

  //
  // We also need to know the tag of the Int, String, and Bool classes
  // during code generation.
  //
  str << INTTAG << LABEL
      << WORD << intclasstag << endl;
  str << BOOLTAG << LABEL 
      << WORD << boolclasstag << endl;
  str << STRINGTAG << LABEL 
      << WORD << stringclasstag << endl;    
}

void CgenClassTable::code_select_gc()
{
  //
  // Generate GC choice constants (pointers to GC functions)
  //
  str << GLOBAL << "_MemMgr_INITIALIZER" << endl;
  str << "_MemMgr_INITIALIZER:" << endl;
  str << WORD << gc_init_names[cgen_Memmgr] << endl;
  str << GLOBAL << "_MemMgr_COLLECTOR" << endl;
  str << "_MemMgr_COLLECTOR:" << endl;
  str << WORD << gc_collect_names[cgen_Memmgr] << endl;
  str << GLOBAL << "_MemMgr_TEST" << endl;
  str << "_MemMgr_TEST:" << endl;
  str << WORD << (cgen_Memmgr_Test == GC_TEST) << endl;
}

//********************************************************
//
// Emit code to reserve space for and initialize all of
// the constants.  Class names should have been added to
// the string table (in the supplied code, is is done
// during the construction of the inheritance graph), and
// code for emitting string constants as a side effect adds
// the string's length to the integer table.  The constants
// are emmitted by running through the stringtable and inttable
// and producing code for each entry.
//
//********************************************************

void CgenClassTable::code_constants()
{
  //
  // Add constants that are required by the code generator.
  //
  stringtable.add_string("");
  inttable.add_string("0");

  stringtable.code_string_table(str, stringclasstag);
  inttable.code_string_table(str, intclasstag);
  code_bools(boolclasstag);
}

void CgenClassTable::code_bools(int boolclasstag)
{
  falsebool.code_def(str,boolclasstag);
  truebool.code_def(str,boolclasstag);
}

void CgenClassTable::code_class_nametab()
{
  // TODO: Should put these into a separate function 
  // that builds up @class_nametab.
  // class_nametab[intclasstag] = stringtable.lookup_string("Int");
  // class_nametab[boolclasstag] = stringtable.lookup_string("Bool");
  // class_nametab[stringclasstag] = stringtable.lookup_string("String");

  str << CLASSNAMETAB << LABEL;
  for (class_tag_t tag = 0; tag < (class_tag_t)class_nametab.size(); ++tag) {
    StringEntryP sym = class_nametab.at(tag);
    str << WORD; sym->code_ref(str);
    str << "\t# class name: " << sym->get_string() << ", class tag: " << tag; 
    str << endl;
  }

}

void CgenClassTable::code_class_objtab()
{
  str << CLASSOBJTAB << LABEL;
  for (class_tag_t tag = 0; tag < (class_tag_t)class_nametab.size(); ++tag)
  {
    StringEntryP sym = class_nametab.at(tag);
    str << "\t# class tag: " << tag << std::endl;
    str << WORD; emit_protobj_ref(sym, str); str << std::endl;
    str << WORD; emit_init_ref(sym, str); str << std::endl;
  }
}

void CgenClassTable::code_disptabs()
{
  List<CgenNode> *l;
  CgenNodeP nd;

  for (l = nds; l; l = l->tl()) {
    nd = l->hd();

    if (is_real_class(nd)) {
      nd->code_disptab(str);
    }
  }
}

void CgenClassTable::code_protobjs()
{
  List<CgenNode> *l;
  CgenNodeP nd;

  for (l = nds; l; l = l->tl()) {
    nd = l->hd();

    if (is_real_class(nd)) {
      nd->code_protobj(str);
    }
  }
}

//***************************************************
//
//  Emit code to start the .text segment and to
//  declare the global names.
//
//***************************************************

void CgenClassTable::code_global_text()
{
  str << GLOBAL << HEAP_START << endl
      << HEAP_START << LABEL 
      << WORD << 0 << endl
      << "\t.text" << endl
      << GLOBAL;
  emit_init_ref(idtable.add_string("Main"), str);
  str << endl << GLOBAL;
  emit_init_ref(idtable.add_string("Int"),str);
  str << endl << GLOBAL;
  emit_init_ref(idtable.add_string("String"),str);
  str << endl << GLOBAL;
  emit_init_ref(idtable.add_string("Bool"),str);
  str << endl << GLOBAL;
  emit_method_ref(idtable.add_string("Main"), idtable.add_string("main"), str);
  str << endl;
}

void CgenClassTable::code_classes()
{
  cur_env = new env_t();
  // cur_env->enterscope();
  // offs_reg_ptr_t self_offs_reg = new offs_reg_t(0, SELF);
  // cur_env->addid(self, self_offs_reg);

  List<CgenNode> *l;
  CgenNodeP nd;

  for (l = nds; l; l = l->tl()) {
    nd = l->hd();

    if (is_real_class(nd)) {
      // cout << "Coding for class: " << nd->get_name() << endl;
      cur_class = nd;
      nd->code_methods(str);
    }
  }

  // cur_env->exitscope();
  // delete self_offs_reg;
  delete cur_env;
}

CgenNodeP CgenClassTable::root()
{
   return probe(Object);
}


///////////////////////////////////////////////////////////////////////
//
// CgenNode methods
//
///////////////////////////////////////////////////////////////////////

CgenNode::CgenNode(Class_ nd, Basicness bstatus, CgenClassTableP cltab) :
   class__class((const class__class &) *nd),
   parentnd(NULL),
   children(NULL),
   basic_status(bstatus)
{ 
   stringtable.add_string(name->get_string());          // Add class name to string table

   // if (name == Int) _class_tag = intclasstag;
   // else if (name == Bool) _class_tag = boolclasstag;
   // else if (name == Str) _class_tag = stringclasstag;
   // else {
   //   _class_tag = get_next_classtag();
   // }
}

void CgenNode::calc_layout() {
  if (_has_calced_layout) return;

  if (parent != No_class) {
    parentnd->calc_layout();
  }

  _attrs_layout = parentnd->_attrs_layout;
  _meths_layout = parentnd->_meths_layout;
  _meth_class_map = parentnd->_meth_class_map;

  Feature feat;

  for (unsigned i = features->first(); features->more(i); i = features->next(i)) {
    feat = features->nth(i);

    if (feat->is_attr()) {
      Attribute attr = (Attribute)feat;
      _attrs_layout.push_back(attr);
    }
    else {
      Method meth = (Method)feat;

      if (_meth_class_map.count(meth->get_name()) == 0) {
        _meths_layout.push_back(meth);
      }
      _meth_class_map[meth->get_name()] = name;
    }
  }

  _has_calced_layout = true;
}

Attribute CgenNode::get_attr(const Symbol a_name) const
{
  for (unsigned i = 0; i < _attrs_layout.size(); ++i) {
    if (a_name == _attrs_layout[i]->get_name()) {
      return _attrs_layout[i];
    }
  }

  return NULL;
}

Method CgenNode::get_method(const Symbol m_name) const
{
  for (unsigned i = 0; i < _meths_layout.size(); ++i) {
    if (m_name == _meths_layout[i]->get_name()) {
      return _meths_layout[i];
    }
  }

  return NULL;
}
unsigned CgenNode::method_offset(const Symbol meth) const
{
  for (unsigned i = 0; i < _meths_layout.size(); ++i) {
    if (meth == _meths_layout[i]->get_name()) return i;
  }
  return (~0);
}

Symbol CgenNode::method_class(const Symbol meth) const
{
  return _meth_class_map.at(meth);
}

void CgenNode::code_disptab(ostream& os) const
{
  emit_disptable_ref(name, os); os << LABEL;

  Symbol meth_name;
  Symbol class_name;
  for (unsigned i = 0; i < _meths_layout.size(); ++i) {
    meth_name = _meths_layout[i]->get_name();
    class_name = _meth_class_map.at(meth_name);

    os << WORD;
    emit_method_ref(class_name, meth_name, os);
    os << endl;
  }
  os << endl;
}

void CgenNode::code_protobj(ostream& os) const
{
  // os << GLOBAL; emit_protobj_ref(name, os); os << endl;

  os << WORD << "-1" << endl;
  
  emit_protobj_ref(name, os); os << LABEL;

  if (name == Int) {
    int_code_def_body(os, _class_tag, "0");
  }
  else if (name == Bool) {
    bool_code_def_body(os, _class_tag, 0);
  }
  else if (name == Str) {
    str_code_def_body(os, _class_tag, 0, "");
  }
  else {
    // class tag
    os << WORD << _class_tag << "\t # class tag" << endl;
    // size
    os << WORD << (DEFAULT_OBJFIELDS + _attrs_layout.size()) << "\t # size" << endl;
    // dispatch table
    os << WORD; emit_disptable_ref(name, os); os << endl;

    Attribute attr;
    Symbol attr_type;
    for (unsigned i = 0; i < _attrs_layout.size(); ++i) {
      attr = _attrs_layout[i];
      attr_type = attr->get_type_decl();
      os << WORD;

      if (attr_type == Int) {
         IntEntryP default_int = inttable.lookup_string("0");
         default_int->code_ref(os);
      }
      else if (attr_type == Str) {
        StringEntryP default_str = stringtable.lookup_string("");
        default_str->code_ref(os);
      }
      else if (attr_type == Bool) {
        falsebool.code_ref(os);
      }
      else {
        os << 0;
      }
      os << "\t #" << attr->get_name()->get_string() << endl;
    }
  }

  os << endl;
}

static void emit_enter_func(ostream& s) {
  // \precondition: all actual arguments are pushed onto the stack, in reverse order
  // \precondition: $a0 should contain the self object upon entering the function

  emit_addiu(SP, SP, -12, s);
  emit_store(FP, 3, SP, s);
  emit_store(SELF, 2, SP, s);
  emit_store(RA, 1, SP, s);
  // new $fp = $sp + 12
  emit_addiu(FP, SP, 12, s);
  // let $s0 point to $a0
  emit_move(SELF, ACC, s);
}

static void emit_exit_func(unsigned num_formals, ostream& s) {
  // \precondition: $a0 should store the return value
  // \precondition: $sp == $fp + 12
  
  // restore $fp
  emit_load(FP, 3, SP, s);
  // restore $s0
  emit_load(SELF, 2, SP, s);
  // restore $ra
  emit_load(RA, 1, SP, s);
  // pop off the entire callee's AR
  emit_addiu(SP, SP, 4 * (num_formals + 3), s);
  // return to $ra
  emit_return(s);
}

void CgenNode::code_init(ostream& s) const
{
  emit_init_ref(name, s); s << LABEL;

  emit_enter_func(s);

  if (parent != No_class) {
    s << JAL; emit_init_ref(parent, s); s << endl;
  }

  // Initialize this class' unique attributes
  Expressions init_exprs = nil_Expressions();
  Expressions init_expr;
  Attribute attr_;
  Symbol init_type;
  
  for (unsigned i = features->first(); features->more(i); i = features->next(i)) {
    if (features->nth(i)->is_attr()) {
      attr_ = (Attribute)features->nth(i);
      init_type = attr_->get_init()->get_type();

      if (init_type == No_type)
      {
        attr_->get_init()->set_type(attr_->get_type_decl());
      }

      // if (init_type != No_type) {
      init_expr = single_Expressions( assign(attr_->get_name(), attr_->get_init()) );
      // be careful with the append orders!
      init_exprs = append_Expressions(init_exprs, init_expr);
      // }
    }
  }
  // the initialization block
  Expression init_block = block(init_exprs);
  int tmp_offs = -3;
  init_block->code(s, tmp_offs);
  // the return value is this initialized object, hence $a0 <- $s0
  emit_move(ACC, SELF, s);
  emit_exit_func(0, s);
}

void CgenNode::code_method(Method meth, ostream& s) const 
{
  // cout << "  Coding for method: " << meth->get_name() << endl;

  emit_method_ref(name, meth->get_name(), s); s << LABEL;

  emit_enter_func(s);
  
  // Create the environment for this method 
  // we want to mask the attributes when there are formal parameters
  // with the same names, hence we enter another scope
  cur_env->enterscope();
  
  Formals formals = meth->get_formals();
  Formal fm;
  unsigned num_args = formals->len();

  for (unsigned i = formals->first(); formals->more(i); i = formals->next(i)) {
    fm = formals->nth(i);
    // $fp is at the end of all the pushed args, hence +1!
    // offs_reg_ptr_t fm_offs_reg = new offs_reg_t(i + 1, FP);
    offs_reg_ptr_t fm_offs_reg = new offs_reg_t(num_args - i, FP);
    cur_env->addid(fm->get_name(), fm_offs_reg);
  }

  int tmp_offs = -3;
  meth->get_expr()->code(s, tmp_offs);

  emit_exit_func(formals->len(), s);

  cur_env->exitscope();
}

void CgenNode::code_methods(ostream& s) const 
{
  cur_env->enterscope();
  Attribute attr;
  // need to include ALL the attributes
  for (unsigned i = 0; i < _attrs_layout.size(); ++i) {
    attr = _attrs_layout[i];

    offs_reg_ptr_t attr_offs_reg = new offs_reg_t(DEFAULT_OBJFIELDS + i, SELF);
    cur_env->addid(attr->get_name(), attr_offs_reg); 
  }

  // Emit the initializer
  code_init(s);
  
  if (!basic()) {
    // loop over each method and code. 
    Method meth;
    for (unsigned i = features->first(); features->more(i); i = features->next(i)) {
      if (!features->nth(i)->is_attr()) {
        meth = (Method)features->nth(i);
        code_method(meth, s);
      }
    }
  }
  
  cur_env->exitscope();
}

// For type generation

void CgenNode::analyze() 
{
  cur_symtab->enterscope();

  for (unsigned i = features->first(); features->more(i); i = features->next(i)) {
    features->nth(i)->analyze();
  }

  cur_symtab->exitscope();
}

void attr_class::analyze() 
{
  init->analyze();
}

void method_class::analyze() 
{
  cur_symtab->enterscope();
  for (unsigned i = formals->first(); formals->more(i); i = formals->next(i)) {
    formals->nth(i)->analyze();
  }
  expr->analyze();

  cur_symtab->exitscope();
}

void formal_class::analyze() 
{
  cur_symtab->addid(name, type_decl);
}

void branch_class::analyze() 
{
  cur_symtab->enterscope();

  cur_symtab->addid(name, type_decl);
  expr->analyze();

  cur_symtab->exitscope();
}

void assign_class::analyze() 
{
  expr->analyze();
  type = expr->get_type();
}

void static_dispatch_class::analyze() 
{
  expr->analyze();
  
  CgenNodeP target_class = class_table->probe(type_name);
  Method meth = target_class->get_method(name);

  Symbol return_type = meth->get_return_type();
  if (return_type == SELF_TYPE) return_type = expr->get_type();
  type = return_type;
}

void dispatch_class::analyze() 
{
  expr->analyze();
  Symbol evaled_expr_type = expr->get_type();
  if (evaled_expr_type == SELF_TYPE) evaled_expr_type = cur_class->get_name();
  CgenNodeP target_class = class_table->probe(evaled_expr_type);
  Method meth = target_class->get_method(name);

  Symbol return_type = meth->get_return_type();
  if (return_type == SELF_TYPE) return_type = expr->get_type();
  type = return_type;
}

void cond_class::analyze() 
{
  pred->analyze();

  then_exp->analyze();
  else_exp->analyze();

  type = class_table->lub(then_exp->get_type(), else_exp->get_type());
}

void loop_class::analyze() 
{
  pred->analyze();
  body->analyze();
  type = Object;
}

void typcase_class::analyze() 
{
  expr->analyze();

  type = NULL;

  for (unsigned i = cases->first(); cases->more(i); i = cases->next(i)) {
    Case case_ = cases->nth(i);
    case_->analyze();

    if (type) type = class_table->lub(case_->get_expr_type(), type);
    else type = case_->get_expr_type();
  }
}

void block_class::analyze() 
{
  Symbol final_type;
  for (unsigned i = body->first(); body->more(i); i = body->next(i)) {
    Expression expr_ = body->nth(i);
    expr_->analyze();
    final_type = expr_->get_type();
  }

  type = final_type;
}

void let_class::analyze()
{
  init->analyze();

  cur_symtab->enterscope();
  cur_symtab->addid(identifier, type_decl);
  
  body->analyze();
  type = body->get_type();

  cur_symtab->exitscope();
}

static void bin_arith_analyze(Expression e1, Expression e2, Symbol& type)
{
  e1->analyze();
  e2->analyze();
  type = Int;
}

void plus_class::analyze() 
{
  bin_arith_analyze(e1, e2, type);
}

void sub_class::analyze() 
{
  bin_arith_analyze(e1, e2, type);
}

void mul_class::analyze() 
{
  bin_arith_analyze(e1, e2, type);
}

void divide_class::analyze() 
{
  bin_arith_analyze(e1, e2, type);
}

void neg_class::analyze() 
{
  e1->analyze();
  type = Int;
}

static void bin_cmp_analyze(Expression e1, Expression e2, Symbol& type)
{
  e1->analyze();
  e2->analyze();
  type = Bool;
}
void lt_class::analyze() 
{
  bin_cmp_analyze(e1, e2, type);
}

void eq_class::analyze() 
{
  bin_cmp_analyze(e1, e2, type);
}

void leq_class::analyze() 
{
  bin_cmp_analyze(e1, e2, type);
}

void comp_class::analyze() 
{
  e1->analyze();
  type = Bool;
}

void int_const_class::analyze() 
{
  type = Int;
}

void bool_const_class::analyze() 
{
  type = Bool;
}

void string_const_class::analyze() 
{
  type = Str;
}

void new__class::analyze() 
{
  type = type_name;
}

void isvoid_class::analyze() 
{
  e1->analyze();
  type = Bool;
}

void no_expr_class::analyze() 
{
  type = No_type;
}

void object_class::analyze() 
{
  type = cur_symtab->lookup(name);
  if (!type) {
    type = cur_class->get_attr(name)->get_type_decl();
  }
}

//******************************************************************
//
//   Fill in the following methods to produce code for the
//   appropriate expression.  You may add or remove parameters
//   as you wish, but if you do, remember to change the parameters
//   of the declarations in `cool-tree.h'  Sample code for
//   constant integers, strings, and booleans are provided.
//
//*****************************************************************

void assign_class::code(ostream &s, int tmp_offs) 
{
  expr->code(s, tmp_offs);
  offs_reg_ptr_t offs_reg = cur_env->lookup(name);
  emit_store(ACC, offs_reg->first, offs_reg->second, s);
}

unsigned assign_class::num_temp()  
{
  return expr->num_temp();
}

void static_dispatch_class::code(ostream &s, int tmp_offs) 
{
  unsigned num_args = actual->len();
  
  Expression arg_i;
  for (unsigned i = actual->first(); actual->more(i); i = actual->next(i))
  {
    arg_i = actual->nth(i);
    arg_i->code(s, tmp_offs);
    emit_push(ACC, s, tmp_offs);
  }
  // reserve stack space for pushing args
  // emit_addiu(SP, SP, -num_args * WORD_SIZE, s);
  // tmp_offs -= num_args;

  // Expression arg_i;
  // for (unsigned i = actual->first(); actual->more(i); i = actual->next(i))
  // {
  //   arg_i = actual->nth(i);
  //   arg_i->code(s, tmp_offs);
  //   emit_store(ACC, (num_args - i), SP, s); 
  // }
  
  // reverse order
  // for (unsigned i = actual->len(); i; --i) 
  // {
  //   arg_i = actual->nth(i - 1);
  //   arg_i->code(s, tmp_offs);
  //   emit_push(ACC, s, tmp_offs);
  // }

  // this has the effect of setting $a0 to be the new self_object
  // when executing inside the callee function body.
  expr->code(s, tmp_offs);

  int call_label = get_next_label();
  emit_bne(ACC, ZERO, call_label, s);
  emit_load_string(ACC, stringtable.lookup(0), s);
  emit_load_imm(T1, 1, s);
  emit_jal("_dispatch_abort", s);

  // jump to class.method 
  CgenNodeP nd = class_table->probe(type_name);
  
  emit_label_def(call_label, s);
  emit_jal(nd->method_class(name), name, s);
}

unsigned dispatch_num_temp_impl(Expression expr, Expressions actual) 
{
  //   NT( expr.f(a1, a2, ..., an) )
  // = max( NT(expr), NT(a1), NT(a2), ..., NT(an) )
  // Do NOT need one place for the evaluation result of @expr: It's evaluated
  // last and stored in $a0.
  // @a1 to @an are stored in the callee's AR.
  unsigned ret = expr->num_temp() + 1;
  Expression arg_i;
  for (unsigned i = actual->first(); actual->more(i); i = actual->next(i)) {
    arg_i = actual->nth(i);
    ret = std::max(arg_i->num_temp(), ret);
  }
  return ret;
}

unsigned static_dispatch_class::num_temp()  
{
  return dispatch_num_temp_impl(expr, actual);
}

void dispatch_class::code(ostream &s, int tmp_offs) 
{
  unsigned num_args = actual->len();
  Expression arg_i;
  for (unsigned i = actual->first(); actual->more(i); i = actual->next(i))
  {
    arg_i = actual->nth(i);
    arg_i->code(s, tmp_offs);
    emit_push(ACC, s, tmp_offs);
  }
  
  // reserve stack space for pushing args
  // emit_addiu(SP, SP, -num_args * WORD_SIZE, s);
  // tmp_offs -= num_args;

  // Expression arg_i;
  // for (unsigned i = actual->first(); actual->more(i); i = actual->next(i))
  // {
  //   arg_i = actual->nth(i);
  //   arg_i->code(s, tmp_offs);
  //   emit_store(ACC, (num_args - i), SP, s); 
  // }
  
  // reverse order
  // for (unsigned i = actual->len(); i; --i) 
  // {
  //   arg_i = actual->nth(i - 1);
  //   arg_i->code(s, tmp_offs);
  //   emit_push(ACC, s, tmp_offs);
  // }

  // this has the effect of setting $a0 to be the new self_object
  // when executing inside the callee function body.
  expr->code(s, tmp_offs);

  int call_label = get_next_label();
  emit_bne(ACC, ZERO, call_label, s);
  emit_load_string(ACC, stringtable.lookup(0), s);
  emit_load_imm(T1, 1, s);
  emit_jal("_dispatch_abort", s);

  // jump to class.method 
  Symbol type = expr->get_type();
  if (type == SELF_TYPE) type = cur_class->get_name();
  CgenNodeP nd = class_table->probe(type);
  
  // cout << "call method: " << name << ", caller type: " << type
  //   << ", method belongs: " << nd->method_class(name) << endl;

  emit_label_def(call_label, s);
  
  // emit_jal(nd->method_class(name), name, s);
  // Load addr of _dispTab into $t1, then load the addr of
  // the calling function into $t1
  emit_load(T1, DISPTABLE_OFFSET, ACC, s);
  emit_load(T1, nd->method_offset(name), T1, s);
  emit_jalr(T1, s);
}

unsigned dispatch_class::num_temp()  
{
  return dispatch_num_temp_impl(expr, actual);
}

void cond_class::code(ostream &s, int tmp_offs) 
{
  pred->code(s, tmp_offs);

  emit_fetch_bool(ACC, ACC, s);

  int false_label_idx = get_next_label();
  int end_label_idx = get_next_label();

  // $a0 == 0 means @pred evaluates to false
  emit_beqz(ACC, false_label_idx, s);

  // then branch
  then_exp->code(s, tmp_offs);
  emit_branch(end_label_idx, s);

  // else branch
  emit_label_def(false_label_idx, s);
  else_exp->code(s, tmp_offs);

  // branch convergence
  emit_label_def(end_label_idx, s);

}

unsigned cond_class::num_temp()  
{
  unsigned ret = pred->num_temp();
  ret = std::max(ret, then_exp->num_temp());
  ret = std::max(ret, else_exp->num_temp());
  return ret;
}

void loop_class::code(ostream &s, int tmp_offs) 
{
  int loop_label = get_next_label();
  int loop_end_label = get_next_label();

  // loop start
  emit_label_def(loop_label, s);

  pred->code(s, tmp_offs);
  emit_fetch_bool(ACC, ACC, s);
  // @pred evaluates to false; break the loop
  emit_beqz(ACC, loop_end_label, s);

  // loop body
  body->code(s, tmp_offs);
  emit_branch(loop_label, s);

  // loop end
  emit_label_def(loop_end_label, s);
}

unsigned loop_class::num_temp()  
{
  unsigned ret = pred->num_temp();
  ret = std::max(ret, body->num_temp());
  return ret;
}

namespace _branch_detail
{

// a little abuse of the type name, only when the second param == true
// is this class tag being visited.
typedef std::map<class_tag_t, bool> visited_t;
typedef visited_t::iterator vis_iter_t;
// class_tag : (class_tag_end, n_th in @branches)
typedef std::pair<class_tag_t, unsigned> meta_t;
typedef std::map<class_tag_t, meta_t> meta_map_t;
typedef std::vector<unsigned> sorted_t;

static void 
arrange_branch_impl(class_tag_t tag, visited_t& visited, 
                    const meta_map_t& meta_map, sorted_t& sorted)
{
  // DFS reverse topological sort
  meta_t meta_info = meta_map.at(tag);
  visited[tag] = true;

  // iterate through all the subclasses of the class identified by @tag
  for (class_tag_t iter_tag = (tag + 1); iter_tag <= meta_info.first; ++iter_tag) {
    // check if @iter_tag matches any types in the branches
    if (visited.count(iter_tag)) {
      // it matches, check if we've visited this branch
      if (!visited.at(iter_tag)) {
        arrange_branch_impl(iter_tag, visited, meta_map, sorted);
      }
    }
  }

  // append (NOT prepend), we want all subclasses to appear first!
  sorted.push_back(meta_info.second);
}

static void arrange_branches(Cases branches, sorted_t& sorted)
{
  Case branch;
  CgenNodeP nd;

  // record if we have examined a class(tag)
  visited_t visited;
  // meta information of the class tags
  meta_map_t meta_map;
  
  for (unsigned i = branches->first(); branches->more(i); i = branches->next(i)) {
    branch = branches->nth(i);
    nd = class_table->probe(branch->get_type_decl());
    visited[nd->tag()] = false;
    meta_map[nd->tag()] = std::make_pair(nd->tag_end(), i);
  }

  sorted.clear();

  for (vis_iter_t iter = visited.begin(); iter != visited.end(); ++iter) {
    if (!iter->second) {
      arrange_branch_impl(iter->first, visited, meta_map, sorted);
    }
  }

  for (unsigned i = 0; i < sorted.size(); ++i)
  {
    unsigned branch_n = sorted[i];
    branch = branches->nth(branch_n);
    nd = class_table->probe(branch->get_type_decl());

    // std::cout << "    " << i << " (branch: " << branch_n 
    //   << ", class: " << nd->get_name()
    //   << ", tag: " << nd->tag() << ")" << std::endl;
  }
}

}; // namespace _branch_detail

void typcase_class::code(ostream &s, int tmp_offs) 
{
  // sort branches in reversed topological order
  using namespace _branch_detail;
  sorted_t sorted;
  arrange_branches(cases, sorted);

  assert(sorted.size() == (unsigned)cases->len());
 
  // branch exit
  int esac_label = get_next_label();
  
  //
  // Start emitting code at this point
  //

  // result is in $a0
  expr->code(s, tmp_offs);
  
  // validity check
  emit_bne(ACC, ZERO, next_label, s);
  emit_load_string(ACC, stringtable.lookup(0), s);
  emit_load_imm(T1, 1, s);
  emit_jal("_case_abort2", s);
 
  Case branch;
  CgenNodeP id_nd;

  int cur_branch_label;
  int next_branch_label = get_next_label();
  int case_abort_label = next_branch_label;

  for (unsigned i = 0; i < sorted.size(); ++i) {
    branch = cases->nth(sorted[i]);
    id_nd = class_table->probe(branch->get_type_decl());

    cur_branch_label = next_branch_label;
    case_abort_label = next_branch_label = get_next_label();

    // on enter branch
    emit_label_def(cur_branch_label, s);
    emit_load(T1, TAG_OFFSET, ACC, s);
    emit_blti(T1, id_nd->tag(), next_branch_label, s);
    emit_bgti(T1, id_nd->tag_end(), next_branch_label, s);

    // branch code
    // update environment
    cur_env->enterscope();
    offs_reg_ptr_t id_offs_reg = new offs_reg_t(tmp_offs, FP);
    cur_env->addid(branch->get_name(), id_offs_reg);
    
    emit_push(ACC, s, tmp_offs);

    branch->get_expr()->code(s, tmp_offs);
    // tearoff the new environment
    emit_pop(s, tmp_offs);
    cur_env->exitscope();

    emit_branch(esac_label, s);
  }

  // _case_abort
  emit_label_def(case_abort_label, s);
  emit_jal("_case_abort", s);

  // esac: quite cases 
  emit_label_def(esac_label, s);
}

unsigned typcase_class::num_temp()  
{
  return 0;
}

void block_class::code(ostream &s, int tmp_offs) 
{
  for (unsigned i = body->first(); body->more(i); i = body->next(i)) {
    body->nth(i)->code(s, tmp_offs);
  }
}

unsigned block_class::num_temp() 
{
  unsigned ret = 0;
  Expression cur;
  for (unsigned i = body->first(); body->more(i); i = body->next(i)) {
    cur = body->nth(i);
    ret = std::max(ret, cur->num_temp());
  }
  return ret;
}

void let_class::code(ostream &s, int tmp_offs) 
{
  // passed elementary test
  bool is_no_expr(false);

  if (init->get_type() == No_type)
  {
    init->set_type(type_decl);
    is_no_expr = true;
  }

  init->code(s, tmp_offs);
  
  if (is_no_expr) init->set_type(No_type);
  
  cur_env->enterscope();
  offs_reg_ptr_t id_offs_reg = new offs_reg_t(tmp_offs, FP);
  cur_env->addid(identifier, id_offs_reg);

  emit_push(ACC, s, tmp_offs);

  body->code(s, tmp_offs);

  emit_pop(s, tmp_offs);
  cur_env->exitscope();
}

unsigned let_class::num_temp() {
  unsigned ret = init->num_temp() + 1;
  ret = std::max(ret, body->num_temp());
  return ret;
}

typedef void (*emit_arith_func)(char*, char*, char*, ostream&);

static void 
code_arithm_expr(Expression e1, Expression e2, 
                emit_arith_func emit_func, ostream& s, int tmp_offs)
{
  // calculate the 1st expr
  e1->code(s, tmp_offs);
  // push $a0 onto the stack
  emit_push(ACC, s, tmp_offs);
  // calculate the 2nd expr
  e2->code(s, tmp_offs);
  // at this point, an new Int object should be in $a0
  // Note that we need to create a copy of an Int, b/c
  // Int objs in COOL are immutable (thus we shouldn't
  // change the value produced by expr2!)
  // emit_jal_objcopy(s);
  CgenNodeP int_nd = class_table->probe(Int);
  emit_jal(int_nd->method_class(copy_), copy_, s);

  // load the first Int object into $t1
  emit_load(T1, 1, SP, s);
  // fetch the 1st native integer value into $t1
  emit_fetch_int(T1, T1, s);
  // fetch the 2nd native integer value into $t2 
  emit_fetch_int(T2, ACC, s);
  // arithmetic operation
  emit_func(T1, T1, T2, s);
  // store the native integer value back to the Int
  // object pointed by ACC
  emit_store_int(T1, ACC, s);
  // pop the stack
  emit_pop(s, tmp_offs);
}

void plus_class::code(ostream &s, int tmp_offs) 
{
  code_arithm_expr(e1, e2, &emit_add, s, tmp_offs);
}

inline unsigned bin_op_num_temp_impl(Expression e1, Expression e2) 
{
  unsigned ret = std::max(e1->num_temp(), e2->num_temp() + 1);
  return ret;
}

unsigned plus_class::num_temp() 
{
  return bin_op_num_temp_impl(e1, e2);
}

void sub_class::code(ostream &s, int tmp_offs) 
{
  code_arithm_expr(e1, e2, &emit_sub, s, tmp_offs);
}

unsigned sub_class::num_temp() 
{
  return bin_op_num_temp_impl(e1, e2);
}

void mul_class::code(ostream &s, int tmp_offs) 
{
  code_arithm_expr(e1, e2, &emit_mul, s, tmp_offs);
}

unsigned mul_class::num_temp() {
  return bin_op_num_temp_impl(e1, e2);
}

void divide_class::code(ostream &s, int tmp_offs) 
{
  code_arithm_expr(e1, e2, &emit_div, s, tmp_offs);
}

unsigned divide_class::num_temp() 
{
  return bin_op_num_temp_impl(e1, e2);
}

void neg_class::code(ostream &s, int tmp_offs) 
{
  // calculate the 1st expr
  e1->code(s, tmp_offs);
  // at this point, an new Int object should be in $a0
  // Note that we need to create a copy of an Int, b/c
  // Int objs in COOL are immutable (thus we shouldn't
  // change the value produced by expr2!)
  // emit_jal_objcopy(s);
  CgenNodeP int_nd = class_table->probe(Int);
  emit_jal(int_nd->method_class(copy_), copy_, s);

  // load the first Int object into $t1
  // fetch the 1st native integer value into $t1
  emit_fetch_int(T1, ACC, s);
  // negate
  emit_neg(T1, T1, s);
  // store the native integer value back to the Int
  // object pointed by ACC
  emit_store_int(T1, ACC, s);
}

unsigned neg_class::num_temp() 
{
  return e1->num_temp();
}

void lt_class::code(ostream &s, int tmp_offs) 
{
  e1->code(s, tmp_offs);
  emit_push(ACC, s, tmp_offs);
  e2->code(s, tmp_offs);

  // load the first Int object into $t1
  emit_load(T1, 1, SP, s);
  // fetch the 1st native integer into $t1
  emit_fetch_int(T1, T1, s);
  // fetch the 2nd native integer value into $t2
  emit_fetch_int(T2, ACC, s);

  int end_label_idx = get_next_label();

  // if $t1 < $t2
  emit_load_bool(ACC, truebool, s);
  emit_blt(T1, T2, end_label_idx, s);

  // else (not $t1 < $t2)
  emit_load_bool(ACC, falsebool, s);
  
  // convergence
  emit_label_def(end_label_idx, s);
  emit_pop(s);
}

unsigned lt_class::num_temp() 
{
  return bin_op_num_temp_impl(e1, e2);
}

void eq_class::code(ostream &s, int tmp_offs) 
{
  e1->code(s, tmp_offs);
  emit_push(ACC, s, tmp_offs);
  e2->code(s, tmp_offs);

  // load the first object into $t1
  emit_load(T1, 1, SP, s);
  // load the second object into $t2
  emit_move(T2, ACC, s);

  int end_label_idx = get_next_label();

  // if $t1 == $t2
  emit_load_bool(ACC, truebool, s);
  emit_beq(T1, T2, end_label_idx, s);

  // else (not $t1 == $t2)
  emit_load_bool(A1, falsebool, s);
  emit_jal("equality_test", s);

  // convergence
  emit_label_def(end_label_idx, s);
  emit_pop(s);
}

unsigned eq_class::num_temp() 
{
  return bin_op_num_temp_impl(e1, e2);
}

void leq_class::code(ostream &s, int tmp_offs) 
{
  e1->code(s, tmp_offs);
  emit_push(ACC, s, tmp_offs);
  e2->code(s, tmp_offs);

  // load the first Int object into $t1
  emit_load(T1, 1, SP, s);
  // fetch the 1st native integer into $t1
  emit_fetch_int(T1, T1, s);
  // fetch the 2nd native integer value into $t2
  emit_fetch_int(T2, ACC, s);

  int end_label_idx = get_next_label();

  // if $t1 <= $t2
  emit_load_bool(ACC, truebool, s);
  emit_bleq(T1, T2, end_label_idx, s);

  // else (not $t1 <= $t2)
  emit_load_bool(ACC, falsebool, s);
  
  // convergence
  emit_label_def(end_label_idx, s);
  emit_pop(s);
}

unsigned leq_class::num_temp() 
{
  return bin_op_num_temp_impl(e1, e2);
}

void comp_class::code(ostream &s, int tmp_offs) 
{
  // calculate the 1st expr
  e1->code(s, tmp_offs);

  emit_fetch_bool(ACC, ACC, s);
  
  int false_label_idx = get_next_label();
  int end_label_idx = get_next_label();

  // $a0 == 0 means @pred evaluates to false
  emit_beqz(ACC, false_label_idx, s);
  // true branch, comp is false
  emit_load_bool(ACC, falsebool, s);
  emit_branch(end_label_idx, s);

  // false branch, comp is true
  emit_label_def(false_label_idx, s);
  emit_load_bool(ACC, truebool, s);

  // branch convergence
  emit_label_def(end_label_idx, s);
}

unsigned comp_class::num_temp() 
{
  return e1->num_temp();
}

void int_const_class::code(ostream& s, int tmp_offs)  
{
  //
  // Need to be sure we have an IntEntry *, not an arbitrary Symbol
  //
  emit_load_int(ACC,inttable.lookup_string(token->get_string()),s);
}

unsigned int_const_class::num_temp() 
{
  return 0;
}

void string_const_class::code(ostream& s, int tmp_offs)
{
  emit_load_string(ACC,stringtable.lookup_string(token->get_string()),s);
}

unsigned string_const_class::num_temp() {
  return 0;
}

void bool_const_class::code(ostream& s, int tmp_offs)
{
  emit_load_bool(ACC, BoolConst(val), s);
}

unsigned bool_const_class::num_temp() 
{
  return 0;
}

void new__class::code(ostream &s, int tmp_offs) 
{ 
  if (type_name == SELF_TYPE)
  {
    // load class tag into $t2
    emit_load_address(T1, CLASSOBJTAB, s);
    emit_load(T2, TAG_OFFSET, SELF, s);
    // offset of the class, indicated by class tag, inside class_objTab
    emit_sll(T2, T2, 3, s);

    emit_addu(T1, T1, T2, s);
    emit_push(T1, s);

    // load class_protObj into $a0
    emit_load(ACC, 0, T1, s);
    // make a copy of the class
    emit_jal(OBJCOPY, s); 

    emit_load(T1, 1, SP, s);
    emit_pop(s);

    emit_load(T1, 1, T1, s);
    emit_jalr(T1, s);
  }
  else
  {
    CgenNodeP nd = class_table->probe(type_name);
    
    emit_partial_load_address(ACC, s);
    emit_protobj_ref(type_name, s); s << endl;
    // jal class.copy
    emit_jal(nd->method_class(copy_), copy_, s);
    // jal class_init
    emit_jal_init(type_name, s);
  }
  
}

unsigned new__class::num_temp() {
  return 0;
}

void isvoid_class::code(ostream &s, int tmp_offs) 
{
  emit_move(T1, ACC, s);

  int end_label_idx = get_next_label();

  emit_load_bool(ACC, truebool, s);
  emit_bne(T1, ZERO, end_label_idx, s);
  emit_load_bool(ACC, falsebool, s);

  emit_label_def(end_label_idx, s);
}

unsigned isvoid_class::num_temp() 
{
  return e1->num_temp();
}

void no_expr_class::code(ostream &s, int tmp_offs) 
{
  if (type == Int)
  {
    emit_load_int(ACC, inttable.lookup_string("0"), s);
  }
  else if (type == Bool)
  {
    emit_load_bool(ACC, falsebool, s);
  }
  else if (type == Str)
  {
    emit_load_string(ACC, stringtable.lookup_string(""), s);
  }
  else if (type != No_type)
  {
    emit_move(ACC, ZERO, s);
  }
}

unsigned no_expr_class::num_temp() 
{
  return 0;
}

void object_class::code(ostream &s, int tmp_offs) {
  // typedef std::pair<char*, unsigned> offs_reg_t;
  if (name == self) {
    emit_move(ACC, SELF, s);
  }
  else{
    offs_reg_ptr_t offs_reg = cur_env->lookup(name);
    emit_load(ACC, offs_reg->first, offs_reg->second, s);
  }
}

unsigned object_class::num_temp() 
{
  return 0;
}
