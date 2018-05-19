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

#include <EasyCpp.hpp>

#include <Thread.hpp>
#include "Timer.hpp"

bool CTimer::sInited = false;
CLock CTimer::sLock;
CEvent CTimer::sEvent;
CTimer::TimerInfoList CTimer::sInfoList;
int CTimer::sFd[] = {0, 0};
fd_set CTimer::sFdSet;

CTimer::CTimer(void)
{
	if (!sInited) {
		CAutoLock _l(sLock);

		if (!sInited) {
			if (pipe(sFd) < 0) {
				throw E("Fail to create pipe");
			}

			FD_ZERO(&sFdSet);
			FD_SET(sFd[0], &sFdSet);

			Platform::CreateThread([&](void) {
				TimerThread();
			});
			sInited = true;
		}
	}
}

CTimer::~CTimer(void)
{
	/* Does nothing */
}

void CTimer::Init(uint32_t timeout, const RunnableFn &run)
{
	CTimerInfoPtr info(GetCurrentTime() + timeout, run, Share());
	CAutoLock _l(sLock);
	QueueInfo(info, false);
}

void CTimer::Restart(uint32_t)
{
	CAutoLock _l(sLock);

	sInfoList.Iter()->First([&](const CTimerInfoPtr &info,
								const TimerInfoList::IteratorPtr &iter) {
		if (info->mTimer == Share()) {
			iter->Erase();
			QueueInfo(info, true);
		}
	})->RestEach([&](const CTimerInfoPtr &info,
				 const TimerInfoList::IteratorPtr &iter) {
		if (info->mTimer == Share()) {
			iter->Erase();
			QueueInfo(info, false);
		}
	});
}

void CTimer::Stop(void)
{
	CAutoLock _l(sLock);

	sInfoList.Iter()->ForEach([&](const CTimerInfoPtr &info,
								  const TimerInfoList::IteratorPtr &iter) {
		if (info->mTimer == Share()) {
			iter->Erase();
			return BREAK;
		} else {
			return CONTINUE;
		}
	});
}

void CTimer::QueueInfo(const CTimerInfoPtr &info, bool reset)
{
	if (0 == sInfoList.GetSize()) {
		sInfoList.PushBack(info);
		Wakeup();
		return;
	}

	sInfoList.Iter()->First([&](const CTimerInfoPtr &_info,
								const TimerInfoList::IteratorPtr &iter) {
		/* The new timer is earlier than all current timer */
		if (info->mTimeout < _info->mTimeout) {
			iter->Insert(info);
			Wakeup();
		}
	})->RestEach([&](const CTimerInfoPtr &_info,
				 const TimerInfoList::IteratorPtr &iter) {
		if (info->mTimeout < _info->mTimeout) {
			iter->Insert(info);
			if (reset) {
				Wakeup();
			}
			return BREAK;
		} else {
			return CONTINUE;
		}
	});
}

void CTimer::TimerThread(void)
{
	while (true) {
		sEvent.Wait();

		CAutoLock _l(sLock);
		uint64_t cur = GetCurrentTime();

		sInfoList.Iter()->ForEach([&](const CTimerInfoPtr &info,
									  const TimerInfoList::IteratorPtr &iter) {
			if (info->mTimeout <= cur) {
				info->mRun();
				iter->Erase();
				return BREAK;
			} else {
				return CONTINUE;
			}
		});

		sInfoList.Iter()->First([&](const CTimerInfoPtr &info) {
			struct timeval tv;
			CHECK_PARAM(info->mTimeout > cur, "Illegal timeout: ",
						DEC(info->mTimeout), " vs ", DEC(cur));
			cur = info->mTimeout - cur;

			tv.tv_sec = cur / 1000000;
			tv.tv_usec = cur - (tv.tv_sec * 1000000);

			int ret = select(sFd[0] + 1, &sFdSet, NULL, NULL, &tv);
			if (ret < 0) {
				throw E("Fail to select. error: ", DEC(errno));
				/* Timeout */
			} else if (ret > 0) {
				char tmp[10];
				read(sFd[0], tmp, 10);
			};

			sEvent.Wakeup();
		});
	}
}

