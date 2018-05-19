/*
 * Copyright (c) 2018 Guo Xiang
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __TIMER_HPP__
#define __TIMER_HPP__

#include <time.h>

#include <EasyCpp.hpp>
#include <Event.hpp>
#include <Lock.hpp>
#include <IMultiThread.hpp>

DEFINE_CLASS(Timer);

class CTimer :
	public ITimer,
	public CEnableSharedPtr<CTimer>
{
private:
	DEFINE_CLASS(TimerInfo);
	class CTimerInfo {
	public:
		CTimerInfo(uint64_t timeout,
				   const RunnableFn &run,
				   const CTimerPtr &timer) :
			mTimeout(timeout),
			mRun(run),
			mTimer(timer)
		{
			/* Does nothing */
		}

		uint64_t mTimeout;
		RunnableFn mRun;
		CTimerPtr mTimer;
	};

	typedef CList<CTimerInfoPtr> TimerInfoList;

public:
	CTimer(void);
	virtual ~CTimer(void);

	void Init(uint32_t timeout, const RunnableFn &run);

	virtual void Restart(uint32_t timeout);
	virtual void Stop(void);

private:
	inline static uint64_t GetCurrentTime(void);
	inline static void Wakeup(void);

private:
	static void QueueInfo(const CTimerInfoPtr &info, bool reset);
	static void TimerThread(void);

private:
	static bool sInited;
	static CLock sLock;
	static CEvent sEvent;
	static TimerInfoList sInfoList;
	static int sFd[2];
	static fd_set sFdSet;
};

inline uint64_t CTimer::GetCurrentTime(void)
{
	struct timespec ts;

	if (0 != clock_gettime(CLOCK_MONOTONIC, &ts)) {
		throw E("Fail to get current time. Fd: ", DEC(errno));
	}

	return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

inline void CTimer::Wakeup(void)
{
	sEvent.Wakeup();
	write(sFd[1], "1", 1);
}

#endif /* __EVENT_HPP__ */

