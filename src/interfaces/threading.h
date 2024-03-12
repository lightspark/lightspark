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

#ifndef INTERFACES_THREADING_H
#define INTERFACES_THREADING_H 1

#include "forwards/scripting/flash/system/flashsystem.h"

namespace lightspark
{

class IThreadJob
{
friend class ThreadPool;
private:
	ASWorker* fromWorker;
public:
	/*
	 * Set to true by the ThreadPool just before threadAbort()
	 * is called. For some implementations, it may be enough
	 * to poll threadAborted and not implement threadAbort().
	 */
	volatile bool threadAborting;
	/*
	 * Called in a dedicated thread to do the actual
	 * work. You may throw a JobTerminationException
	 * to quit the job. (Or better: just return)
	 */
	virtual void execute()=0;
	/*
	 * Called asynchronously to abort a job
	 * who is currently in execute().
	 * 'aborting' is set to true before calling
	 * this function.
	 */
	virtual void threadAbort() {}
	/*
	 * Called after the job has finished execute()'ing or
	 * if the ThreadPool aborts and the job did not have
	 * a chance to run yet.
	 * You should use jobFence to signal blocking semaphores
	 * and a like. There is no access to this object after
	 * jobFence() is called, so you may safely call
	 * 'delete this'.
	 */
	virtual void jobFence()=0;
	IThreadJob() : fromWorker(nullptr),threadAborting(false) {}
	virtual ~IThreadJob() {}
	void setWorker(ASWorker* w) { fromWorker = w;}
};

};
#endif /* INTERFACEs_THREADING_H */

