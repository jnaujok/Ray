/*************************************************************************
*   TWorkQueue                                                           *
*                                                                        *
*   A thread-safe first-in, first-out queue that consumers can block on. *
*   Built on IASLib's mutex and counting semaphore.                      *
*                                                                        *
*************************************************************************/
#ifndef __WORKQUEUE_H__
#define __WORKQUEUE_H__

#include "Threading/Mutex.h"
#include "Threading/Semaphore.h"

#include <deque>
#include <utility>

template <class T>
class TWorkQueue
{
    protected:
        IASLib::CMutex      m_mutex;
        IASLib::CSemaphore  m_semItems;
        std::deque<T>       m_aItems;

    public:
                            TWorkQueue( void ) : m_semItems( 0 ) {}

        void                Push( T oItem )
                            {
                                {
                                    IASLib::CMutexLock oLock( m_mutex );
                                    m_aItems.push_back( std::move( oItem ) );
                                }
                                m_semItems.Post();
                            }

            /// Waits for an item and takes it.
        T                   Pop( void )
                            {
                                m_semItems.Wait();
                                return Take();
                            }

            /// Waits at most nTimeoutMs for an item. Returns false if none came.
        bool                PopFor( T &oItem, int nTimeoutMs )
                            {
                                if ( ! m_semItems.TimedWait( nTimeoutMs ) )
                                    return false;
                                oItem = Take();
                                return true;
                            }
            /// Takes an item if one is waiting.
        bool                TryPop( T &oItem )
                            {
                                if ( ! m_semItems.TryWait() )
                                    return false;
                                oItem = Take();
                                return true;
                            }

    private:
        T                   Take( void )
                            {
                                IASLib::CMutexLock oLock( m_mutex );
                                T oItem( std::move( m_aItems.front() ) );
                                m_aItems.pop_front();
                                return oItem;
                            }

                            TWorkQueue( const TWorkQueue & );
        TWorkQueue         &operator =( const TWorkQueue & );
};

#endif // __WORKQUEUE_H__
