#include "abc.h"
#include "compat.h"
#include <string>
#include <sstream>

using namespace std;
using namespace lightspark;

ASObject* ABCVm::executeFunction(SyntheticFunction* function, call_context* context)
{
	method_info* mi=function->mi;

	int code_len=mi->body->code.size();
	stringstream code(mi->body->code);

	u8 opcode;

	//Each case block builds the correct parameters for the interpreter function and call it
	while(1)
	{
		code >> opcode;
		if(code.eof())
			abort();

		switch(opcode)
		{
/*			case 0x64:
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
			case 0x03:
			{
				//throw
				_throw(context);
				break;
			}
			case 0x04:
			{
				//getsuper
				u30 t;
				code >> t;
				getSuper(context,t);
				break;
			}
			case 0x05:
			{
				//setsuper
				u30 t;
				code >> t;
				setSuper(context,t);
				break;
			}
			case 0x08:
			{
				//kill
				u30 t;
				code >> t;
				assert(context->locals[t]);
				context->locals[t]->decRef();
				context->locals[t]=new Undefined;
				break;
			}
			case 0x09:
			{
				//label
				break;
			}
			case 0x0c:
			{
				//ifnlt
				s24 t;
				code >> t;
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifNLT(v1, v2);

				int here=code.tellg();
				int dest=here+t;

				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
					{
						LOG(LOG_ERROR,"Jump outside of code");
						abort();
					}
					code.seekg(dest);
				}
				break;
			}
			case 0x0d:
			{
				//ifnle
				s24 t;
				code >> t;
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifNLE(v1, v2);

				int here=code.tellg();
				int dest=here+t;

				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
					{
						LOG(LOG_ERROR,"Jump outside of code");
						abort();
					}
					code.seekg(dest);
				}
				break;
			}
			case 0x0e:
			{
				//ifngt
				s24 t;
				code >> t;
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifNGT(v1, v2);

				int here=code.tellg();
				int dest=here+t;

				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to contexte destination, validate on contexte lengcontext
					if(dest >= code_len)
					{
						LOG(LOG_ERROR,"Jump outside of code");
						abort();
					}
					code.seekg(dest);
				}
				break;
			}
			case 0x0f:
			{
				//ifnge
				s24 t;
				code >> t;
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifNGE(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
					{
						LOG(LOG_ERROR,"Jump outside of code");
						abort();
					}
					code.seekg(dest);
				}
				break;
			}
			case 0x10:
			{
				//jump
				s24 t;
			code >> t;

				int here=code.tellg();
				int dest=here+t;

				//Now 'jump' to the destination, validate on the length
				if(dest >= code_len)
				{
					LOG(LOG_ERROR,"Jump outside of code");
					abort();
				}
				code.seekg(dest);
				break;
			}
			case 0x11:
			{
				//iftrue
				s24 t;
				code >> t;

				ASObject* v1=context->runtime_stack_pop();
				bool cond=ifTrue(v1);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
					{
						LOG(LOG_ERROR,"Jump outside of code");
						abort();
					}
					code.seekg(dest);
				}
				break;
			}
			case 0x12:
			{
				//iffalse
				s24 t;
				code >> t;

				ASObject* v1=context->runtime_stack_pop();
				bool cond=ifFalse(v1);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
					{
						LOG(LOG_ERROR,"Jump outside of code");
						abort();
					}
					code.seekg(dest);
				}
				break;
			}
			case 0x13:
			{
				//ifeq
				s24 t;
				code >> t;

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifEq(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
					{
						LOG(LOG_ERROR,"Jump outside of code");
						abort();
					}
					code.seekg(dest);
				}
				break;
			}
			case 0x14:
			{
				//ifne
				s24 t;
				code >> t;

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifNE(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
					{
						LOG(LOG_ERROR,"Jump outside of code");
						abort();
					}
					code.seekg(dest);
				}
				break;
			}
			case 0x15:
			{
				//iflt
				s24 t;
				code >> t;

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifLT(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
					{
						LOG(LOG_ERROR,"Jump outside of code");
						abort();
					}
					code.seekg(dest);
				}
				break;
			}
			case 0x16:
			{
				//ifle
				s24 t;
				code >> t;

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifLE(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
					{
						LOG(LOG_ERROR,"Jump outside of code");
						abort();
					}
					code.seekg(dest);
				}
				break;
			}
			case 0x17:
			{
				//ifgt
				s24 t;
				code >> t;

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifGT(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
					{
						LOG(LOG_ERROR,"Jump outside of code");
						abort();
					}
					code.seekg(dest);
				}
				break;
			}
			case 0x18:
			{
				//ifge
				s24 t;
				code >> t;

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifGE(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
					{
						LOG(LOG_ERROR,"Jump outside of code");
						abort();
					}
					code.seekg(dest);
				}
				break;
			}
			case 0x19:
			{
				//ifstricteq
				s24 t;
				code >> t;

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifStrictEq(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
					{
						LOG(LOG_ERROR,"Jump outside of code");
						abort();
					}
					code.seekg(dest);
				}
				break;
			}
			case 0x1a:
			{
				//ifstrictne
				s24 t;
				code >> t;

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifStrictNE(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
					{
						LOG(LOG_ERROR,"Jump outside of code");
						abort();
					}
					code.seekg(dest);
				}
				break;
			}
			case 0x1b:
			{
				//lookupswitch
				int here=int(code.tellg())-1; //Base for the jumps is the instruction itself for the switch
				s24 t;
				code >> t;
				int defaultdest=here+t;
				LOG(LOG_CALLS,"Switch default dest " << defaultdest);
				u30 count;
				code >> count;
				vector<s24> offsets(count+1);
				for(int i=0;i<count+1;i++)
				{
					code >> offsets[i];
					LOG(LOG_CALLS,"Switch dest " << i << ' ' << offsets[i]);
				}

				ASObject* index_obj=context->runtime_stack_pop();
				assert(index_obj->getObjectType()==T_INTEGER);
				int index=index_obj->toInt();

				int dest=defaultdest;
				if(index>=0 && index<=count)
					dest=offsets[index];

				if(dest >= code_len)
				{
					LOG(LOG_ERROR,"Jump outside of code");
					abort();
				}
				code.seekg(dest);
				break;
			}
			case 0x1c:
			{
				//pushwith
				pushWith(context);
				break;
			}
			case 0x1d:
			{
				//popscope
				popScope(context);
				break;
			}
			case 0x1e:
			{
				//nextname
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				context->runtime_stack_push(nextName(v1,v2));
				break;
			}
			case 0x20:
			{
				//pushnull
				context->runtime_stack_push(pushNull());
				break;
			}
			case 0x21:
			{
				//pushundefined
				context->runtime_stack_push(pushUndefined());
				break;
			}
			case 0x23:
			{
				//nextvalue
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				context->runtime_stack_push(nextValue(v1,v2));
				break;
			}
			case 0x24:
			{
				//pushbyte
				int8_t t;
				code.read((char*)&t,1);
				context->runtime_stack_push(abstract_i(t));
				pushByte(t);
				break;
			}
			case 0x25:
			{
				//pushshort
				u30 t;
				code >> t;
				context->runtime_stack_push(abstract_i(t));
				pushShort(t);
				break;
			}
			case 0x26:
			{
				//pushtrue
				context->runtime_stack_push(abstract_b(pushTrue()));
				break;
			}
			case 0x27:
			{
				//pushfalse
				context->runtime_stack_push(abstract_b(pushFalse()));
				break;
			}
			case 0x28:
			{
				//pushnan
				context->runtime_stack_push(pushNaN());
				break;
			}
			case 0x29:
			{
				//pop
				pop();
				ASObject* o=context->runtime_stack_pop();
				o->decRef();
				break;
			}
			case 0x2a:
			{
				//dup
				dup();
				ASObject* o=context->runtime_stack_peek();
				o->incRef();
				context->runtime_stack_push(o);
				break;
			}
			case 0x2b:
			{
				//swap
				swap();
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();

				context->runtime_stack_push(v1);
				context->runtime_stack_push(v2);
				break;
			}
			case 0x2c:
			{
				//pushstring
				u30 t;
				code >> t;
				context->runtime_stack_push(pushString(context,t));
				break;
			}
			case 0x2d:
			{
				//pushint
				u30 t;
				code >> t;
				pushInt(context, t);

				ASObject* i=abstract_i(context->context->constant_pool.integer[t]);
				context->runtime_stack_push(i);
				break;
			}
			case 0x2f:
			{
				//pushdouble
				u30 t;
				code >> t;
				pushDouble(context, t);

				ASObject* d=abstract_d(context->context->constant_pool.doubles[t]);
				context->runtime_stack_push(d);
				break;
			}
			case 0x30:
			{
				//pushscope
				pushScope(context);
				break;
			}
			case 0x32:
			{
				//hasnext2
				u30 t,t2;
				code >> t;
				code >> t2;

				bool ret=hasNext2(context,t,t2);
				context->runtime_stack_push(abstract_b(ret));
				break;
			}
			case 0x40:
			{
				//newfunction
				u30 t;
				code >> t;
				context->runtime_stack_push(newFunction(context,t));
				break;
			}
			case 0x41:
			{
				//call
				u30 t;
				code >> t;
				call(context,t);
				break;
			}
			case 0x42:
			{
				//construct
				u30 t;
				code >> t;
				construct(context,t);
				break;
			}
			case 0x45:
			{
				//callsuper
				u30 t,t2;
				code >> t;
				code >> t2;
				callSuper(context,t,t2);
				break;
			}
			case 0x46:
			{
				//callproperty
				u30 t,t2;
				code >> t;
				code >> t2;
				callProperty(context,t,t2);
				break;
			}
			case 0x47:
			{
				//returnvoid
				LOG(LOG_CALLS,"returnVoid");
				return NULL;
			}
			case 0x48:
			{
				//returnvalue
				ASObject* ret=context->runtime_stack_pop();
				LOG(LOG_CALLS,"returnValue " << ret);
				return ret;
			}
			case 0x49:
			{
				//constructsuper
				u30 t;
				code >> t;
				constructSuper(context,t);
				break;
			}
			case 0x4a:
			{
				//constructprop
				u30 t,t2;
				code >> t;
				code >> t2;
				constructProp(context,t,t2);
				break;
			}
			case 0x4e:
			{
				//callsupervoid
				u30 t,t2;
				code >> t;
				code >> t2;
				callSuperVoid(context,t,t2);
				break;
			}
			case 0x4f:
			{
				//callpropvoid
				u30 t,t2;
				code >> t;
				code >> t2;
				callPropVoid(context,t,t2);
				break;
			}
			case 0x53:
			{
				//constructgenerictype
				u30 t;
				code >> t;
				constructGenericType(context, t);
				break;
			}
			case 0x55:
			{
				//newobject
				u30 t;
				code >> t;
				newObject(context,t);
				break;
			}
			case 0x56:
			{
				//newarray
				u30 t;
				code >> t;
				newArray(context,t);
				break;
			}
			case 0x57:
			{
				//newactivation
				context->runtime_stack_push(newActivation(context, mi));
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
			case 0x59:
			{
				//getdescendants
				u30 t;
				code >> t;
				getDescendants(context, t);
				break;
			}
			case 0x5a:
			{
				//newcatch
				u30 t;
				code >> t;
				context->runtime_stack_push(newCatch(context,t));
				break;
			}
			case 0x5d:
			{
				//findpropstrict
				u30 t;
				code >> t;
				context->runtime_stack_push(findPropStrict(context,t));
				break;
			}
			case 0x5e:
			{
				//findproperty
				u30 t;
				code >> t;
				context->runtime_stack_push(findProperty(context,t));
				break;
			}
			case 0x60:
			{
				//getlex
				u30 t;
				code >> t;
				getLex(context,t);
				break;
			}
			case 0x61:
			{
				//setproperty
				u30 t;
				code >> t;
				ASObject* value=context->runtime_stack_pop();

				multiname* name=context->context->getMultiname(t,context);

				ASObject* obj=context->runtime_stack_pop();

				setProperty(value,obj,name);
				break;
			}
			case 0x62:
			{
				//getlocal
				u30 i;
				code >> i;
				LOG(LOG_CALLS, "getLocal " << i );
				assert(context->locals[i]);
				context->runtime_stack_push(context->locals[i]);
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

				u30 i;
				code >> i;
				LOG(LOG_CALLS, "setLocal " << i );
				ASObject* obj=context->runtime_stack_pop();
				assert(obj);
				context->locals[i]=obj;
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
			case 0x68:
			{
				//initproperty
				u30 t;
				code >> t;
				initProperty(context,t);
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
				assert(context->locals[i]);
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
