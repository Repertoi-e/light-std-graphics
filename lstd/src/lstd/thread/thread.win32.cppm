module;

#include "lstd_platform/windows.h"  // Declarations of Win32 functions

export module lstd.thread.win32;

import lstd.os;

LSTD_BEGIN_NAMESPACE

export {
    // Blocks the calling thread for at least a given period of time in ms.
    // sleep(0) supposedly tells the os to yield execution to another thread.
    void sleep(u32 ms);

    // For synchronizing access to shared memory between several threads.
    // The mutex can be recursive i.e. a program doesn't deadlock if the thread
    // that owns a mutex object calls lock() again.
    //
    // For recursive mutexes, see _recursive_mutex_.
    //
    // Scoped lock can be done with a defer statement.
    struct mutex {
        union {
            struct alignas(64) {
                char Handle[40]{};
            } Win;
        } PlatformData{};
    };

    mutex create_mutex();
    void free_mutex(mutex * m);

    // Block the calling thread until a lock on the mutex can
    // be obtained. The mutex remains locked until unlock() is called.
    void lock(mutex * m);

    // Try to lock the mutex.
    //
    // If the mutex isn't currently locked by any thread, the calling thread
    // locks it and the function returns true.
    //
    // If the mutex is currently locked by another thread, the function returns false
    // without blocking (the calling thread continues its execution).
    bool try_lock(mutex * m);

    // Unlock the mutex.
    // If any threads are waiting for the lock on this mutex, one of them will be unblocked
    // unless the calling thread has locked this mutex multiple times (recursive mutex).
    void unlock(mutex * m);

    // Mutex, but instead of using system level functions it is implemented
    // as an atomic spin lock with very low CPU overhead.
    //
    // fast_mutex is NOT compatible with condition_variable.
    // It should also be noted that fast_mutex typically does not provide as
    // accurate thread scheduling as the standard mutex does.
    //
    // Because of the limitations of this object, it should only be used in
    // situations where the mutex needs to be locked/unlocked very frequently.
    struct fast_mutex {
        s32 Lock = 0;
    };

    // Try to lock the mutex. If it fails, the function will
    // return immediately (non-blocking).
    //
    // Returns true if the lock was acquired
    bool try_lock(fast_mutex * m) {
        s32 oldLock = atomic_swap(&m->Lock, 1);
        return oldLock == 0;
    }

    // Block the calling thread until a lock on the mutex can
    // be obtained. The mutex remains locked until unlock() is called.
    void lock(fast_mutex * m) {
        while (!try_lock(m)) sleep(0);
    }

    // Unlock the mutex.
    // If any threads are waiting for the lock on this mutex, one of them will be unblocked.
    void unlock(*m) {
        atomic_swap(&m->Lock, 0);
    }

    //
    // Condition variable.
    // @TODO: Example usage
    struct condition_variable {
#if OS == WINDOWS
        void pre_wait();
        void do_wait();
#endif

        char Handle[64] = {0};
    };

    // This condition variable won't work until init() is called.
    condition_variable create_condition_variable();
    void free_condition_variable(condition_variable * c);

    // Wait for the condition.
    // The function will block the calling thread until the condition variable
    // is woken by notify_one(), notify_all() or a spurious wake up.
    template <typename MutexT>
    void wait(condition_variable * c, MutexT * m) {
#if OS == WINDOWS
        c->pre_wait();
#endif

        // Release the mutex while waiting for the condition (will decrease
        // the number of waiters when done)...
        unlock(m);

#if OS == WINDOWS
        c->do_wait();
#endif
        lock(m);
    }

    // Notify one thread that is waiting for the condition.
    // If at least one thread is blocked waiting for this condition variable, one will be woken up.
    //
    // Only threads that started waiting prior to this call will be woken up.
    void notify_one(condition_variable * c);

    // Wake up all threads that are blocked waiting for this condition variable.
    //
    // Only threads that started waiting prior to this call will be woken up.
    void notify_all(condition_variable * c);

    struct thread {
        // Don't read this directly, use an atomic operation:   atomic_compare_exchange_pointer(&Handle, null, null).
        // You probably don't want this anyway?
        void *Handle = null;

        thread() {}

        // This is the internal thread wrapper function.
#if OS == WINDOWS
        static u32 __stdcall wrapper_function(void *data);
#else
        static void *wrapper_function(void *data);
#endif
    };

    void create_and_launch_thread(const delegate<void(void *)> &function, void *userData = null);

    // Wait for the thread to finish
    void wait(thread t);

    // Terminate the thread without waiting
    void terminate(thread t);

    // The Context also stores the current thread's ID
    u32 get_id(thread t);
}

LSTD_MODULE_PRIVATE

LSTD_END_NAMESPACE
