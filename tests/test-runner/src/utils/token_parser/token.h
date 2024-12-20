/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef UTILS_TOKEN_PARSER_TOKEN_H
#define UTILS_TOKEN_PARSER_TOKEN_H 1

#include <exception>
#include <list>
#include <memory>

#include <lightspark/tiny_string.h>
#include <lightspark/utils/optional.h>
#include <lightspark/utils/visitor.h>
#include <lightspark/swftypes.h>

using namespace lightspark;

// TODO: Move this into the main codebase, at some point.

class LSTokenException : public std::exception
{
private:
	tiny_string reason;
public:
	LSTokenException(const tiny_string& _reason) : reason(_reason) {}

	const char* what() const noexcept override { return reason.raw_buf(); }
	const tiny_string& getReason() const noexcept { return reason; }
};

struct LSToken
{
	struct Expr
	{
		using Block = std::list<LSToken>;
		enum OpType
		{
			Assign,
			Plus,
			Minus,
			Multiply,
			Divide,
			LeftShift,
			RightShift,
			BitNot,
			BitAnd,
			BitOr,
			BitXor,
			Not,
			And,
			Or,
			Equal,
			NotEqual,
			LessThan,
			LessEqual,
			GreaterThan,
			GreaterEqual,
		};

		template<typename T>
		struct EvalResult
		{
			struct Error { Optional<tiny_string> msg; };
			struct Op
			{
				OpType type;
				T value;

				Op(const OpType& _type, const T& _value) :
				type(_type),
				value(_value) {}
			};

			enum class Type
			{
				Invalid,
				Done,
				Skip,
				NoOverride,
				Error,
				Op,
				Value,
			};

			Type type;
			Optional<const Expr&> doneExpr;
			union
			{
				Error error;
				Op op;
				T value;
			};

			EvalResult(const Type& _type) : type(_type)
			{
				assert
				(
					type != Type::Error &&
					type != Type::Op &&
					type != Type::Value
				);
			}

			EvalResult(const Error& _error) : type(Type::Error), error(_error) {}
			EvalResult(const Op& _op) : type(Type::Op), op(_op) {}
			EvalResult(const T& _value) : type(Type::Value), value(_value) {}

			EvalResult& withDoneExpr(Optional<const Expr&> expr)
			{
				doneExpr = expr;
				return *this;
			}

			EvalResult& tryWithDoneExpr(Optional<const Expr&> expr)
			{
				if (!doneExpr.hasValue())
					doneExpr = expr;
				return *this;
			}

			Optional<const Error&> getError() const
			{
				if (isError())
					return error;
				return {};
			}

			Optional<const Op&> getOp() const
			{
				if (isOp())
					return op;
				return {};
			}

			Optional<const T&> getValue() const
			{
				if (isValue())
					return value;
				return {};
			}

			bool isInvalid() const { return type == Type::Invalid; }
			bool isDone() const
			{
				return
				(
					type == Type::Done ||
					doneExpr.hasValue()
				);
			}
			bool isNoOverride() const { return type == Type::NoOverride; }
			bool isSkip() const { return type == Type::Skip; }
			bool isError() const { return type == Type::Error; }
			bool isOp() const { return type == Type::Op; }
			bool isValue() const { return type == Type::Value; }
			bool hasValue() const { return isValue(); }

			EvalResult() : type(Type::Invalid) {}
			EvalResult(const EvalResult& other) :
			type(other.type),
			doneExpr(other.doneExpr)
			{
				switch (type)
				{
					case Type::Error: new(&error) Error(other.error); break;
					case Type::Op: new(&op) Op(other.op); break;
					case Type::Value: new(&value) T(other.value); break;
					default: break;
				}
			}

			EvalResult& operator=(const EvalResult& other)
			{
				type = other.type;
				doneExpr = other.doneExpr;
				switch (type)
				{
					case Type::Error: new(&error) Error(other.error); break;
					case Type::Op: new(&op) Op(other.op); break;
					case Type::Value: new(&value) T(other.value); break;
					default: break;
				}
				return *this;
			}

			~EvalResult()
			{
				switch (type)
				{
					case Type::Error: error.~Error(); break;
					case Type::Op: op.~Op(); break;
					case Type::Value: value.~T(); break;
					default: break;
				}
			}
		};

		struct Group
		{
			using Iter = std::list<Expr>::iterator;
			using ConstIter = std::list<Expr>::const_iterator;
			using GroupIterPair = std::pair<Group::ConstIter&, Group::ConstIter>;

			std::list<Expr> list;

			template<typename T, typename V>
			EvalResult<T> eval
			(
				V&& visitor,
				const Expr& parent,
				Optional<GroupIterPair> groupIters,
				Optional<size_t> listIndex,
				size_t depth
			) const;
		};

		using GroupIterPair = Group::GroupIterPair;

		struct Value
		{
			using List = std::list<Expr>;
			
			struct Number
			{
				enum class Type
				{
					Int,
					Float,
				};

				Type type;
				union
				{
					int64_t intVal;
					number_t floatVal;
				};

				template<typename T, EnableIf<std::is_integral<T>::value, bool> = false>
				Number(const T& num) : type(Type::Int), intVal(num) {}
				template<typename T, EnableIf<std::is_floating_point<T>::value, bool> = false>
				Number(const T& num) : type(Type::Float), floatVal(num) {}

				Number(const Number& other) : type(other.type)
				{
					switch (type)
					{
						case Type::Int: new(&intVal) int64_t(other.intVal); break;
						case Type::Float: new(&floatVal) number_t(other.floatVal); break;
					}
				}

				Number& operator=(const Number& other)
				{
					type = other.type;
					switch (type)
					{
						case Type::Int: new(&intVal) int64_t(other.intVal); break;
						case Type::Float: new(&floatVal) number_t(other.floatVal); break;
					}
					return *this;
				}

				template<typename T, EnableIf<std::is_enum<T>::value, bool> = false>
				T as() const
				{
					return T(as<std::underlying_type_t<T>>());
				}

				template<typename T, EnableIf<std::is_arithmetic<T>::value, bool> = false>
				T as() const
				{
					if (type == Type::Int)
						return static_cast<T>(intVal);
					return static_cast<T>(floatVal);
				}

				template<typename T, EnableIf<std::is_same<T, tiny_string>::value, bool> = false>
				T as() const
				{
					if (type == Type::Int)
						return std::to_string(intVal);
					return std::to_string(floatVal);
				}

				bool isInt() const { return type == Type::Int; }
				bool isFloat() const { return type == Type::Float; }
			};

			enum class Type
			{
				String,
				Char,
				Bool,
				Number,
				List,
			};

			Type type;
			union
			{
				tiny_string str;
				uint32_t ch;
				bool flag;
				Number num;
				List list;
			};

			Value(const tiny_string& _str) : type(Type::String), str(_str) {}
			Value(uint32_t _ch) : type(Type::Char), ch(_ch) {}
			Value(bool _flag) : type(Type::Bool), flag(_flag) {}
			Value(const Number& _num) : type(Type::Number), num(_num) {}
			Value(const List& _list) : type(Type::List), list(_list) {}

			Value(const Value& other) : type(other.type)
			{
				switch (type)
				{
					case Type::String: new(&str) tiny_string(other.str); break;
					case Type::Char: new(&ch) uint32_t(other.ch); break;
					case Type::Bool: new(&flag) bool(other.flag); break;
					case Type::Number: new (&num) Number(other.num); break;
					case Type::List: new(&list) List(other.list); break;
				}
			}

			Value& operator=(const Value& other)
			{
				type = other.type;
				switch (type)
				{
					case Type::String: new(&str) tiny_string(other.str); break;
					case Type::Char: new(&ch) uint32_t(other.ch); break;
					case Type::Bool: new(&flag) bool(other.flag); break;
					case Type::Number: new (&num) Number(other.num); break;
					case Type::List: new(&list) List(other.list); break;
				}
				return *this;
			}

			~Value()
			{
				switch (type)
				{
					case Type::String: str.~tiny_string(); break;
					case Type::List: list.~List(); break;
					default: break;
				}
			}

			int64_t asInt() const { return as<int64_t>(); }
			number_t asFloat() const { return as<number_t>(); }

			tiny_string asString() const
			{
				switch (type)
				{
					case Type::String: return str; break;
					case Type::Char: return tiny_string::fromChar(ch); break;
					case Type::Bool: return std::to_string(flag); break;
					case Type::Number: return num.as<tiny_string>(); break;
					case Type::List:
						throw LSTokenException
						(
							"Value::as: Can't convert a list to a string."
						);
						break;
				}
				return "";
			}

			template<typename T, EnableIf<std::is_arithmetic<T>::value, bool> = false>
			T as() const
			{
				switch (type)
				{
					case Type::Char: return ch; break;
					case Type::Bool: return flag; break;
					case Type::Number: return num.as<T>(); break;
					case Type::String:
						throw LSTokenException
						(
							"Value::as: Can't convert a string to a numeric type."
						);
						break;
					case Type::List:
						throw LSTokenException
						(
							"Value::as: Can't convert a list to a numeric type."
						);
						break;
				}
				return T {};
			}

			template<typename T, EnableIf<std::is_enum<T>::value, bool> = false>
			T as() const
			{
				return T(as<std::underlying_type_t<T>>());
			}

			template<typename T, EnableIf<std::is_same<T, tiny_string>::value, bool> = false>
			T as() const { return asString(); }
			template<typename T, EnableIf<std::is_same<T, List>::value, bool> = false>
			T as() const { return T(*this); }

			template<typename T, typename V>
			EvalResult<T> eval
			(
				V&& visitor,
				const Expr& parent,
				Optional<GroupIterPair> groupIters,
				Optional<size_t> listIndex,
				size_t depth
			) const;

			bool isString() const { return type == Type::String; }
			bool isChar() const { return type == Type::Char; }
			bool isBool() const { return type == Type::Bool; }
			bool isNumber() const { return type == Type::Number; }
			bool isList() const { return type == Type::List; }

			bool isNumeric() const
			{
				switch (type)
				{
					case Type::Char:
					case Type::Bool:
					case Type::Number: return true; break;
					default: return false; break;
				}
			}
			void dump(size_t depth = 0) const;
		};

		struct Op
		{
			OpType type;
			Expr* lhs;
			Expr* rhs;

			Op(const OpType& _type, const Expr& expr) :
			type(_type),
			lhs(new Expr(expr)),	
			rhs(nullptr) {}

			Op(const OpType& _type, const Expr& _lhs, const Expr& _rhs) :
			type(_type),
			lhs(new Expr(_lhs)),
			rhs(new Expr(_rhs)) {}

			Op(const Op& other) :
			type(other.type),
			lhs(nullptr),
			rhs(nullptr)
			{
				if (other.lhs != nullptr)
					lhs = new Expr(*other.lhs);
				if (other.rhs != nullptr)
					rhs = new Expr(*other.rhs);
			}

			Op& operator=(const Op& other)
			{
				if (lhs != nullptr)
				{
					delete lhs;
					lhs = nullptr;
				}
				if (rhs != nullptr)
				{
					delete rhs;
					rhs = nullptr;
				}

				type = other.type;
				if (other.lhs != nullptr)
					lhs = new Expr(*other.lhs);
				if (other.rhs != nullptr)
					rhs = new Expr(*other.rhs);
				return *this;
			}

			~Op()
			{
				if (lhs != nullptr)
				{
					delete lhs;
					lhs = nullptr;
				}
				if (rhs != nullptr)
				{
					delete rhs;
					rhs = nullptr;
				}
			}

			template<typename T, typename V>
			EvalResult<T> eval
			(
				V&& visitor,
				const Expr& parent,
				Optional<GroupIterPair> groupIters,
				Optional<size_t> listIndex,
				size_t depth
			) const;

			static bool isUnaryOp(const OpType& type)
			{
				switch (type)
				{
					case OpType::Plus:
					case OpType::Minus:
					case OpType::BitNot:
					case OpType::Not: return true; break;
					default: return false; break;
				}
			}

			static bool isBinaryOp(const OpType& type)
			{
				switch (type)
				{
					case OpType::Assign:
					case OpType::Plus:
					case OpType::Minus:
					case OpType::Multiply:
					case OpType::Divide:
					case OpType::LeftShift:
					case OpType::RightShift:
					case OpType::BitAnd:
					case OpType::BitOr:
					case OpType::BitXor:
					case OpType::And:
					case OpType::Or:
					case OpType::Equal:
					case OpType::NotEqual:
					case OpType::LessThan:
					case OpType::LessEqual:
					case OpType::GreaterThan:
					case OpType::GreaterEqual: return true; break;
					default: return false; break;
				}
			}

			static bool isSingleCharOp(const OpType& type)
			{
				switch (type)
				{
					case OpType::Assign:
					case OpType::Plus:
					case OpType::Minus:
					case OpType::Multiply:
					case OpType::Divide:
					case OpType::BitNot:
					case OpType::BitAnd:
					case OpType::BitOr:
					case OpType::BitXor:
					case OpType::Not:
					case OpType::LessThan:
					case OpType::GreaterThan: return true; break;
					default: return false; break;
				}
			}

			static bool isOpChar(uint32_t ch)
			{
				switch (ch)
				{
					// Assignment, or Equality.
					case '=':
					// Plus.
					case '+':
					// Minus.
					case '-':
					// Multiply.
					case '*':
					// Divide.
					case '/':
					// Less/Greater than (or equal), or Left/Right shift.
					case '<':
					case '>':
					// Not (equal).
					case '!':
					// (Bitwise) And/Or.
					case '&':
					case '|':
					// Bitwise Xor.
					case '^':
					// Bitwise Not.
					case '~': return true; break;
					default: return false; break;
				}
			}

			bool isAssign() const { return type == OpType::Assign; }
			void dump(size_t depth = 0) const;
		};

		enum class Type
		{
			Op,
			Value,
			Identifier,
			Block,
			Group,
		};

		Type type;
		union
		{
			Op op;
			Value value;
			tiny_string ident;
			Block block;
			Group group;
		};

		Expr(const Op& _op) : type(Type::Op), op(_op) {}
		Expr(const Value& _value) : type(Type::Value),  value(_value) {}
		Expr(const tiny_string& _ident) : type(Type::Identifier), ident(_ident) {}
		Expr(const Block& _block) : type(Type::Block), block(_block) {}
		Expr(const Group& _group) : type(Type::Group), group(_group) {}

		Expr(const Expr& other) : type(other.type)
		{
			switch (type)
			{
				case Type::Op: new(&op) Op(other.op); break;
				case Type::Value: new(&value) Value(other.value); break;
				case Type::Identifier: new(&ident) tiny_string(other.ident); break;
				case Type::Block: new(&block) Block(other.block); break;
				case Type::Group: new(&group) Group(other.group); break;
			}
		}

		Expr& operator=(const Expr& other)
		{
			type = other.type;
			switch (type)
			{
				case Type::Op: new(&op) Op(other.op); break;
				case Type::Value: new(&value) Value(other.value); break;
				case Type::Identifier: new(&ident) tiny_string(other.ident); break;
				case Type::Block: new(&block) Block(other.block); break;
				case Type::Group: new(&group) Group(other.group); break;
			}
			return *this;
		}

		~Expr()
		{
			switch (type)
			{
				case Type::Op: op.~Op(); break;
				case Type::Value: value.~Value(); break;
				case Type::Identifier: ident.~tiny_string(); break;
				case Type::Block: block.~Block(); break;
				case Type::Group: group.~Group(); break;
			}
		}

		template<typename T>
		T eval() const
		{
			return eval<T>(makeVisitor(
				[]
				(
					const auto&,
					Optional<const Expr&>,
					Optional<GroupIterPair>,
					Optional<size_t>,
					size_t
				)
				{
					return EvalResult<T>::Type::NoOverride;
				},
				[]
				(
					const auto&,
					bool,
					Optional<const Expr&>,
					Optional<GroupIterPair>,
					Optional<size_t>,
					size_t
				)
				{
					return EvalResult<T>::Type::NoOverride;
				}
			));
		}

		template<typename T, typename V>
		T eval(V&& visitor) const;

		template<typename T, typename V>
		EvalResult<T> resultEval
		(
			V&& visitor,
			Optional<const Expr&> parent = {},
			Optional<GroupIterPair> groupIters = {},
			Optional<size_t> listIndex = {},
			size_t depth = 0
		) const;

		bool isOp() const { return type == Type::Op; }
		bool isValue() const { return type == Type::Value; }
		bool isIdent() const { return type == Type::Identifier; }
		bool isBlock() const { return type == Type::Block; }
		bool isGroup() const { return type == Type::Group; }

		// `Op` types.
		bool isAssign() const { return isOp() && op.isAssign(); }

		// `Value` types.
		bool isString() const { return isValue() && value.isString(); }
		bool isChar() const { return isValue() && value.isChar(); }
		bool isBool() const { return isValue() && value.isBool(); }
		bool isNumber() const { return isValue() && value.isNumber(); }
		bool isList() const { return isValue() && value.isList(); }
		bool isNumeric() const { return isValue() && value.isNumeric(); }
		// Returns true if either the expression is a numeric value, or
		// both sides of the expression are numeric.
		bool isNumericExpr() const
		{
			if (isNumeric())
				return true;
			if (!isOp())
				return false;
			bool isLhsNumeric = op.lhs != nullptr && op.lhs->isNumericExpr();
			bool isRhsNumeric = op.rhs != nullptr && op.rhs->isNumericExpr();
			if (op.rhs == nullptr)
				return isLhsNumeric;
			return isLhsNumeric && isRhsNumeric;
		}

		void dump(size_t depth = 0) const;
	};

	struct Comment { tiny_string str; };
	struct Directive { tiny_string name; };

	enum class Type
	{
		Comment,
		Directive,
		Expr,
	};

	Type type;
	union
	{
		Comment comment;
		Directive dir;
		Expr expr;
	};

	LSToken(const Comment& _comment) : type(Type::Comment), comment(_comment) {}
	LSToken(const Directive& _dir) : type(Type::Directive), dir(_dir) {}
	LSToken(const Expr _expr) : type(Type::Expr), expr(_expr) {}

	LSToken(const LSToken& other) : type(other.type)
	{
		switch (type)
		{
			case Type::Comment: new(&comment) Comment(other.comment); break;
			case Type::Directive: new(&dir) Directive(other.dir); break;
			case Type::Expr: new(&expr) Expr(other.expr); break;
		}
	}

	LSToken& operator=(const LSToken& other)
	{
		type = other.type;
		switch (type)
		{
			case Type::Comment: new(&comment) Comment(other.comment); break;
			case Type::Directive: new(&dir) Directive(other.dir); break;
			case Type::Expr: new(&expr) Expr(other.expr); break;
		}
		return *this;
	}

	~LSToken()
	{
		switch (type)
		{
			case Type::Comment: comment.~Comment(); break;
			case Type::Directive: dir.~Directive(); break;
			case Type::Expr: expr.~Expr(); break;
		}
	}

	bool isComment() const { return type == Type::Comment; }
	bool isDirective() const { return type == Type::Directive; }
	bool isExpr() const { return type == Type::Expr; }

	// `Expr` types.
	bool isBlock() const { return isExpr() && expr.isBlock(); }
	bool isGroup() const { return isExpr() && expr.isGroup(); }
	bool isNumericExpr() const { return isExpr() && expr.isNumericExpr(); }
	bool isList() const { return isExpr() && expr.isList(); }

	void dump(size_t depth = 0) const;
};

template<typename T, EnableIf<std::is_enum<T>::value, bool> = false>
static T evalOp(const LSToken::Expr::OpType& type, bool isBinary, const T& a, const T& b)
{
	using Type = std::underlying_type_t<T>;
	return T(evalOp<Type>
	(
		type,
		isBinary,
		static_cast<const Type&>(a),
		static_cast<const Type&>(b)
	));
}

template<typename T, EnableIf<std::is_integral<T>::value && !std::is_same<T, bool>::value, bool> = false>
static T evalOp(const LSToken::Expr::OpType& type, bool isBinary, const T& a, const T& b)
{
	using OpType = LSToken::Expr::OpType;
	switch (type)
	{
		case OpType::Assign: return b; break;
		case OpType::Plus:
			return isBinary ? a + b : +a;
			break;
		case OpType::Minus:
			return isBinary ? a - b : -a;
			break;
		case OpType::Multiply: return a * b; break;
		case OpType::Divide: return a / b; break;
		case OpType::LeftShift: return a << b; break;
		case OpType::RightShift: return a >> b; break;
		case OpType::BitNot: return ~a; break;
		case OpType::BitAnd: return a & b; break;
		case OpType::BitOr: return a | b; break;
		case OpType::BitXor: return a ^ b; break;
		case OpType::Not: return !a; break;
		case OpType::And: return a && b; break;
		case OpType::Or: return a || b; break;
		case OpType::Equal: return a == b; break;
		case OpType::NotEqual: return a != b; break;
		case OpType::LessThan: return a < b; break;
		case OpType::LessEqual: return a <= b; break;
		case OpType::GreaterThan: return a > b; break;
		case OpType::GreaterEqual: return a >= b; break;
		default: return a; break;
	}
}

template<typename T, EnableIf<std::is_floating_point<T>::value, bool> = false>
static T evalOp(const LSToken::Expr::OpType& type, bool isBinary, const T& a, const T& b)
{
	using OpType = LSToken::Expr::OpType;
	switch (type)
	{
		case OpType::Assign: return b; break;
		case OpType::Plus:
			return isBinary ? a + b : +a;
			break;
		case OpType::Minus:
			return isBinary ? a - b : -a;
			break;
		case OpType::Multiply: return a * b; break;
		case OpType::Divide: return a / b; break;
		case OpType::LeftShift:
		case OpType::RightShift:
			throw LSTokenException("Can't use bit shifts on floats.");
			break;
		case OpType::BitNot:
		case OpType::BitAnd:
		case OpType::BitOr:
		case OpType::BitXor:
			throw LSTokenException("Can't use bitwise operations on floats.");
			break;
		case OpType::Not: return !a; break;
		case OpType::And: return a && b; break;
		case OpType::Or: return a || b; break;
		case OpType::Equal: return a == b; break;
		case OpType::NotEqual: return a != b; break;
		case OpType::LessThan: return a < b; break;
		case OpType::LessEqual: return a <= b; break;
		case OpType::GreaterThan: return a > b; break;
		case OpType::GreaterEqual: return a >= b; break;
		default: return a; break;
	}
}

template<typename T, EnableIf<std::is_same<T, bool>::value, bool> = false>
static T evalOp(const LSToken::Expr::OpType& type, bool isBinary, const T& a, const T& b)
{
	using OpType = LSToken::Expr::OpType;
	switch (type)
	{
		case OpType::Assign: return b; break;
		case OpType::Plus:
		case OpType::Minus:
		case OpType::Multiply:
		case OpType::Divide:
			throw LSTokenException("Can't use arithmentic operations on bools.");
			break;
		case OpType::LeftShift:
		case OpType::RightShift:
			throw LSTokenException("Can't use bit shifts on bools.");
			break;
		case OpType::BitNot:
			throw LSTokenException("Can't use bitwise NOT on bools.");
			break;
		case OpType::BitAnd: return a & b; break;
		case OpType::BitOr: return a | b; break;
		case OpType::BitXor: return a ^ b; break;
		case OpType::Not: return !a; break;
		case OpType::And: return a && b; break;
		case OpType::Or: return a || b; break;
		case OpType::Equal: return a == b; break;
		case OpType::NotEqual: return a != b; break;
		case OpType::LessThan: return a < b; break;
		case OpType::LessEqual: return a <= b; break;
		case OpType::GreaterThan: return a > b; break;
		case OpType::GreaterEqual: return a >= b; break;
		default: return a; break;
	}
}

template<typename T, typename V>
static LSToken::Expr::EvalResult<T> evalList
(
	const std::list<LSToken::Expr>& list,
	V&& visitor,
	const LSToken::Expr& parent,
	Optional<LSToken::Expr::GroupIterPair> groupIters,
	Optional<size_t> listIndex,
	size_t depth,
	bool isList
)
{
	using EvalResult = LSToken::Expr::EvalResult<T>;
	using EvalResultType = typename EvalResult::Type;
	using GroupIterPair = LSToken::Expr::GroupIterPair;
	size_t i = 0;
	EvalResult ret;

	for (auto it = list.cbegin(); it != list.cend(); ++it, ++i)
	{
		auto elem = *it;
		GroupIterPair pair(it, list.cend());
		auto result = elem.resultEval<T>
		(
			visitor,
			parent,
			!isList ? pair : groupIters,
			isList ? i : listIndex,
			depth
		);
		switch (result.type)
		{
			case EvalResultType::Op:
				ret.value = evalOp
				(
					result.op.type,
					true,
					ret.value,
					result.op.value
				);
				break;
			case EvalResultType::Invalid:
			case EvalResultType::Error: return result; break;
			case EvalResultType::Done:
			{
				auto nextIt = std::next(it);
				const auto& expr = *(nextIt != list.cend() ? nextIt : it);
				return ret.withDoneExpr(expr);
				break;
			}
			case EvalResultType::Value: ret = result; break;
			default: break;
		}
	}
	return ret;
}

template<typename T, typename V>
LSToken::Expr::EvalResult<T> LSToken::Expr::Group::eval
(
	V&& visitor,
	const LSToken::Expr& parent,
	Optional<LSToken::Expr::GroupIterPair> groupIters,
	Optional<size_t> listIndex,
	size_t depth
) const
{
	EvalResult<T> result = visitor(*this, parent, groupIters, listIndex, depth);
	if (result.isDone())
		return result.tryWithDoneExpr(parent);
	if (!result.isNoOverride())
		return result;

	return evalList<T>(list, visitor, parent, groupIters, listIndex, depth, false);
}

#define CANT_OVERRIDE(str) \
	str ", and visitor didn't override."

#define NO_DEFAULT(str, typeName) \
	CANT_OVERRIDE(str #typeName "s have no default eval")

template<typename T, typename V>
LSToken::Expr::EvalResult<T> LSToken::Expr::Value::eval
(
	V&& visitor,
	const LSToken::Expr& parent,
	Optional<LSToken::Expr::GroupIterPair> groupIters,
	Optional<size_t> listIndex,
	size_t depth
) const
{
	EvalResult<T> result = visitor(*this, parent, groupIters, listIndex, depth);
	if (result.isDone())
		return result.tryWithDoneExpr(parent);
	if (!result.isNoOverride())
		return result;

	switch (type)
	{
		case Type::String:
			result = visitor(str, true, parent, groupIters, listIndex, depth);
			break;
		case Type::Char:
			result = visitor(ch, parent, groupIters, listIndex, depth);
			break;
		case Type::Bool:
			result = visitor(flag, parent, groupIters, listIndex, depth);
			break;
		case Type::Number:
			result = visitor(num, parent, groupIters, listIndex, depth);
			break;
		case Type::List:
			result = visitor(list, parent, groupIters, listIndex, depth);
			if (result.isNoOverride())
				return evalList<T>(list, visitor, parent, groupIters, listIndex, depth + 1, true);
			break;
	}
	if (result.isDone())
		return result.tryWithDoneExpr(parent);
	if (result.isNoOverride())
		return as<T>();
	return result;
}

template<typename T, typename V>
LSToken::Expr::EvalResult<T> LSToken::Expr::Op::eval
(
	V&& visitor,
	const LSToken::Expr& parent,
	Optional<LSToken::Expr::GroupIterPair> groupIters,
	Optional<size_t> listIndex,
	size_t depth
) const
{
	using EvalResultType = typename EvalResult<T>::Type;

	EvalResult<T> result = visitor(*this, parent, groupIters, listIndex, depth);
	if (!result.isNoOverride())
		return result;

	EvalResult<T> lhsResult = lhs != nullptr ? lhs->resultEval<T>
	(
		visitor,
		parent,
		groupIters,
		listIndex,
		depth + 1
	) : EvalResultType::Skip;

	if (lhsResult.isDone())
		return lhsResult.tryWithDoneExpr(parent);

	EvalResult<T> rhsResult = rhs != nullptr ? rhs->resultEval<T>
	(
		visitor,
		parent,
		groupIters,
		listIndex,
		depth + 1
	) : EvalResultType::Skip;

	if (rhsResult.isDone())
		return rhsResult.tryWithDoneExpr(parent);

	return evalOp
	(
		type,
		rhsResult.hasValue(),
		lhsResult.getValue().valueOr(T()),
		rhsResult.getValue().valueOr(T())
	);
}

template<typename T, typename V>
T LSToken::Expr::eval(V&& visitor) const
{
	using EvalResultType = typename EvalResult<T>::Type;
	#define UNHANDLED_TYPE(type) \
		throw LSTokenException("eval: Unhandled `" #type "`.")

	auto result = resultEval<T>(visitor);
	switch (result.type)
	{
		case EvalResultType::Invalid:
			throw LSTokenException("eval: Result is invalid.");
			break;
		case EvalResultType::Done: UNHANDLED_TYPE(Done); break;
		case EvalResultType::Skip: UNHANDLED_TYPE(Skip); break;
		case EvalResultType::NoOverride: UNHANDLED_TYPE(NoOverride); break;
		case EvalResultType::Error:
		{
			auto msg = result.error.msg;
			throw LSTokenException
			(
				msg.valueOr("eval: Unknown error occured.")
			);
			break;
		}
		case EvalResultType::Op: UNHANDLED_TYPE(Op); break;
		case EvalResultType::Value: return result.value; break;
	}
	#undef UNHANDLED_TYPE
	return T {};
}

template<typename T, typename V>
LSToken::Expr::EvalResult<T> LSToken::Expr::resultEval
(
	V&& visitor,
	Optional<const LSToken::Expr&> parent,
	Optional<GroupIterPair> groupIters,
	Optional<size_t> listIndex,
	size_t depth
) const
{
	using EvalError = typename EvalResult<T>::Error;

	auto& _parent = parent.valueOr(*this);
	EvalResult<T> result;
	switch (type)
	{
		case Type::Op:
			result = op.eval<T>(visitor, *this, groupIters, listIndex, depth);
			break;
		case Type::Value:
			result = value.eval<T>(visitor, *this, groupIters, listIndex, depth);
			break;
		case Type::Identifier:
			result = visitor(ident, false, _parent, groupIters, listIndex, depth);
			if (!result.isNoOverride())
				break;
			return EvalError { tiny_string(NO_DEFAULT("resultEval: ", Identifier), true) };
			break;
		case Type::Block:
			result = visitor(block, _parent, groupIters, listIndex, depth);
			if (!result.isNoOverride())
				break;
			return EvalError { tiny_string(NO_DEFAULT("resultEval: ", Block), true) };
			break;
		case Type::Group:
			result = group.eval<T>(visitor, *this, groupIters, listIndex, depth);
			break;
	}
	return result;
}

#undef CANT_OVERRIDE
#undef NO_DEFAULT

#endif /* UTILS_TOKEN_PARSER_TOKEN_H */
