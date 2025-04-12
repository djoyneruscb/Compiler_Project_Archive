#include <cassert>
#include <typeinfo>

#include "ast.hpp"
#include "symtab.hpp"
#include "primitive.hpp"


class Codegen : public Visitor
{
  private:
    FILE* m_outputfile;
    SymTab *m_st;
    // Basic size of a word (integers and booleans) in bytes
    static const int wordsize = 4;

    int label_count; // Access with new_label
    int al;
    int addr;
    int pushes;
    // Helpers
    // This is used to get new unique labels (cleverly names label1, label2, ...)
    int new_label()
    {
        return label_count++;
    }

    void set_text_mode()
    {
        fprintf(m_outputfile, ".text\n\n");
    }

    void set_data_mode()
    {
        fprintf(m_outputfile, ".data\n\n");
    }

    // PART 1:
    // 1) get arithmetic expressions on integers working:
    //  you wont really be able to run your code,
    //  but you can visually inspect it to see that the correct
    //  chains of opcodes are being generated.
    // 2) get procedure calls working:
    //  if you want to see at least a very simple program compile
    //  and link successfully against gcc-produced code, you
    //  need to get at least this far
    // 3) get boolean operation working
    //  before we can implement any of the conditional control flow
    //  stuff, we need to have booleans worked out.
    // 4) control flow:
    //  we need a way to have if-elses and while loops in our language.
    // 5) arrays: just like variables, but with an index

    // Hint: the symbol table has been augmented to track an offset
    //  with all of the symbols.  That offset can be used to figure
    //  out where in the activation record you should look for a particuar
    //  variable


    ///////////////////////////////////////////////////////////////////////////////
    //
    //  function_prologue
    //  function_epilogue
    //
    //  Together these two functions implement the callee-side of the calling
    //  convention.  A stack frame has the following layout:
    //
    //                         <- SP (before pre-call / after epilogue)
    //  high -----------------
    //       | actual arg 1  |
    //       |    ...        |
    //       | actual arg n  |
    //       -----------------
    //       |  Return Addr  |
    //       =================
    //       | temporary 1   | <- SP (when starting prologue)
    //       |    ...        |
    //       | temporary n   |
    //   low ----------------- <- SP (when done prologue)
    //
    //
    //              ||
    //              ||
    //             \  /
    //              \/
    //
    //
    //  The caller is responsible for placing the actual arguments
    //  and the return address on the stack. Actually, the return address
    //  is put automatically on the stack as part of the x86 call instruction.
    //
    //  On function entry, the callee
    //
    //  (1) allocates space for the callee's temporaries on the stack
    //
    //  (2) saves callee-saved registers (see below) - including the previous activation record pointer (%ebp)
    //
    //  (3) makes the activation record pointer (frmae pointer - %ebp) point to the start of the temporary region
    //
    //  (4) possibly copies the actual arguments into the temporary variables to allow easier access
    //
    //  On function exit, the callee:
    //
    //  (1) pops the callee's activation record (temporay area) off the stack
    //
    //  (2) restores the callee-saved registers, including the activation record of the caller (%ebp)
    //
    //  (3) jumps to the return address (using the x86 "ret" instruction, this automatically pops the
    //      return address off the stack
    //
    //////////////////////////////////////////////////////////////////////////////
    //
    // Since we are interfacing with code produced by GCC, we have to respect the
    // calling convention that GCC demands:
    //
    // Contract between caller and callee on x86:
    //    * after call instruction:
    //           o %eip points at first instruction of function
    //           o %esp+4 points at first argument
    //           o %esp points at return address
    //    * after ret instruction:
    //           o %eip contains return address
    //           o %esp points at arguments pushed by caller
    //           o called function may have trashed arguments
    //           o %eax contains return value (or trash if function is void)
    //           o %ecx, %edx may be trashed
    //           o %ebp, %ebx, %esi, %edi must contain contents from time of call
    //    * Terminology:
    //           o %eax, %ecx, %edx are "caller save" registers
    //           o %ebp, %ebx, %esi, %edi are "callee save" registers
    ////////////////////////////////////////////////////////////////////////////////


    void emit_prologue(SymName *name, unsigned int size_locals, unsigned int num_args)
    {

    }

    void emit_epilogue()
    {
    }

  // WRITEME: more functions to emit code

  public:

    Codegen(FILE* outputfile, SymTab* st)
    {
        m_outputfile = outputfile;
        m_st = st;
        label_count = 0;
        al = 1;
        addr = 0;
        pushes = 0;
    }

    void visitProgramImpl(ProgramImpl* p)
    {   m_st->open_scope();
        fprintf(m_outputfile,".text\n\n");
        fprintf(m_outputfile,".globl Main\n");
        p->visit_children(this);
        m_st->close_scope();
    }

    void visitProcImpl(ProcImpl* p)
    { 
        m_st->open_scope();
        //p->visit_children(this);
        p->m_symname->accept(this);
        fprintf(m_outputfile,"\tpushl\t%%ebp\n");
        fprintf(m_outputfile,"\tmovl\t%%esp, %%ebp\n");
        std::list<Decl_ptr>::iterator m_decl_list_iter;
        for(m_decl_list_iter = p->m_decl_list->begin();
        m_decl_list_iter != p->m_decl_list->end();
        ++m_decl_list_iter){
            (*m_decl_list_iter)->accept( this );
        }

        auto it = p->m_decl_list->rbegin();
        for(; it != p->m_decl_list->rend(); it++){
            auto it2 = ((DeclImpl*)*it)->m_symname_list->rbegin();
            for(; it2 != ((DeclImpl*)*it)->m_symname_list->rend(); it2++){
                Symbol* s = m_st->lookup(((*it2)->spelling()));
                //fprintf(m_outputfile, "\tpopl\t%%eax\n");
                //fprintf(m_outputfile, "\tmovl\t%d(%%ebp), -%d(%%ebp)\n", s->get_offset() + 8, s->get_offset() + 4);
                fprintf(m_outputfile, "\tmovl\t%d(%%ebp), %%eax\n", s->get_offset() + 8);
                fprintf(m_outputfile, "\tmovl\t%%eax,\t-%d(%%ebp)\n", s->get_offset() + 4);
            }
        }
        p->m_type->accept(this);
        p->m_procedure_block->accept(this);
        m_st->close_scope();
    }

    void visitProcedure_blockImpl(Procedure_blockImpl* p)
    {
        p->visit_children(this);
    }

    void visitNested_blockImpl(Nested_blockImpl* p)
    {   m_st->open_scope();
        p->visit_children(this);
        m_st->close_scope();
    }

    void visitAssignment(Assignment* p)
    {
        p->m_expr->accept(this);
        p->m_lhs->accept(this);

    }

    void visitCall(Call* p)
    {
        std::list<Expr_ptr>::iterator m_expr_list_iter;
        for(m_expr_list_iter = p->m_expr_list->begin();
        m_expr_list_iter != p->m_expr_list->end();
        ++m_expr_list_iter){
            (*m_expr_list_iter)->accept( this);
        }
        fprintf(m_outputfile, "\tcall\t");
        fprintf(m_outputfile, p->m_symname->spelling());
        fprintf(m_outputfile, "\n");
        for(int i = 0; i < p->m_expr_list->size(); i++){
            fprintf(m_outputfile, "\tpopl\t%%edx\n");
        }
        //fprintf(m_outputfile, "\taddl\t$%d, %%esp\n", 4 * p->m_expr_list->size());
        fprintf(m_outputfile, "\tpushl\t%%eax\n");
        p->m_lhs->accept(this);
    }

    void visitReturn(Return* p)
    {
        p->visit_children(this);
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\tmovl\t%%ebp, %%esp\n");
        fprintf(m_outputfile, "\tpopl\t%%ebp\n");
        fprintf(m_outputfile, "\tret\n");
    }

    // Control flow
    void visitIfNoElse(IfNoElse* p)
    {
        p->m_expr->accept(this);
        int label = new_label();
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\tmovl\t$0, %%edx\n");
        fprintf(m_outputfile, "\tcmpl\t%%edx, %%eax\n");
        fprintf(m_outputfile, "\tje Skip_if_1_%d\n", label);
        p->m_nested_block->accept(this);
        fprintf(m_outputfile, "Skip_if_1_%d:\n", label);
    }

    void visitIfWithElse(IfWithElse* p)
    {
        p->m_expr->accept(this);
        int label = new_label();
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\tmovl\t$0, %%edx\n");
        fprintf(m_outputfile, "\tcmpl\t%%edx, %%eax\n");
        fprintf(m_outputfile, "\tje\tSkip_if_1_%d\n", label);
        p->m_nested_block_1->accept(this);
        fprintf(m_outputfile, "\tjmp\tdone%d\n", label);
        fprintf(m_outputfile, "Skip_if_1_%d:\n", label);
        p->m_nested_block_2->accept(this);
        fprintf(m_outputfile, "done%d:\n", label);
    }

    void visitWhileLoop(WhileLoop* p)
    {
        int label = new_label();
        fprintf(m_outputfile, "While_Expression_%d:\n", label);
        p->m_expr->accept(this);
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\tmovl\t$0, %%edx\n");
        fprintf(m_outputfile, "\tcmpl\t%%edx, %%eax\n");
        fprintf(m_outputfile, "\tje\tSkip_if_1_%d\n", label);
        p->m_nested_block->accept(this);
        fprintf(m_outputfile, "\tjmp\tWhile_Expression_%d\n", label);
        fprintf(m_outputfile, "Skip_if_1_%d:\n", label);
    }

    void visitCodeBlock(CodeBlock *p) 
    {
        p->visit_children(this);
    }

    // Variable declarations (no code generation needed)
    void visitDeclImpl(DeclImpl* p)
    {
        p->m_type->accept(this);
        int allocation = p->m_symname_list->size();
        fprintf(m_outputfile,"\tsubl\t$%d, %%esp\n", 4 * allocation * al);
        auto i = p->m_symname_list->begin();
        for(i; i != p->m_symname_list->end(); i++){
            Symbol* s = new Symbol();
            s->m_basetype = p->m_type->m_attribute.m_basetype;
            char* name = (char*)(*i)->spelling();
            m_st->insert(name, s);

        }
        p->visit_children(this);   
    }

    void visitTInteger(TInteger* p)
    {
        al = 1;
    }

    void visitTIntPtr(TIntPtr* p)
    {
        al = 1;
    }

    void visitTBoolean(TBoolean* p)
    {
        al = 1;
   
    }

    void visitTCharacter(TCharacter* p)
    {
        al = 1;
    }

    void visitTCharPtr(TCharPtr* p)
    {
        al = 1;
    }

    void visitTString(TString* p)
    {
        al = p->m_primitive->m_data;
    }


    // Comparison operations
    void visitCompare(Compare* p)
    {
        p->visit_children(this);
        fprintf(m_outputfile, "\tpopl\t%%edx\n");
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\tcmpl\t%%edx, %%eax\n");
        int label = new_label();
        fprintf(m_outputfile, "\tje\tequal%d\n", label);
        //Code for if not equal
        fprintf(m_outputfile, "not_equal%d:\n", label);
        fprintf(m_outputfile, "\tpushl\t$0\n");
        fprintf(m_outputfile, "\tjmp\tdone%d\n", label);
        //code for if equal
        fprintf(m_outputfile, "equal%d:\n", label);
        fprintf(m_outputfile, "\tpushl\t$1\n");
        fprintf(m_outputfile, "done%d:\n", label);
    }

    void visitNoteq(Noteq* p)
    {
        p->visit_children(this);
        fprintf(m_outputfile, "\tpopl\t%%edx\n");
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\tcmpl\t%%edx, %%eax\n");
        int label = new_label();
        fprintf(m_outputfile, "\tje\tequal%d\n", label);
        //Code for if not equal
        fprintf(m_outputfile, "not_equal%d:\n", label);
        fprintf(m_outputfile, "\tpushl\t$1\n");
        fprintf(m_outputfile, "\tjmp\tdone%d\n", label);
        //code for if equal
        fprintf(m_outputfile, "equal%d:\n", label);
        fprintf(m_outputfile, "\tpushl\t$0\n");
        fprintf(m_outputfile, "done%d:\n", label);
    }

    void visitGt(Gt* p)
    {
        p->visit_children(this);
        fprintf(m_outputfile, "\tpopl\t%%edx\n");
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\tcmpl\t%%edx, %%eax\n");
        int label = new_label();
        fprintf(m_outputfile, "\tjg greater_than_label%d\n", label);
        fprintf(m_outputfile, "not_greater_than%d:\n", label);
        fprintf(m_outputfile, "\tpushl\t$0\n");
        fprintf(m_outputfile, "\tjmp\tdone%d\n", label);
        fprintf(m_outputfile, "greater_than_label%d:\n", label);
        fprintf(m_outputfile, "\tpushl\t$1\n");
        fprintf(m_outputfile, "done%d:\n", label);
    }

    void visitGteq(Gteq* p)
    {
        p->visit_children(this);
        fprintf(m_outputfile, "\tpopl\t%%edx\n");
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\tcmpl\t%%edx, %%eax\n");
        int label = new_label();
        fprintf(m_outputfile, "\tjge greater_eq_than_label%d\n", label);
        fprintf(m_outputfile, "not_greater_eq_than%d:\n", label);
        fprintf(m_outputfile, "\tpushl\t$0\n");
        fprintf(m_outputfile, "\tjmp\tdone%d\n", label);
        fprintf(m_outputfile, "greater_eq_than_label%d:\n", label);
        fprintf(m_outputfile, "\tpushl\t$1\n");
        fprintf(m_outputfile, "done%d:\n", label);
    }

    void visitLt(Lt* p)
    {
        p->visit_children(this);
        fprintf(m_outputfile, "\tpopl\t%%edx\n");
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\tcmpl\t%%edx, %%eax\n");
        int label = new_label();
        fprintf(m_outputfile, "\tjl less_than_label%d\n", label);
        fprintf(m_outputfile, "not_less_than%d:\n", label);
        fprintf(m_outputfile, "\tpushl\t$0\n");
        fprintf(m_outputfile, "\tjmp\tdone%d\n", label);
        fprintf(m_outputfile, "less_than_label%d:\n", label);
        fprintf(m_outputfile, "\tpushl\t$1\n");
        fprintf(m_outputfile, "done%d:\n", label);
    }

    void visitLteq(Lteq* p)
    {
        p->visit_children(this);
        fprintf(m_outputfile, "\tpopl\t%%edx\n");
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\tcmpl\t%%edx, %%eax\n");
        int label = new_label();
        fprintf(m_outputfile, "\tjle less_eq_than_label%d\n", label);
        fprintf(m_outputfile, "not_less_eq_than%d:\n", label);
        fprintf(m_outputfile, "\tpushl\t$0\n");
        fprintf(m_outputfile, "\tjmp\tdone%d\n", label);
        fprintf(m_outputfile, "less_eq_than_label%d:\n", label);
        fprintf(m_outputfile, "\tpushl\t$1\n");
        fprintf(m_outputfile, "done%d:\n", label);
    }

    // Arithmetic and logic operations
    void visitAnd(And* p)
    {
        p->visit_children(this);
        fprintf(m_outputfile, "\tpopl\t%%edx\n");
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\tandl\t%%edx, %%eax\n");
        fprintf(m_outputfile, "\tpushl\t%%eax\n");
    }

    void visitOr(Or* p)
    {
        p->visit_children(this);
        fprintf(m_outputfile, "\tpopl\t%%edx\n");
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\torl\t%%edx, %%eax\n");
        fprintf(m_outputfile, "\tpushl\t%%eax\n");    }

    void visitMinus(Minus* p)
    {
        p->visit_children(this);
        fprintf(m_outputfile, "\tpopl\t%%edx\n");
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\tsubl\t%%edx, %%eax\n");
        fprintf(m_outputfile, "\tpushl\t%%eax\n");

    }

    void visitPlus(Plus* p)
    {
        p->visit_children(this);
        fprintf(m_outputfile, "\tpopl\t%%edx\n");
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\taddl\t%%edx, %%eax\n");
        fprintf(m_outputfile, "\tpushl\t%%eax\n");
    }

    void visitTimes(Times* p)
    {
        p->visit_children(this);
        fprintf(m_outputfile, "\tpopl\t%%edx\n");
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\timull\t%%edx, %%eax\n");
        fprintf(m_outputfile, "\tpushl\t%%eax\n");
    }

    void visitDiv(Div* p)
    {
        p->visit_children(this);
        fprintf(m_outputfile, "\tpopl\t%%ecx\n");
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\tcdq\n");
        fprintf(m_outputfile, "\tidivl\t%%ecx\n");
        fprintf(m_outputfile, "\tpushl\t%%eax\n");

        /*Need to set edx sign correctly. Note that Numerator has to be loaded into eax & edx contains
        the remainder after the operation is completed*/
    }

    void visitNot(Not* p)
    {
        p->visit_children(this);
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\txorl\t$1, %%eax\n");
        fprintf(m_outputfile, "\tpushl\t%%eax\n");
    }

    void visitUminus(Uminus* p)
    {
        p->visit_children(this);
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\tnegl\t%%eax\n");
        fprintf(m_outputfile, "\tpushl\t%%eax\n");
    }

    // Variable and constant access
    void visitIdent(Ident* p)
    {
        Symbol* s = m_st->lookup(p->m_symname->spelling());
        int offset = s->get_offset();
        fprintf(m_outputfile,"\tpushl\t-%d(%%ebp)\n", offset + 4);
    }

    void visitBoolLit(BoolLit* p)
    {
        fprintf(m_outputfile, "\tpushl\t$");
        visitPrimitive(p->m_primitive);
    }

    void visitCharLit(CharLit* p)
    {
        fprintf(m_outputfile, "\tpushl\t$");
        p->m_primitive->accept(this);
    }

    void visitIntLit(IntLit* p)
    {
        fprintf(m_outputfile, "\tpushl\t$");
        visitPrimitive(p->m_primitive);
    }

    void visitNullLit(NullLit* p)
    {
        fprintf(m_outputfile, "\tpushl\t$0\n");
    }

    void visitArrayAccess(ArrayAccess* p)
    {
        Symbol*s = m_st->lookup(p->m_symname->spelling());
        int offset = s->get_offset();
        p->m_expr->accept(this);
        fprintf(m_outputfile, "\tpopl\t%%ecx\n");
        fprintf(m_outputfile, "\tleal\t-%d(%%ebp), %%eax\n", offset + 4);
        fprintf(m_outputfile, "\timull\t$4,%%ecx\n");
        fprintf(m_outputfile, "\tsubl\t%%ecx, %%eax\n");
        fprintf(m_outputfile, "\tmovl\t(%%eax), %%eax\n");
        //fprintf(m_outputfile, "\tandl\t$0xFF, %%eax\n");
        fprintf(m_outputfile, "\tpushl\t%%eax\n");
    }

    // LHS
    void visitVariable(Variable* p)
    {
        Symbol* s = m_st->lookup(p->m_symname->spelling());
        int offset = s->get_offset();
        if(addr == 0){
            fprintf(m_outputfile, "\tpopl\t%%eax\n");
            fprintf(m_outputfile, "\tmovl\t%%eax, -%d(%%ebp)\n", offset + 4);
        }
        else if(addr == 1){ //Address of
            fprintf(m_outputfile, "\tleal\t-%d(%%ebp), %%eax\n", offset + 4);
            fprintf(m_outputfile, "\tpushl\t%%eax\n");
        }
        else if(addr == 2){ // string assignment
            for(int i = 0; i < pushes; i++){
                fprintf(m_outputfile, "\tpopl\t%%eax\n");
                fprintf(m_outputfile, "\tmovl\t%%eax, -%d(%%ebp)\n", offset + 4 * (i + 1));
            }
        }

    }

    void visitDerefVariable(DerefVariable* p)
    {
        Symbol* s = m_st->lookup(p->m_symname->spelling());
        int offset = s->get_offset();
        fprintf(m_outputfile, "\tpopl\t%%eax\n");
        fprintf(m_outputfile, "\tmovl\t-%d(%%ebp), %%ecx\n", offset + 4);
        fprintf(m_outputfile, "\tmovl\t%%eax, (%%ecx)\n");
    }

    void visitArrayElement(ArrayElement* p)
    {
        Symbol*s = m_st->lookup(p->m_symname->spelling());
        int offset = s->get_offset();
        p->m_expr->accept(this);
        fprintf(m_outputfile, "\tpopl\t%%eax");
        //fprintf()

    }

    // Special cases
    void visitSymName(SymName* p)
    {
        if(p->m_parent_attribute->m_basetype == bt_procedure){
            fprintf(m_outputfile, p->spelling());
            fprintf(m_outputfile, ":\n");
        }
    }

    void visitPrimitive(Primitive* p)
    {
        fprintf(m_outputfile, "%d\n", p->m_data);
    }

    // Strings
    void visitStringAssignment(StringAssignment* p)
    {   
        int prev = addr;
        addr = 2;
        p->m_stringprimitive->accept(this);
        p->m_lhs->accept(this);   
        addr = prev;
        pushes = 0;
    }

    void visitStringPrimitive(StringPrimitive* p)
    {
        auto s = p->m_string;
        int start = 0;
        int end = 0;
        while(s[end] != '\0'){
            end ++;
        }
        pushes = end;

        while(end >= start){
            fprintf(m_outputfile, "\tpushl\t$%d\n", (int)s[end]);
            end--;
        }
        //fprintf(m_outputfile, "\tpushl\t$0\n");
    }

    void visitAbsoluteValue(AbsoluteValue* p)
    {
        p->visit_children(this);
        fprintf(m_outputfile, "\tpop\t%%eax\n");
        fprintf(m_outputfile, "\tcdq\n");
        fprintf(m_outputfile, "\txorl\t%%edx, %%eax\n");
        fprintf(m_outputfile, "\tsubl\t%%edx,%%eax\n");
        fprintf(m_outputfile, "\tpushl\t%%eax\n");
    }

    // Pointer
    void visitAddressOf(AddressOf* p)
    {
        int prev = addr;
        addr = 1;
        p->m_lhs->accept(this);
        addr = prev;
        //printf("THIS LINE");
    }

    void visitDeref(Deref* p)
    {
        p->m_expr->accept(this);
        fprintf(m_outputfile, "\tpopl\t%%ecx\n");
        fprintf(m_outputfile, "\tmovl\t(%%ecx), %%eax\n");
        fprintf(m_outputfile, "\tpushl\t%%eax\n");
    }
};


void dopass_codegen(Program_ptr ast, SymTab* st)
{
    Codegen* codegen = new Codegen(stdout, st);
    ast->accept(codegen);
    delete codegen;
}