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

#include <unistd.h>

#include "Timer.hpp"

namespace Platform {

void Sleep(uint32_t sec)
{
	::sleep(sec);
}

void MSleep(uint32_t ms)
{
	::usleep(ms * 1000);
}

void USleep(uint32_t us)
{
	::usleep(us);
}

ITimerPtr CreateTimer(uint32_t timeout, const RunnableFn &run)
{
	CTimerPtr timer;

	timer->Init(timeout, run);

	return timer;
}

}

