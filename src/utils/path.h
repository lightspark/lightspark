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

#include "backends/path.h"
#include "compat.h"
#include "tiny_string.h"

namespace std
{

template<>
struct iterator_traits<lightspark::CharIterator>
{
	using difference_type = ssize_t;
	using value_type = uint32_t;
	using pointer = char*;
	using reference = uint32_t;
	using iterator_category = bidirectional_iterator_tag;
};

};

namespace lightspark
{

class Path;

namespace FileSystem
{

class DirIter;
enum class Perms : uint16_t;

Path canonical(const Path& path);
bool createDirs(const Path& path, const Perms& perms);
bool exists(const Path& path);
bool isDir(const Path& path);
bool isFile(const Path& path);

};

// Based on `path` from https://github.com/gulrak/filesystem
class DLL_PUBLIC Path : public PathHelper<uint32_t>
{
public:
	using StringType = tiny_string;
	using StringSizeType = size_t;
	using StringIter = CharIterator;
	using StringPtr = char*;
	using ConstStrPtr = const char*;
	using ConstStrIter = CharIterator;

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

	class Iter;

	using iterator = Iter;
	using const_iterator = Iter;
	using ConstIter = Iter;
private:
	friend class FileSystem::DirIter;

	void appendName(const StringPtr name);

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

	friend Path FileSystem::canonical(const Path& path);
	friend bool FileSystem::createDirs
	(
		const Path& path,
		const FileSystem::Perms& perms
	);

	size_t rootNameLengthImpl() const;
	bool hasRootNamePrefix() const;
	size_t rootNameLength() const;
	int compareImpl(const Path& other) const;
	void postprocessPathImpl(const Format& format);
	void postprocessPath(const Format& format);
	void checkLongPath();

	static constexpr ValueType genericSeparator = U'/';
	StringType path;
	StringSizeType prefixLength { 0 };
public:
	// NOTE: Can't use `= default` in the default constructor because of
	// bugs in both GCC, and Clang.
	Path() {}
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

	Path
	(
		const char* str,
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
		path = std::move(other);
		postprocessPath(Format::Native);
		return *this;
	}

	template<typename T>
	Path& assign(const T& other)
	{
		path = StringType(other);
		postprocessPath(Format::Native);
		return *this;
	}

	template<typename InputIter>
	Path& assign(InputIter first, InputIter last)
	{
		path = StringType(first, last);
		postprocessPath(Format::Native);
		return *this;
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
		postprocessPath(Format::Native);
		return *this;
	}

	Path& operator+=(const ValueType& other)
	{
		if (path.empty() || !path.endsWith(nativeSeparator))
			path += other == genericSeparator ? nativeSeparator : other;

		checkLongPath();
		return *this;
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
		postprocessPath(Format::Native);
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

	// NOTE: Because we always use `Format::Generic` internally, this has
	// to be a no-op.
	Path& makeNative() { return *this; }
	Path& makePreferred() { return makeNative(); }
	Path& removeFilename();
	Path& replaceFilename(const Path& path);
	Path& replaceExtension(const Path& path = Path());

	const StringType& getNativeStr() const { return path; }
	StringType getGenericStr() const;
	const StringType& getStr() const { return getNativeStr(); }
	ConstStrPtr rawBuf() const { return getNativeStr().raw_buf(); }
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

	bool empty() const { return path.empty(); }
	bool hasRootName() const { return rootNameLength() > 0; }
	bool hasRootDir() const;
	bool hasRoot() const { return hasRootName() || hasRootDir(); }
	bool hasRelativePath() const;
	bool hasParent() const { return !getParent().empty(); }
	bool hasFilename() const;
	bool hasStem() const { return !getStem().empty(); }
	bool hasExtension() const { return !getExtension().empty(); }

	bool isAbsolute() const;
	bool isRelative() const { return !isAbsolute(); }

	Path lexicallyNormal() const;
	Path lexicallyRelative(const Path& base) const;
	Path lexicallyProximate(const Path& base) const;

	Iter begin() const;
	Iter end() const;

	bool operator==(const Path& other) const { return !compare(other); }
	bool operator!=(const Path& other) const { return compare(other); }
	bool operator<(const Path& other) const { return compare(other) < 0; }
	bool operator<=(const Path& other) const { return compare(other) <= 0; }
	bool operator>(const Path& other) const { return compare(other) > 0; }
	bool operator>=(const Path& other) const { return compare(other) >= 0; }

	Path operator/(const Path& other) const { return Path(*this) /= other; }

	Path operator+(const Path& other) const { return Path(*this) += other; }

	bool exists() const { return FileSystem::exists(*this); }
	bool isHiddenFile() const;
	bool isDir() const { return FileSystem::isDir(*this); }
	bool isFile() const { return FileSystem::isFile(*this); }
	bool contains(const Path& _path) const
	{
		return path.contains(_path.path);
	}
};

class Path::Iter
{
private:
	friend class Path;

	ConstStrIter incImpl(const ConstStrIter& pos) const;
	ConstStrIter decImpl(const ConstStrIter& pos) const;

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

	Iter() = default;
	Iter(const Path& path, const ConstStrIter& pos) :
	first(path.path.begin()),
	last(path.path.end()),
	prefix(std::next(first, path.prefixLength)),
	root
	(
		path.hasRootDir() ?
		std::next(first, path.prefixLength + path.rootNameLength()) :
		last
	),
	iter(pos)
	{
		if (pos != last)
			updateCurrent();
	}

	Iter& operator++();
	Iter operator++(int)
	{
		auto it = *this;
		++(*this);
		return it;
	}

	Iter& operator--()
	{
		iter = dec(iter);
		updateCurrent();
		return *this;
	}

	Iter operator--(int)
	{
		auto it = *this;
		--(*this);
		return it;
	}

	bool operator==(const Iter& other) const { return iter == other.iter; }
	bool operator!=(const Iter& other) const { return iter != other.iter; }

	Ref operator*() const { return current; }
	Ptr operator->() const { return &current; }
};

};
#endif /* UTILS_PATH_H */
