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

#ifndef UTILS_TIMESPEC_H
#define UTILS_TIMESPEC_H 1

namespace lightspark
{

class TimeSpec
{
private:
	uint64_t sec;
	uint64_t nsec;
	constexpr TimeSpec(uint64_t secs, uint64_t nsecs) : sec(secs), nsec(nsecs) {}
public:

	static constexpr uint64_t usPerSec = 1000000;
	static constexpr uint64_t nsPerSec = 1000000000;
	static constexpr uint64_t nsPerMs = 1000000;
	static constexpr uint64_t nsPerUs = 1000;

	constexpr TimeSpec() : sec(0), nsec(0) {}

	static constexpr TimeSpec fromFloat(float secs) { return fromNs(secs * nsPerSec); }
	static constexpr TimeSpec fromFloatUs(float secs) { return fromUs(secs * usPerSec); }
	static constexpr TimeSpec fromSec(uint64_t sec) { return TimeSpec(sec, 0); }
	static constexpr TimeSpec fromMs(uint64_t ms) { return fromNs(ms * nsPerMs); }
	static constexpr TimeSpec fromUs(uint64_t us) { return fromNs(us * nsPerUs); }
	static constexpr TimeSpec fromNs(uint64_t ns) { return TimeSpec(ns / nsPerSec, ns % nsPerSec); }

	constexpr bool operator==(const TimeSpec &other) const { return sec == other.sec && nsec == other.nsec; }
	constexpr bool operator!=(const TimeSpec &other) const { return sec != other.sec || nsec != other.nsec; }
	constexpr bool operator>(const TimeSpec &other) const { return sec > other.sec || (sec == other.sec && nsec > other.nsec); }
	constexpr bool operator<(const TimeSpec &other) const { return sec < other.sec || (sec == other.sec && nsec < other.nsec); }
	constexpr bool operator>=(const TimeSpec &other) const { return sec > other.sec || (sec == other.sec && nsec >= other.nsec); }
	constexpr bool operator<=(const TimeSpec &other) const { return sec < other.sec || (sec == other.sec && nsec <= other.nsec); }

	TimeSpec operator-(const TimeSpec &other) const { return TimeSpec(sec - other.sec, nsec - other.nsec).normalize(); }
	TimeSpec operator+(const TimeSpec &other) const { return TimeSpec(sec + other.sec, nsec + other.nsec).normalize(); }

	TimeSpec &operator+=(const TimeSpec &other) { return *this = *this + other; }
	TimeSpec &operator-=(const TimeSpec &other) { return *this = *this - other; }

	TimeSpec &normalize()
	{
		auto normal_secs = ((int64_t)nsec >= 0 ? nsec : nsec - (nsPerSec-1)) / nsPerSec;
		sec += normal_secs;
		nsec -= normal_secs * nsPerSec;
		return *this;
	}

	operator struct timespec() const
	{
		struct timespec ret;
		ret.tv_sec = sec;
		ret.tv_nsec = nsec;
		return ret;
	}

	constexpr uint64_t toMs() const { return toNs() / nsPerMs; }
	constexpr uint64_t toUs() const { return toNs() / nsPerUs; }
	constexpr uint64_t toNs() const { return (sec * nsPerSec) + nsec; }
	constexpr uint64_t toMsRound() const { return (toNs() + (nsPerMs / 2)) / nsPerMs; }
	constexpr uint64_t toUsRound() const { return (toNs() + (nsPerUs / 2)) / nsPerUs; }

	constexpr uint64_t getSecs() const { return sec; }
	constexpr uint64_t getNsecs() const { return nsec; }
};

};
#endif /* UTILS_TIMESPEC_H */
