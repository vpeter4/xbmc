/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/SingleLock.h"

#include <chrono>
#include <condition_variable>
#include <functional>

namespace XbmcThreads
{

  /**
   * This is a thin wrapper around std::condition_variable_any. It is subject
   *  to "spurious returns"
   */
  class ConditionVariable
  {
  private:
    std::condition_variable_any cond;
    ConditionVariable(const ConditionVariable&) = delete;
    ConditionVariable& operator=(const ConditionVariable&) = delete;

  public:
    ConditionVariable() = default;

    inline void wait(CCriticalSection& lock, std::function<bool()>& predicate)
    {
      int count = lock.count;
      lock.count = 0;
      cond.wait(lock.get_underlying(), predicate);
      lock.count = count;
    }

    inline void wait(CCriticalSection& lock)
    {
      int count  = lock.count;
      lock.count = 0;
      cond.wait(lock.get_underlying());
      lock.count = count;
    }

    template<typename Rep, typename Period>
    inline bool wait(CCriticalSection& lock,
                     std::chrono::duration<Rep, Period> duration,
                     std::function<bool()>& predicate)
    {
      int count = lock.count;
      lock.count = 0;
      bool ret = cond.wait_for(lock.get_underlying(), duration, predicate);
      lock.count = count;
      return ret;
    }

    template<typename Rep, typename Period>
    inline bool wait(CCriticalSection& lock, std::chrono::duration<Rep, Period> duration)
    {
      int count  = lock.count;
      lock.count = 0;
      std::cv_status res = cond.wait_for(lock.get_underlying(), duration);
      lock.count = count;
      return res == std::cv_status::no_timeout;
    }

    inline void wait(CSingleLock& lock, std::function<bool()>& predicate)
    {
      cond.wait(*lock.mutex(), predicate);
    }

    inline void wait(CSingleLock& lock) { wait(*lock.mutex()); }

    template<typename Rep, typename Period>
    inline bool wait(CSingleLock& lock,
                     std::chrono::duration<Rep, Period> duration,
                     std::function<bool()>& predicate)
    {
      return wait(*lock.mutex(), duration, predicate);
    }

    template<typename Rep, typename Period>
    inline bool wait(CSingleLock& lock, std::chrono::duration<Rep, Period> duration)
    {
      return wait(*lock.mutex(), duration);
    }

    inline void notifyAll()
    {
      cond.notify_all();
    }

    inline void notify()
    {
      cond.notify_one();
    }
  };

  /**
   * This is a condition variable along with its predicate. This allows the use of a
   *  condition variable without the spurious returns since the state being monitored
   *  is also part of the condition.
   *
   * L should implement the Lockable concept
   *
   * The requirements on P are that it can act as a predicate (that is, I can use
   *  it in an 'while(!predicate){...}' where 'predicate' is of type 'P').
   */
  class TightConditionVariable
  {
    ConditionVariable& cond;
    std::function<bool()> predicate;

  public:
    inline TightConditionVariable(ConditionVariable& cv, std::function<bool()> predicate_)
      : cond(cv), predicate(predicate_)
    {
    }

    template<typename L>
    inline void wait(L& lock)
    {
      cond.wait(lock, predicate);
    }

    template<typename L, typename Rep, typename Period>
    inline bool wait(L& lock, std::chrono::duration<Rep, Period> duration)
    {
      return cond.wait(lock, duration, predicate);
    }

    inline void notifyAll() { cond.notifyAll(); }
    inline void notify() { cond.notify(); }
  };
}

