#ifndef PA5_CGEN_H
#define PA5_CGEN_H

#include <assert.h>
#include <stdio.h>
#include "emit.h"
#include "cool-tree.h"
#include "symtab.h"

#include <map>
#include <vector>

enum Basicness     {Basic, NotBasic};
#define TRUE 1
#define FALSE 0

class CgenClassTable;
typedef CgenClassTable *CgenClassTableP;

class CgenNode;
typedef CgenNode *CgenNodeP;

typedef int class_tag_t;

int stringclasstag;
int intclasstag;
int boolclasstag;

class CgenClassTable : public SymbolTable<Symbol,CgenNode> {
private:
  List<CgenNode> *nds;
  ostream& str;


// The following methods emit code for
// constants and global declarations.

  void code_global_data();
  void code_global_text();
  void code_bools(int);
  void code_select_gc();
  void code_constants();

  void code_class_nametab();
  void code_class_objtab();
  void code_disptabs();
  void code_protobjs();
// The following creates an inheritance graph from
// a list of classes.  The graph is implemented as
// a tree of `CgenNode', and class names are placed
// in the base class symbol table.

  void install_basic_classes();
  void install_class(CgenNodeP nd);
  void install_classes(Classes cs);
  void build_inheritance_tree();
  void set_relations(CgenNodeP nd);

  void assign_class_tags();

  void calc_classes_layout();
  void analyze();
  
  void code_classes();

public:
  CgenNodeP lub(CgenNodeP a, CgenNodeP b);
  Symbol lub(Symbol a, Symbol b);
  
  CgenClassTable(ostream& str);
  void code();
  CgenNodeP root();

  void work(Classes);
};


class CgenNode : public class__class {
private: 
  CgenNodeP parentnd;                        // Parent of class
  List<CgenNode> *children;                  // Children of class
  Basicness basic_status;                    // `Basic' if class is basic
                                              // `NotBasic' otherwise
  typedef std::vector<Attribute> attr_vec_t;
  typedef std::map<Symbol, Expression> attr_init_map_t;

  typedef std::vector<Method> meth_vec_t;
  typedef std::map<Symbol, Symbol> meth_class_map_t; 

  bool _has_calced_layout;
  attr_vec_t _attrs_layout;
  // attr_init_map_t _attr_init_map;
  meth_vec_t _meths_layout;
  meth_class_map_t _meth_class_map;

  class_tag_t _class_tag;
  // This attr records the end of the tag of all this class's subclasses
  class_tag_t _class_tag_end;
public:
  CgenNode(Class_ c,
           Basicness bstatus,
           CgenClassTableP cltab);

  inline void set_tag(class_tag_t t) { _class_tag = t; }
  inline class_tag_t tag() const { return _class_tag; }
  inline void set_tag_end(class_tag_t e) { _class_tag_end = e; }
  inline class_tag_t tag_end() const { return _class_tag_end; }

  void add_child(CgenNodeP child);
  List<CgenNode> *get_children() const { return children; }
  void set_parentnd(CgenNodeP p);
  CgenNodeP get_parentnd() const { return parentnd; }
  int basic() const { return (basic_status == Basic); }

  void analyze();

  void calc_layout();
  
  void code_disptab(ostream& os) const;
  void code_protobj(ostream& os) const;
  
  void code_init(ostream& os) const;
  void code_method(Method meth, ostream& os) const;
  void code_methods(ostream& os) const;

  Attribute get_attr(const Symbol a_name) const;
  Method get_method(const Symbol m_name) const;
  unsigned method_offset(const Symbol meth) const;
  Symbol method_class(const Symbol meth) const;
};

class BoolConst 
{
 private: 
  int val;
 public:
  BoolConst(int);
  void code_def(ostream&, int boolclasstag);
  void code_ref(ostream&) const;
};

#endif
