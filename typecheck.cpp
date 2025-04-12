#include <iostream>
#include <cstdio>
#include <cstring>

#include "ast.hpp"
#include "symtab.hpp"
#include "primitive.hpp"
#include "assert.h"

// WRITEME: The default attribute propagation rule
#define default_rule(X) X

#include <typeinfo>

class Typecheck : public Visitor
{
  private:
    FILE* m_errorfile;
    SymTab* m_st;

    // The set of recognized errors
    enum errortype
    {
        no_main,
        nonvoid_main,
        dup_proc_name,
        dup_var_name,
        proc_undef,
        call_type_mismatch,
        narg_mismatch,
        expr_type_err,
        var_undef,
        ifpred_err,
        whilepred_err,
        incompat_assign,
        who_knows,
        ret_type_mismatch,
        array_index_error,
        no_array_var,
        arg_type_mismatch,
        expr_pointer_arithmetic_err,
        expr_abs_error,
        expr_addressof_error,
        invalid_deref
    };

    // Print the error to file and exit
    void t_error(errortype e, Attribute a)
    {
        fprintf(m_errorfile,"on line number %d, ", a.lineno);

        switch(e)
        {
            case no_main:
                fprintf(m_errorfile, "error: no main\n");
                exit(2);
            case nonvoid_main:
                fprintf(m_errorfile, "error: the Main procedure has arguments\n");
                exit(3);
            case dup_proc_name:
                fprintf(m_errorfile, "error: duplicate procedure names in same scope\n");
                exit(4);
            case dup_var_name:
                fprintf(m_errorfile, "error: duplicate variable names in same scope\n");
                exit(5);
            case proc_undef:
                fprintf(m_errorfile, "error: call to undefined procedure\n");
                exit(6);
            case var_undef:
                fprintf(m_errorfile, "error: undefined variable\n");
                exit(7);
            case narg_mismatch:
                fprintf(m_errorfile, "error: procedure call has different number of args than declartion\n");
                exit(8);
            case arg_type_mismatch:
                fprintf(m_errorfile, "error: argument type mismatch\n");
                exit(9);
            case ret_type_mismatch:
                fprintf(m_errorfile, "error: type mismatch in return statement\n");
                exit(10);
            case call_type_mismatch:
                fprintf(m_errorfile, "error: type mismatch in procedure call args\n");
                exit(11);
            case ifpred_err:
                fprintf(m_errorfile, "error: predicate of if statement is not boolean\n");
                exit(12);
            case whilepred_err:
                fprintf(m_errorfile, "error: predicate of while statement is not boolean\n");
                exit(13);
            case array_index_error:
                fprintf(m_errorfile, "error: array index not integer\n");
                exit(14);
            case no_array_var:
                fprintf(m_errorfile, "error: attempt to index non-array variable\n");
                exit(15);
            case incompat_assign:
                fprintf(m_errorfile, "error: type of expr and var do not match in assignment\n");
                exit(16);
            case expr_type_err:
                fprintf(m_errorfile, "error: incompatible types used in expression\n");
                exit(17);
            case expr_abs_error:
                fprintf(m_errorfile, "error: absolute value can only be applied to integers and strings\n");
                exit(17);
            case expr_pointer_arithmetic_err:
                fprintf(m_errorfile, "error: invalid pointer arithmetic\n");
                exit(18);
            case expr_addressof_error:
                fprintf(m_errorfile, "error: AddressOf can only be applied to integers, chars, and indexed strings\n");
                exit(19);
            case invalid_deref:
                fprintf(m_errorfile, "error: Deref can only be applied to integer pointers and char pointers\n");
                exit(20);
            default:
                fprintf(m_errorfile, "error: no good reason\n");
                exit(21);
        }
    }

    // Helpers
    // WRITEME: You might want write some hepler functions.

    // Type Checking
    // WRITEME: You need to implement type-checking for this project

    // Check that there is one and only one main
    void check_for_one_main(ProgramImpl* p)
    {
        std::list<Proc_ptr>::iterator iter;
        char* name;
        //Symbol *s;
        bool isAMain = false;

        for(iter = p->m_proc_list->begin(); iter != p->m_proc_list->end(); ++iter){
            ProcImpl* procedure = static_cast<ProcImpl*>(*iter);
            //ProcImpl* procedure = *iter;
            name = strdup(procedure->m_symname->spelling());
            if(strcmp(name, "Main") == 0){
                if(isAMain == true){
                    //More than one main throw an error
                    this->t_error(dup_proc_name, p->m_attribute);
                }
                else{
                    //printf(name);
                    isAMain = true;
                }
            }
        }
    
        //Main does not exist, throw an error.
        if(isAMain == false){
            this->t_error(no_main, p->m_attribute);
        }
    }

    // Create a symbol for the procedure and check there is none already
    // existing
    void add_proc_symbol(ProcImpl* p)
    {
        char* name = strdup(p->m_symname->spelling());
        if(strcmp(name, "Main") == 0){
            if(m_st->lookup("Main") != NULL){
                this->t_error(dup_proc_name, p->m_attribute);
            }
            //make sure that main has no arguments.
            if(p->m_decl_list->size() != 0){
                this->t_error(nonvoid_main, p->m_attribute);
            }   
        }
        Symbol *s = new Symbol();
        s->m_basetype = p->m_type->m_attribute.m_basetype;    
        std::list<Decl_ptr>::iterator iter;
        /* Will Probably have to comeback and fix this shit. Not quite sure what i am doing.
        */
        for(iter = p->m_decl_list->begin(); iter != p->m_decl_list->end(); iter++){
            auto iter2 = ((DeclImpl*)*iter)->m_symname_list->begin();
            for( ; iter2!= ((DeclImpl*)*iter)->m_symname_list->end(); iter2++){
                Symbol* s2 = m_st->lookup(strdup((*iter2)->spelling()));
                s->m_arg_type.push_back(s2->m_basetype);

            }   
        }
        if(!m_st->insert_in_parent_scope(name, s)){
            this->t_error(dup_proc_name, p->m_attribute);
        }
    }

    // Add symbol table information for all the declarations following
    void add_decl_symbol(DeclImpl* p)
    {
        std::list<SymName_ptr>::iterator iter;
        char* name;
        Symbol* s;
        for(iter = p->m_symname_list->begin(); iter != p->m_symname_list->end(); ++iter){
            name = strdup((*iter)->spelling());
            s = new Symbol();
            s->m_basetype = p->m_type->m_attribute.m_basetype;
            if(!m_st->insert(name,s)){
                this->t_error(dup_var_name, p->m_attribute);
            }
        }
    }

    // Check that the return statement of a procedure has the appropriate type
    void check_proc(ProcImpl *p)
    {
        Basetype type_return = p->m_procedure_block->m_attribute.m_basetype;
        Basetype type_function = p->m_type->m_attribute.m_basetype;
        if(type_function != type_return){
            if(!((type_function == bt_intptr || type_function == bt_charptr) && type_return == bt_ptr)){
                this->t_error(ret_type_mismatch, p->m_attribute);
            }
        }
    }

    // Check that the declared return type is not an array
    void check_return(Return *p)
    {
        Basetype type = p->m_attribute.m_basetype;
        if(type == bt_string){
            this->t_error(ret_type_mismatch, p->m_attribute);
        }
    }

    // Create a symbol for the procedure and check there is none already
    // existing
    void check_call(Call *p)
    {
        char* name = strdup(p->m_symname->spelling());
        Symbol* s = m_st->lookup(name);
        
        
        if(s == nullptr){ // function has not yet been defined.
            this->t_error(proc_undef, p->m_attribute);
        }

        // check that there are the same number of parameter as in the original definition.
        if(p->m_expr_list->size() != s->m_arg_type.size()){
            this->t_error(narg_mismatch, p->m_attribute);
        }
        //check that the types are the same as defined in the procedure call.
        std::list<Expr_ptr>::iterator it  = p->m_expr_list->begin();
        for(int i = 0; i < s->m_arg_type.size(); i++){
            Basetype def_type = s->m_arg_type[i];
            Basetype arg_type = (*it)->m_attribute.m_basetype;
            //printf("%i", def_type);
            //printf("\n%i\n", arg_type);
            if(def_type != arg_type){
                this->t_error(arg_type_mismatch, p->m_attribute);
            }
            it++;
        }
        Basetype type_lhs = p->m_lhs->m_attribute.m_basetype;
        Basetype type_func = s->m_basetype;
        if(type_lhs != type_func){
            this->t_error(call_type_mismatch, p->m_attribute);
        }

    }

    // For checking that this expressions type is boolean used in if/else
    void check_pred_if(Expr* p)
    {
        Basetype type = p->m_attribute.m_basetype;
        if(type != bt_boolean){
            this->t_error(ifpred_err, p->m_attribute);
        }
    }

    // For checking that this expressions type is boolean used in while
    void check_pred_while(Expr* p)
    {
        Basetype type = p->m_attribute.m_basetype;
        if(type != bt_boolean){
            this->t_error(whilepred_err, p->m_attribute);
        }
    }

    void check_assignment(Assignment* p)
    { // make sure lhs and rhs have same type (nullptr can be used for both intptr & charptr)
        Basetype typeLHS = p->m_lhs->m_attribute.m_basetype;
        Basetype typeExp = p->m_expr->m_attribute.m_basetype;
        if(typeLHS == bt_charptr){
            if(!(typeExp == bt_charptr || typeExp == bt_ptr)){
                //printf("TEST 1\n");
                this->t_error(incompat_assign, p->m_attribute);
            }
        }
        else if(typeLHS == bt_intptr){
            if(!(typeExp == bt_intptr || typeExp == bt_ptr)){
                //printf("TEST 2\n");
                this->t_error(incompat_assign, p->m_attribute);
            }
        }
        else if(typeLHS != typeExp){
            //printf("TEST 3\n");

            this->t_error(incompat_assign, p->m_attribute);
        }
    }

    void check_string_assignment(StringAssignment* p)
    {  // check that the lhs is of type string
        Basetype type_lhs = p->m_lhs->m_attribute.m_basetype;
        if(type_lhs != bt_string){
            this->t_error(incompat_assign, p->m_attribute);
            //printf("Test 4\n");
        }
    }

    //check that indexed variable is of type string.
    //check that the index evaluates to a integer.
    //check that the variable is also defined.
    void check_array_access(ArrayAccess* p)
    {
        char* name = strdup(p->m_symname->spelling());
        Symbol* s = m_st->lookup(name);
        if(s == nullptr){
            this->t_error(var_undef, p->m_attribute);
        }

        if(s->m_basetype != bt_string){
            this->t_error(no_array_var, p->m_attribute);
        }

        Basetype type = p->m_expr->m_attribute.m_basetype;
        if(type != bt_integer){
            this->t_error(array_index_error, p->m_attribute);
        }

    }

    void check_array_element(ArrayElement* p)
    {
        char* name = strdup(p->m_symname->spelling());
        Symbol* s = m_st->lookup(name);
        if(s == nullptr){
            this->t_error(var_undef, p->m_attribute);
        }
        if(s->m_basetype != bt_string){
            this->t_error(no_array_var, p->m_attribute);
        }

        Basetype  type = p->m_expr->m_attribute.m_basetype;
        if(type != bt_integer){
            this->t_error(array_index_error, p->m_attribute);
        }

    }

    // For checking boolean operations(and, or ...)
    void checkset_boolexpr(Expr* parent, Expr* child1, Expr* child2)
    {
        Basetype type_child1 = child1->m_attribute.m_basetype;
        Basetype type_child2 = child2->m_attribute.m_basetype;
        //printf("RUN 1\n");
        if(!(type_child1 == bt_boolean || type_child2 == bt_boolean)){
            this->t_error(expr_type_err, parent->m_attribute);
        }
    }

    // For checking arithmetic expressions(plus, times, ...)
    void checkset_arithexpr(Expr* parent, Expr* child1, Expr* child2)
    {
         //printf("RUN 2\n");
        Basetype type_child1 = child1->m_attribute.m_basetype;
        Basetype type_child2 = child2->m_attribute.m_basetype;
        if(!(type_child1 == bt_integer && type_child2 == bt_integer)){
            this->t_error(expr_type_err, parent->m_attribute);
        }
    }

    // Called by plus and minus: in these cases we allow pointer arithmetics
    void checkset_arithexpr_or_pointer(Expr* parent, Expr* child1, Expr* child2)
    {
        Basetype type_child1 = child1->m_attribute.m_basetype;
        Basetype type_child2 = child2->m_attribute.m_basetype;
        //printf("RUN 3\n");
        if(type_child1 == bt_integer){
            if(type_child2 != bt_integer){
                this->t_error(expr_type_err, parent->m_attribute);                
            }
        }
        else if (type_child1 == bt_charptr){
            if(type_child2 != bt_integer){
                this->t_error(expr_pointer_arithmetic_err, parent->m_attribute);
            }
        }
        else if (type_child1 == bt_intptr){
            this->t_error(expr_pointer_arithmetic_err, parent->m_attribute);
        }
        else{
            this->t_error(expr_type_err, parent->m_attribute);
        }
    }

    // For checking relational(less than , greater than, ...)
    void checkset_relationalexpr(Expr* parent, Expr* child1, Expr* child2)
    {
        Basetype type_child1 = child1->m_attribute.m_basetype;
        Basetype type_child2 = child1->m_attribute.m_basetype;
        if(!(type_child1 == bt_integer || type_child2 == bt_integer)){
            this->t_error(expr_type_err, parent->m_attribute);
        }
    }

    // For checking equality ops(equal, not equal)
    void checkset_equalityexpr(Expr* parent, Expr* child1, Expr* child2)
    { // sides can be either integer, or boolean, or char
        Basetype type_child1 = child1->m_attribute.m_basetype;
        Basetype type_child2 = child2->m_attribute.m_basetype;

        if(!(type_child1 == type_child2 && (type_child1 == bt_integer || type_child1 == bt_boolean || type_child1 == bt_intptr || type_child1 == bt_charptr || type_child1 == bt_char || bt_ptr))){
            this->t_error(expr_type_err, parent->m_attribute);
        }
    }

    // For checking not
    void checkset_not(Expr* parent, Expr* child)
    {
        Basetype type_child = child->m_attribute.m_basetype;
        if(type_child != bt_boolean){
            this->t_error(expr_type_err, parent->m_attribute);
        }
    }

    // For checking unary minus
    void checkset_uminus(Expr* parent, Expr* child)
    {
       Basetype type_child = child->m_attribute.m_basetype;
       if(type_child != bt_integer){
            this->t_error(expr_type_err, parent->m_attribute);
       } 
    }

    void checkset_absolute_value(Expr* parent, Expr* child)
    {
        Basetype type_child = child->m_attribute.m_basetype;
        if(!(type_child == bt_integer || type_child == bt_string)){
            //printf("ThiS RUNS?");
            this->t_error(expr_abs_error, parent->m_attribute);
        }   
    }

    void checkset_addressof(Expr* parent, Lhs* child)
    {
        Basetype type_child = child->m_attribute.m_basetype;
        if(!(type_child == bt_integer || type_child == bt_char)){
            this->t_error(expr_addressof_error, parent->m_attribute);
        }
    }

    void checkset_deref_expr(Deref* parent,Expr* child)
    {   
        Basetype type_child = child->m_attribute.m_basetype;
        //printf("TypeChild: %i\n", type_child);
        if(!(type_child == bt_intptr || type_child == bt_charptr)){
        //    printf("TEST 1\n");
            this->t_error(invalid_deref, parent->m_attribute);
        }
    }

    // Check that if the right-hand side is an lhs, such as in case of
    // addressof
    void checkset_deref_lhs(DerefVariable* p)
    {
        char* name = strdup(p->m_symname->spelling());
        Symbol* s = m_st->lookup(name);
        if(s == nullptr){
            this->t_error(var_undef, p->m_attribute);
        }
        if(!(s->m_basetype == bt_charptr || s->m_basetype == bt_intptr)){
        //    printf("TEST 2\n");
            this->t_error(invalid_deref, p->m_attribute);
        }
    }

    void checkset_variable(Variable* p)
    {
        char* name = strdup(p->m_symname->spelling());
        Symbol* s = m_st->lookup(name);
        if(s == nullptr){ // symbol doesn't exist
            this->t_error(var_undef, p->m_attribute);
        }
    }

    void checkset_identifier(Ident* p){
        char* name = strdup(p->m_symname->spelling());
        Symbol* s = m_st->lookup(name);
        if(s == nullptr){
            this->t_error(var_undef, p->m_attribute);
        }
    }


  public:

    Typecheck(FILE* errorfile, SymTab* st) {
        m_errorfile = errorfile;
        m_st = st;
    }

    void visitProgramImpl(ProgramImpl* p)
    {
        p->m_attribute.m_scope = m_st->get_scope(); 
        m_st->open_scope();
        p->visit_children(this); 
        check_for_one_main(p);
        m_st->close_scope();
    }

    void visitProcImpl(ProcImpl* p)
    {

        m_st->open_scope();
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        check_proc(p);
        add_proc_symbol(p);
        m_st->close_scope();
        p->m_attribute.m_basetype = bt_procedure;
    }

    void visitCall(Call* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        check_call(p);
        p->m_attribute.m_basetype = p->m_lhs->m_attribute.m_basetype;
    }

    void visitNested_blockImpl(Nested_blockImpl* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        m_st->open_scope();
        p->visit_children(this);
        m_st->close_scope();
    }

    void visitProcedure_blockImpl(Procedure_blockImpl* p)
    {

        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        p->m_attribute.m_basetype = p->m_return_stat->m_attribute.m_basetype;
    }

    void visitDeclImpl(DeclImpl* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        
        add_decl_symbol(p);
    }

    void visitAssignment(Assignment* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        check_assignment(p);
        p->m_attribute.m_basetype = p->m_expr->m_attribute.m_basetype;

    }

    void visitStringAssignment(StringAssignment *p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        check_string_assignment(p);
        p->m_attribute.m_basetype = bt_string;
    }

    void visitIdent(Ident* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_identifier(p);
        p->m_attribute.m_basetype = m_st->lookup(strdup(p->m_symname->spelling()))->m_basetype;  
    }

    void visitReturn(Return* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        check_return(p);
        p->m_attribute.m_basetype = p->m_expr->m_attribute.m_basetype;
    }

    void visitIfNoElse(IfNoElse* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        check_pred_if(p->m_expr);
        p->m_attribute.m_basetype = p->m_expr->m_attribute.m_basetype;
    }

    void visitIfWithElse(IfWithElse* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        check_pred_if(p->m_expr);
        p->m_attribute.m_basetype = bt_boolean;
    }

    void visitWhileLoop(WhileLoop* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        check_pred_while(p->m_expr);
        p->m_attribute.m_basetype = bt_boolean;
    }

    void visitCodeBlock(CodeBlock *p) 
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        p->m_attribute.m_basetype = p->m_nested_block->m_attribute.m_basetype;

    }

    void visitTInteger(TInteger* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        p->m_attribute.m_basetype = bt_integer;
    }

    void visitTBoolean(TBoolean* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        p->m_attribute.m_basetype = bt_boolean;
    }

    void visitTCharacter(TCharacter* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        p->m_attribute.m_basetype = bt_char;
    }

    void visitTString(TString* p)
    {

        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        p->m_attribute.m_basetype = bt_string;
    }

    void visitTCharPtr(TCharPtr* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        p->m_attribute.m_basetype = bt_charptr;
    }

    void visitTIntPtr(TIntPtr* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        p->m_attribute.m_basetype = bt_intptr;
    }

    void visitAnd(And* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        p->m_attribute.m_basetype = bt_boolean;
    }

    void visitDiv(Div* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_arithexpr(p, p->m_expr_1, p->m_expr_2);
        p->m_attribute.m_basetype = bt_integer;
    }

    void visitCompare(Compare* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_equalityexpr(p, p->m_expr_1, p->m_expr_2);
        p->m_attribute.m_basetype = bt_boolean;
    }

    void visitGt(Gt* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_relationalexpr(p, p->m_expr_1, p->m_expr_2);
        p->m_attribute.m_basetype = bt_boolean;        
    }

    void visitGteq(Gteq* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_relationalexpr(p, p->m_expr_1, p->m_expr_2);
        p->m_attribute.m_basetype = bt_boolean;
    }

    void visitLt(Lt* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_relationalexpr(p, p->m_expr_1, p->m_expr_2);
        p->m_attribute.m_basetype = bt_boolean;
    }

    void visitLteq(Lteq* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_relationalexpr(p, p->m_expr_1, p->m_expr_2);
        p->m_attribute.m_basetype = bt_boolean;
    }

    void visitMinus(Minus* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_arithexpr_or_pointer(p, p->m_expr_1, p->m_expr_2);
        Basetype type_expr1 = p->m_expr_1->m_attribute.m_basetype;
        if(type_expr1 == bt_charptr){
            p->m_attribute.m_basetype = bt_charptr;
        }
        else if (type_expr1 == bt_intptr){
            p->m_attribute.m_basetype = bt_intptr;
        }
        else if (type_expr1 == bt_integer){
            p->m_attribute.m_basetype = bt_integer;
        }
        //p->m_attribute.m_basetype = bt_integer;
    }

    void visitNoteq(Noteq* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_equalityexpr(p, p->m_expr_1, p->m_expr_2);
        p->m_attribute.m_basetype = bt_boolean;
    }

    void visitOr(Or* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_boolexpr(p, p->m_expr_1, p->m_expr_2);
        p->m_attribute.m_basetype = bt_boolean;
    }

    void visitPlus(Plus* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_arithexpr_or_pointer(p, p->m_expr_1, p->m_expr_2);
        Basetype type_expr1 = p->m_expr_1->m_attribute.m_basetype;
        if(type_expr1 == bt_charptr){
            p->m_attribute.m_basetype = bt_charptr;
        }
        else if (type_expr1 == bt_intptr){
            p->m_attribute.m_basetype = bt_intptr;
        }
        else if (type_expr1 == bt_integer){
            p->m_attribute.m_basetype = bt_integer;
        }
    }

    void visitTimes(Times* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_arithexpr(p, p->m_expr_1, p->m_expr_2);
        p->m_attribute.m_basetype = bt_integer;
    }

    void visitNot(Not* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_not(p, p->m_expr);
        p->m_attribute.m_basetype = bt_boolean;
    }

    void visitUminus(Uminus* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_uminus(p, p->m_expr);
        p->m_attribute.m_basetype = bt_integer;
    }

    void visitArrayAccess(ArrayAccess* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        check_array_access(p);
        p->m_attribute.m_basetype = bt_char;
    }

    void visitIntLit(IntLit* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        p->m_attribute.m_basetype = bt_integer;
    }

    void visitCharLit(CharLit* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        p->m_attribute.m_basetype = bt_char;
    }

    void visitBoolLit(BoolLit* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        p->m_attribute.m_basetype = bt_boolean;
    }

    void visitNullLit(NullLit* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        p->m_attribute.m_basetype = bt_ptr;
    }

    void visitAbsoluteValue(AbsoluteValue* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_absolute_value(p, p->m_expr);
        p->m_attribute.m_basetype = bt_integer;
    }

    void visitAddressOf(AddressOf* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_addressof(p, p->m_lhs);
        Basetype childType = p->m_lhs->m_attribute.m_basetype;
        if(childType == bt_integer){
            p->m_attribute.m_basetype = bt_intptr;
        }
        else if(childType == bt_char){
            p->m_attribute.m_basetype = bt_charptr;
        }
    }

    void visitVariable(Variable* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_variable(p);
        p->m_attribute.m_basetype = m_st->lookup(strdup(p->m_symname->spelling()))->m_basetype;
    }

    void visitDeref(Deref* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_deref_expr(p, p->m_expr);
        if(p->m_expr->m_attribute.m_basetype == bt_charptr){
            p->m_attribute.m_basetype = bt_char;
        }
        else if(p->m_expr->m_attribute.m_basetype == bt_intptr){
            p->m_attribute.m_basetype = bt_integer;
        }
    }

    void visitDerefVariable(DerefVariable* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        checkset_deref_lhs(p);
        Basetype type = m_st->lookup(strdup(p->m_symname->spelling()))->m_basetype;
        if(type == bt_charptr){
            p->m_attribute.m_basetype = bt_char;
        }
        else if(type == bt_intptr){
            p->m_attribute.m_basetype = bt_integer;
        }
    }

    void visitArrayElement(ArrayElement* p)
    {
        p->m_attribute.m_scope = m_st->get_scope();
        p->visit_children(this);
        check_array_element(p);
        p->m_attribute.m_basetype = bt_char;
    }

    // Special cases
    void visitPrimitive(Primitive* p) {}
    void visitSymName(SymName* p) {}
    void visitStringPrimitive(StringPrimitive* p) {}
};


void dopass_typecheck(Program_ptr ast, SymTab* st)
{
    Typecheck* typecheck = new Typecheck(stderr, st);
    ast->accept(typecheck); // Walk the tree with the visitor above
    delete typecheck;
}