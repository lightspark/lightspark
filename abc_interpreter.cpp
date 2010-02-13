#include "abc.h"
#include "compat.h"
#include <string>
#include <sstream>

using namespace std;
using namespace lightspark;

ASObject* ABCVm::executeFunction(SyntheticFunction* function, call_context* context)
{
	method_info* mi=function->mi;

	stringstream code(mi->body->code);

	u8 opcode;

	//Each case block builds the correct parameters for the interpreter function and call it
	while(1)
	{
		code >> opcode;

		switch(opcode)
		{
/*			case 0x03:
			{
				//throw
				LOG(LOG_TRACE, "synt throw" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				//syncLocals(ex,Builder,static_locals,locals,cur_block->locals,*cur_block);
				Builder.CreateCall(ex->FindFunctionNamed("throw"), context);
				//Right now we set up like we do for retunrvoid
				last_is_branch=true;
				constant = llvm::ConstantInt::get(int_type, NULL);
				value = llvm::ConstantExpr::getIntToPtr(constant, llvm::PointerType::getUnqual(int_type));
				for(int i=0;i<static_locals.size();i++)
				{
					if(static_locals[i].second==STACK_OBJECT)
						Builder.CreateCall(ex->FindFunctionNamed("decRef"),static_locals[i].first);
					static_locals[i].second=STACK_NONE;
				}
				for(int i=0;i<static_stack.size();i++)
				{
					if(static_stack[i].second==STACK_OBJECT)
						Builder.CreateCall(ex->FindFunctionNamed("decRef"),static_stack[i].first);
				}
				Builder.CreateRet(value);
				break;
			}
			case 0x04:
			{
				//getsuper
				LOG(LOG_TRACE, "synt getsuper" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("getSuper"), context, constant);
				break;
			}
			case 0x05:
			{
				//setsuper
				LOG(LOG_TRACE, "synt setsuper" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("setSuper"), context, constant);
				break;
			}
			case 0x08:
			{
				//kill
				u30 t;
				code2 >> t;
				LOG(LOG_TRACE, "synt kill " << t );
				if(Log::getLevel()==LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, t);
					Builder.CreateCall2(ex->FindFunctionNamed("kill"), context, constant);
				}
				int i=t;
				if(static_locals[i].second==STACK_OBJECT)
					Builder.CreateCall(ex->FindFunctionNamed("decRef"), static_locals[i].first);

				static_locals[i].second=STACK_NONE;

				break;
			}
			case 0x09:
			{
				//label
				//Create a new block and insert it in the mapping
				LOG(LOG_TRACE, "synt label" );
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(ex->FindFunctionNamed("label"));
				break;
			}
			case 0x0c:
			{
				//ifnlt
				LOG(LOG_TRACE, "synt ifnlt" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code2 >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				llvm::Value* cond;
				if(v1.second==STACK_INT && v2.second==STACK_INT)
					cond=Builder.CreateICmpSGE(v2.first,v1.first); //GE == NLT
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifNLT"), v1.first, v2.first);
				}

				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code2.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x0d:
			{
				//ifnle
				LOG(LOG_TRACE, "synt ifnle");
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code2 >> t;
				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				llvm::Value* cond=Builder.CreateCall2(ex->FindFunctionNamed("ifNLE"), v1.first, v2.first);
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code2.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x0e:
			{
				//ifngt
				LOG(LOG_TRACE, "synt ifngt" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code2 >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				llvm::Value* cond=Builder.CreateCall2(ex->FindFunctionNamed("ifNGT"), v1.first, v2.first);
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code2.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x0f:
			{
				//ifnge
				LOG(LOG_TRACE, "synt ifnge");
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code2 >> t;
				//Make comparision

				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				llvm::Value* cond=Builder.CreateCall2(ex->FindFunctionNamed("ifNGE"), v1.first, v2.first);
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code2.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x10:
			{
				//jump
				last_is_branch=true;

				s24 t;
				code2 >> t;
				LOG(LOG_TRACE, "synt jump " << t );
				if(Log::getLevel()==LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, t);
					Builder.CreateCall2(ex->FindFunctionNamed("jump"), context, constant);
				}
				int here=code2.tellg();
				int dest=here+t;
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x11:
			{
				//iftrue
				LOG(LOG_TRACE, "synt iftrue" );
				last_is_branch=true;
				s24 t;
				code2 >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				if(v1.second==STACK_BOOLEAN)
					cond=v1.first;
				else
					cond=Builder.CreateCall(ex->FindFunctionNamed("ifTrue"), v1.first);
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code2.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x12:
			{
				//iffalse

				last_is_branch=true;
				s24 t;
				code2 >> t;
				LOG(LOG_TRACE, "synt iffalse " << t );

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				if(v1.second==STACK_BOOLEAN)
					cond=Builder.CreateNot(v1.first);
				else
				{
					abstract_value(ex,Builder,v1);
					cond=Builder.CreateCall(ex->FindFunctionNamed("ifFalse"), v1.first);
				}
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code2.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x13:
			{
				//ifeq
				LOG(LOG_TRACE, "synt ifeq" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code2 >> t;
			
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				//Make comparision
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifEq"), v1.first, v2.first);
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
				{
					v1.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v1.first);
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifEq"), v1.first, v2.first);
				}
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
				{
					v2.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v2.first);
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifEq"), v1.first, v2.first);
				}
				else if(v1.second==STACK_INT && v2.second==STACK_NUMBER)
				{
					v1.first=Builder.CreateSIToFP(v1.first,number_type);
					cond=Builder.CreateFCmpOEQ(v1.first,v2.first);
				}
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifEq"), v1.first, v2.first);
				}
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code2.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x14:
			{
				//ifne
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code2 >> t;
				LOG(LOG_TRACE, "synt ifne " << t );
			
				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifNE_oi"), v2.first, v1.first);
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifNE_oi"), v1.first, v2.first);
				else if(v1.second==STACK_INT && v2.second==STACK_NUMBER)
				{
					v1.first=Builder.CreateSIToFP(v1.first,number_type);
					cond=Builder.CreateFCmpONE(v1.first,v2.first);
				}
				else if(v1.second==STACK_INT && v2.second==STACK_INT)
				{
					cond=Builder.CreateICmpNE(v1.first,v2.first);
				}
				else
				{
					//Abstract default
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifNE"), v1.first, v2.first);
				}
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code2.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x15:
			{
				//iflt
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code2 >> t;
				LOG(LOG_TRACE, "synt iflt "<< t );

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifLT"), v1.first, v2.first);
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifLT_io"), v1.first, v2.first);
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifLT_oi"), v1.first, v2.first);
				else if(v1.second==STACK_INT && v2.second==STACK_INT)
					cond=Builder.CreateICmpSLT(v2.first,v1.first);
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifLT"), v1.first, v2.first);
				}
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code2.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x16:
			{
				//ifle
				LOG(LOG_TRACE, "synt ifle" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code2 >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifLE"), v1.first, v2.first);
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifLE"), v1.first, v2.first);
				}
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code2.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x17:
			{
				//ifgt
				LOG(LOG_TRACE, "synt ifgt" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code2 >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				cond=Builder.CreateCall2(ex->FindFunctionNamed("ifGT"), v1.first, v2.first);
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code2.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x18:
			{
				//ifge
				LOG(LOG_TRACE, "synt ifge");
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code2 >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				cond=Builder.CreateCall2(ex->FindFunctionNamed("ifGE"), v1.first, v2.first);
			
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code2.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x19:
			{
				//ifstricteq
				LOG(LOG_TRACE, "synt ifstricteq" );
				//TODO: implement common data comparison
				last_is_branch=true;
				s24 t;
				code2 >> t;
			
				//Make comparision
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* cond=Builder.CreateCall2(ex->FindFunctionNamed("ifStrictEq"), v1, v2);

				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code2.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x1a:
			{
				//ifstrictne
				LOG(LOG_TRACE, "synt ifstrictne" );
				last_is_branch=true;
				s24 t;
				code2 >> t;

				//Make comparision
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* cond;
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifStrictNE"), v1.first, v2.first);
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
				{
					v1.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v1.first);
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifStrictNE"), v1.first, v2.first);
				}
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
				{
					v2.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v2.first);
					cond=Builder.CreateCall2(ex->FindFunctionNamed("ifStrictNE"), v1.first, v2.first);
				}
				else
					abort();

				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				int here=code2.tellg();
				int dest=here+t;
				llvm::BasicBlock* A=llvm::BasicBlock::Create(llvm_context,"epilogueA", llvmf);
				llvm::BasicBlock* B=llvm::BasicBlock::Create(llvm_context,"epilogueB", llvmf);
				Builder.CreateCondBr(cond,B,A);
				Builder.SetInsertPoint(A);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[here]);
				Builder.CreateBr(blocks[here].BB);

				Builder.SetInsertPoint(B);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[dest]);
				Builder.CreateBr(blocks[dest].BB);
				break;
			}
			case 0x1b:
			{
				//lookupswitch
				LOG(LOG_TRACE, "synt lookupswitch" );
				if(Log::getLevel()>=LOG_CALLS)
					Builder.CreateCall(ex->FindFunctionNamed("lookupswitch"));
				last_is_branch=true;

				int here=int(code2.tellg())-1; //Base for the jumps is the instruction itself for the switch
				s24 t;
				code2 >> t;
				int defaultdest=here+t;
				LOG(LOG_TRACE,"default " << int(t));
				u30 count;
				code2 >> count;
				LOG(LOG_TRACE,"count "<< int(count));
				vector<s24> offsets(count+1);
				for(int i=0;i<count+1;i++)
				{
					code2 >> offsets[i];
					LOG(LOG_TRACE,"Case " << i << " " << offsets[i]);
				}
				stack_entry e=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(e.second==STACK_INT);
				else if(e.second==STACK_OBJECT)
				{
					e.first=Builder.CreateCall(ex->FindFunctionNamed("toInt"),e.first);
				}
				else
					abort();
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);

				//Generate epilogue for default dest
				llvm::BasicBlock* Default=llvm::BasicBlock::Create(llvm_context,"epilogueDest", llvmf);

				llvm::SwitchInst* sw=Builder.CreateSwitch(e.first,Default);
				Builder.SetInsertPoint(Default);
				syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[defaultdest]);
				Builder.CreateBr(blocks[defaultdest].BB);

				for(int i=0;i<offsets.size();i++)
				{
					int casedest=here+offsets[i];
					llvm::ConstantInt* constant = static_cast<llvm::ConstantInt*>(llvm::ConstantInt::get(int_type, i));
					llvm::BasicBlock* Case=llvm::BasicBlock::Create(llvm_context,"epilogueCase", llvmf);
					sw->addCase(constant,Case);
					Builder.SetInsertPoint(Case);
					syncLocals(ex,Builder,static_locals,locals,cur_block->locals,blocks[casedest]);
					Builder.CreateBr(blocks[casedest].BB);
				}

				break;
			}
			case 0x1c:
			{
				//pushwith
				LOG(LOG_TRACE, "synt pushwith" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				Builder.CreateCall(ex->FindFunctionNamed("pushWith"), context);
				break;
			}
			case 0x1d:
			{
				//popscope
				LOG(LOG_TRACE, "synt popscope" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				Builder.CreateCall(ex->FindFunctionNamed("popScope"), context);
				break;
			}
			case 0x1e:
			{
				//nextname
				LOG(LOG_TRACE, "synt nextname" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;

				value=Builder.CreateCall2(ex->FindFunctionNamed("nextName"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x21:
			{
				//pushundefined
				LOG(LOG_TRACE, "synt pushundefined" );
				value=Builder.CreateCall(ex->FindFunctionNamed("pushUndefined"));
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x23:
			{
				//nextvalue
				LOG(LOG_TRACE, "synt nextvalue" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;

				value=Builder.CreateCall2(ex->FindFunctionNamed("nextValue"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x24:
			{
				//pushbyte
				LOG(LOG_TRACE, "synt pushbyte" );
				int8_t t;
				code2.read((char*)&t,1);
				constant = llvm::ConstantInt::get(int_type, (int)t);
				static_stack_push(static_stack,stack_entry(constant,STACK_INT));
				if(Log::getLevel()==LOG_CALLS)
					value=Builder.CreateCall(ex->FindFunctionNamed("pushByte"), constant);
				break;
			}
			case 0x25:
			{
				//pushshort
				LOG(LOG_TRACE, "synt pushshort" );
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				static_stack_push(static_stack,stack_entry(constant,STACK_INT));
				if(Log::getLevel()==LOG_CALLS)
					Builder.CreateCall(ex->FindFunctionNamed("pushShort"), constant);
				break;
			}
			case 0x26:
			{
				//pushtrue
				LOG(LOG_TRACE, "synt pushtrue" );
				value=Builder.CreateCall(ex->FindFunctionNamed("pushTrue"));
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0x27:
			{
				//pushfalse
				LOG(LOG_TRACE, "synt pushfalse" );
				value=Builder.CreateCall(ex->FindFunctionNamed("pushFalse"));
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0x28:
			{
				//pushnan
				LOG(LOG_TRACE, "synt pushnan" );
				value=Builder.CreateCall(ex->FindFunctionNamed("pushNaN"));
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x29:
			{
				//pop
				LOG(LOG_TRACE, "synt pop" );
				if(Log::getLevel()==LOG_CALLS)
					Builder.CreateCall(ex->FindFunctionNamed("pop"));
				stack_entry e=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(e.second==STACK_OBJECT)
					Builder.CreateCall(ex->FindFunctionNamed("decRef"), e.first);
				break;
			}
			case 0x2a:
			{
				//dup
				LOG(LOG_TRACE, "synt dup" );
				if(Log::getLevel()==LOG_CALLS)
					Builder.CreateCall(ex->FindFunctionNamed("dup"));
				stack_entry e=static_stack_peek(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				static_stack_push(static_stack,e);

				if(e.second==STACK_OBJECT)
					Builder.CreateCall(ex->FindFunctionNamed("incRef"), e.first);
				break;
			}
			case 0x2b:
			{
				//swap
				LOG(LOG_TRACE, "synt swap" );
				if(Log::getLevel()==LOG_CALLS)
					Builder.CreateCall(ex->FindFunctionNamed("swap"));
				stack_entry e1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry e2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				static_stack_push(static_stack,e1);
				static_stack_push(static_stack,e2);
				break;
			}
			case 0x2c:
			{
				//pushstring
				LOG(LOG_TRACE, "synt pushstring" );
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				value=Builder.CreateCall2(ex->FindFunctionNamed("pushString"), context, constant);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x2d:
			{
				//pushint
				LOG(LOG_TRACE, "synt pushint" );
				u30 t;
				code2 >> t;
				if(Log::getLevel()==LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, t);
					Builder.CreateCall2(ex->FindFunctionNamed("pushInt"), context, constant);
				}
				s32 i=this->context->constant_pool.integer[t];
				constant = llvm::ConstantInt::get(int_type, i);
				static_stack_push(static_stack,stack_entry(constant,STACK_INT));
				break;
			}
			case 0x2f:
			{
				//pushdouble
				LOG(LOG_TRACE, "synt pushdouble" );
				u30 t;
				code2 >> t;

				if(Log::getLevel()==LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, t);
					Builder.CreateCall2(ex->FindFunctionNamed("pushDouble"), context, constant);
				}
				number_t d=this->context->constant_pool.doubles[t];
				constant = llvm::ConstantFP::get(number_type,d);
				static_stack_push(static_stack,stack_entry(constant,STACK_NUMBER));
				break;
			}
			case 0x32:
			{
				//hasnext2
				LOG(LOG_TRACE, "synt hasnext2" );
				u30 t,t2;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code2 >> t2;
				constant2 = llvm::ConstantInt::get(int_type, t2);
				//Sync the locals to memory
				if(static_locals[t].second!=STACK_NONE)
				{
					llvm::Value* gep=Builder.CreateGEP(locals,constant);
					llvm::Value* old=Builder.CreateLoad(gep);
					Builder.CreateCall(ex->FindFunctionNamed("decRef"), old);
					abstract_value(ex,Builder,static_locals[t]);
					Builder.CreateStore(static_locals[t].first,gep);
					static_locals[t].second=STACK_NONE;
				}

				if(static_locals[t2].second!=STACK_NONE)
				{
					llvm::Value* gep=Builder.CreateGEP(locals,constant2);
					llvm::Value* old=Builder.CreateLoad(gep);
					Builder.CreateCall(ex->FindFunctionNamed("decRef"), old);
					abstract_value(ex,Builder,static_locals[t2]);
					Builder.CreateStore(static_locals[t2].first,gep);
					static_locals[t2].second=STACK_NONE;
				}

				value=Builder.CreateCall3(ex->FindFunctionNamed("hasNext2"), context, constant, constant2);
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0x40:
			{
				//newfunction
				LOG(LOG_TRACE, "synt newfunction" );
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				value=Builder.CreateCall2(ex->FindFunctionNamed("newFunction"), context, constant);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x41:
			{
				//call
				LOG(LOG_TRACE, "synt call" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("call"), context, constant);
				break;
			}
			case 0x42:
			{
				//construct
				LOG(LOG_TRACE, "synt construct" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("construct"), context, constant);
				break;
			}
			case 0x45:
			{
				//callsuper
				LOG(LOG_TRACE, "synt callsuper" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code2 >> t;
				constant2 = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall3(ex->FindFunctionNamed("callSuper"), context, constant, constant2);
				break;
			}
			case 0x46:
			{
				//callproperty
				//TODO: Implement static resolution where possible
				LOG(LOG_TRACE, "synt callproperty" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code2 >> t;
				constant2 = llvm::ConstantInt::get(int_type, t);

				//Call the function resolver, static case could be resolved at this time (TODO)
				Builder.CreateCall3(ex->FindFunctionNamed("callProperty"), context, constant, constant2);

				break;
			}
			case 0x47:
			{
				//returnvoid
				LOG(LOG_TRACE, "synt returnvoid" );
				last_is_branch=true;
				constant = llvm::ConstantInt::get(int_type, NULL);
				value = llvm::ConstantExpr::getIntToPtr(constant, llvm::PointerType::getUnqual(int_type));
				for(int i=0;i<static_locals.size();i++)
				{
					if(static_locals[i].second==STACK_OBJECT)
						Builder.CreateCall(ex->FindFunctionNamed("decRef"),static_locals[i].first);
					static_locals[i].second=STACK_NONE;
				}
				for(int i=0;i<static_stack.size();i++)
				{
					if(static_stack[i].second==STACK_OBJECT)
						Builder.CreateCall(ex->FindFunctionNamed("decRef"),static_stack[i].first);
				}
				Builder.CreateRet(value);
				break;
			}
			case 0x48:
			{
				//returnvalue
				//TODO: Should coerce the return type to the expected one
				LOG(LOG_TRACE, "synt returnvalue" );
				last_is_branch=true;
				stack_entry e=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(e.second==STACK_INT)
					e.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),e.first);
				else if(e.second==STACK_NUMBER)
					e.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_d"),e.first);
				else if(e.second==STACK_OBJECT)
				{
				}
				else if(e.second==STACK_BOOLEAN)
					e.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_b"),e.first);
				else
					abort();
				for(int i=0;i<static_locals.size();i++)
				{
					if(static_locals[i].second==STACK_OBJECT)
						Builder.CreateCall(ex->FindFunctionNamed("decRef"),static_locals[i].first);
					static_locals[i].second=STACK_NONE;
				}
				for(int i=0;i<static_stack.size();i++)
				{
					if(static_stack[i].second==STACK_OBJECT)
						Builder.CreateCall(ex->FindFunctionNamed("decRef"),static_stack[i].first);
				}
				Builder.CreateRet(e.first);
				break;
			}
			case 0x49:
			{
				//constructsuper
				LOG(LOG_TRACE, "synt constructsuper" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("constructSuper"), context, constant);
				break;
			}
			case 0x4a:
			{
				//constructprop
				LOG(LOG_TRACE, "synt constructprop" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code2 >> t;
				constant2 = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall3(ex->FindFunctionNamed("constructProp"), context, constant, constant2);
				break;
			}
			case 0x4e:
			{
				//callsupervoid
				LOG(LOG_TRACE, "synt callsupervoid" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code2 >> t;
				constant2 = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall3(ex->FindFunctionNamed("callSuperVoid"), context, constant, constant2);
				break;
			}
			case 0x4f:
			{
				//callpropvoid
				LOG(LOG_TRACE, "synt callpropvoid" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				code2 >> t;
				constant2 = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall3(ex->FindFunctionNamed("callPropVoid"), context, constant, constant2);
				break;
			}
			case 0x53:
			{
				//constructgenerictype
				LOG(LOG_TRACE, "synt constructgenerictype" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("constructGenericType"), context, constant);
				break;
			}
			case 0x55:
			{
				//newobject
				LOG(LOG_TRACE, "synt newobject" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("newObject"), context, constant);
				break;
			}
			case 0x56:
			{
				//newarray
				LOG(LOG_TRACE, "synt newarray" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("newArray"), context, constant);
				break;
			}
			case 0x57:
			{
				//newactivation
				LOG(LOG_TRACE, "synt newactivation" );
				value=Builder.CreateCall2(ex->FindFunctionNamed("newActivation"), context, th);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x59:
			{
				//getdescendants
				LOG(LOG_TRACE, "synt getdescendants" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("getDescendants"), context, constant);
				break;
			}
			case 0x5a:
			{
				//newcatch
				LOG(LOG_TRACE, "synt newcatch" );
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				value=Builder.CreateCall2(ex->FindFunctionNamed("newCatch"), context, constant);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x5d:
			{
				//findpropstrict
				LOG(LOG_TRACE, "synt findpropstrict" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("findPropStrict"), context, constant);
				break;
			}
			case 0x5e:
			{
				//findproperty
				LOG(LOG_TRACE, "synt findproperty" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("findProperty"), context, constant);
				break;
			}
			case 0x60:
			{
				//getlex
				LOG(LOG_TRACE, "synt getlex" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("getLex"), context, constant);
				break;
			}
			case 0x61:
			{
				//setproperty
				LOG(LOG_TRACE, "synt setproperty" );
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				int rtdata=this->context->getMultinameRTData(t);
				stack_entry value=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				llvm::Value* name;
				//HACK: we need to reinterpret the pointer to the generic type
				llvm::Value* reint_context=Builder.CreateBitCast(context,voidptr_type);
				if(rtdata==0)
				{
					//We pass a dummy second context param
					name = Builder.CreateCall3(ex->FindFunctionNamed("getMultiname"), reint_context, reint_context, constant);
				}
				else if(rtdata==1)
				{
					stack_entry rt1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

					if(rt1.second==STACK_INT)
						name = Builder.CreateCall3(ex->FindFunctionNamed("getMultiname_i"), reint_context, rt1.first, constant);
					else if(rt1.second==STACK_NUMBER)
						name = Builder.CreateCall3(ex->FindFunctionNamed("getMultiname_d"), reint_context, rt1.first, constant);
					else if(rt1.second==STACK_OBJECT)
						name = Builder.CreateCall3(ex->FindFunctionNamed("getMultiname"), reint_context, rt1.first, constant);
					else
						abort();
				}
				stack_entry obj=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(value.second==STACK_INT)
					Builder.CreateCall3(ex->FindFunctionNamed("setProperty_i"),value.first, obj.first, name);
				else if(value.second==STACK_NUMBER)
				{
					value.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_d"),value.first);
					Builder.CreateCall3(ex->FindFunctionNamed("setProperty"),value.first, obj.first, name);
				}
				else if(value.second==STACK_BOOLEAN)
				{
					value.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_b"),value.first);
					Builder.CreateCall3(ex->FindFunctionNamed("setProperty"),value.first, obj.first, name);
				}
				else
					Builder.CreateCall3(ex->FindFunctionNamed("setProperty"),value.first, obj.first, name);
				break;
			}
			case 0x62:
			{
				//getlocal
				LOG(LOG_TRACE, "synt getlocal" );
				u30 i;
				code2 >> i;
				constant = llvm::ConstantInt::get(int_type, i);
				if(Log::getLevel()==LOG_CALLS)
					Builder.CreateCall2(ex->FindFunctionNamed("getLocal"), context, constant);

				if(static_locals[i].second==STACK_NONE)
				{
					llvm::Value* t=Builder.CreateGEP(locals,constant);
					t=Builder.CreateLoad(t,"stack");
					static_stack_push(static_stack,stack_entry(t,STACK_OBJECT));
					static_locals[i]=stack_entry(t,STACK_OBJECT);
					Builder.CreateCall(ex->FindFunctionNamed("incRef"), t);
					Builder.CreateCall(ex->FindFunctionNamed("incRef"), t);
				}
				else if(static_locals[i].second==STACK_OBJECT)
				{
					Builder.CreateCall(ex->FindFunctionNamed("incRef"), static_locals[i].first);
					static_stack_push(static_stack,static_locals[i]);
				}
				else if(static_locals[i].second==STACK_INT || static_locals[i].second==STACK_NUMBER || static_locals[i].second==STACK_BOOLEAN)
					static_stack_push(static_stack,static_locals[i]);
				else
					abort();

				break;
			}
			case 0x63:
			{
				//setlocal
				u30 i;
				code2 >> i;
				LOG(LOG_TRACE, "synt setlocal " << i);
				stack_entry e=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(static_locals[i].second==STACK_OBJECT)
					Builder.CreateCall(ex->FindFunctionNamed("decRef"), static_locals[i].first);

				static_locals[i]=e;
				if(Log::getLevel()==LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, i);
					Builder.CreateCall2(ex->FindFunctionNamed("setLocal"), context, constant);
				}
				break;
			}
			case 0x64:
			{
				//getglobalscope
				LOG(LOG_TRACE, "synt getglobalscope" );
				value=Builder.CreateCall(ex->FindFunctionNamed("getGlobalScope"), context);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x66:
			{
				//getproperty
				LOG(LOG_TRACE, "synt getproperty" );
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				int rtdata=this->context->getMultinameRTData(t);
				llvm::Value* name;
				//HACK: we need to reinterpret the pointer to the generic type
				llvm::Value* reint_context=Builder.CreateBitCast(context,voidptr_type);
				if(rtdata==0)
				{
					//We pass a dummy second context param
					name = Builder.CreateCall3(ex->FindFunctionNamed("getMultiname"), reint_context, reint_context, constant);
				}
				else if(rtdata==1)
				{
					stack_entry rt1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

					if(rt1.second==STACK_INT)
						name = Builder.CreateCall3(ex->FindFunctionNamed("getMultiname_i"), reint_context, rt1.first, constant);
					else if(rt1.second==STACK_NUMBER)
						name = Builder.CreateCall3(ex->FindFunctionNamed("getMultiname_d"), reint_context, rt1.first, constant);
					else if(rt1.second==STACK_OBJECT)
						name = Builder.CreateCall3(ex->FindFunctionNamed("getMultiname"), reint_context, rt1.first, constant);
					else
						abort();
				}
				stack_entry obj=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				if(cur_block->push_types[local_ip]==STACK_OBJECT ||
					cur_block->push_types[local_ip]==STACK_BOOLEAN)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("getProperty"), obj.first, name);
					static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				}
				else if(cur_block->push_types[local_ip]==STACK_INT)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("getProperty_i"), obj.first, name);
					static_stack_push(static_stack,stack_entry(value,STACK_INT));
				}
				else
					abort();
				break;
			}
			case 0x68:
			{
				//initproperty
				LOG(LOG_TRACE, "synt initproperty" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("initProperty"), context, constant);
				break;
			}
			case 0x6a:
			{
				//deleteproperty
				LOG(LOG_TRACE, "synt deleteproperty" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("deleteProperty"), context, constant);
				break;
			}
			case 0x6c:
			{
				//getslot
				LOG(LOG_TRACE, "synt getslot" );
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall2(ex->FindFunctionNamed("getSlot"), v1, constant);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x6d:
			{
				//setslot
				LOG(LOG_TRACE, "synt setslot" );
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
				{
				}
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					v1.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v1.first);
				else if(v1.second==STACK_NUMBER && v2.second==STACK_OBJECT)
					v1.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_d"),v1.first);
				else if(v1.second==STACK_BOOLEAN && v2.second==STACK_OBJECT)
					v1.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_b"),v1.first);
				else
					abort();

				Builder.CreateCall3(ex->FindFunctionNamed("setSlot"), v1.first, v2.first, constant);
				break;
			}
			case 0x70:
			{
				//convert_s
				LOG(LOG_TRACE, "synt convert_s" );
				break;
			}
			case 0x73:
			{
				//convert_i
				LOG(LOG_TRACE, "synt convert_i" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall(ex->FindFunctionNamed("convert_i"), v1);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x74:
			{
				//convert_u
				LOG(LOG_TRACE, "synt convert_u" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall(ex->FindFunctionNamed("convert_i"), v1);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x75:
			{
				//convert_d
				LOG(LOG_TRACE, "synt convert_d" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall(ex->FindFunctionNamed("convert_d"), v1);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x76:
			{
				//convert_b
				LOG(LOG_TRACE, "synt convert_b" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT)
				{
					value=Builder.CreateCall(ex->FindFunctionNamed("convert_b"), v1.first);
					static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				}
				else if(v1.second==STACK_BOOLEAN)
					static_stack_push(static_stack,v1);
				else
					abort();
				break;
			}
			case 0x78:
			{
				//checkfilter
				LOG(LOG_TRACE, "synt checkfilter" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(ex,Builder,v1);
				value=Builder.CreateCall(ex->FindFunctionNamed("checkfilter"), v1.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x80:
			{
				//coerce
				LOG(LOG_TRACE, "synt coerce" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("coerce"), context, constant);
				break;
			}
			case 0x82:
			{
				//coerce_a
				LOG(LOG_TRACE, "synt coerce_a" );
				if(Log::getLevel()==LOG_CALLS)
					Builder.CreateCall(ex->FindFunctionNamed("coerce_a"));
				stack_entry v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				else
					value=v1.first;
				
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x85:
			{
				//coerce_s
				LOG(LOG_TRACE, "synt coerce_s" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall(ex->FindFunctionNamed("coerce_s"), v1);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x87:
			{
				//astypelate
				LOG(LOG_TRACE, "synt astypelate" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				assert(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT);
				value=Builder.CreateCall2(ex->FindFunctionNamed("asTypelate"),v1.first,v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x90:
			{
				//negate
				LOG(LOG_TRACE, "synt negate" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				abstract_value(ex,Builder,v1);
				value=Builder.CreateCall(ex->FindFunctionNamed("negate"), v1.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x91:
			{
				//increment
				LOG(LOG_TRACE, "synt increment" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT)
					value=Builder.CreateCall(ex->FindFunctionNamed("increment"), v1.first);
				else if(v1.second==STACK_INT)
				{
					constant = llvm::ConstantInt::get(int_type, 1);
					value=Builder.CreateAdd(v1.first,constant);
				}
				else
					abort();
				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0x93:
			{
				//decrement
				LOG(LOG_TRACE, "synt decrement" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT)
				{
					constant = llvm::ConstantInt::get(int_type, 1);
					value=Builder.CreateSub(v1.first,constant);
				}
				else
				{
					abstract_value(ex,Builder,v1);
					value=Builder.CreateCall(ex->FindFunctionNamed("decrement"), v1.first);
				}
				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0x95:
			{
				//typeof
				LOG(LOG_TRACE, "synt typeof" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall(ex->FindFunctionNamed("typeOf"), v1);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0x96:
			{
				//not
				LOG(LOG_TRACE, "synt not" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT)
					value=Builder.CreateCall(ex->FindFunctionNamed("not"), v1.first);
				else if(v1.second==STACK_BOOLEAN)
					value=Builder.CreateNot(v1.first);
				else
					abort();
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0x97:
			{
				//bitnot
				LOG(LOG_TRACE, "synt bitnot" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT)
					value=Builder.CreateCall(ex->FindFunctionNamed("bitNot"), v1.first);
				else
					abort();
				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xa0:
			{
				//add
				LOG(LOG_TRACE, "synt add" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("add"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				}
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("add_oi"), v2.first, v1.first);
					static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				}
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("add_oi"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				}
				else if(v1.second==STACK_OBJECT && v2.second==STACK_NUMBER)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("add_od"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_OBJECT)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("add_od"), v2.first, v1.first);
					static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				}
				else if(v1.second==STACK_INT && v2.second==STACK_INT)
				{
					value=Builder.CreateAdd(v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_INT));
				}
				else if(v1.second==STACK_INT && v2.second==STACK_NUMBER)
				{
					//TODO: 32bit check
					v1.first=Builder.CreateSIToFP(v1.first,number_type);
					value=Builder.CreateAdd(v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_INT)
				{
					//TODO: 32bit check
					v2.first=Builder.CreateSIToFP(v2.first,number_type);
					value=Builder.CreateAdd(v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_NUMBER)
				{
					//TODO: 32bit check
					value=Builder.CreateAdd(v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else
					abort();

				break;
			}
			case 0xa1:
			{
				//subtract
				LOG(LOG_TRACE, "synt subtract" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_INT)
				{
					value=Builder.CreateSub(v2.first, v1.first);
					static_stack_push(static_stack,stack_entry(value,STACK_INT));
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_NUMBER)
				{
					value=Builder.CreateSub(v2.first, v1.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_INT && v2.second==STACK_NUMBER)
				{
					v1.first=Builder.CreateSIToFP(v1.first,number_type);
					value=Builder.CreateSub(v2.first, v1.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_INT)
				{
					v2.first=Builder.CreateSIToFP(v2.first,number_type);
					value=Builder.CreateSub(v2.first, v1.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("subtract_oi"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_OBJECT)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("subtract_do"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("subtract_io"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("subtract"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}

				break;
			}
			case 0xa2:
			{
				//multiply
				LOG(LOG_TRACE, "synt multiply" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("multiply_oi"), v2.first, v1.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
				{
					value=Builder.CreateCall2(ex->FindFunctionNamed("multiply_oi"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_INT && v2.second==STACK_INT)
				{
					value=Builder.CreateMul(v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_INT));
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_INT)
				{
					v2.first=Builder.CreateSIToFP(v2.first,number_type);
					value=Builder.CreateMul(v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_INT && v2.second==STACK_NUMBER)
				{
					v1.first=Builder.CreateSIToFP(v1.first,number_type);
					value=Builder.CreateMul(v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("multiply"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}

				break;
			}
			case 0xa3:
			{
				//divide
				LOG(LOG_TRACE, "synt divide" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=	static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				if(v1.second==STACK_INT && v2.second==STACK_NUMBER)
				{
					v1.first=Builder.CreateSIToFP(v1.first,number_type);
					value=Builder.CreateFDiv(v2.first,v1.first);
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_INT)
				{
					v2.first=Builder.CreateSIToFP(v2.first,number_type);
					value=Builder.CreateFDiv(v2.first,v1.first);
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_NUMBER)
				{
					value=Builder.CreateFDiv(v2.first,v1.first);
				}
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("divide"), v1.first, v2.first);
				}
				static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				break;
			}
			case 0xa4:
			{
				//modulo
				LOG(LOG_TRACE, "synt modulo" );
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_NUMBER)
				{
					abort();
					v1.first=Builder.CreateSIToFP(v1.first,number_type);
					value=Builder.CreateCall2(ex->FindFunctionNamed("modulo"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_NUMBER));
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_INT)
				{
					v1.first=Builder.CreateFPToSI(v1.first,int_type);
					value=Builder.CreateSRem(v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_INT));
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_NUMBER)
				{
					v1.first=Builder.CreateFPToSI(v1.first,int_type);
					v2.first=Builder.CreateFPToSI(v2.first,int_type);
					value=Builder.CreateSRem(v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_INT));
				}
				else if(v1.second==STACK_INT && v2.second==STACK_INT)
				{
					value=Builder.CreateSRem(v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_INT));
				}
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("modulo"), v1.first, v2.first);
					static_stack_push(static_stack,stack_entry(value,STACK_INT));
				}
				break;
			}
			case 0xa5:
			{
				//lshift
				LOG(LOG_TRACE, "synt lshift" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					value=Builder.CreateCall2(ex->FindFunctionNamed("lShift_io"), v1.first, v2.first);
				else if(v1.second==STACK_INT && v2.second==STACK_INT)
					value=Builder.CreateShl(v2.first,v1.first); //Check for trucation of v1.first
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("lShift"), v1.first, v2.first);
				}

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xa6:
			{
				//rshift
				LOG(LOG_TRACE, "synt rshift" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
				{
					v1.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v1.first);
					value=Builder.CreateCall2(ex->FindFunctionNamed("rShift"), v1.first, v2.first);
				}
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("lShift"), v1.first, v2.first);
				}

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xa7:
			{
				//urshift
				LOG(LOG_TRACE, "synt urshift" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
					value=Builder.CreateCall2(ex->FindFunctionNamed("urShift"), v1.first, v2.first);
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
					exit(43);
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					value=Builder.CreateCall2(ex->FindFunctionNamed("urShift_io"), v1.first, v2.first);
				else if(v1.second==STACK_INT && v2.second==STACK_INT)
					value=Builder.CreateLShr(v2.first,v1.first); //Check for trucation of v1.first
				else if(v1.second==STACK_INT && v2.second==STACK_NUMBER)
				{
					v2.first=Builder.CreateFPToSI(v2.first,int_type);
					value=Builder.CreateLShr(v2.first,v1.first); //Check for trucation of v1.first
				}
				else if(v1.second==STACK_NUMBER && v2.second==STACK_OBJECT)
				{
					v1.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_d"),v1.first);
					value=Builder.CreateCall2(ex->FindFunctionNamed("urShift"), v1.first, v2.first);
				}
				else
					abort();

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xa8:
			{
				//bitand
				LOG(LOG_TRACE, "synt bitand" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					value=Builder.CreateCall2(ex->FindFunctionNamed("bitAnd_oi"), v2.first, v1.first);
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
					value=Builder.CreateCall2(ex->FindFunctionNamed("bitAnd_oi"), v1.first, v2.first);
				else if(v1.second==STACK_INT && v2.second==STACK_INT)
					value=Builder.CreateAnd(v1.first,v2.first);
				else if(v1.second==STACK_NUMBER && v2.second==STACK_INT)
				{
					v1.first=Builder.CreateFPToUI(v1.first,int_type);
					value=Builder.CreateAnd(v1.first,v2.first);
				}
				else if(v1.second==STACK_INT && v2.second==STACK_NUMBER)
				{
					v2.first=Builder.CreateFPToUI(v2.first,int_type);
					value=Builder.CreateAnd(v1.first,v2.first);
				}
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("bitAnd_oo"), v1.first, v2.first);
				}

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xa9:
			{
				//bitor
				LOG(LOG_TRACE, "synt bitor" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_INT)
					value=Builder.CreateOr(v1.first,v2.first);
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
					value=Builder.CreateCall2(ex->FindFunctionNamed("bitOr_oi"), v1.first, v2.first);
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
					value=Builder.CreateCall2(ex->FindFunctionNamed("bitOr_oi"), v2.first, v1.first);
				else if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
					value=Builder.CreateCall2(ex->FindFunctionNamed("bitOr"), v1.first, v2.first);
				else
					abort();

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xaa:
			{
				//bitxor
				LOG(LOG_TRACE, "synt bitxor" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_INT && v2.second==STACK_INT)
					value=Builder.CreateXor(v1.first,v2.first);
				else if(v1.second==STACK_OBJECT && v2.second==STACK_INT)
				{
					v2.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v2.first);
					value=Builder.CreateCall2(ex->FindFunctionNamed("bitXor"), v1.first, v2.first);
				}
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("bitXor"), v1.first, v2.first);
				}

				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xab:
			{
				//equals
				LOG(LOG_TRACE, "synt equals" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT)
					value=Builder.CreateCall2(ex->FindFunctionNamed("equals"), v1.first, v2.first);
				else if(v1.second==STACK_INT && v2.second==STACK_OBJECT)
				{
					v1.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_i"),v1.first);
					value=Builder.CreateCall2(ex->FindFunctionNamed("equals"), v1.first, v2.first);
				}
				else if(v1.second==STACK_BOOLEAN && v2.second==STACK_OBJECT)
				{
					v1.first=Builder.CreateCall(ex->FindFunctionNamed("abstract_b"),v1.first);
					value=Builder.CreateCall2(ex->FindFunctionNamed("equals"), v1.first, v2.first);
				}
				else if(v1.second==STACK_INT && v2.second==STACK_INT)
					value=Builder.CreateICmpEQ(v1.first,v2.first);
				else
				{
					abstract_value(ex,Builder,v1);
					abstract_value(ex,Builder,v2);
					value=Builder.CreateCall2(ex->FindFunctionNamed("equals"), v1.first, v2.first);
				}
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xac:
			{
				//strictequals
				LOG(LOG_TRACE, "synt strictequals" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall2(ex->FindFunctionNamed("strictEquals"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xad:
			{
				//lessthan
				LOG(LOG_TRACE, "synt lessthan" );
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);

				value=Builder.CreateCall2(ex->FindFunctionNamed("lessThan"), v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0xae:
			{
				//lessequals
				LOG(LOG_TRACE, "synt lessequals" );
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				value=Builder.CreateCall2(ex->FindFunctionNamed("lessEquals"), v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0xaf:
			{
				//greaterthan
				LOG(LOG_TRACE, "synt greaterthan" );
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				value=Builder.CreateCall2(ex->FindFunctionNamed("greaterThan"), v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0xb0:
			{
				//greaterequals
				LOG(LOG_TRACE, "synt greaterequals" );
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);

				abstract_value(ex,Builder,v1);
				abstract_value(ex,Builder,v2);
				value=Builder.CreateCall2(ex->FindFunctionNamed("greaterEquals"), v1.first, v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				break;
			}
			case 0xb3:
			{
				//istypelate
				LOG(LOG_TRACE, "synt istypelate" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				stack_entry v2=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				assert(v1.second==STACK_OBJECT && v2.second==STACK_OBJECT);
				value=Builder.CreateCall2(ex->FindFunctionNamed("isTypelate"),v1.first,v2.first);
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xb4:
			{
				//in
				LOG(LOG_TRACE, "synt in" );
				llvm::Value* v1=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				llvm::Value* v2=
					static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index).first;
				value=Builder.CreateCall2(ex->FindFunctionNamed("in"), v1, v2);
				static_stack_push(static_stack,stack_entry(value,STACK_BOOLEAN));
				break;
			}
			case 0xc0:
			{
				//increment_i
				LOG(LOG_TRACE, "synt increment_i" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT)
					value=Builder.CreateCall(ex->FindFunctionNamed("increment_i"), v1.first);
				else if(v1.second==STACK_INT)
				{
					constant = llvm::ConstantInt::get(int_type, 1);
					value=Builder.CreateAdd(v1.first,constant);
				}
				else
					abort();
				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xc1:
			{
				//decrement_i
				LOG(LOG_TRACE, "synt decrement_i" );
				stack_entry v1=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				if(v1.second==STACK_OBJECT)
					value=Builder.CreateCall(ex->FindFunctionNamed("decrement_i"), v1.first);
				else if(v1.second==STACK_INT)
				{
					constant = llvm::ConstantInt::get(int_type, 1);
					value=Builder.CreateSub(v1.first,constant);
				}
				else
					abort();
				static_stack_push(static_stack,stack_entry(value,STACK_INT));
				break;
			}
			case 0xc2:
			{
				//inclocal_i
				LOG(LOG_TRACE, "synt inclocal_i" );
				syncStacks(ex,Builder,static_stack,dynamic_stack,dynamic_stack_index);
				u30 t;
				code2 >> t;
				constant = llvm::ConstantInt::get(int_type, t);
				Builder.CreateCall2(ex->FindFunctionNamed("incLocal_i"), context, constant);
				break;
			}
			case 0xd5:
			case 0xd6:
			case 0xd7:
			{
				//setlocal_n
				int i=opcode&3;
				LOG(LOG_TRACE, "synt setlocal_n " << i );
				stack_entry e=static_stack_pop(Builder,static_stack,dynamic_stack,dynamic_stack_index);
				//DecRef previous local
				if(static_locals[i].second==STACK_OBJECT)
					Builder.CreateCall(ex->FindFunctionNamed("decRef"), static_locals[i].first);

				static_locals[i]=e;
				if(Log::getLevel()==LOG_CALLS)
				{
					constant = llvm::ConstantInt::get(int_type, opcode&3);
					Builder.CreateCall2(ex->FindFunctionNamed("setLocal"), context, constant);
				}
				break;
			}
			case 0xef:
			{
				//debug
				LOG(LOG_TRACE, "synt debug" );
				uint8_t debug_type;
				u30 index;
				uint8_t reg;
				u30 extra;
				code2.read((char*)&debug_type,1);
				code2 >> index;
				code2.read((char*)&reg,1);
				code2 >> extra;
				break;
			}
			case 0xf0:
			{
				//debugline
				LOG(LOG_TRACE, "synt debugline" );
				u30 t;
				code2 >> t;
				break;
			}
			case 0xf1:
			{
				//debugfile
				LOG(LOG_TRACE, "synt debugfile" );
				u30 t;
				code2 >> t;
				break;
			}*/
			case 0x20:
			{
				//pushnull
				context->runtime_stack_push(new Null);
				break;
			}
			case 0x30:
			{
				//pushscope
				pushScope(context);
				break;
			}
			case 0x58:
			{
				//newclass
				u30 t;
				code >> t;
				newClass(context,t);
				break;
			}
			case 0x65:
			{
				//getscopeobject
				u30 t;
				code >> t;
				context->runtime_stack_push(getScopeObject(context,t));
				break;
			}
			case 0xd0:
			case 0xd1:
			case 0xd2:
			case 0xd3:
			{
				//getlocal_n
				int i=opcode&3;
				LOG(LOG_CALLS, "getLocal " << i );
				context->runtime_stack_push(context->locals[i]);

				break;
			}
			default:
				LOG(LOG_ERROR,"Not intepreted instruction @" << code.tellg());
				LOG(LOG_ERROR,"dump " << hex << (unsigned int)opcode);
				abort();
		}
	}

	//We managed to execute all the function
	return context->runtime_stack_pop();
}
