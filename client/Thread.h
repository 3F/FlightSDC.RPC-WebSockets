/*
 * Copyright (C) 2001-2011 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef DCPLUSPLUS_DCPP_THREAD_H
#define DCPLUSPLUS_DCPP_THREAD_H

#ifdef _WIN32
#include "w.h"
// [+] IRainman fix.
#define GetSelfThreadID() ::GetCurrentThreadId()
typedef DWORD ThreadID;
// [~] IRainman fix.
#else
#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>
// [+] IRainman fix.
#define GetSelfThreadID() pthread_self()
typedef pthread_t ThreadID;
// [~] IRainman fix.
#endif

#ifdef _DEBUG
#include <boost/noncopyable.hpp>
#endif
#include "Exception.h"
#include <boost/utility.hpp>

#include <boost/thread.hpp>

namespace dcpp
{

STANDARD_EXCEPTION(ThreadException);

typedef boost::recursive_mutex  CriticalSection;
typedef boost::detail::spinlock FastCriticalSection;
typedef boost::lock_guard<boost::recursive_mutex> Lock;
typedef boost::lock_guard<boost::detail::spinlock> FastLock;

class Thread
#ifdef _DEBUG
	: private boost::noncopyable
#endif
{
	public:
#ifdef _WIN32
		enum Priority
		{
			IDLE = THREAD_PRIORITY_IDLE,
			LOW = THREAD_PRIORITY_BELOW_NORMAL,
			NORMAL = THREAD_PRIORITY_NORMAL,
			HIGH = THREAD_PRIORITY_ABOVE_NORMAL
		};
		
		explicit Thread() : threadHandle(INVALID_HANDLE_VALUE) { }
		virtual ~Thread()
		{
			if (threadHandle != INVALID_HANDLE_VALUE)
				CloseHandle(threadHandle);
		}
		
		void start();
		void join()
		{
			if (threadHandle == INVALID_HANDLE_VALUE)
			{
				return;
			}
			
			WaitForSingleObject(threadHandle, INFINITE);
			CloseHandle(threadHandle);
			threadHandle = INVALID_HANDLE_VALUE;
		}
		
		void setThreadPriority(Priority p)
		{
			::SetThreadPriority(threadHandle, p);
		}
		
		static void sleep(uint64_t millis)
		{
			::Sleep(static_cast<DWORD>(millis));
		}
		static void yield()
		{
			::Sleep(0);
		}
		
#else
		
		enum Priority
		{
			IDLE = 1,
			LOW = 1,
			NORMAL = 0,
			HIGH = -1
		};
		Thread() : threadHandle(0) { }
		virtual ~Thread()
		{
			if (threadHandle != 0)
			{
				pthread_detach(threadHandle);
			}
		}
		void start();
		void join()
		{
			if (threadHandle)
			{
				pthread_join(threadHandle, 0);
				threadHandle = 0;
			}
		}
		
		void setThreadPriority(Priority p)
		{
			setpriority(PRIO_PROCESS, 0, p);
		}
		static void sleep(uint32_t millis)
		{
			::usleep(millis * 1000);
		}
		static void yield()
		{
			::sched_yield();
		}
#endif
		
	protected:
		virtual int run() = 0;
		
#ifdef _WIN32
		HANDLE threadHandle;
		
		static unsigned int  WINAPI starter(void* p)
		{
			Thread* t = (Thread*)p;
			t->run();
			return 0;
			// 2012-05-13_06-33-00_7MVDWDWCOHT7QGSTYJ6VV2CRKS6ZW4Z26ZLBTDQ_86F3C4CB_crash-strong-stack-r9957.dmp
			// 2012-05-13_06-33-00_6L4RNPK74RC3S3K2K4AXX6AXEVFHHLF5MTZA2KQ_B1A20C72_crash-strong-stack-r9957.dmp
		}
#else
		pthread_t threadHandle;
		static void* starter(void* p)
		{
			Thread* t = (Thread*)p;
			t->run();
			return NULL;
		}
#endif
};

} // namespace dcpp

#endif // !defined(THREAD_H)

/**
 * @file
 * $Id: Thread.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
