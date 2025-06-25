/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2025  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef UTILS_PATH_H
#define UTILS_PATH_H 1

#include "compat.h"
#include "tiny_string.h"

namespace lightspark
{

// Based on `path` from https://github.com/gulrak/filesystem
class Path
{
public:
	using ValueType = uint32_t;
	using StringType = tiny_string;
	using StringSizeType = size_t;
	using StringIter = CharIterator;
	using ConstStrIter = const CharIterator;

	class Iter
	{
	private:
		friend class Path;

		ConstStrIter inc(const ConstStrIter& pos) const;
		ConstStrIter dec(const ConstStrIter& pos) const;
		void updateCurrent();

		ConstStrIter first;
		ConstStrIter last;
		ConstStrIter prefix;
		ConstStrIter root;
		ConstStrIter iter;

		Path current;
	public:
		using ValueType = const Path;
		using Ptr = ValueType*;
		using Ref = ValueType&;
		using DiffType = ptrdiff_t;
		using IterCategory = std::bidirectional_iterator_tag;

		using value_type = ValueType;
		using pointer = Ptr;
		using reference = Ref;
		using difference_type = DiffType;
		using iterator_category = IterCategory;

		Iter();
		Iter(const Path& path, const ConstStrIter& pos);

		Iter& operator++();
		Iter operator++(int);
		Iter& operator--();
		Iter operator--(int);

		bool operator==(const Iter& other) const;
		bool operator!=(const Iter& other) const;

		Ref operator*() const;
		Ptr operator->() const;
	};

	using iterator = Iter;
	using const_iterator = Iter;
	using ConstIter = Iter;
private:
	template<typename InputIter>
	class InputIterRange
	{
	private:
		InputIter first;
		InputIter last;
	public:
		using iterator = InputIter;
		using const_iterator = InputIter;
		using difference_type = typename InputIter::difference_type;

		InputIterRange
		(
			const InputIter& _first,
			const InputIter& _last
		) : first(_first), last(_last) {}

		InputIter begin() const { return first; }
		InputIter end() const { return last; }
	};

	size_t rootNameLength() const;
	int compareImpl(const Path& other) const;

	static constexpr ValueType genericSeparator = U'/';
	StringType path;
	StringSizeType prefixLength { 0 };
public:
	static constexpr ValueType getNativeSeparator();
	static constexpr ValueType getPreferredSeparator()
	{
		return getNativeSeparator();
	}

	static constexpr ValueType nativeSeparator = getNativeSeparator();
	static constexpr ValueType preferredSeparator = getPreferredSeparator();

	// The pathname format.
	enum Format
	{
		// The generic format, used internally for dealing with portable
		// separators (`/`).
		Generic,
		// The native format of the current platform.
		Native,
		// Auto detect the format, fallback to `Native`.
		Auto,
	};

	Path() = default;
	Path(const Path& other) = default;
	Path(Path&& other) = default;

	// TODO: Add locale based versions.
	Path
	(
		const StringType& str,
		const Format& fmt = Format::Auto
	) : path(str) { postprocessPath(fmt); }

	Path
	(
		StringType&& str,
		const Format& fmt = Format::Auto
	) : path(std::move(str)) { postprocessPath(fmt); }

	template<typename InputIter>
	Path
	(
		InputIter first,
		InputIter last,
		const Format& fmt = Format::Auto
	) : Path(StringType(first, last), fmt) {}

	~Path() {}

	Path& operator=(const Path& other) = default;
	Path& operator=(Path&& other) = default;
	Path& operator=(StringType&& other) { return assign(std::move(other)); }
	template<typename T>
	Path& operator=(const T& other) { return assign(other); }

	Path& assign(const Path& other) { return *this = other; }
	Path& assign(StringType&& other)
	{
		path = std:move(other);
		postprocessPath(Format::NativeFormat);
		return *this;
	}

	template<typename T>
	Path& assign(const T& other)
	{
		path = toUTF8(other);
		postprocessPath(Format::NativeFormat);
		return *this;
	}

	template<typename InputIter>
	Path& assign(InputIter first, InputIter last)
	{
		path = StringType(first, last);
		postprocessPath(Format::NativeFormat);
	}

	Path& operator/=(const Path& other);
	template<typename T>
	Path& operator/=(const T& other) { return *this /= Path(other); }

	Path& append(const Path& other) { return *this /= other; }
	template<typename T>
	Path& append(const T& other) { return append(Path(other)); }
	template<typename InputIter>
	Path& append(InputIter first, InputIter last)
	{
		return append(StringType(first, last));
	}

	Path& operator+=(const Path& other)
	{
		path += other.path;
		postprocessPath(Format::NativeFormat);
		return *this;
	}

	Path& operator+=(const ValueType& other)
	{
		if (path.empty() || path.endsWith(nativeSeparator))
			path += other == genericSeparator ? nativeSeparator : other;

		checkLongPath();
	}
	

	template<typename T>
	Path& operator+=(const T& other) { return *this += Path(other); }

	Path& concat(const Path& other) { return *this += other; }
	template<typename T>
	Path& concat(const T& other) { return concat(Path(other)); }
	template<typename InputIter>
	Path& concat(InputIter first, InputIter last)
	{
		#if 1
		return *this += Path(first, last);
		#elif 1
		path += StringType(first, last);
		postprocessPath(Format::NativeFormat);
		return *this;
		#else
		return concat(StringType(first, last));
		#endif
	}

	void clear()
	{
		path.clear();
		prefixLength = 0;
	}

	void swap(Path& other)
	{
		std::swap(path, other.path);
		std::swap(prefixLength, other.prefixLength);
	}

	Path& makeNative();
	Path& makePreferred();
	Path& removeFilename();
	Path& replaceFilename(const Path& path);
	Path& replaceExtension(const Path& path = Path());

	const StringType& getNativeStr() const { return path; }
	const StringType& getGenericStr() const;
	const StringType& getStr() const { return getNativeStr(); }
	operator StringType() const { return getNativeStr(); }

	int compare(const Path& other) const;
	int compare(const StringType& other) const
	{
		return compare(Path(other));
	}

	Path getRootName() const;
	Path getRootDir() const;
	Path getRoot() const;
	Path getRelative() const;
	Path getParent() const;
	Path getDir() const { return getParent(); }
	Path getFilename() const;
	Path getStem() const;
	Path getExtension() const;

	bool empty() const;
	bool hasRootName() const;
	bool hasRootDir() const;
	bool hasRoot() const;
	bool hasRelativePath() const;
	bool hasParent() const;
	bool hasFilename() const;
	bool hasStem() const;
	bool hasExtension() const;

	bool isAbsolute() const;
	bool isRelative() const;

	Path lexicallyNormal() const;
	Path lexicallyRelative(const Path& base) const;
	Path lexicallyProximate(const Path& base) const;

	Iter begin() const;
	Iter end() const;

	bool operator==(const Path& other) const;
	bool operator!=(const Path& other) const;
	bool operator<(const Path& other) const;
	bool operator<=(const Path& other) const;
	bool operator>(const Path& other) const;
	bool operator>=(const Path& other) const;

	Path operator/(const Path& other);
	Path operator/(const StringType& other);
	Path operator/(const ValueType& other);

	Path operator+(const Path& other);
	Path operator+(const StringType& other);
	Path operator+(const ValueType& other);

	bool exists() const;
	bool isHiddenFile() const;
	bool isDir() const;
	bool isFile() const;
	bool contains(const Path& path) const;
};

};
#endif /* UTILS_PATH_H */
