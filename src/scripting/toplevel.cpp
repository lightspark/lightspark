/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include <list>
#include <algorithm>
#include <pcre.h>
#include <string.h>
#include <sstream>
#include <iomanip>
#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>
#include <cstdio>

#include "abc.h"
#include "toplevel.h"
#include "flashevents.h"
#include "swf.h"
#include "compat.h"
#include "class.h"
#include "exceptions.h"
#include "backends/urlutils.h"
#include <libxml++/nodes/textnode.h>

using namespace std;
using namespace lightspark;

SET_NAMESPACE("");

REGISTER_CLASS_NAME(Array);
REGISTER_CLASS_NAME2(ASQName,"QName","");
REGISTER_CLASS_NAME2(IFunction,"Function","");
REGISTER_CLASS_NAME2(UInteger,"uint","");
REGISTER_CLASS_NAME(Boolean);
REGISTER_CLASS_NAME(Integer);
REGISTER_CLASS_NAME(Number);
REGISTER_CLASS_NAME(Namespace);
REGISTER_CLASS_NAME(Date);
REGISTER_CLASS_NAME(RegExp);
REGISTER_CLASS_NAME(Math);
REGISTER_CLASS_NAME(ASString);
REGISTER_CLASS_NAME2(ASError, "Error", "");
REGISTER_CLASS_NAME(SecurityError);
REGISTER_CLASS_NAME(ArgumentError);
REGISTER_CLASS_NAME(DefinitionError);
REGISTER_CLASS_NAME(EvalError);
REGISTER_CLASS_NAME(RangeError);
REGISTER_CLASS_NAME(ReferenceError);
REGISTER_CLASS_NAME(SyntaxError);
REGISTER_CLASS_NAME(TypeError);
REGISTER_CLASS_NAME(URIError);
REGISTER_CLASS_NAME(VerifyError);
REGISTER_CLASS_NAME(XML);
REGISTER_CLASS_NAME(XMLList);

Array::Array()
{
	type=T_ARRAY;
	//constructor=new Function(_constructor);
	//ASObject::setVariableByQName("toString","",new Function(ASObject::_toString));
}

void Array::sinit(Class_base* c)
{
	// public constants
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setVariableByQName("CASEINSENSITIVE","",abstract_d(CASEINSENSITIVE));
	c->setVariableByQName("DESCENDING","",abstract_d(DESCENDING));
	c->setVariableByQName("NUMERIC","",abstract_d(NUMERIC));
	c->setVariableByQName("RETURNINDEXEDARRAY","",abstract_d(RETURNINDEXEDARRAY));
	c->setVariableByQName("UNIQUESORT","",abstract_d(UNIQUESORT));

	// properties
	c->setGetterByQName("length","",Class<IFunction>::getFunction(_getLength),true);

	// public functions
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setMethodByQName("concat",AS3,Class<IFunction>::getFunction(_concat),true);
	//c->setMethodByQName("every",AS3,Class<IFunction>::getFunction(every),true);
	c->setMethodByQName("filter",AS3,Class<IFunction>::getFunction(filter),true);
	c->setMethodByQName("forEach",AS3,Class<IFunction>::getFunction(forEach),true);
	c->setMethodByQName("indexOf",AS3,Class<IFunction>::getFunction(indexOf),true);
	c->setMethodByQName("lastIndexOf",AS3,Class<IFunction>::getFunction(lastIndexOf),true);
	c->setMethodByQName("join",AS3,Class<IFunction>::getFunction(join),true);
	c->setMethodByQName("map",AS3,Class<IFunction>::getFunction(_map),true);
	c->setMethodByQName("pop",AS3,Class<IFunction>::getFunction(_pop),true);
	c->setMethodByQName("push",AS3,Class<IFunction>::getFunction(_push),true);
	c->setMethodByQName("reverse",AS3,Class<IFunction>::getFunction(_reverse),true);
	c->setMethodByQName("shift",AS3,Class<IFunction>::getFunction(shift),true);
	//c->setMethodByQName("slice",AS3,Class<IFunction>::getFunction(slice),true);
	//c->setMethodByQName("some",AS3,Class<IFunction>::getFunction(some),true);
	c->setMethodByQName("sort",AS3,Class<IFunction>::getFunction(_sort),true);
	//c->setMethodByQName("sortOn",AS3,Class<IFunction>::getFunction(sortOn),true);
	c->setMethodByQName("splice",AS3,Class<IFunction>::getFunction(splice),true);
	//c->setMethodByQName("toLocaleString",AS3,Class<IFunction>::getFunction(toLocaleString),true);
	c->setMethodByQName("toString","",Class<IFunction>::getFunction(_toString),true);
	c->setMethodByQName("unshift",AS3,Class<IFunction>::getFunction(unshift),true);

	// workaround, pop was encountered not in the AS3 namespace before, need to investigate it further
	c->setMethodByQName("pop","",Class<IFunction>::getFunction(_pop),true);
}

void Array::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Array,_constructor)
{
	Array* th=static_cast<Array*>(obj);

	if(argslen==1 && args[0]->getObjectType()==T_INTEGER)
	{
		int size=args[0]->toInt();
		LOG(LOG_CALLS,_("Creating array of length ") << size);
		th->resize(size);
	}
	else
	{
		LOG(LOG_CALLS,_("Called Array constructor"));
		th->resize(argslen);
		for(unsigned int i=0;i<argslen;i++)
		{
			th->set(i,args[i]);
			args[i]->incRef();
		}
	}
	return NULL;
}

ASFUNCTIONBODY(Array,_concat)
{
	Array* th=static_cast<Array*>(obj);
	Array* ret=Class<Array>::getInstanceS();
	ret->data=th->data;
	if(argslen==1 && args[0]->getObjectType()==T_ARRAY)
	{
		Array* tmp=Class<Array>::cast(args[0]);
		ret->data.insert(ret->data.end(),tmp->data.begin(),tmp->data.end());
	}
	else
	{
		//Insert the arguments in the array
		ret->data.reserve(ret->data.size()+argslen);
		for(unsigned int i=0;i<argslen;i++)
			ret->push(args[i]);
	}

	//All the elements in the new array should be increffed, as args will be deleted and
	//this array could die too
	for(unsigned int i=0;i<ret->data.size();i++)
	{
		if(ret->data[i].type==DATA_OBJECT)
			ret->data[i].data->incRef();
	}
	
	return ret;
}

ASFUNCTIONBODY(Array,filter)
{
	Array* th=static_cast<Array*>(obj);
	assert_and_throw(argslen==1);
	IFunction* f = static_cast<IFunction*>(args[0]);
	ASObject* params[1];
	Array* ret=Class<Array>::getInstanceS();

	for(unsigned int i=0;i<ret->data.size();i++)
	{
		assert_and_throw(th->data[i].type==DATA_OBJECT);
		params[0] = th->data[i].data;
		ASObject* funcRet=f->call(new Null, params, 1);
		if(funcRet)
		{
			if(Boolean_concrete(funcRet))
			{
				th->data[i].data->incRef();
				ret->push(th->data[i].data);
			}
			funcRet->decRef();
		}
	}
	return ret;
}

ASFUNCTIONBODY(Array,_getLength)
{
	Array* th=static_cast<Array*>(obj);
	return abstract_i(th->data.size());
}

ASFUNCTIONBODY(Array,forEach)
{
	assert_and_throw(argslen == 1 || argslen == 2);
	Array* th=static_cast<Array*>(obj);
	IFunction* f = static_cast<IFunction*>(args[0]);
	ASObject* params[3];

	for(unsigned int i=0; i < th->data.size(); i++)
	{
		assert_and_throw(th->data[i].type==DATA_OBJECT);
		params[0] = th->data[i].data;
		th->data[i].data->incRef();
		params[1] = abstract_i(i);
		params[2] = th;
		th->incRef();

		if( argslen == 1 )
		{
			f->call(new Null, params, 3);
		}
		else
		{
			args[1]->incRef();
			f->call(args[1], params, 3);
		}
	}

	return NULL;
}

ASFUNCTIONBODY(Array, _reverse)
{
	Array* th = static_cast<Array*>(obj);

	reverse(th->data.begin(), th->data.end());

	return NULL;
}

ASFUNCTIONBODY(Array,lastIndexOf)
{
	Array* th=static_cast<Array*>(obj);
	assert_and_throw(argslen==1 || argslen==2);
	int ret=-1;
	ASObject* arg0=args[0];

	int unsigned i = th->data.size()-1;
	int j;
	if(argslen == 2 && args[1]->getObjectType() != T_UNDEFINED && !std::isnan(args[1]->toNumber()))
	{
		j = args[1]->toInt(); //Preserve sign
		if(j < 0) //Negative offset, use it as offset from the end of the array
			i = th->data.size()+j;
		else //Positive offset, use it directly
			i = j;
		if(i > th->data.size()) //If the passed offset is bigger than the array, cap the offset
			i = th->data.size()-1;
	}

	DATA_TYPE dtype = th->data[i].type;
	for(;i>=0;i--)
	{
		assert_and_throw(dtype==DATA_OBJECT || dtype==DATA_INT);
		dtype = th->data[i].type;
		if((dtype == DATA_OBJECT && ABCVm::strictEqualImpl(th->data[i].data,arg0)) ||
			(dtype == DATA_INT && arg0->toInt() == th->data[i].data_i))
		{
			ret=i;
			break;
		}
	}
	return abstract_i(ret);
}

ASFUNCTIONBODY(Array,shift)
{
	Array* th=static_cast<Array*>(obj);
	if(th->data.empty())
		return new Undefined;
	ASObject* ret;
	if(th->data[0].type==DATA_OBJECT)
		ret=th->data[0].data;
	else
		throw UnsupportedException("Array::shift not completely implemented");
	th->data.erase(th->data.begin());
	return ret;
}

ASFUNCTIONBODY(Array,splice)
{
	Array* th=static_cast<Array*>(obj);
	
	int startIndex=args[0]->toInt();
	int deleteCount=args[1]->toUInt();
	int totalSize=th->data.size();
	
	//A negative startIndex is relative to the end
	assert_and_throw(abs(startIndex)<totalSize);
	startIndex=(startIndex+totalSize)%totalSize;
	assert_and_throw((startIndex+deleteCount)<=totalSize);
	
	Array* ret=Class<Array>::getInstanceS();
	ret->data.reserve(deleteCount);

	for(int i=0;i<deleteCount;i++)
		ret->data.push_back(th->data[startIndex+i]);
	
	th->data.erase(th->data.begin()+startIndex,th->data.begin()+startIndex+deleteCount);

	//Insert requested values starting at startIndex
	for(unsigned int i=2,n=0;i<argslen;i++,n++)
	{
		args[i]->incRef();
		th->data.insert(th->data.begin()+startIndex+n,data_slot(args[i]));
	}

	return ret;
}

ASFUNCTIONBODY(Array,join)
{
	Array* th=static_cast<Array*>(obj);
	ASObject* del=args[0];
	string ret;
	for(int i=0;i<th->size();i++)
	{
		ret+=th->at(i)->toString().raw_buf();
		if(i!=th->size()-1)
			ret+=del->toString().raw_buf();
	}
	return Class<ASString>::getInstanceS(ret);
}

ASFUNCTIONBODY(Array,indexOf)
{
	Array* th=static_cast<Array*>(obj);
	assert_and_throw(argslen==1 || argslen==2);
	int ret=-1;
	ASObject* arg0=args[0];

	int unsigned i = 0;
	if(argslen == 2)
	{
		i = args[1]->toInt();
	}

	DATA_TYPE dtype;
	for(;i<th->data.size();i++)
	{
		dtype = th->data[i].type;
		assert_and_throw(dtype==DATA_OBJECT || dtype==DATA_INT);
		if((dtype == DATA_OBJECT && ABCVm::strictEqualImpl(th->data[i].data,arg0)) ||
			(dtype == DATA_INT && arg0->toInt() == th->data[i].data_i))
		{
			ret=i;
			break;
		}
	}
	return abstract_i(ret);
}


ASFUNCTIONBODY(Array,_pop)
{
	Array* th=static_cast<Array*>(obj);
	ASObject* ret;
	if(!th->data.empty() && th->data.back().type==DATA_OBJECT)
		ret=th->data.back().data;
	else
		throw UnsupportedException("Array::pop not completely implemented");
	th->data.pop_back();
	return ret;
}

bool Array::sortComparatorDefault::operator()(const data_slot& d1, const data_slot& d2)
{
	if(isNumeric)
	{
		number_t a=numeric_limits<double>::quiet_NaN();
		number_t b=numeric_limits<double>::quiet_NaN();
		if(d1.type==DATA_INT)
			a=d1.data_i;
		else if(d1.type==DATA_OBJECT && d1.data)
			a=d1.data->toNumber();
		
		if(d2.type==DATA_INT)
			b=d2.data_i;
		else if(d2.type==DATA_OBJECT && d2.data)
			b=d2.data->toNumber();

		if(std::isnan(a) || std::isnan(b))
			throw RunTimeException("Cannot sort non number with Array.NUMERIC option");
		return a<b;
	}
	else
	{
		//Comparison is always in lexicographic order
		tiny_string s1;
		tiny_string s2;
		if(d1.type==DATA_INT)
			s1=tiny_string(d1.data_i);
		else if(d1.type==DATA_OBJECT && d1.data)
			s1=d1.data->toString();
		else
			s1="undefined";
		if(d2.type==DATA_INT)
			s2=tiny_string(d2.data_i);
		else if(d2.type==DATA_OBJECT && d2.data)
			s2=d2.data->toString();
		else
			s2="undefined";

		return s1<s2;
	}
}

bool Array::sortComparatorWrapper::operator()(const data_slot& d1, const data_slot& d2)
{
	ASObject* objs[2];
	if(d1.type==DATA_INT)
		objs[0]=abstract_i(d1.data_i);
	else if(d1.type==DATA_OBJECT && d1.data)
	{
		objs[0]=d1.data;
		objs[0]->incRef();
	}
	else
		objs[0]=new Undefined;

	if(d2.type==DATA_INT)
		objs[1]=abstract_i(d2.data_i);
	else if(d2.type==DATA_OBJECT && d2.data)
	{
		objs[1]=d2.data;
		objs[1]->incRef();
	}
	else
		objs[1]=new Undefined;

	assert(comparator);
	ASObject* ret=comparator->call(new Null, objs, 2);
	assert_and_throw(ret);
	return (ret->toInt()<0); //Less
}

ASFUNCTIONBODY(Array,_sort)
{
	Array* th=static_cast<Array*>(obj);
	IFunction* comp=NULL;
	bool isNumeric=false;
	for(uint32_t i=0;i<argslen;i++)
	{
		if(args[i]->getObjectType()==T_FUNCTION) //Comparison func
		{
			assert_and_throw(comp==NULL);
			comp=static_cast<IFunction*>(args[i]);
		}
		else
		{
			uint32_t options=args[i]->toInt();
			if(options&NUMERIC)
				isNumeric=true;
			if(options&(~NUMERIC))
				throw UnsupportedException("Array::sort not completely implemented");
		}
	}
	
	if(comp)
		sort(th->data.begin(),th->data.end(),sortComparatorWrapper(comp));
	else
		sort(th->data.begin(),th->data.end(),sortComparatorDefault(isNumeric));

	obj->incRef();
	return obj;
}

ASFUNCTIONBODY(Array,sortOn)
{
//	Array* th=static_cast<Array*>(obj);
/*	if(th->data.size()>1)
		throw UnsupportedException("Array::sort not completely implemented");
	LOG(LOG_NOT_IMPLEMENTED,_("Array::sort not really implemented"));*/
	obj->incRef();
	return obj;
}

ASFUNCTIONBODY(Array,unshift)
{
	Array* th=static_cast<Array*>(obj);
	for(uint32_t i=0;i<argslen;i++)
	{
		th->data.insert(th->data.begin(),data_slot(args[i],DATA_OBJECT));
		args[i]->incRef();
	}
	return abstract_i(th->size());;
}

ASFUNCTIONBODY(Array,_push)
{
	Array* th=static_cast<Array*>(obj);
	for(unsigned int i=0;i<argslen;i++)
	{
		th->push(args[i]);
		args[i]->incRef();
	}
	return abstract_i(th->size());
}

ASFUNCTIONBODY(Array,_map)
{
	Array* th=static_cast<Array*>(obj);
	assert_and_throw(argslen==1 && args[0]->getObjectType()==T_FUNCTION);
	IFunction* func=static_cast<IFunction*>(args[0]);
	Array* arrayRet=Class<Array>::getInstanceS();

	for(uint32_t i=0;i<th->data.size();i++)
	{
		ASObject* funcArgs[3];
		const data_slot& slot=th->data[i];
		if(slot.type==DATA_INT)
			funcArgs[0]=abstract_i(slot.data_i);
		else if(slot.type==DATA_OBJECT && slot.data)
		{
			funcArgs[0]=slot.data;
			funcArgs[0]->incRef();
		}
		else
			funcArgs[0]=new Undefined;

		funcArgs[1]=abstract_i(i);
		funcArgs[2]=th;
		funcArgs[2]->incRef();
		ASObject* funcRet=func->call(new Null, funcArgs, 3);
		assert_and_throw(funcRet);
		arrayRet->push(funcRet);
	}

	return arrayRet;
}

ASFUNCTIONBODY(Array,_toString)
{
	Array* th=static_cast<Array*>(obj);
	return Class<ASString>::getInstanceS(th->toString_priv());
}

XML::XML():root(NULL),node(NULL),constructed(false)
{
}

XML::XML(const string& str):root(NULL),node(NULL),constructed(true)
{
	buildFromString(str);
}

XML::XML(_R<XML> _r, xmlpp::Node* _n):root(_r),node(_n),constructed(true)
{
	assert(node);
}

XML::XML(xmlpp::Node* _n):root(NULL),constructed(true)
{
	assert(_n);
	node=parser.get_document()->create_root_node_by_import(_n);
}

void XML::finalize()
{
	ASObject::finalize();
	root.reset();
}

void XML::sinit(Class_base* c)
{
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setMethodByQName("toString",AS3,Class<IFunction>::getFunction(XML::_toString),true);
	c->setMethodByQName("toXMLString",AS3,Class<IFunction>::getFunction(toXMLString),true);
	c->setMethodByQName("nodeKind",AS3,Class<IFunction>::getFunction(nodeKind),true);
	c->setMethodByQName("children",AS3,Class<IFunction>::getFunction(children),true);
	c->setMethodByQName("attributes",AS3,Class<IFunction>::getFunction(attributes),true);
	c->setMethodByQName("localName",AS3,Class<IFunction>::getFunction(localName),true);
	c->setMethodByQName("appendChild",AS3,Class<IFunction>::getFunction(appendChild),true);
	c->setMethodByQName("hasSimpleContent",AS3,Class<IFunction>::getFunction(_hasSimpleContent),true);
	c->setMethodByQName("hasComplexContent",AS3,Class<IFunction>::getFunction(_hasComplexContent),true);
}

void XML::buildFromString(const string& str)
{
	if(str.empty())
	{
		xmlpp::Element *el=parser.get_document()->create_root_node("");
		node=el->add_child_text();
		// TODO: node's parent (root) should be inaccessible from AS code
	}
	else
	{
		parser.parse_memory_raw((const unsigned char*)str.c_str(), str.size());
		node=parser.get_document()->get_root_node();
	}
}

ASFUNCTIONBODY(XML,generator)
{
	assert(obj==NULL);
	assert_and_throw(argslen==1);
	if(args[0]->getObjectType()==T_STRING)
	{
		ASString* str=Class<ASString>::cast(args[0]);
		return Class<XML>::getInstanceS(str->data);
	}
	else if(args[0]->getObjectType()==T_NULL ||
		args[0]->getObjectType()==T_UNDEFINED)
	{
		return Class<XML>::getInstanceS("");
	}
	else if(args[0]->getPrototype()==Class<XML>::getClass())
	{
		args[0]->incRef();
		return args[0];
	}
	else
		throw RunTimeException("Type not supported in XML()");
}

ASFUNCTIONBODY(XML,_constructor)
{
	assert_and_throw(argslen<=1);
	XML* th=Class<XML>::cast(obj);
	if(argslen==0 && th->constructed) //If root is already set we have been constructed outside AS code
		return NULL;

	if(argslen==0 ||
	   args[0]->getObjectType()==T_NULL || 
	   args[0]->getObjectType()==T_UNDEFINED)
	{
		th->buildFromString("");
	}
	else
	{
		assert_and_throw(args[0]->getObjectType()==T_STRING);
		ASString* str=Class<ASString>::cast(args[0]);
		th->buildFromString(str->data);
	}
	return NULL;
}

ASFUNCTIONBODY(XML,nodeKind)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	assert(th->node);
	xmlNodePtr libXml2Node=th->node->cobj();
	switch(libXml2Node->type)
	{
		case XML_ATTRIBUTE_NODE:
			return Class<ASString>::getInstanceS("attribute");
		case XML_ELEMENT_NODE:
			return Class<ASString>::getInstanceS("element");
		case XML_TEXT_NODE:
			return Class<ASString>::getInstanceS("text");
		default:
		{
			LOG(LOG_ERROR,"Unsupported XML type " << libXml2Node->type);
			throw UnsupportedException("Unsupported XML node type");
		}
	}
}

ASFUNCTIONBODY(XML,localName)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	assert(th->node);
	xmlElementType nodetype=th->node->cobj()->type;
	if(nodetype==XML_TEXT_NODE || nodetype==XML_COMMENT_NODE)
		return new Null;
	else
		return Class<ASString>::getInstanceS(th->node->get_name());
}

ASFUNCTIONBODY(XML,appendChild)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==1);
	XML* arg=NULL;
	if(args[0]->getPrototype()==Class<XML>::getClass())
		arg=Class<XML>::cast(args[0]);
	else if(args[0]->getPrototype()==Class<XMLList>::getClass())
		arg=Class<XMLList>::cast(args[0])->convertToXML().getPtr();

	if(arg==NULL)
		throw RunTimeException("Invalid argument for XML::appendChild");
	//Change the root of the appended XML node
	_NR<XML> rootXML=NullRef;
	if(th->root.isNull())
	{
		th->incRef();
		rootXML=_MR(th);
	}
	else
		rootXML=th->root;

	arg->root=rootXML;
	xmlUnlinkNode(arg->node->cobj());
	xmlNodePtr ret=xmlAddChild(th->node->cobj(),arg->node->cobj());
	assert_and_throw(ret);
	arg->incRef();
	return arg;
}

ASFUNCTIONBODY(XML,attributes)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	assert(th->node);
	//Needed dynamic cast, we want the type check
	xmlpp::Element* elem=dynamic_cast<xmlpp::Element*>(th->node);
	if(elem==NULL)
		return Class<XMLList>::getInstanceS();
	const xmlpp::Element::AttributeList& list=elem->get_attributes();
	xmlpp::Element::AttributeList::const_iterator it=list.begin();
	std::vector<_R<XML>> ret;
	_NR<XML> rootXML=NullRef;
	if(th->root.isNull())
	{
		th->incRef();
		rootXML=_MR(th);
	}
	else
		rootXML=th->root;

	for(;it!=list.end();it++)
		ret.push_back(_MR(Class<XML>::getInstanceS(rootXML, *it)));

	XMLList* retObj=Class<XMLList>::getInstanceS(ret);
	return retObj;
}

void XML::toXMLString_priv(xmlBufferPtr buf)
{
	//NOTE: this function is not thread-safe, as it can modify the xmlNode
	_NR<XML> rootXML=NullRef;
	if(root.isNull())
	{
		this->incRef();
		rootXML=_MR(this);
	}
	else
		rootXML=root;

	xmlDocPtr xmlDoc=rootXML->parser.get_document()->cobj();
	assert(xmlDoc);
	xmlNodePtr cNode=node->cobj();
	//As libxml2 does not automatically add the needed namespaces to the dump
	//we have to workaround the issue

	//Get the needed namespaces
	xmlNsPtr* neededNamespaces=xmlGetNsList(xmlDoc,cNode);
	//Save a copy of the namespaces actually defined in the node
	xmlNsPtr oldNsDef=cNode->nsDef;

	//Copy the namespaces (we need to modify them to create a customized list)
	vector<xmlNs> localNamespaces;
	if(neededNamespaces)
	{
		xmlNsPtr* cur=neededNamespaces;
		while(*cur)
		{
			localNamespaces.emplace_back(**cur);
			cur++;
		}
		for(uint32_t i=0;i<localNamespaces.size()-1;i++)
			localNamespaces[i].next=&localNamespaces[i+1];
		localNamespaces.back().next=NULL;
		//Free the namespaces arrary
		xmlFree(neededNamespaces);
		//Override the node defined namespaces
		cNode->nsDef=&localNamespaces.front();
	}

	int retVal=xmlNodeDump(buf, xmlDoc, cNode, 0, 0);
	//Restore the previously defined namespaces
	cNode->nsDef=oldNsDef;
	if(retVal==-1)
		throw RunTimeException("Error om XML::toXMLString_priv");
}

ASFUNCTIONBODY(XML,toXMLString)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	assert(th->node);
	//Allocate a page at the beginning
	xmlBufferPtr xmlBuffer=xmlBufferCreateSize(4096);
	th->toXMLString_priv(xmlBuffer);
	ASString* ret=Class<ASString>::getInstanceS((char*)xmlBuffer->content);
	xmlBufferFree(xmlBuffer);
	return ret;
}

ASFUNCTIONBODY(XML,children)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	assert(th->node);
	const xmlpp::Node::NodeList& list=th->node->get_children();
	xmlpp::Node::NodeList::const_iterator it=list.begin();
	std::vector<_R<XML>> ret;

	_NR<XML> rootXML=NullRef;
	if(th->root.isNull())
	{
		th->incRef();
		rootXML=_MR(th);
	}
	else
		rootXML=th->root;

	for(;it!=list.end();it++)
	{
		rootXML->incRef();
		ret.push_back(_MR(Class<XML>::getInstanceS(rootXML, *it)));
	}
	XMLList* retObj=Class<XMLList>::getInstanceS(ret);
	return retObj;
}

ASFUNCTIONBODY(XML,_hasSimpleContent)
{
	XML *th=static_cast<XML*>(obj);
	return abstract_b(th->hasSimpleContent());
}

ASFUNCTIONBODY(XML,_hasComplexContent)
{
	XML *th=static_cast<XML*>(obj);
	return abstract_b(th->hasComplexContent());
}

bool XML::hasSimpleContent() const
{
	xmlElementType nodetype=node->cobj()->type;
	if(nodetype==XML_COMMENT_NODE || nodetype==XML_PI_NODE)
		return false;

	const xmlpp::Node::NodeList& children=node->get_children();
	xmlpp::Node::NodeList::const_iterator it=children.begin();
	for(;it!=children.end();++it)
	{
		if((*it)->cobj()->type==XML_ELEMENT_NODE)
			return false;
	}

	return true;
}

bool XML::hasComplexContent() const
{
	const xmlpp::Node::NodeList& children=node->get_children();
	xmlpp::Node::NodeList::const_iterator it=children.begin();
	for(;it!=children.end();++it)
	{
		if((*it)->cobj()->type==XML_ELEMENT_NODE)
			return true;
	}

	return false;
}

xmlElementType XML::getNodeKind() const
{
	return node->cobj()->type;
}

void XML::recursiveGetDescendantsByQName(_R<XML> root, xmlpp::Node* node, const tiny_string& name, const tiny_string& ns,
		std::vector<_R<XML>>& ret)
{
	//Check if this node is being requested
	if(node->get_name()==name.raw_buf())
	{
		root->incRef();
		ret.push_back(_MR(Class<XML>::getInstanceS(root, node)));
	}
	//NOTE: Creating a temporary list is quite a large overhead, but there is no way in libxml++ to access the first child
	const xmlpp::Node::NodeList& list=node->get_children();
	xmlpp::Node::NodeList::const_iterator it=list.begin();
	for(;it!=list.end();it++)
		recursiveGetDescendantsByQName(root, *it, name, ns, ret);
}

void XML::getDescendantsByQName(const tiny_string& name, const tiny_string& ns, std::vector<_R<XML> >& ret)
{
	assert(node);
	assert_and_throw(ns=="");
	_NR<XML> rootXML=NullRef;
	if(root.isNull())
	{
		this->incRef();
		rootXML=_MR(this);
	}
	else
		rootXML=root;

	recursiveGetDescendantsByQName(rootXML, node, name, ns, ret);
}

ASObject* XML::getVariableByMultiname(const multiname& name, bool skip_impl, ASObject* base)
{
	if(skip_impl)
		return ASObject::getVariableByMultiname(name, skip_impl, base);

	const tiny_string& normalizedName=name.normalizedName();
	if(node==NULL)
	{
		//This is possible if the XML object was created from an empty string
		return NULL;
	}
	if(name.isAttribute)
	{
		//Lookup attribute
		//TODO: support namespaces
		assert_and_throw(name.ns.size()>0 && name.ns[0].name=="");
		//Normalize the name to the string form
		assert(node);
		//To have attributes we must be an Element
		xmlpp::Element* element=dynamic_cast<xmlpp::Element*>(node);
		if(element==NULL)
			return NULL;
		xmlpp::Attribute* attr=element->get_attribute(normalizedName.raw_buf());
		if(attr==NULL)
			return NULL;

		_NR<XML> rootXML=NullRef;
		if(root.isNull())
		{
			this->incRef();
			rootXML=_MR(this);
		}
		else
			rootXML=root;

		ASObject* ret=Class<XML>::getInstanceS(rootXML, attr);
		//The new object will be incReffed by the calling code
		ret->fake_decRef();
		return ret;
	}
	else
	{
		//Lookup children
		//TODO: support namespaces
		assert_and_throw(name.ns.size()>0 && name.ns[0].name=="");
		//Normalize the name to the string form
		assert(node);
		const xmlpp::Node::NodeList& children=node->get_children(normalizedName.raw_buf());
		xmlpp::Node::NodeList::const_iterator it=children.begin();

		std::vector<_R<XML>> ret;

		_NR<XML> rootXML=NullRef;
		if(root.isNull())
		{
			this->incRef();
			rootXML=_MR(this);
		}
		else
			rootXML=root;

		for(;it!=children.end();it++)
			ret.push_back(_MR(Class<XML>::getInstanceS(rootXML, *it)));
		XMLList* retObj=Class<XMLList>::getInstanceS(ret);
		return retObj;
	}
}

ASFUNCTIONBODY(XML,_toString)
{
	XML* th=Class<XML>::cast(obj);
	return Class<ASString>::getInstanceS(th->toString_priv());
}

tiny_string XML::toString_priv()
{
	//We have to use vanilla libxml2, libxml++ is not enough
	xmlNodePtr libXml2Node=node->cobj();
	tiny_string ret;
	if(hasSimpleContent())
	{
		xmlChar* content=xmlNodeGetContent(libXml2Node);
		ret=tiny_string((char*)content,true);
		xmlFree(content);
	}
	else
	{
		assert_and_throw(!node->get_children().empty());
		xmlBufferPtr xmlBuffer=xmlBufferCreateSize(4096);
		toXMLString_priv(xmlBuffer);
		ret=tiny_string((char*)xmlBuffer->content,true);
		xmlBufferFree(xmlBuffer);
	}
	return ret;
}

tiny_string XML::toString(bool debugMsg)
{
	if(debugMsg)
		return ASObject::toString(true);
	return toString_priv();
}

bool XML::nodesEqual(xmlpp::Node *a, xmlpp::Node *b) const
{
	assert(a && b);

	// type
	if(a->cobj()->type!=b->cobj()->type)
		return false;

	// name
	if(a->get_name()!=b->get_name() || 
	   (!a->get_name().empty() && 
	    a->get_namespace_uri()!=b->get_namespace_uri()))
		return false;

	// attributes
	xmlpp::Element *el1=dynamic_cast<xmlpp::Element *>(a);
	xmlpp::Element *el2=dynamic_cast<xmlpp::Element *>(b);
	if(el1 && el2)
	{
		xmlpp::Element::AttributeList attrs1=el1->get_attributes();
		xmlpp::Element::AttributeList attrs2=el2->get_attributes();
		if(attrs1.size()!=attrs2.size())
			return false;

		xmlpp::Element::AttributeList::iterator it=attrs1.begin();
		while(it!=attrs1.end())
		{
			xmlpp::Attribute *attr=el2->get_attribute((*it)->get_name(),
								  (*it)->get_namespace_prefix());
			if(!attr || (*it)->get_value()!=attr->get_value())
				return false;

			++it;
		}
	}

	// content
	xmlpp::ContentNode *c1=dynamic_cast<xmlpp::ContentNode *>(a);
	xmlpp::ContentNode *c2=dynamic_cast<xmlpp::ContentNode *>(b);
	if(el1 && el2)
	{
		xmlpp::TextNode *text1=el1->get_child_text();
		xmlpp::TextNode *text2=el2->get_child_text();

		if(text1 && text2)
		{
			if(text1->get_content()!=text2->get_content())
				return false;
		}
		else if(text1 || text2)
			return false;

	}
	else if(c1 && c2)
	{
		if(c1->get_content()!=c2->get_content())
			return false;
	}
	
	// children
	xmlpp::Node::NodeList myChildren=a->get_children();
	xmlpp::Node::NodeList otherChildren=b->get_children();
	if(myChildren.size()!=otherChildren.size())
		return false;

	xmlpp::Node::NodeList::iterator it1=myChildren.begin();
	xmlpp::Node::NodeList::iterator it2=otherChildren.begin();
	while(it1!=myChildren.end())
	{
		if (!nodesEqual(*it1, *it2))
			return false;
		++it1;
		++it2;
	}

	return true;
}

bool XML::isEqual(ASObject* r)
{
	XML *x=dynamic_cast<XML *>(r);
	if(x)
		return nodesEqual(node, x->node);

	XMLList *xl=dynamic_cast<XMLList *>(r);
	if(xl)
		return xl->isEqual(this);

	if(hasSimpleContent())
		return toString()==r->toString();

	return false;
}

XMLList::XMLList(const std::string& str):constructed(true)
{
	buildFromString(str);
}

void XMLList::finalize()
{
	nodes.clear();
}

void XMLList::sinit(Class_base* c)
{
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setMethodByQName("length","",Class<IFunction>::getFunction(_getLength),true);
	c->setMethodByQName("appendChild",AS3,Class<IFunction>::getFunction(appendChild),true);
	c->setMethodByQName("hasSimpleContent",AS3,Class<IFunction>::getFunction(_hasSimpleContent),true);
	c->setMethodByQName("hasComplexContent",AS3,Class<IFunction>::getFunction(_hasComplexContent),true);
	c->setMethodByQName("toString",AS3,Class<IFunction>::getFunction(_toString),true);
}

ASFUNCTIONBODY(XMLList,_constructor)
{
	assert_and_throw(argslen<=1);
	XMLList* th=Class<XMLList>::cast(obj);
	if(argslen==0 && th->constructed)
	{
		//Called from internal code
		return NULL;
	}
	if(argslen==0 ||
	   args[0]->getObjectType()==T_NULL || 
	   args[0]->getObjectType()==T_UNDEFINED)
		return NULL;

	assert_and_throw(args[0]->getObjectType()==T_STRING);
	ASString* str=Class<ASString>::cast(args[0]);
	th->buildFromString(str->data);
	return NULL;
}

void XMLList::buildFromString(const std::string& str)
{
	xmlpp::DomParser parser;
	std::string expanded="<parent>" + str + "</parent>";
	parser.parse_memory(expanded);
	const xmlpp::Node::NodeList& children=\
	  parser.get_document()->get_root_node()->get_children();
	xmlpp::Node::NodeList::const_iterator it;
	for(it=children.begin(); it!=children.end(); ++it)
		nodes.push_back(_MR(Class<XML>::getInstanceS(*it)));
}

ASFUNCTIONBODY(XMLList,_getLength)
{
	XMLList* th=Class<XMLList>::cast(obj);
	assert_and_throw(argslen==0);
	return abstract_i(th->nodes.size());
}

ASFUNCTIONBODY(XMLList,appendChild)
{
	XMLList* th=Class<XMLList>::cast(obj);
	assert_and_throw(th->nodes.size()==1);
	//Forward to the XML object
	return XML::appendChild(th->nodes[0].getPtr(),args,argslen);
}

ASFUNCTIONBODY(XMLList,_hasSimpleContent)
{
	XMLList* th=Class<XMLList>::cast(obj);
	assert_and_throw(argslen==0);
	return abstract_b(th->hasSimpleContent());
}

ASFUNCTIONBODY(XMLList,_hasComplexContent)
{
	XMLList* th=Class<XMLList>::cast(obj);
	assert_and_throw(argslen==0);
	return abstract_b(th->hasComplexContent());
}

ASFUNCTIONBODY(XMLList,generator)
{
	assert(obj==NULL);
	assert_and_throw(argslen==1);
	if(args[0]->getObjectType()==T_STRING)
	{
		ASString* str=Class<ASString>::cast(args[0]);
		return Class<XMLList>::getInstanceS(str->data);
	}
	else if(args[0]->getPrototype()==Class<XMLList>::getClass())
	{
		args[0]->incRef();
		return args[0];
	}
	else if(args[0]->getPrototype()==Class<XML>::getClass())
	{
		std::vector< _R<XML> > nodes;
		nodes.push_back(_MR(Class<XML>::cast(args[0])));
		return Class<XMLList>::getInstanceS(nodes);
	}
	else if(args[0]->getObjectType()==T_NULL ||
		args[0]->getObjectType()==T_UNDEFINED)
	{
		return Class<XMLList>::getInstanceS();
	}
	else
		throw RunTimeException("Type not supported in XMLList()");
}

ASObject* XMLList::getVariableByMultiname(const multiname& name, bool skip_impl, ASObject* base)
{
	if(skip_impl || !implEnable)
		return ASObject::getVariableByMultiname(name,skip_impl,base);

	//Check if this XMLList contains a single element
	assert_and_throw(name.ns.size()>0);
	if(name.ns[0].name!="")
		return ASObject::getVariableByMultiname(name,skip_impl,base);

	unsigned int index=0;
	if(Array::isValidMultiname(name,index))
	{
		if(index<nodes.size())
			return nodes[index].getPtr();
		else
			return NULL;
	}
	else
	{
		//Check if this XMLList contains a single element, if so forward the request
		ASObject* ret=NULL;
		if(nodes.size()==1)
			ret=nodes[0]->getVariableByMultiname(name, skip_impl, base);

		//No result yet, ask the base object
		if(ret==NULL)
			ret=ASObject::getVariableByMultiname(name,skip_impl,base);

		return ret;
	}
}

_NR<XML> XMLList::convertToXML() const
{
	if(nodes.size()==1)
		return nodes[0];
	else
		return NullRef;
}

bool XMLList::hasSimpleContent() const
{
	if(nodes.size()==0)
		return true;
	else if(nodes.size()==1)
		return nodes[0]->hasSimpleContent();
	else
	{
		std::vector<_R<XML> >::const_iterator it=nodes.begin();
		for(; it!=nodes.end(); ++it)
		{
			if((*it)->getNodeKind()==XML_ELEMENT_NODE)
				return false;
		}
	}

	return true;
}

bool XMLList::hasComplexContent() const
{
	if(nodes.size()==0)
		return false;
	else if(nodes.size()==1)
		return nodes[0]->hasComplexContent();
	else
	{
		std::vector<_R<XML>>::const_iterator it=nodes.begin();
		for(; it!=nodes.end(); ++it)
		{
			if((*it)->getNodeKind()==XML_ELEMENT_NODE)
				return true;
		}
	}

	return false;
}

void XMLList::append(_R<XML> x)
{
	nodes.push_back(x);
}

void XMLList::append(_R<XMLList> x)
{
	nodes.insert(nodes.end(),x->nodes.begin(),x->nodes.end());
}

tiny_string XMLList::toString_priv() const
{
	//TODO: implement ECMA-356 toString
	tiny_string ret;
	for(uint32_t i=0;i<nodes.size();i++)
		ret+=nodes[i]->toString();
	return ret;
}

tiny_string XMLList::toString(bool debugMsg)
{
	if(debugMsg)
		return ASObject::toString(true);

	return toString_priv();
}

ASFUNCTIONBODY(XMLList,_toString)
{
	XMLList* th=Class<XMLList>::cast(obj);
	return Class<ASString>::getInstanceS(th->toString_priv());
}

bool XMLList::isEqual(ASObject* r)
{
	if(nodes.size()==0 && r->getObjectType()==T_UNDEFINED)
		return true;

	XMLList *x=dynamic_cast<XMLList *>(r);
	if(x)
	{
		if(nodes.size()!=x->nodes.size())
			return false;

		for(unsigned int i=0; i<nodes.size(); i++)
			if(!nodes[i]->isEqual(x->nodes[i].getPtr()))
				return false;

		return true;
	}
	else if(nodes.size()==1)
	{
		return nodes[0]->isEqual(r);
	}

	return false;
}

/*bool XMLList::nextValue(unsigned int index, ASObject*& out)
{
	__asm__("int $3");
	assert_and_throw(implEnable);
	assert_and_throw(index<nodes.size());
	out=nodes[index];
	return true;
}

bool XMLList::hasNext(unsigned int& index, bool& out)
{
	__asm__("int $3");
	assert_and_throw(implEnable);
	out=index<nodes.size();
	index++;
	return true;
}

bool XMLList::nextName(unsigned int index, ASObject*& out)
{
	__asm__("int $3");
	assert(index>0);
	index--;
	assert_and_throw(implEnable);
	assert_and_throw(index<nodes.size());
	out=abstract_i(index);
	return true;
}*/

bool Array::isEqual(ASObject* r)
{
	assert_and_throw(implEnable);
	if(r->getObjectType()!=T_ARRAY)
		return false;
	else
	{
		const Array* ra=static_cast<const Array*>(r);
		int size=data.size();
		if(size!=ra->size())
			return false;

		for(int i=0;i<size;i++)
		{
			if(data[i].type!=DATA_OBJECT)
				throw UnsupportedException("Array::isEqual not completely implemented");
			if(!data[i].data->isEqual(ra->at(i)))
				return false;
		}
		return true;
	}
}

intptr_t Array::getVariableByMultiname_i(const multiname& name)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidMultiname(name,index))
		return ASObject::getVariableByMultiname_i(name);

	if(index<data.size())
	{
		switch(data[index].type)
		{
			case DATA_OBJECT:
			{
				assert(data[index].data!=NULL);
				if(data[index].data->getObjectType()==T_INTEGER)
				{
					Integer* i=static_cast<Integer*>(data[index].data);
					return i->toInt();
				}
				else if(data[index].data->getObjectType()==T_NUMBER)
				{
					Number* i=static_cast<Number*>(data[index].data);
					return i->toInt();
				}
				else
					throw UnsupportedException("Array::getVariableByMultiname_i not completely implemented");
			}
			case DATA_INT:
				return data[index].data_i;
		}
	}
	
	return ASObject::getVariableByMultiname_i(name);
}

ASObject* Array::getVariableByMultiname(const multiname& name, bool skip_impl, ASObject* base)
{
	if(skip_impl || !implEnable)
		return ASObject::getVariableByMultiname(name,skip_impl,base);
		
	assert_and_throw(name.ns.size()>0);
	if(name.ns[0].name!="")
		return ASObject::getVariableByMultiname(name,skip_impl,base);

	unsigned int index=0;
	if(!isValidMultiname(name,index))
		return ASObject::getVariableByMultiname(name,skip_impl,base);

	if(index<data.size())
	{
		ASObject* ret=NULL;
		switch(data[index].type)
		{
			case DATA_OBJECT:
				ret=data[index].data;
				if(ret==NULL)
				{
					ret=new Undefined;
					data[index].data=ret;
				}
				break;
			case DATA_INT:
				ret=abstract_i(data[index].data_i);
				ret->fake_decRef();
				break;
		}
		return ret;
	}
	else
		return NULL;
}

void Array::setVariableByMultiname_i(const multiname& name, intptr_t value)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidMultiname(name,index))
	{
		ASObject::setVariableByMultiname_i(name,value);
		return;
	}

	if(index>=data.capacity())
	{
		//Heuristic, we increse the array 20%
		int new_size=imax(index+1,data.size()*2);
		data.reserve(new_size);
	}
	if(index>=data.size())
		resize(index+1);

	if(data[index].type==DATA_OBJECT && data[index].data)
		data[index].data->decRef();
	data[index].data_i=value;
	data[index].type=DATA_INT;
}

bool Array::isValidMultiname(const multiname& name, unsigned int& index)
{
	//First of all the multiname has to contain the null namespace
	//As the namespace vector is sorted, we check only the first one
	assert_and_throw(name.ns.size()!=0);
	if(name.ns[0].name!="")
		return false;

	index=0;
	int len;
	switch(name.name_type)
	{
		//We try to convert this to an index, otherwise bail out
		case multiname::NAME_STRING:
			len=name.name_s.len();
			assert_and_throw(len);
			for(int i=0;i<len;i++)
			{
				if(name.name_s[i]<'0' || name.name_s[i]>'9')
					return false;

				index*=10;
				index+=(name.name_s[i]-'0');
			}
			break;
		//This is already an int, so its good enough
		case multiname::NAME_INT:
			index=name.name_i;
			break;
		case multiname::NAME_NUMBER:
			//TODO: check that this is really an integer
			index=name.name_d;
			break;
		default:
			throw UnsupportedException("Array::isValidMultiname not completely implemented");
	}
	return true;
}

void Array::setVariableByMultiname(const multiname& name, ASObject* o, ASObject* base)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidMultiname(name,index))
		return ASObject::setVariableByMultiname(name,o,base);

	if(index>=data.capacity())
	{
		//Heuristic, we increse the array 20%
		int new_size=imax(index+1,data.size()*2);
		data.reserve(new_size);
	}
	if(index>=data.size())
		resize(index+1);

	if(data[index].type==DATA_OBJECT && data[index].data)
		data[index].data->decRef();

	if(o->getObjectType()==T_INTEGER)
	{
		Integer* i=static_cast<Integer*>(o);
		data[index].data_i=i->val;
		data[index].type=DATA_INT;
		o->decRef();
	}
	else
	{
		data[index].data=o;
		data[index].type=DATA_OBJECT;
	}
}

bool Array::isValidQName(const tiny_string& name, const tiny_string& ns, unsigned int& index)
{
	if(ns!="")
		return false;
	assert_and_throw(name.len()!=0);
	index=0;
	//First we try to convert the string name to an index, at the first non-digit
	//we bail out
	for(int i=0;i<name.len();i++)
	{
		if(!isdigit(name[i]))
			return false;

		index*=10;
		index+=(name[i]-'0');
	}
	return true;
}

ASString::ASString()
{
	type=T_STRING;
}

ASString::ASString(const string& s):data(s)
{
	type=T_STRING;
}

ASString::ASString(const tiny_string& s):data(s.raw_buf())
{
	type=T_STRING;
}

ASString::ASString(const char* s):data(s)
{
	type=T_STRING;
}

ASString::ASString(const char* s, uint32_t len):data(s, len)
{
	type=T_STRING;
}

ASFUNCTIONBODY(ASString,_constructor)
{
	ASString* th=static_cast<ASString*>(obj);
	if(args && argslen==1)
		th->data=args[0]->toString().raw_buf();
	return NULL;
}

ASFUNCTIONBODY(ASString,_getLength)
{
	ASString* th=static_cast<ASString*>(obj);
	return abstract_i(th->data.size());
}

void ASString::sinit(Class_base* c)
{
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setMethodByQName("split",AS3,Class<IFunction>::getFunction(split),true);
	c->setMethodByQName("substr",AS3,Class<IFunction>::getFunction(substr),true);
	c->setMethodByQName("substring",AS3,Class<IFunction>::getFunction(substring),true);
	c->setMethodByQName("replace",AS3,Class<IFunction>::getFunction(replace),true);
	c->setMethodByQName("concat",AS3,Class<IFunction>::getFunction(concat),true);
	c->setMethodByQName("match",AS3,Class<IFunction>::getFunction(match),true);
	c->setMethodByQName("search",AS3,Class<IFunction>::getFunction(search),true);
	c->setMethodByQName("indexOf",AS3,Class<IFunction>::getFunction(indexOf),true);
	c->setMethodByQName("lastIndexOf",AS3,Class<IFunction>::getFunction(lastIndexOf),true);
	c->setMethodByQName("charCodeAt",AS3,Class<IFunction>::getFunction(charCodeAt),true);
	c->setMethodByQName("charAt",AS3,Class<IFunction>::getFunction(charAt),true);
	c->setMethodByQName("slice",AS3,Class<IFunction>::getFunction(slice),true);
	c->setMethodByQName("toLowerCase",AS3,Class<IFunction>::getFunction(toLowerCase),true);
	c->setMethodByQName("toUpperCase",AS3,Class<IFunction>::getFunction(toUpperCase),true);
	c->setMethodByQName("fromCharCode",AS3,Class<IFunction>::getFunction(fromCharCode),false);
	c->setGetterByQName("length","",Class<IFunction>::getFunction(_getLength),true);
	//Fake method to override the default behavior
	c->setMethodByQName("toString",AS3,Class<IFunction>::getFunction(ASString::_toString),true);
}

void ASString::buildTraits(ASObject* o)
{
}

void Array::finalize()
{
	for(unsigned int i=0;i<data.size();i++)
	{
		if(data[i].type==DATA_OBJECT && data[i].data)
			data[i].data->decRef();
	}
	data.clear();
}

ASFUNCTIONBODY(ASString,search)
{
	ASString* th=static_cast<ASString*>(obj);
	int ret=-1;
	if(args[0]->getPrototype() && args[0]->getPrototype()==Class<RegExp>::getClass())
	{
		RegExp* re=static_cast<RegExp*>(args[0]);

		const char* error;
		int errorOffset;
		int options=0;
		if(re->ignoreCase)
			options|=PCRE_CASELESS;
		if(re->extended)
			options|=PCRE_EXTENDED;
		if(re->multiline)
			options|=PCRE_MULTILINE;
		pcre* pcreRE=pcre_compile(re->re.c_str(), options, &error, &errorOffset,NULL);
		if(error)
			return abstract_i(ret);
		//Verify that 30 for ovector is ok, it must be at least (captGroups+1)*3
		int capturingGroups;
		int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
		if(infoOk!=0)
		{
			pcre_free(pcreRE);
			return abstract_i(ret);
		}
		assert_and_throw(capturingGroups<10);
		int ovector[30];
		int offset=0;
		//Global is not used in search
		int rc=pcre_exec(pcreRE, NULL, th->data.c_str(), th->data.size(), offset, 0, ovector, 30);
		if(rc<0)
		{
			//No matches or error
			pcre_free(pcreRE);
			return abstract_i(ret);
		}
		ret=ovector[0];
	}
	else
	{
		const string& arg0=args[0]->toString().raw_buf();
		size_t index=th->data.find(arg0);
		if(index!=th->data.npos)
			ret=index;
	}
	return abstract_i(ret);
}

ASFUNCTIONBODY(ASString,match)
{
	ASString* th=static_cast<ASString*>(obj);
	if(args[0]->getObjectType()==T_NULL || args[0]->getObjectType()==T_UNDEFINED)
		return new Null;
	Array* ret=NULL;
	if(args[0]->getPrototype() && args[0]->getPrototype()==Class<RegExp>::getClass())
	{
		RegExp* re=static_cast<RegExp*>(args[0]);

		const char* error;
		int errorOffset;
		int options=0;
		if(re->ignoreCase)
			options|=PCRE_CASELESS;
		if(re->extended)
			options|=PCRE_EXTENDED;
		if(re->multiline)
			options|=PCRE_MULTILINE;
		pcre* pcreRE=pcre_compile(re->re.c_str(), options, &error, &errorOffset,NULL);
		if(error)
			return new Null;
		//Verify that 30 for ovector is ok, it must be at least (captGroups+1)*3
		int capturingGroups;
		int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
		if(infoOk!=0)
		{
			pcre_free(pcreRE);
			return new Null;
		}
		assert_and_throw(capturingGroups<10);
		int ovector[30];
		int offset=0;
		ret=Class<Array>::getInstanceS();
		do
		{
			int rc=pcre_exec(pcreRE, NULL, th->data.c_str(), th->data.size(), offset, 0, ovector, 30);
			if(rc<0)
			{
				//No matches or error
				pcre_free(pcreRE);
				return ret;
			}
			ret->push(Class<ASString>::getInstanceS(th->data.substr(ovector[0], ovector[1]-ovector[0])));
			offset=ovector[1];
		}
		while(re->global);
	}
	else
	{
		ret=Class<Array>::getInstanceS();
		const tiny_string& arg0=args[0]->toString();
		if(th->data.find(arg0.raw_buf())!=th->data.npos) //Match found
			ret->push(Class<ASString>::getInstanceS(arg0));
	}
	return ret;
}

ASFUNCTIONBODY(ASString,_toString)
{
	ASString* th=static_cast<ASString*>(obj);
	assert_and_throw(argslen==0);
	return Class<ASString>::getInstanceS(th->ASString::toString(false));
}

ASFUNCTIONBODY(ASString,split)
{
	ASString* th=static_cast<ASString*>(obj);
	Array* ret=Class<Array>::getInstanceS();
	ASObject* delimiter=args[0];
	if(delimiter->getObjectType()==T_UNDEFINED)
	{
		ret->push(Class<ASString>::getInstanceS(th->data));
		return ret;
	}

	if(args[0]->getPrototype() && args[0]->getPrototype()==Class<RegExp>::getClass())
	{
		RegExp* re=static_cast<RegExp*>(args[0]);

		const char* error;
		int offset;
		int options=0;
		if(re->ignoreCase)
			options|=PCRE_CASELESS;
		if(re->extended)
			options|=PCRE_EXTENDED;
		if(re->multiline)
			options|=PCRE_MULTILINE;
		pcre* pcreRE=pcre_compile(re->re.c_str(), options, &error, &offset,NULL);
		if(error)
			return ret;
		//Verify that 30 for ovector is ok, it must be at least (captGroups+1)*3
		int capturingGroups;
		int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
		if(infoOk!=0)
		{
			pcre_free(pcreRE);
			return ret;
		}
		assert_and_throw(capturingGroups<10);
		int ovector[30];
		offset=0;
		unsigned int end;
		do
		{
			int rc=pcre_exec(pcreRE, NULL, th->data.c_str(), th->data.size(), offset, 0, ovector, 30);
			end=ovector[0];
			if(rc<0)
				end=th->data.size();
			ASString* s=Class<ASString>::getInstanceS(th->data.substr(offset,end-offset));
			ret->push(s);
			offset=ovector[1];
			//Insert capturing groups
			for(int i=1;i<rc;i++)
			{
				ASString* s=Class<ASString>::getInstanceS(th->data.substr(ovector[i*2],ovector[i*2+1]-ovector[i*2]));
				ret->push(s);
			}
		}
		while(end<th->data.size());
		pcre_free(pcreRE);
	}
	else
	{
		const tiny_string& del=args[0]->toString();
		unsigned int start=0;
		do
		{
			int match=th->data.find(del.raw_buf(),start);
			if(del.len()==0)
				match++;
			if(match==-1)
				match=th->data.size();
			ASString* s=Class<ASString>::getInstanceS(th->data.substr(start,(match-start)));
			ret->push(s);
			start=match+del.len();
		}
		while(start<th->data.size());
	}

	return ret;
}

ASFUNCTIONBODY(ASString,substr)
{
	ASString* th=static_cast<ASString*>(obj);
	int start=0;
	if(argslen>=1)
		start=args[0]->toInt();
	if(start<0) {
		start=th->data.size()+start;
		if(start<0)
			start=0;
	}
	if(start>(int)th->data.size())
		start=th->data.size();

	int len=0x7fffffff;
	if(argslen==2)
		len=args[1]->toInt();

	return Class<ASString>::getInstanceS(th->data.substr(start,len));
}

ASFUNCTIONBODY(ASString,substring)
{
	ASString* th=static_cast<ASString*>(obj);
	int start=0;
	if (argslen>=1)
		start=args[0]->toInt();
	if(start<0)
		start=0;
	if(start>(int)th->data.size())
		start=th->data.size();

	int end=0x7fffffff;
	if(argslen>=2)
		end=args[1]->toInt();
	if(end<0)
		end=0;
	if(end>(int)th->data.size())
		end=th->data.size();

	if(start>end) {
		int tmp=start;
		start=end;
		end=tmp;
	}

	return Class<ASString>::getInstanceS(th->data.substr(start,end-start));
}

tiny_string Array::toString(bool debugMsg)
{
	assert_and_throw(implEnable);
	if(debugMsg)
		return ASObject::toString(debugMsg);
	return toString_priv();
}

tiny_string Array::toString_priv() const
{
	string ret;
	for(unsigned int i=0;i<data.size();i++)
	{
		if(data[i].type==DATA_OBJECT)
		{
			if(data[i].data)
				ret+=data[i].data->toString().raw_buf();
		}
		else if(data[i].type==DATA_INT)
		{
			char buf[20];
			snprintf(buf,20,"%i",data[i].data_i);
			ret+=buf;
		}
		else
			throw UnsupportedException("Array::toString not completely implemented");

		if(i!=data.size()-1)
			ret+=',';
	}
	return ret;
}

_R<ASObject> Array::nextValue(uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=data.size())
	{
		index--;
		if(data[index].type==DATA_OBJECT)
		{
			if(data[index].data==NULL)
				return _MR(new Undefined);
			else
			{
				data[index].data->incRef();
				return _MR(data[index].data);
			}
		}
		else if(data[index].type==DATA_INT)
			return _MR(abstract_i(data[index].data_i));
		else
			throw UnsupportedException("Unexpected data type");
	}
	else
	{
		//Fall back on object properties
		return ASObject::nextValue(index-data.size());
	}
}

uint32_t Array::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	if(cur_index<data.size())
		return cur_index+1;
	else
	{
		//Fall back on object properties
		uint32_t ret=ASObject::nextNameIndex(cur_index-data.size());
		if(ret==0)
			return 0;
		else
			return ret+data.size();

	}
}

_R<ASObject> Array::nextName(uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=data.size())
		return _MR(abstract_i(index-1));
	else
	{
		//Fall back on object properties
		return ASObject::nextName(index-data.size());
	}
}

void Array::outofbounds() const
{
	throw ParseException("Array access out of bounds");
}

tiny_string Boolean::toString(bool debugMsg)
{
	return (val)?"true":"false";
}

tiny_string ASString::toString_priv() const
{
	return data;
}

tiny_string ASString::toString(bool debugMsg)
{
	assert_and_throw(implEnable);
	return toString_priv();
}

double ASString::toNumber()
{
	assert_and_throw(implEnable);
	for(unsigned int i=0;i<data.size();i++)
	{
		if(!(data[i]>='0' && data[i]<='9' && data[i]!='.')) //not a number
			return numeric_limits<double>::quiet_NaN();
	}
	return atof(data.c_str());
}

int32_t ASString::toInt()
{
	assert_and_throw(implEnable);
	if(data.empty() || !isdigit(data[0]))
		return 0;
	return atoi(data.c_str());
}

bool ASString::isEqual(ASObject* r)
{
	assert_and_throw(implEnable);
	//TODO: check conversion
	if(r->getObjectType()==T_STRING)
	{
		const ASString* s=static_cast<const ASString*>(r);
		return s->data==data;
	}
	else if(r->getObjectType()==T_OBJECT)
	{
		XMLList *xl=dynamic_cast<XMLList *>(r);
		if(xl)
			return xl->isEqual(this);
		XML *x=dynamic_cast<XML *>(r);
		if(x && x->hasSimpleContent())
			return x->toString()==data;
	}
	
	return false;
}

TRISTATE ASString::isLess(ASObject* r)
{
	//ECMA-262 11.8.5 algorithm
	assert_and_throw(implEnable);
	if(getObjectType()==T_STRING && r->getObjectType()==T_STRING)
	{
		ASString* rstr=Class<ASString>::cast(r);
		return (data<rstr->data)?TTRUE:TFALSE;
	}
	number_t a=toNumber();
	number_t b=r->toNumber();
	if(std::isnan(a) || std::isnan(b))
		return TUNDEFINED;
	//TODO: Should we special handle infinite values?
	return (a<b)?TTRUE:TFALSE;
}

bool Boolean::isEqual(ASObject* r)
{
	if(r->getObjectType()==T_BOOLEAN)
	{
		const Boolean* b=static_cast<const Boolean*>(r);
		return b->val==val;
	}
	else
	{
		return ASObject::isEqual(r);
	}
}

Undefined::Undefined()
{
	type=T_UNDEFINED;
}

ASFUNCTIONBODY(Undefined,call)
{
	LOG(LOG_CALLS,_("Undefined function"));
	return NULL;
}

tiny_string Undefined::toString(bool debugMsg)
{
	return "undefined";
}

TRISTATE Undefined::isLess(ASObject* r)
{
	//ECMA-262 complaiant
	//As undefined became NaN when converted to number the operation is undefined
	return TUNDEFINED;
}

bool Undefined::isEqual(ASObject* r)
{
	if(r->getObjectType()==T_UNDEFINED)
		return true;
	if(r->getObjectType()==T_NULL)
		return true;
	if(r->getObjectType()==T_OBJECT)
	{
		XMLList *xl=dynamic_cast<XMLList *>(r);
		if(xl)
			return xl->isEqual(this);
	}

	return false;
}

int Undefined::toInt()
{
	return 0;
}

double Undefined::toNumber()
{
	return numeric_limits<double>::quiet_NaN();
}

ASFUNCTIONBODY(Integer,_toString)
{
	Integer* th=static_cast<Integer*>(obj);
	int radix=10;
	char buf[20];
	if(argslen==1)
		radix=args[0]->toUInt();
	assert_and_throw(radix==10 || radix==16);
	if(radix==10)
		snprintf(buf,20,"%i",th->val);
	else if(radix==16)
		snprintf(buf,20,"%x",th->val);

	return Class<ASString>::getInstanceS(buf);
}

ASFUNCTIONBODY(Integer,generator)
{
	//Int is specified as 32bit
	return abstract_i(args[0]->toInt()&0xffffffff);
}

TRISTATE Integer::isLess(ASObject* o)
{
	switch(o->getObjectType())
	{
		case T_INTEGER:
			{
				Integer* i=static_cast<Integer*>(o);
				return (val < i->toInt())?TTRUE:TFALSE;
			}
			break;
		
		case T_NUMBER:
			{
				Number* i=static_cast<Number*>(o);
				if(std::isnan(i->toNumber())) return TUNDEFINED;
				return (val < i->toNumber())?TTRUE:TFALSE;
			}
			break;
		
		case T_STRING:
			{
				const ASString* s=static_cast<const ASString*>(o);
				//Check if the string may be converted to integer
				//TODO: check whole string?
				if(isdigit(s->data[0]))
				{
					int val2=atoi(s->data.c_str());
					return (val < val2)?TTRUE:TFALSE;
				}
				else
					return TFALSE;
			}
			break;
		
		case T_BOOLEAN:
			{
				Boolean* i=static_cast<Boolean*>(o);
				return (val < i->toInt())?TTRUE:TFALSE;
			}
			break;
		
		case T_UNDEFINED:
			{
				return TFALSE;
			}
			break;
			
		case T_NULL:
			{
				return (val < 0)?TTRUE:TFALSE;
			}
			break;

		default:
			break;
	}
	
	//If unhandled by switch, kick up to parent
	return ASObject::isLess(o);
}

bool Integer::isEqual(ASObject* o)
{
	if(o->getObjectType()==T_INTEGER)
		return val==o->toInt();
	else if(o->getObjectType()==T_UINTEGER)
	{
		//CHECK: somehow wrong
		return val==o->toInt();
	}
	else if(o->getObjectType()==T_NUMBER)
		return val==o->toNumber();
	else
	{
		return ASObject::isEqual(o);
	}
}

tiny_string Integer::toString(bool debugMsg)
{
	char buf[20];
	if(val<0)
	{
		//This can be a slow path, as it not used for array access
		snprintf(buf,20,"%i",val);
		return tiny_string(buf,true);
	}
	buf[19]=0;
	char* cur=buf+19;

	int v=val;
	do
	{
		cur--;
		*cur='0'+(v%10);
		v/=10;
	}
	while(v!=0);
	return tiny_string(cur,true); //Create a copy
}

void Integer::sinit(Class_base* c)
{
	c->setVariableByQName("MAX_VALUE","",new Integer(2147483647));
	c->setVariableByQName("MIN_VALUE","",new Integer(-2147483648));
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setMethodByQName("toString",AS3,Class<IFunction>::getFunction(Integer::_toString),true);
}

tiny_string UInteger::toString(bool debugMsg)
{
	char buf[20];
	buf[19]=0;
	char* cur=buf+19;

	int v=val;
	do
	{
		cur--;
		*cur='0'+(v%10);
		v/=10;
	}
	while(v!=0);
	return tiny_string(cur,true); //Create a copy
}

TRISTATE UInteger::isLess(ASObject* o)
{
	if(o->getObjectType()==T_INTEGER)
	{
		uint32_t val1=val;
		int32_t val2=o->toInt();
		if(val2<0)
			return TFALSE;
		else
			return (val1<(uint32_t)val2)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_NUMBER)
	{
		Number* i=static_cast<Number*>(o);
		if(std::isnan(i->toNumber())) return TUNDEFINED;
		return (val < i->toUInt())?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_NULL)
	{
		// UInteger is never less than int(null) == 0
		return TFALSE;
	}
	else
		throw UnsupportedException("UInteger::isLess is not completely implemented");
}

bool Number::isEqual(ASObject* o)
{
	if(o->getObjectType()==T_INTEGER)
		return val==o->toNumber();
	else if(o->getObjectType()==T_NUMBER)
		return val==o->toNumber();
	else
	{
		return ASObject::isEqual(o);
	}
}

TRISTATE Number::isLess(ASObject* o)
{
	if(std::isnan(val))
		return TUNDEFINED;
	if(o->getObjectType()==T_INTEGER)
	{
		const Integer* i=static_cast<const Integer*>(o);
		return (val<i->val)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_NUMBER)
	{
		const Number* i=static_cast<const Number*>(o);
		if(std::isnan(i->val)) return TUNDEFINED;
		return (val<i->val)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_UNDEFINED)
	{
		//Undefined is NaN, so the result is undefined
		return TUNDEFINED;
	}
	else if(o->getObjectType()==T_STRING)
	{
		return (val<o->toNumber())?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_NULL)
	{
		return (val<0)?TTRUE:TFALSE;
	}
	else
	{
		return ASObject::isLess(o);
	}
}

ASFUNCTIONBODY(Number,_toString)
{
	Number* th=static_cast<Number*>(obj);
	assert_and_throw(argslen==0);
	return Class<ASString>::getInstanceS(th->Number::toString(false));
}

ASFUNCTIONBODY(Number,generator)
{
	return abstract_d(args[0]->toNumber());
}

tiny_string Number::toString(bool debugMsg)
{
	char buf[20];
	snprintf(buf,20,"%.18g",val);
	return tiny_string(buf,true);
}

void Number::sinit(Class_base* c)
{
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
	//Must create and link the number the hard way
	Number* ninf=new Number(-numeric_limits<double>::infinity());
	Number* pinf=new Number(numeric_limits<double>::infinity());
	Number* pmax=new Number(numeric_limits<double>::max());
	Number* pmin=new Number(numeric_limits<double>::min());
	ninf->setPrototype(c);
	pinf->setPrototype(c);
	pmax->setPrototype(c);
	pmin->setPrototype(c);
	c->setVariableByQName("NEGATIVE_INFINITY","",ninf);
	c->setVariableByQName("POSITIVE_INFINITY","",pinf);
	c->setVariableByQName("MAX_VALUE","",pmax);
	c->setVariableByQName("MIN_VALUE","",pmin);
	c->setMethodByQName("toString",AS3,Class<IFunction>::getFunction(Number::_toString),true);
}

Date::Date():year(-1),month(-1),date(-1),hour(-1),minute(-1),second(-1),millisecond(-1)
{
}

void Date::sinit(Class_base* c)
{
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setMethodByQName("getTimezoneOffset",AS3,Class<IFunction>::getFunction(getTimezoneOffset),true);
	c->setMethodByQName("valueOf",AS3,Class<IFunction>::getFunction(valueOf),true);
	c->setMethodByQName("getTime",AS3,Class<IFunction>::getFunction(getTime),true);
	c->setMethodByQName("getFullYear",AS3,Class<IFunction>::getFunction(getFullYear),true);
	c->setMethodByQName("getHours",AS3,Class<IFunction>::getFunction(getHours),true);
	c->setMethodByQName("getMinutes",AS3,Class<IFunction>::getFunction(getMinutes),true);
	c->setMethodByQName("getSeconds",AS3,Class<IFunction>::getFunction(getMinutes),true);
	c->setMethodByQName("getUTCFullYear",AS3,Class<IFunction>::getFunction(getUTCFullYear),true);
	c->setMethodByQName("getUTCMonth",AS3,Class<IFunction>::getFunction(getUTCMonth),true);
	c->setMethodByQName("getUTCDate",AS3,Class<IFunction>::getFunction(getUTCDate),true);
	c->setMethodByQName("getUTCHours",AS3,Class<IFunction>::getFunction(getUTCHours),true);
	c->setMethodByQName("getUTCMinutes",AS3,Class<IFunction>::getFunction(getUTCMinutes),true);
	c->setGetterByQName("fullYear","",Class<IFunction>::getFunction(getFullYear),true);
	//o->setVariableByQName("toString",AS3,Class<IFunction>::getFunction(ASObject::_toString));
}

void Date::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Date,_constructor)
{
	Date* th=static_cast<Date*>(obj);
	th->year=1969;
	th->month=1;
	th->date=1;
	th->hour=0;
	th->minute=0;
	th->second=0;
	th->millisecond=0;
	return NULL;
}

ASFUNCTIONBODY(Date,getTimezoneOffset)
{
	LOG(LOG_NOT_IMPLEMENTED,_("getTimezoneOffset"));
	return abstract_d(120);
}

ASFUNCTIONBODY(Date,getUTCFullYear)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->year);
}

ASFUNCTIONBODY(Date,getUTCMonth)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->month);
}

ASFUNCTIONBODY(Date,getUTCDate)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->date);
}

ASFUNCTIONBODY(Date,getUTCHours)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->hour);
}

ASFUNCTIONBODY(Date,getUTCMinutes)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->minute);
}

ASFUNCTIONBODY(Date,getFullYear)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->year);
}

ASFUNCTIONBODY(Date,getHours)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->hour);
}

ASFUNCTIONBODY(Date,getMinutes)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->minute);
}

ASFUNCTIONBODY(Date,getTime)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->toInt());
}

ASFUNCTIONBODY(Date,valueOf)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->toInt());
}

bool Date::getIsLeapYear(int year)
{
	return ( ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0) );
}

int Date::getDaysInMonth(int month, bool isLeapYear)
{
	enum { JANUARY = 1, FEBRUARY, MARCH, APRIL, MAY, JUNE, JULY, AUGUST, SEPTEMBER, OCTOBER, NOVEMBER, DECEMBER };

	int days;

	switch(month)
	{
	case FEBRUARY:
		days = isLeapYear ? 29 : 28;
		break;
	case JANUARY:
	case MARCH:
	case MAY:
	case JULY:
	case AUGUST:
	case OCTOBER:
	case DECEMBER:
		days = 31;
		break;
	case APRIL:
	case JUNE:
	case SEPTEMBER:
	case NOVEMBER:
		days = 30;
		break;
	default:
		days = -1;
	}

	return days;
}

int Date::toInt()
{
	int ret=0;

	ret+=((year-1990)*365 + ((year-1989)/4 - (year-1901)/100) + (year-1601)/400)*24*3600*1000;

	bool isLeapYear;
	for(int j = 1; j < month; j++)
	{
		isLeapYear = getIsLeapYear(year);
		ret+=getDaysInMonth(j, isLeapYear)*24*3600*1000;
	}

	ret+=(date-1)*24*3600*1000;
	ret+=hour*3600*1000;
	ret+=minute*60*1000;
	ret+=second*1000;
	ret+=millisecond;
	return ret;
}

tiny_string Date::toString(bool debugMsg)
{
	assert_and_throw(implEnable);
	return toString_priv();
}

tiny_string Date::toString_priv() const
{
	return "Wed Dec 31 16:00:00 GMT-0800 1969";
}

IFunction* SyntheticFunction::toFunction()
{
	return this;
}

IFunction* Function::toFunction()
{
	return this;
}

IFunction::IFunction():closure_this(NULL),closure_level(-1),bound(false)
{
	type=T_FUNCTION;
}

void IFunction::finalize()
{
	ASObject::finalize();
	closure_this.reset();
}

ASFUNCTIONBODY(IFunction,apply)
{
	IFunction* th=static_cast<IFunction*>(obj);
	assert_and_throw(argslen==2);

	ASObject** newArgs=NULL;
	int newArgsLen=0;

	//Validate parameters
	if(args[1]->getObjectType()==T_ARRAY)
	{
		Array* array=Class<Array>::cast(args[1]);

		newArgsLen=array->size();
		newArgs=new ASObject*[newArgsLen];
		for(int i=0;i<newArgsLen;i++)
		{
			newArgs[i]=array->at(i);
			newArgs[i]->incRef();
		}
	}

	args[0]->incRef();
	bool overrideThis=true;
	//Only allow overriding if the type of args[0] is a subclass of closure_this
	if(!(th->closure_this.getPtr() && th->closure_this->prototype && args[0]->prototype && 
				args[0]->prototype->isSubClass(th->closure_this->prototype)) ||	args[0]->prototype==NULL)
	{
		overrideThis=false;
	}
	ASObject* ret=th->call(args[0],newArgs,newArgsLen,overrideThis);
	if(ret==NULL)
		ret=new Undefined;
	delete[] newArgs;
	return ret;
}

ASFUNCTIONBODY(IFunction,_call)
{
	IFunction* th=static_cast<IFunction*>(obj);
	ASObject* newObj=NULL;
	ASObject** newArgs=NULL;
	uint32_t newArgsLen=0;
	if(argslen==0)
		newObj=abstract_d(numeric_limits<double>::quiet_NaN());
	else
	{
		newObj=args[0];
		newObj->incRef();
		newArgsLen=argslen-1;
		newArgs=new ASObject*[newArgsLen];
		for(unsigned int i=0;i<newArgsLen;i++)
		{
			newArgs[i]=args[i+1];
			newArgs[i]->incRef();
		}
	}
	bool overrideThis=true;
	//Only allow overriding if the type of args[0] is a subclass of closure_this
	if(!(th->closure_this.getPtr() && th->closure_this->prototype && args[0]->prototype && 
			args[0]->prototype->isSubClass(th->closure_this->prototype)) ||	args[0]->prototype==NULL)
	{
		overrideThis=false;
	}
	ASObject* ret=th->call(newObj,newArgs,newArgsLen,overrideThis);
	if(ret==NULL)
		ret=new Undefined;
	delete[] newArgs;
	return ret;
}

SyntheticFunction::SyntheticFunction(method_info* m):hit_count(0),mi(m),val(NULL)
{
//	class_index=-2;
}

void SyntheticFunction::finalize()
{
	IFunction::finalize();
	func_scope.clear();
}

ASObject* SyntheticFunction::call(ASObject* obj, ASObject* const* args, uint32_t numArgs, bool thisOverride)
{
	const int hit_threshold=10;
	if(mi->body==NULL)
	{
//		LOG(LOG_NOT_IMPLEMENTED,_("Not initialized function"));
		return NULL;
	}

	//Temporarily disable JITting
	if(sys->useJit && (hit_count==hit_threshold || sys->useInterpreter==false))
	{
		//We passed the hot function threshold, synt the function
		val=mi->synt_method();
		assert_and_throw(val);
	}

	//Prepare arguments
	uint32_t args_len=mi->numArgs();
	int passedToLocals=imin(numArgs,args_len);
	uint32_t passedToRest=(numArgs > args_len)?(numArgs-mi->numArgs()):0;
	//We use the stored level or the object's level
	int realLevel=(closure_level!=-1)?closure_level:obj->getLevel();

	call_context* cc=new call_context(mi,realLevel,args,passedToLocals);
	cc->code=new istringstream(mi->body->code);
	uint32_t i=passedToLocals;
	cc->scope_stack=func_scope;
	cc->initialScopeStack=func_scope.size();

	if(bound && !closure_this.isNull() && !thisOverride)
	{
		LOG(LOG_CALLS,_("Calling with closure ") << this);
		if(obj)
			obj->decRef();
		obj=closure_this.getPtr();
	}

	cc->locals[0]=obj;
	assert_and_throw(obj);
	obj->incRef();

	//Fixup missing parameters
	unsigned int missing_params=args_len-i;
	assert_and_throw(missing_params<=mi->option_count);
	int starting_options=mi->option_count-missing_params;

	for(unsigned int j=starting_options;j<mi->option_count;j++)
	{
		cc->locals[i+1]=mi->getOptional(j);
		i++;
	}

	assert_and_throw(i==args_len);

	assert_and_throw(mi->needsArgs()==false || mi->needsRest()==false);
	if(mi->needsRest()) //TODO
	{
		Array* rest=Class<Array>::getInstanceS();
		rest->resize(passedToRest);
		for(uint32_t j=0;j<passedToRest;j++)
			rest->set(j,args[passedToLocals+j]);

		cc->locals[i+1]=rest;
	}
	else if(mi->needsArgs())
	{
		Array* argumentsArray=Class<Array>::getInstanceS();
		argumentsArray->resize(args_len+passedToRest);
		for(uint32_t j=0;j<args_len;j++)
		{
			cc->locals[j+1]->incRef();
			argumentsArray->set(j,cc->locals[j+1]);
		}
		for(uint32_t j=0;j<passedToRest;j++)
			argumentsArray->set(j+args_len,args[passedToLocals+j]);
		//Add ourself as the callee property
		incRef();
		argumentsArray->setVariableByQName("callee","",this);

		cc->locals[i+1]=argumentsArray;
	}
	//Parameters are ready

	//As we are changing execution context (e.g. 'this' and level), reset the level of the current
	//object and add the new 'this' and level to the stack
	thisAndLevel tl=getVm()->getCurObjAndLevel();
	tl.cur_this->resetLevel();

	getVm()->pushObjAndLevel(obj,realLevel);
	//Set the current level
	obj->setLevel(realLevel);

	ASObject* ret;

	while (true)
	{
		try
		{
			if(val==NULL && sys->useInterpreter)
			{
				//This is not an hot function, execute it using the intepreter
				ret=ABCVm::executeFunction(this,cc);
			}
			else
				ret=val(cc);
		}
		catch (ASObject* excobj) // Doesn't have to be an ASError at all.
		{
			unsigned int pos = cc->code->tellg();
			bool no_handler = true;

			LOG(LOG_TRACE, "got an " << excobj->toString());
			LOG(LOG_TRACE, "pos=" << pos);
			for (unsigned int i=0;i<mi->body->exception_count;i++)
			{
				exception_info exc=mi->body->exceptions[i];
				multiname* name=mi->context->getMultiname(exc.exc_type, cc);
				LOG(LOG_TRACE, "f=" << exc.from << " t=" << exc.to);
				if (pos > exc.from && pos <= exc.to && ABCContext::isinstance(excobj, name))
				{
					no_handler = false;
					cc->code->seekg((uint32_t)exc.target);
					cc->runtime_stack_clear();
					cc->runtime_stack_push(excobj);
					cc->scope_stack.clear();
					cc->initialScopeStack=0;
					break;
				}
			}
			if (no_handler)
			{
				getVm()->popObjAndLevel();
				obj->resetLevel();
				tl=getVm()->getCurObjAndLevel();
				tl.cur_this->setLevel(tl.cur_level);
				delete cc;
				throw excobj;
			}
			continue;
		}
		break;
	}

	//Now pop this context and reset the level correctly
	tl=getVm()->popObjAndLevel();
	assert_and_throw(tl.cur_this==obj);
	assert_and_throw(tl.cur_this->getLevel()==realLevel);
	obj->resetLevel();

	tl=getVm()->getCurObjAndLevel();
	tl.cur_this->setLevel(tl.cur_level);

	delete cc;
	hit_count++;
	return ret;
}

ASObject* Function::call(ASObject* obj, ASObject* const* args, uint32_t num_args, bool thisOverride)
{
	ASObject* ret;
	if(bound && !closure_this.isNull() && !thisOverride)
	{
		LOG(LOG_CALLS,_("Calling with closure ") << this);
		if(obj)
			obj->decRef();
		obj=closure_this.getPtr();
		obj->incRef();
	}
	assert_and_throw(obj);
	ret=val(obj,args,num_args);

	for(uint32_t i=0;i<num_args;i++)
		args[i]->decRef();
	obj->decRef();
	return ret;
}

void Math::sinit(Class_base* c)
{
	// public constants
	c->setVariableByQName("E","",abstract_d(2.71828182845905));
	c->setVariableByQName("LN10","",abstract_d(2.302585092994046));
	c->setVariableByQName("LN2","",abstract_d(0.6931471805599453));
	c->setVariableByQName("LOG10E","",abstract_d(0.4342944819032518));
	c->setVariableByQName("LOG2E","",abstract_d(1.442695040888963387));
	c->setVariableByQName("PI","",abstract_d(3.141592653589793));
	c->setVariableByQName("SQRT1_2","",abstract_d(0.7071067811865476));
	c->setVariableByQName("SQRT2","",abstract_d(1.4142135623730951));

	// public methods
	c->setMethodByQName("abs","",Class<IFunction>::getFunction(abs),false);
	c->setMethodByQName("acos","",Class<IFunction>::getFunction(acos),false);
	c->setMethodByQName("asin","",Class<IFunction>::getFunction(asin),false);
	c->setMethodByQName("atan","",Class<IFunction>::getFunction(atan),false);
	c->setMethodByQName("atan2","",Class<IFunction>::getFunction(atan2),false);
	c->setMethodByQName("ceil","",Class<IFunction>::getFunction(ceil),false);
	c->setMethodByQName("cos","",Class<IFunction>::getFunction(cos),false);
	c->setMethodByQName("exp","",Class<IFunction>::getFunction(exp),false);
	c->setMethodByQName("floor","",Class<IFunction>::getFunction(floor),false);
	c->setMethodByQName("log","",Class<IFunction>::getFunction(log),false);
	c->setMethodByQName("max","",Class<IFunction>::getFunction(_max),false);
	c->setMethodByQName("min","",Class<IFunction>::getFunction(_min),false);
	c->setMethodByQName("pow","",Class<IFunction>::getFunction(pow),false);
	c->setMethodByQName("random","",Class<IFunction>::getFunction(random),false);
	c->setMethodByQName("round","",Class<IFunction>::getFunction(round),false);
	c->setMethodByQName("sin","",Class<IFunction>::getFunction(sin),false);
	c->setMethodByQName("sqrt","",Class<IFunction>::getFunction(sqrt),false);
	c->setMethodByQName("tan","",Class<IFunction>::getFunction(tan),false);
}

int Math::hexToInt(char c)
{
	if(c>='0' && c<='9')
		return c-'0';
	else if(c>='a' && c<='f')
		return c-'a'+10;
	else if(c>='A' && c<='F')
		return c-'A'+10;
	else
		return -1;
}

ASFUNCTIONBODY(Math,atan2)
{
	double n1=args[0]->toNumber();
	double n2=args[1]->toNumber();
	return abstract_d(::atan2(n1,n2));
}

ASFUNCTIONBODY(Math,_max)
{
	double largest = args[0]->toNumber();

	for(unsigned int i = 1; i < argslen; i++)
	{
		largest = dmax(largest, args[i]->toNumber());
	}

	return abstract_d(largest);
}

ASFUNCTIONBODY(Math,_min)
{
	double smallest = args[0]->toNumber();

	for(unsigned int i = 1; i < argslen; i++)
	{
		smallest = dmin(smallest, args[i]->toNumber());
	}

	return abstract_d(smallest);
}

ASFUNCTIONBODY(Math,exp)
{
	double n=args[0]->toNumber();
	return abstract_d(::exp(n));
}

ASFUNCTIONBODY(Math,acos)
{
	//Angle is in radians
	double n=args[0]->toNumber();
	return abstract_d(::acos(n));
}

ASFUNCTIONBODY(Math,asin)
{
	//Angle is in radians
	double n=args[0]->toNumber();
	return abstract_d(::asin(n));
}

ASFUNCTIONBODY(Math,atan)
{
	//Angle is in radians
	double n=args[0]->toNumber();
	return abstract_d(::atan(n));
}

ASFUNCTIONBODY(Math,cos)
{
	//Angle is in radians
	double n=args[0]->toNumber();
	return abstract_d(::cos(n));
}

ASFUNCTIONBODY(Math,sin)
{
	//Angle is in radians
	double n=args[0]->toNumber();
	return abstract_d(::sin(n));
}

ASFUNCTIONBODY(Math,tan)
{
	//Angle is in radians
	double n=args[0]->toNumber();
	return abstract_d(::tan(n));
}

ASFUNCTIONBODY(Math,abs)
{
	double n=args[0]->toNumber();
	return abstract_d(::fabs(n));
}

ASFUNCTIONBODY(Math,ceil)
{
	double n=args[0]->toNumber();
	return abstract_d(::ceil(n));
}

ASFUNCTIONBODY(Math,log)
{
	double n=args[0]->toNumber();
	return abstract_d(::log(n));
}

ASFUNCTIONBODY(Math,floor)
{
	double n=args[0]->toNumber();
	return abstract_d(::floor(n));
}

ASFUNCTIONBODY(Math,round)
{
	double n=args[0]->toNumber();
	return abstract_d(::round(n));
}

ASFUNCTIONBODY(Math,sqrt)
{
	double n=args[0]->toNumber();
	return abstract_d(::sqrt(n));
}

ASFUNCTIONBODY(Math,pow)
{
	double x=args[0]->toNumber();
	double y=args[1]->toNumber();
	return abstract_d(::pow(x,y));
}

ASFUNCTIONBODY(Math,random)
{
	double ret=rand();
	ret/=RAND_MAX;
	return abstract_d(ret);
}

tiny_string Null::toString(bool debugMsg)
{
	return "null";
}

bool Null::isEqual(ASObject* r)
{
	if(r->getObjectType()==T_NULL)
		return true;
	else if(r->getObjectType()==T_UNDEFINED)
		return true;
	else
		return false;
}

TRISTATE Null::isLess(ASObject* r)
{
	if(r->getObjectType()==T_INTEGER)
	{
		Integer* i=static_cast<Integer*>(r);
		return (0<i->toInt())?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_UINTEGER)
	{
		UInteger* i=static_cast<UInteger*>(r);
		return (0<i->toUInt())?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_NUMBER)
	{
		Number* i=static_cast<Number*>(r);
		if(std::isnan(i->toNumber())) return TUNDEFINED;
		return (0<i->toNumber())?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_NULL)
	{
		return TFALSE;
	}
	else
	{
		return ASObject::isLess(r);
	}
}

int Null::toInt()
{
	return 0;
}

double Null::toNumber()
{
	return 0.0;
}

RegExp::RegExp():global(false),ignoreCase(false),extended(false),multiline(false),lastIndex(0)
{
}

void RegExp::sinit(Class_base* c)
{
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setMethodByQName("exec",AS3,Class<IFunction>::getFunction(exec),true);
	c->setMethodByQName("test",AS3,Class<IFunction>::getFunction(test),true);
	c->setGetterByQName("global","",Class<IFunction>::getFunction(_getGlobal),true);
}

void RegExp::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(RegExp,_constructor)
{
	RegExp* th=static_cast<RegExp*>(obj);
	th->re=args[0]->toString().raw_buf();
	if(argslen>1)
	{
		const tiny_string& flags=args[1]->toString();
		for(int i=0;i<flags.len();i++)
		{
			switch(flags[i])
			{
				case 'g':
					th->global=true;
					break;
				case 'i':
					th->ignoreCase=true;
					break;
				case 'x':
					th->extended=true;
					break;
				case 'm':
					th->multiline=true;
					break;
				case 's':
					throw UnsupportedException("RegExp not completely implemented");

			}
		}
	}
	return NULL;
}

ASFUNCTIONBODY(RegExp,_getGlobal)
{
	RegExp* th=static_cast<RegExp*>(obj);
	return abstract_b(th->global);
}

ASFUNCTIONBODY(RegExp,exec)
{
	RegExp* th=static_cast<RegExp*>(obj);
	assert_and_throw(argslen==1);
	const tiny_string& arg0=args[0]->toString();
	const char* error;
	int errorOffset;
	int options=0;
	if(th->ignoreCase)
		options|=PCRE_CASELESS;
	if(th->extended)
		options|=PCRE_EXTENDED;
	if(th->multiline)
		options|=PCRE_MULTILINE;
	pcre* pcreRE=pcre_compile(th->re.c_str(), options, &error, &errorOffset,NULL);
	if(error)
		return new Null;
	//Verify that 30 for ovector is ok, it must be at least (captGroups+1)*3
	int capturingGroups;
	int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		return new Null;
	}
	assert_and_throw(capturingGroups<10);
	//Get information about named capturing groups
	int namedGroups;
	infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_NAMECOUNT, &namedGroups);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		return new Null;
	}
	//Get information about the size of named entries
	int namedSize;
	infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_NAMEENTRYSIZE, &namedSize);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		return new Null;
	}
	struct nameEntry
	{
		uint16_t number;
		char name[0];
	};
	char* entries;
	infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_NAMETABLE, &entries);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		return new Null;
	}

	int ovector[30];
	int offset=(th->global)?th->lastIndex:0;
	const char* str=arg0.raw_buf();
	int strLen=arg0.len();
	int rc=pcre_exec(pcreRE, NULL, str, strLen, offset, 0, ovector, 30);
	if(rc<0)
	{
		//No matches or error
		pcre_free(pcreRE);
		return new Null;
	}
	Array* a=Class<Array>::getInstanceS();
	//Push the whole result and the captured strings
	for(int i=0;i<capturingGroups+1;i++)
		a->push(Class<ASString>::getInstanceS(str+ovector[i*2],ovector[i*2+1]-ovector[i*2]));
	args[0]->incRef();
	a->setVariableByQName("input","",args[0]);
	a->setVariableByQName("index","",abstract_i(ovector[0]));
	for(int i=0;i<namedGroups;i++)
	{
		nameEntry* entry=(nameEntry*)entries;
		uint16_t num=BigEndianToHost16(entry->number);
		ASObject* captured=a->at(num);
		captured->incRef();
		a->setVariableByQName(entry->name,"",captured);
		entries+=namedSize;
	}
	th->lastIndex=ovector[1];
	return a;
}

ASFUNCTIONBODY(RegExp,test)
{
	RegExp* th=static_cast<RegExp*>(obj);

	const tiny_string& arg0 = args[0]->toString();

	int options = 0;
	if(th->ignoreCase)
		options |= PCRE_CASELESS;
	if(th->extended)
		options |= PCRE_EXTENDED;
	if(th->multiline)
		options |= PCRE_MULTILINE;

	const char * error;
	int errorOffset;
	pcre * pcreRE = pcre_compile(th->re.c_str(), options, &error, &errorOffset, NULL);
	if(error)
		return new Null;

	const char* str=arg0.raw_buf();
	int strLen=arg0.len();
	int ovector[30];
	int offset=(th->global)?th->lastIndex:0;
	int rc = pcre_exec(pcreRE, NULL, str, strLen, offset, 0, ovector, 30);
	bool ret = (rc >= 0);

	return abstract_b(ret);
}

ASFUNCTIONBODY(ASString,slice)
{
	ASString* th=static_cast<ASString*>(obj);
	int startIndex=0;
	if(argslen>=1)
		startIndex=args[0]->toInt();
	if(startIndex<0) {
		startIndex=th->data.size()+startIndex;
		if(startIndex<0)
			startIndex=0;
	}
	if(startIndex>(int)th->data.size())
		startIndex=th->data.size();

	int endIndex=0x7fffffff;
	if(argslen>=2)
		endIndex=args[1]->toInt();
	if(endIndex<0) {
		endIndex=th->data.size()+endIndex;
		if(endIndex<0)
			endIndex=0;
	}
	if(endIndex>(int)th->data.size())
		endIndex=th->data.size();

	if(endIndex<=startIndex)
		return Class<ASString>::getInstanceS("");
	else
		return Class<ASString>::getInstanceS(th->data.substr(startIndex,endIndex-startIndex));
}

ASFUNCTIONBODY(ASString,charAt)
{
	ASString* th=static_cast<ASString*>(obj);
	int index=args[0]->toInt();
	int maxIndex=th->data.size();
	if(index<0 || index>=maxIndex)
		return Class<ASString>::getInstanceS();
	return Class<ASString>::getInstanceS(th->data.c_str()+index, 1);
}

ASFUNCTIONBODY(ASString,charCodeAt)
{
	//TODO: should return utf16
	LOG(LOG_CALLS,_("ASString::charCodeAt not really implemented"));
	ASString* th=static_cast<ASString*>(obj);
	unsigned int index=args[0]->toInt();
	assert_and_throw(index>=0 && index<th->data.size());
	//Character codes are expected to be positive
	return abstract_i((uint8_t)th->data[index]);
}

ASFUNCTIONBODY(ASString,indexOf)
{
	ASString* th=static_cast<ASString*>(obj);
	const tiny_string& arg0=args[0]->toString();
	int startIndex=0;
	if(argslen>1)
		startIndex=args[1]->toInt();
	
	bool found=false;
	unsigned int i;
	for(i=startIndex;i<th->data.size();i++)
	{
		if(th->data[i]==arg0[0])
		{
			found=true;
			for(int j=1;j<arg0.len();j++)
			{
				if(th->data[i+j]!=arg0[j])
				{
					found=false;
					break;
				}
			}
		}
		if(found)
			break;
	}

	if(!found)
		return abstract_i(-1);
	else
		return abstract_i(i);
}

ASFUNCTIONBODY(ASString,lastIndexOf)
{
	assert_and_throw(argslen==1 || argslen==2);
	ASString* th=static_cast<ASString*>(obj);
	const tiny_string& val=args[0]->toString();
	int startIndex=0x7fffffff;
	if(argslen == 2 && args[1]->getObjectType() != T_UNDEFINED && !std::isnan(args[1]->toNumber()))
		startIndex=args[1]->toInt();
		
	if(startIndex < 0) //If negative offset is passed, clamp to 0 for start-of-string matches
		return (th->data.substr(0, val.len()) == val.raw_buf() ? abstract_i(0) : abstract_i(-1));

	size_t pos=th->data.rfind(val.raw_buf(), startIndex);
	if(pos==th->data.npos)
		return abstract_i(-1);
	else
		return abstract_i(pos);
}

ASFUNCTIONBODY(ASString,toLowerCase)
{
	ASString* th=static_cast<ASString*>(obj);
	ASString* ret=Class<ASString>::getInstanceS(th->data);
	transform(th->data.begin(), th->data.end(), ret->data.begin(), ::tolower);
	return ret;
}

ASFUNCTIONBODY(ASString,toUpperCase)
{
	ASString* th=static_cast<ASString*>(obj);
	ASString* ret=Class<ASString>::getInstanceS(th->data);
	transform(th->data.begin(), th->data.end(), ret->data.begin(), ::toupper);
	return ret;
}

ASFUNCTIONBODY(ASString,fromCharCode)
{
	ASString* ret=Class<ASString>::getInstanceS();
	for(uint32_t i=0;i<argslen;i++)
	{
		int newChar=args[i]->toInt();
		if(newChar>127)
			LOG(LOG_NOT_IMPLEMENTED,_("Unicode not supported in String::fromCharCode"));
		ret->data+=char(newChar);
	}
	return ret;
}

ASFUNCTIONBODY(ASString,replace)
{
	ASString* th=static_cast<ASString*>(obj);
	enum REPLACE_TYPE { STRING=0, FUNC };
	REPLACE_TYPE type=(args[1]->getObjectType()==T_FUNCTION)?FUNC:STRING;
	ASString* ret=Class<ASString>::getInstanceS(th->data);
	assert_and_throw(argslen==2);

	string replaceWith;
	if(type==STRING)
	{
		replaceWith=args[1]->toString().raw_buf();
		//Look if special substitution are needed
		assert_and_throw(replaceWith.find('$')==replaceWith.npos);
	}

	if(args[0]->getPrototype()==Class<RegExp>::getClass())
	{
		RegExp* re=static_cast<RegExp*>(args[0]);

		const char* error;
		int errorOffset;
		int options=0;
		if(re->ignoreCase)
			options|=PCRE_CASELESS;
		if(re->extended)
			options|=PCRE_EXTENDED;
		if(re->multiline)
			options|=PCRE_MULTILINE;
		pcre* pcreRE=pcre_compile(re->re.c_str(), options, &error, &errorOffset,NULL);
		if(error)
			return ret;
		//Verify that 30 for ovector is ok, it must be at least (captGroups+1)*3
		int capturingGroups;
		int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
		if(infoOk!=0)
		{
			pcre_free(pcreRE);
			return ret;
		}
		assert_and_throw(capturingGroups<10);
		int ovector[30];
		int offset=0;
		int retDiff=0;
		do
		{
			int rc=pcre_exec(pcreRE, NULL, ret->data.c_str(), ret->data.size(), offset, 0, ovector, 30);
			if(rc<0)
			{
				//No matches or error
				pcre_free(pcreRE);
				return ret;
			}
			if(type==FUNC)
			{
				//Get the replace for this match
				IFunction* f=static_cast<IFunction*>(args[1]);
				ASObject* subargs[3+capturingGroups];
				subargs[0]=Class<ASString>::getInstanceS(ret->data.substr(ovector[0],ovector[1]-ovector[0]));
				for(int i=0;i<capturingGroups;i++)
					subargs[i+1]=Class<ASString>::getInstanceS(ret->data.substr(ovector[i*2+2],ovector[i*2+3]-ovector[i*2+2]));
				subargs[capturingGroups+1]=abstract_i(ovector[0]-retDiff);
				th->incRef();
				subargs[capturingGroups+2]=th;
				ASObject* ret=f->call(new Null, subargs, 3+capturingGroups);
				replaceWith=ret->toString().raw_buf();
				ret->decRef();
			}
			ret->data.replace(ovector[0],ovector[1]-ovector[0],replaceWith);
			offset=ovector[0]+replaceWith.size();
			retDiff+=replaceWith.size()-(ovector[1]-ovector[0]);
		}
		while(re->global);
	}
	else
	{
		const tiny_string& s=args[0]->toString();
		int index=ret->data.find(s.raw_buf(),0);
		if(index==-1) //No result
			return ret;
		assert_and_throw(type==STRING);
		ret->data.replace(index,s.len(),replaceWith);
	}

	return ret;
}

ASFUNCTIONBODY(ASString,concat)
{
	ASString* th=static_cast<ASString*>(obj);
	ASString* ret=Class<ASString>::getInstanceS(th->data);
	for(unsigned int i=0;i<argslen;i++)
		ret->data+=args[i]->toString().raw_buf();

	return ret;
}

ASFUNCTIONBODY(ASError,getStackTrace)
{
	ASError* th=static_cast<ASError*>(obj);
	ASString* ret=Class<ASString>::getInstanceS(th->toString(true));
	LOG(LOG_NOT_IMPLEMENTED,_("Error.getStackTrace not yet implemented."));
	return ret;
}

tiny_string ASError::toString(bool debugMsg)
{
	return message.len() > 0 ? message : name;
}

ASFUNCTIONBODY(ASError,_getErrorID)
{
	ASError* th=static_cast<ASError*>(obj);
	return abstract_i(th->errorID);
}

ASFUNCTIONBODY(ASError,_toString)
{
	ASError* th=static_cast<ASError*>(obj);
	return Class<ASString>::getInstanceS(th->ASError::toString(false));
}

ASFUNCTIONBODY(ASError,_setName)
{
	ASError* th=static_cast<ASError*>(obj);
	assert_and_throw(argslen==1);
	th->name = args[0]->toString();
	return NULL;
}

ASFUNCTIONBODY(ASError,_getName)
{
	ASError* th=static_cast<ASError*>(obj);
	return Class<ASString>::getInstanceS(th->name);
}

ASFUNCTIONBODY(ASError,_setMessage)
{
	ASError* th=static_cast<ASError*>(obj);
	assert_and_throw(argslen==1);
	th->message = args[0]->toString();
	return NULL;
}

ASFUNCTIONBODY(ASError,_getMessage)
{
	ASError* th=static_cast<ASError*>(obj);
	return Class<ASString>::getInstanceS(th->toString(false));
}

ASFUNCTIONBODY(ASError,_constructor)
{
	ASError* th=static_cast<ASError*>(obj);
	assert_and_throw(argslen <= 2);
	if(argslen >= 1)
	{
		th->message = args[0]->toString();
	}
	if(argslen == 2)
	{
		th->errorID = static_cast<Integer*>(args[0])->toInt();
	}
	return NULL;
}

void ASError::sinit(Class_base* c)
{
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setMethodByQName("getStackTrace",AS3,Class<IFunction>::getFunction(getStackTrace),true);
	c->setMethodByQName("toString",AS3,Class<IFunction>::getFunction(_toString),true);
	c->setGetterByQName("errorID","",Class<IFunction>::getFunction(_getErrorID),true);
	c->setGetterByQName("message","",Class<IFunction>::getFunction(_getMessage),true);
	c->setSetterByQName("message","",Class<IFunction>::getFunction(_setMessage),true);
	c->setGetterByQName("name","",Class<IFunction>::getFunction(_getName),true);
	c->setSetterByQName("name","",Class<IFunction>::getFunction(_setName),true);
}

void ASError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(SecurityError,_constructor)
{
	assert(argslen<=1);
	SecurityError* th=static_cast<SecurityError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

void SecurityError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<ASError>::getClass();
	c->max_level=c->super->max_level+1;
}

void SecurityError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(ArgumentError,_constructor)
{
	assert(argslen<=1);
	ArgumentError* th=static_cast<ArgumentError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

void ArgumentError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<ASError>::getClass();
	c->max_level=c->super->max_level+1;
}

void ArgumentError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(DefinitionError,_constructor)
{
	assert(argslen<=1);
	DefinitionError* th=static_cast<DefinitionError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

void DefinitionError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<ASError>::getClass();
	c->max_level=c->super->max_level+1;
}

void DefinitionError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(EvalError,_constructor)
{
	assert(argslen<=1);
	EvalError* th=static_cast<EvalError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

void EvalError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<ASError>::getClass();
	c->max_level=c->super->max_level+1;
}

void EvalError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(RangeError,_constructor)
{
	assert(argslen<=1);
	RangeError* th=static_cast<RangeError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

void RangeError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<ASError>::getClass();
	c->max_level=c->super->max_level+1;
}

void RangeError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(ReferenceError,_constructor)
{
	assert(argslen<=1);
	ReferenceError* th=static_cast<ReferenceError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

void ReferenceError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<ASError>::getClass();
	c->max_level=c->super->max_level+1;
}

void ReferenceError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(SyntaxError,_constructor)
{
	assert(argslen<=1);
	SyntaxError* th=static_cast<SyntaxError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

void SyntaxError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<ASError>::getClass();
	c->max_level=c->super->max_level+1;
}

void SyntaxError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(TypeError,_constructor)
{
	assert(argslen<=1);
	TypeError* th=static_cast<TypeError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

void TypeError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<ASError>::getClass();
	c->max_level=c->super->max_level+1;
}

void TypeError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(URIError,_constructor)
{
	assert(argslen<=1);
	URIError* th=static_cast<URIError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

void URIError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<ASError>::getClass();
	c->max_level=c->super->max_level+1;
}

void URIError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(VerifyError,_constructor)
{
	assert(argslen<=1);
	VerifyError* th=static_cast<VerifyError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

void VerifyError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<ASError>::getClass();
	c->max_level=c->super->max_level+1;
}

void VerifyError::buildTraits(ASObject* o)
{
}

Class_base::Class_base(const QName& name):use_protected(false),protected_ns("",NAMESPACE),constructor(NULL),referencedObjectsMutex("referencedObjects"),
	super(NULL),context(NULL),class_name(name),class_index(-1),max_level(0)
{
	type=T_CLASS;
}

Class_base::~Class_base()
{
	if(!referencedObjects.empty())
		LOG(LOG_ERROR,_("Class destroyed without cleanUp called"));
}

ASObject* Class_base::generator(ASObject* const* args, const unsigned int argslen)
{
	return ASObject::generator(NULL, args, argslen);
}

void Class_base::addImplementedInterface(const multiname& i)
{
	interfaces.push_back(i);
}

void Class_base::addImplementedInterface(Class_base* i)
{
	interfaces_added.push_back(i);
}

tiny_string Class_base::toString(bool debugMsg)
{
	tiny_string ret="[Class ";
	ret+=class_name.name;
	ret+="]";
	return ret;
}

void Class_base::recursiveBuild(ASObject* target)
{
	if(super)
		super->recursiveBuild(target);

	LOG(LOG_TRACE,_("Building traits for ") << class_name);
	target->setLevel(max_level);
	buildInstanceTraits(target);
}

void Class_base::setConstructor(IFunction* c)
{
	assert_and_throw(constructor==NULL);
	constructor=c;
}

ASObject* Class_base::getBorrowedVariableByMultiname(const multiname& name, bool skip_impl, ASObject* base)
{
	check();
	assert(base);

	obj_var* obj=Variables.findObjVar(name,false,true);
	if(obj)
	{
		//It seems valid for a class to redefine only the setter, so if we can't find
		//something to get, it's ok
		if(!(obj->getter || obj->var))
			obj=NULL;
	}

	if(obj!=NULL)
	{
		if(obj->getter)
		{
			//Call the getter
			ASObject* target=base;
			if(target->prototype)
			{
				LOG(LOG_CALLS,_("Calling the getter on type ") << target->prototype->class_name);
			}
			else
			{
				LOG(LOG_CALLS,_("Calling the getter"));
			}
			IFunction* getter=obj->getter;
			target->incRef();
			ASObject* ret=getter->call(target,NULL,0);
			LOG(LOG_CALLS,_("End of getter"));
			if(ret==NULL)
				ret=new Undefined;
			//The returned value is already owned by the caller
			ret->fake_decRef();
			return ret;
		}
		else
		{
			assert_and_throw(!obj->setter);
			assert_and_throw(obj->var);
			return obj->var;
		}
	}
	else if(super)
	{
		ASObject* ret=super->getBorrowedVariableByMultiname(name, skip_impl, base);
		return ret;
	}
	//If it has not been found
	return NULL;
}

void Class_base::handleConstruction(ASObject* target, ASObject* const* args, unsigned int argslen, bool buildAndLink)
{
/*	if(getActualPrototype()->class_index==-2)
	{
		abort();
		//We have to build the method traits
		SyntheticFunction* sf=static_cast<SyntheticFunction*>(this);
		LOG(LOG_CALLS,_("Building method traits"));
		for(int i=0;i<sf->mi->body->trait_count;i++)
			sf->mi->context->buildTrait(this,&sf->mi->body->traits[i]);
		sf->call(this,args,max_level);
	}*/
	if(buildAndLink)
	{
	#ifndef NDEBUG
		assert_and_throw(!target->initialized);
	#endif
		//HACK: suppress implementation handling of variables just now
		bool bak=target->implEnable;
		target->implEnable=false;
		recursiveBuild(target);
		//And restore it
		target->implEnable=bak;
		assert_and_throw(target->getLevel()==max_level);
	#ifndef NDEBUG
		target->initialized=true;
	#endif
		/* We set this before the actual call to the constructor
		 * or any superclass constructor
		 * so that functions called from within the constructor see
		 * the object as already constructed.
		 * We also have to set this for objects without constructor,
		 * so they are not tried to buildAndLink again.
		 */
		RELEASE_WRITE(target->constructed,true);
	}

	//As constructors are not binded, we should change here the level
	assert_and_throw(max_level==target->getLevel());
	if(constructor)
	{
		LOG(LOG_CALLS,_("Calling Instance init ") << class_name);
		target->incRef();
		ASObject* ret=constructor->call(target,args,argslen);
		assert_and_throw(ret==NULL);
	}
	if(buildAndLink)
	{
		//Tell the object that the construction is complete
		target->constructionComplete();
	}
}

void Class_base::acquireObject(ASObject* ob)
{
	Locker l(referencedObjectsMutex);
	bool ret=referencedObjects.insert(ob).second;
	assert_and_throw(ret);
}

void Class_base::abandonObject(ASObject* ob)
{
	Locker l(referencedObjectsMutex);
	set<ASObject>::size_type ret=referencedObjects.erase(ob);
	if(ret!=1)
	{
		LOG(LOG_ERROR,_("Failure in reference counting in ") << class_name);
	}
}

void Class_base::finalizeObjects() const
{
	set<ASObject*>::iterator it=referencedObjects.begin();
	for(;it!=referencedObjects.end();)
	{
		//A reference is acquired before finalizing the object, to make sure it will survive
		//the call
		ASObject* tmp=*it;
		tmp->incRef();
		tmp->finalize();
		//Advance the iterator before decReffing the current object (decReffing may destroy the object right now
		it++;
		tmp->decRef();
	}
}

void Class_base::finalize()
{
	finalizeObjects();

	ASObject::finalize();
	if(constructor)
	{
		constructor->decRef();
		constructor=NULL;
	}

	if(super)
	{
		super->decRef();
		super=NULL;
	}
}

void Class_base::cleanUp()
{
	//finalize must have been called before using cleanUp

	//cleanUp may be called multiple times, be sure all the code is executable multiple times
	//without double freeing anything
	//Destroy all the object reference by us
	if(!referencedObjects.empty())
		LOG(LOG_CALLS, "Class " << class_name << " references " << referencedObjects.size());

	while(!referencedObjects.empty())
	{
		set<ASObject*>::iterator it=referencedObjects.begin();
		delete *it;
	}
}

Class_object* Class_object::getClass()
{
	//We check if we are registered in the class map
	//if not we register ourselves (see also Class<T>::getClass)
	std::map<QName, Class_base*>::iterator it=sys->classes.find(QName("Class",""));
	Class_object* ret=NULL;
	if(it==sys->classes.end()) //This class is not yet in the map, create it
	{
		ret=new Class_object();
		sys->classes.insert(std::make_pair(QName("Class",""),ret));
	}
	else
		ret=static_cast<Class_object*>(it->second);

	ret->incRef();
	return ret;
}

void IFunction::sinit(Class_base* c)
{
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setMethodByQName("call",AS3,Class<IFunction>::getFunction(IFunction::_call),true);
	c->setMethodByQName("apply",AS3,Class<IFunction>::getFunction(IFunction::apply),true);
}

Class_function::Class_function(IFunction* _f, ASObject* _p):Class_base(QName("Function","")),f(_f),asprototype(_p)
{
	setPrototype(Class<IFunction>::getClass());
}

const std::vector<Class_base*>& Class_base::getInterfaces() const
{
	if(!interfaces.empty())
	{
		//Recursively get interfaces implemented by this interface
		for(unsigned int i=0;i<interfaces.size();i++)
		{
			ASObject* target;
			ASObject* interface_obj=getGlobal()->getVariableAndTargetByMultiname(interfaces[i], target);
			assert_and_throw(interface_obj && interface_obj->getObjectType()==T_CLASS);
			Class_base* inter=static_cast<Class_base*>(interface_obj);

			interfaces_added.push_back(inter);
			//Probe the interface for its interfaces
			inter->getInterfaces();
		}
		//Clean the interface vector to save some space
		interfaces.clear();
	}
	return interfaces_added;
}

void Class_base::linkInterface(Class_base* c) const
{
	if(class_index==-1)
	{
		//LOG(LOG_NOT_IMPLEMENTED,_("Linking of builtin interface ") << class_name << _(" not supported"));
		return;
	}
	//Recursively link interfaces implemented by this interface
	for(unsigned int i=0;i<getInterfaces().size();i++)
		getInterfaces()[i]->linkInterface(c);

	assert_and_throw(context);

	//Link traits of this interface
	for(unsigned int j=0;j<context->instances[class_index].trait_count;j++)
	{
		traits_info* t=&context->instances[class_index].traits[j];
		context->linkTrait(c,t);
	}

	if(constructor)
	{
		LOG(LOG_CALLS,_("Calling interface init for ") << class_name);
		ASObject* ret=constructor->call(c,NULL,0);
		assert_and_throw(ret==NULL);
	}
}

bool Class_base::isSubClass(const Class_base* cls) const
{
	check();
	if(cls==this || cls==Class<ASObject>::getClass())
		return true;

	//Now check the interfaces
	for(unsigned int i=0;i<getInterfaces().size();i++)
	{
		if(getInterfaces()[i]->isSubClass(cls))
			return true;
	}

	//Now ask the super
	if(super && super->isSubClass(cls))
		return true;
	return false;
}

tiny_string Class_base::getQualifiedClassName() const
{
	//TODO: use also the namespace
	if(class_index==-1)
		return class_name.name;
	else
	{
		assert_and_throw(context);
		int name_index=context->instances[class_index].name;
		assert_and_throw(name_index);
		const multiname* mname=context->getMultiname(name_index,NULL);
		return mname->qualifiedString();
	}
}

void ASQName::sinit(Class_base* c)
{
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setGetterByQName("uri","",Class<IFunction>::getFunction(_getURI),true);
	c->setGetterByQName("local_name","",Class<IFunction>::getFunction(_getLocalName),true);
}

ASFUNCTIONBODY(ASQName,_constructor)
{
	ASQName* th=static_cast<ASQName*>(obj);
	if(argslen!=2)
		throw UnsupportedException("ArgumentError");

	assert_and_throw(args[0]->getObjectType()==T_STRING || args[0]->getObjectType()==T_NAMESPACE);
	assert_and_throw(args[1]->getObjectType()==T_STRING);

	switch(args[0]->getObjectType())
	{
		case T_STRING:
		{
			ASString* s=static_cast<ASString*>(args[0]);
			th->uri=s->data;
			break;
		}
		case T_NAMESPACE:
		{
			Namespace* n=static_cast<Namespace*>(args[0]);
			th->uri=n->uri;
			break;
		}
		default:
			throw UnsupportedException("QName not completely implemented");
	}
	th->local_name=args[1]->toString();
	return NULL;
}

ASFUNCTIONBODY(ASQName,_getURI)
{
	ASQName* th=static_cast<ASQName*>(obj);
	return Class<ASString>::getInstanceS(th->uri);
}

ASFUNCTIONBODY(ASQName,_getLocalName)
{
	ASQName* th=static_cast<ASQName*>(obj);
	return Class<ASString>::getInstanceS(th->local_name);
}

void Namespace::sinit(Class_base* c)
{
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSetterByQName("uri","",Class<IFunction>::getFunction(_setURI),true);
	c->setGetterByQName("uri","",Class<IFunction>::getFunction(_getURI),true);
	c->setSetterByQName("prefix","",Class<IFunction>::getFunction(_setPrefix),true);
	c->setGetterByQName("prefix","",Class<IFunction>::getFunction(_getPrefix),true);
}

void Namespace::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Namespace,_constructor)
{
	Namespace* th=static_cast<Namespace*>(obj);
	//The Namespace class has two constructors, this is the one with a single argument, uriValue:*
	assert_and_throw(argslen<2);

	th->prefix = "";
	th->uri = "";
	if (argslen == 0)
	    return NULL;

	switch(args[0]->getObjectType())
	{
		case T_NULL:
		case T_UNDEFINED:
			break;
		case T_STRING:
		{
			ASString* s=static_cast<ASString*>(args[0]);
			th->uri=s->data;
			break;
		}
		case T_QNAME:
		{
			ASQName* q=static_cast<ASQName*>(args[0]);
			th->uri=q->uri;
			break;
		}
		case T_NAMESPACE:
		{
			Namespace* n=static_cast<Namespace*>(args[0]);
			th->uri=n->uri;
			th->prefix=n->prefix;
			break;
		}
		default:
			throw UnsupportedException("Namespace not completely implemented");
	}
	return NULL;
}

ASFUNCTIONBODY(Namespace,_setURI)
{
	Namespace* th=static_cast<Namespace*>(obj);
	th->uri=args[0]->toString();
	return NULL;
}

ASFUNCTIONBODY(Namespace,_getURI)
{
	Namespace* th=static_cast<Namespace*>(obj);
	return Class<ASString>::getInstanceS(th->uri);
}

ASFUNCTIONBODY(Namespace,_setPrefix)
{
	Namespace* th=static_cast<Namespace*>(obj);
	th->prefix=args[0]->toString();
	return NULL;
}

ASFUNCTIONBODY(Namespace,_getPrefix)
{
	Namespace* th=static_cast<Namespace*>(obj);
	return Class<ASString>::getInstanceS(th->prefix);
}

void InterfaceClass::lookupAndLink(Class_base* c, const tiny_string& name, const tiny_string& interfaceNs)
{
	obj_var* var=NULL;
	Class_base* cur=c;
	//Find the origin
	while(cur)
	{
		var=cur->Variables.findObjVar(name,nsNameAndKind("",NAMESPACE),false,true);
		if(var)
			break;
		cur=cur->super;
	}
	assert_and_throw(var->var);
	var->var->incRef();
	c->setVariableByQName(name,interfaceNs,var->var);
}

void UInteger::sinit(Class_base* c)
{
	//TODO: add in the JIT support for unsigned number
	//Right now we pretend to be signed, to make comparisons work
	c->setVariableByQName("MAX_VALUE","",new UInteger(0x7fffffff));
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
}

bool UInteger::isEqual(ASObject* o)
{
	if(o->getObjectType()==T_INTEGER)
	{
		//CHECK: somehow wrong
		return val==o->toUInt();
	}
	else if(o->getObjectType()==T_UINTEGER)
		return val==o->toUInt();
	else if(o->getObjectType()==T_NUMBER)
		return val==o->toUInt();
	else
	{
		return ASObject::isEqual(o);
	}
}

ASObject* Class<IFunction>::getInstance(bool construct, ASObject* const* args, const unsigned int argslen)
{
	return new Undefined;
}

Class<IFunction>* Class<IFunction>::getClass()
{
	std::map<QName, Class_base*>::iterator it=sys->classes.find(QName(ClassName<IFunction>::name,ClassName<IFunction>::ns));
	Class<IFunction>* ret=NULL;
	if(it==sys->classes.end()) //This class is not yet in the map, create it
	{
		ret=new Class<IFunction>;
		sys->classes.insert(std::make_pair(QName(ClassName<IFunction>::name,ClassName<IFunction>::ns),ret));
		IFunction::sinit(ret);
	}
	else
		ret=static_cast<Class<IFunction>*>(it->second);

	ret->incRef();
	return ret;
}

void GlobalObject::registerGlobalScope(ASObject* scope)
{
	globalScopes.push_back(scope);
}

ASObject* GlobalObject::getVariableByString(const std::string& str, ASObject*& target)
{
	size_t index=str.rfind('.');
	multiname name;
	name.name_type=multiname::NAME_STRING;
	if(index==str.npos) //No dot
	{
		name.name_s=str;
		name.ns.push_back(nsNameAndKind("",NAMESPACE)); //TODO: use ns kind
	}
	else
	{
		name.name_s=str.substr(index+1);
		name.ns.push_back(nsNameAndKind(str.substr(0,index),NAMESPACE));
	}
	return getVariableAndTargetByMultiname(name, target);
}

ASObject* GlobalObject::getVariableAndTargetByMultiname(const multiname& name, ASObject*& target)
{
	ASObject* o=NULL;
	for(uint32_t i=0;i<globalScopes.size();i++)
	{
		o=globalScopes[i]->getVariableByMultiname(name);
		if(o)
		{
			target=globalScopes[i];
			break;
		}
	}
	return o;
}

/*ASObject* GlobalObject::getVariableByMultiname(const multiname& name, bool skip_impl, bool enableOverride, ASObject* base)
{
	ASObject* ret=ASObject::getVariableByMultiname(name, skip_impl, enableOverride, base);
	if(ret==NULL)
	{
		for(uint32_t i=0;i<globalScopes.size();i++)
		{
			ret=globalScopes[i]->getVariableByMultiname(name, skip_impl, enableOverride, base);
			if(ret)
				break;
		}
	}
	return ret;
}*/

ASFUNCTIONBODY(Boolean,generator)
{
	assert_and_throw(argslen==1);
	return abstract_b(Boolean_concrete(args[0]));
}

//We follow the Boolean() algorithm, but return a concrete result, not a Boolean object
bool lightspark::Boolean_concrete(ASObject* obj)
{
	if(obj->getObjectType()==T_STRING)
	{
		LOG(LOG_CALLS,_("String to bool"));
		const tiny_string& s=obj->toString();
		if(s.len()==0)
			return false;
		else
			return true;
	}
	else if(obj->getObjectType()==T_BOOLEAN)
	{
		Boolean* b=static_cast<Boolean*>(obj);
		LOG(LOG_CALLS,_("Boolean to bool ") << b->val);
		return b->val;
	}
	else if(obj->getObjectType()==T_OBJECT)
	{
		LOG(LOG_CALLS,_("Object to bool"));
		return true;
	}
	else if(obj->getObjectType()==T_CLASS)
	{
		LOG(LOG_CALLS,_("Class to bool"));
		return true;
	}
	else if(obj->getObjectType()==T_ARRAY)
	{
		LOG(LOG_CALLS,_("Array to bool"));
		return true;
	}
	else if(obj->getObjectType()==T_FUNCTION)
	{
		LOG(LOG_CALLS,_("Function to bool"));
		return true;
	}
	else if(obj->getObjectType()==T_UNDEFINED)
	{
		LOG(LOG_CALLS,_("Undefined to bool"));
		return false;
	}
	else if(obj->getObjectType()==T_NULL)
	{
		LOG(LOG_CALLS,_("Null to bool"));
		return false;
	}
	else if(obj->getObjectType()==T_NUMBER)
	{
		LOG(LOG_CALLS,_("Number to bool"));
		double val=obj->toNumber();
		if(val==0 || std::isnan(val))
			return false;
		else
			return true;
	}
	else if(obj->getObjectType()==T_INTEGER)
	{
		LOG(LOG_CALLS,_("Integer to bool"));
		int32_t val=obj->toInt();
		if(val==0)
			return false;
		else
			return true;
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,_("Boolean conversion for type ") << obj->getObjectType() << endl);
		return false;
	}
}

ASFUNCTIONBODY(Boolean,_toString)
{
	Boolean* th=static_cast<Boolean*>(obj);
	return Class<ASString>::getInstanceS(th->toString(false));
}

void Boolean::sinit(Class_base* c)
{
	c->setMethodByQName("toString",AS3,Class<IFunction>::getFunction(Boolean::_toString),true);
}

ASFUNCTIONBODY(lightspark,parseInt)
{
	assert_and_throw(argslen==1 || argslen==2);
	if(args[0]->getObjectType()==T_UNDEFINED)
	{
		LOG(LOG_ERROR,"Undefined passed to parseInt");
		return new Undefined;
	}
	else
	{
		int radix=10;
		if(argslen==2)
		{
			radix=args[1]->toInt();
			assert_and_throw(radix==10 || radix==16);
		}
		const tiny_string& val=args[0]->toString();
		const char* cur=val.raw_buf();
		//Also 0x could be used to flag the number is hexadecimal
		if(val.len()>=2 && strncmp(cur,"0x",2)==0)
		{
			radix=16;
			cur+=2;
		}
		int ret=0;
		if(radix==16) // Should be an exadecimal number
		{
			while(*cur)
			{
				ret<<=4; //*16
				ret+=Math::hexToInt(*cur);
				cur++;
			}
		}
		else if(radix==10)
			ret=atoi(cur);
		return abstract_i(ret);
	}
}

ASFUNCTIONBODY(lightspark,parseFloat)
{
	if(args[0]->getObjectType()==T_UNDEFINED)
		return new Undefined;
	else
		return abstract_d(atof(args[0]->toString().raw_buf()));
}

ASFUNCTIONBODY(lightspark,isNaN)
{
	if(args[0]->getObjectType()==T_UNDEFINED)
		return abstract_b(true);
	else if(args[0]->getObjectType()==T_INTEGER)
		return abstract_b(false);
	else if(args[0]->getObjectType()==T_NUMBER)
	{
		if(std::isnan(args[0]->toNumber()))
			return abstract_b(true);
		else
			return abstract_b(false);
	}
	else if(args[0]->getObjectType()==T_BOOLEAN)
		return abstract_b(false);
	else if(args[0]->getObjectType()==T_STRING)
	{
		double n=args[0]->toNumber();
		return abstract_b(std::isnan(n));
	}
	else if(args[0]->getObjectType()==T_NULL)
		return abstract_b(false); // because Number(null) == 0
	else
		throw UnsupportedException("Weird argument for isNaN");
}

ASFUNCTIONBODY(lightspark,isFinite)
{
	if(args[0]->getObjectType()==T_NUMBER ||
		args[0]->getObjectType()==T_INTEGER)
	{
		if(isfinite(args[0]->toNumber()))
			return abstract_b(true);
		else
			return abstract_b(false);
	}
	else
		throw UnsupportedException("Weird argument for isFinite");
}

ASFUNCTIONBODY(lightspark,encodeURI)
{
	if(args[0]->getObjectType() == T_STRING)
	{
		ASString* th=static_cast<ASString*>(args[0]);
		return Class<ASString>::getInstanceS(URLInfo::encode(th->data, URLInfo::ENCODE_URI));
	}
	else
	{
		return Class<ASString>::getInstanceS(URLInfo::encode(args[0]->toString(), URLInfo::ENCODE_URI));
	}
}

ASFUNCTIONBODY(lightspark,decodeURI)
{
	if(args[0]->getObjectType() == T_STRING)
	{
		ASString* th=static_cast<ASString*>(args[0]);
		return Class<ASString>::getInstanceS(URLInfo::decode(th->data, URLInfo::ENCODE_URI));
	}
	else
	{
		return Class<ASString>::getInstanceS(URLInfo::decode(args[0]->toString(), URLInfo::ENCODE_URI));
	}
}

ASFUNCTIONBODY(lightspark,encodeURIComponent)
{
	if(args[0]->getObjectType() == T_STRING)
	{
		ASString* th=static_cast<ASString*>(args[0]);
		return Class<ASString>::getInstanceS(URLInfo::encode(th->data, URLInfo::ENCODE_URICOMPONENT));
	}
	else
	{
		return Class<ASString>::getInstanceS(URLInfo::encode(args[0]->toString(), URLInfo::ENCODE_URICOMPONENT));
	}
}

ASFUNCTIONBODY(lightspark,decodeURIComponent)
{
	if(args[0]->getObjectType() == T_STRING)
	{
		ASString* th=static_cast<ASString*>(args[0]);
		return Class<ASString>::getInstanceS(URLInfo::decode(th->data, URLInfo::ENCODE_URICOMPONENT));
	}
	else
	{
		return Class<ASString>::getInstanceS(URLInfo::decode(args[0]->toString(), URLInfo::ENCODE_URICOMPONENT));
	}
}

ASFUNCTIONBODY(lightspark,escape)
{
	if(args[0]->getObjectType() == T_STRING)
	{
		ASString* th=static_cast<ASString*>(args[0]);
		return Class<ASString>::getInstanceS(URLInfo::encode(th->data, URLInfo::ENCODE_ESCAPE));
	}
	else
	{
		return Class<ASString>::getInstanceS(URLInfo::encode(args[0]->toString(), URLInfo::ENCODE_ESCAPE));
	}
}

ASFUNCTIONBODY(lightspark,unescape)
{
	if(args[0]->getObjectType() == T_STRING)
	{
		ASString* th=static_cast<ASString*>(args[0]);
		return Class<ASString>::getInstanceS(URLInfo::decode(th->data, URLInfo::ENCODE_ESCAPE));
	}
	else
	{
		return Class<ASString>::getInstanceS(URLInfo::encode(args[0]->toString(), URLInfo::ENCODE_ESCAPE));
	}
}

ASFUNCTIONBODY(lightspark,print)
{
	if(args[0]->getObjectType() == T_STRING)
	{
		ASString* str = static_cast<ASString*>(args[0]);
		cerr << str->data << endl;
	}
	else
		cerr << args[0]->toString() << endl;
	return NULL;
}

ASFUNCTIONBODY(lightspark,trace)
{
	for(uint32_t i = 0; i< argslen;i++)
	{
		if(i > 0)
			cerr << " ";

		if(args[i]->getObjectType() == T_STRING)
		{
			ASString* str = static_cast<ASString*>(args[i]);
			cerr << str->data;
		}
		else
			cerr << args[i]->toString();
	}
	cerr << endl;
	return NULL;
}
