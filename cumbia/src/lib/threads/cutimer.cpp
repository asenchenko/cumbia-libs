#include "cutimer.h"
#include "cumacros.h"
#include "cutimerlistener.h"
#include <limits.h>

/*! \brief create the timer and install the listener
 *
 * @param l a CuTimerListener
 *
 * CuThread is a CuTimerListener
 *
 * By default, the timeout is set to 1000 milliseconds, and the single shot
 * property is true.
 */
CuTimer::CuTimer(CuTimerListener *l)
{
    m_listener = l;
    m_quit = false;
    m_pause = false;
    m_exited = false;
    m_timeout = 1000;
    m_singleShot = true;
    m_thread = NULL;
}

/*! \brief class destructor
 *
 * If still running, CuTimer::stop is called to interrupt the timer
 * and join the thread
 */
CuTimer::~CuTimer()
{
    pdelete("CuTimer %p", this);
    if(!m_quit)
        stop();
}

/*!
 * \brief change the timeout
 * \param millis the new timeout
 */
void CuTimer::setTimeout(int millis)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    pbblue("CuTimer.setTimeout %d", millis);
    m_timeout = millis;
    m_terminate.notify_one();
}

/*!
 * \brief enable or disable the single shot mode
 * @param single true the timer is run once
 * @param single false the timer is run continuously
 */
void CuTimer::setSingleShot(bool single)
{
    m_singleShot = single;
}

/*!
 * \brief return the timeout in milliseconds
 * \return the timeout in milliseconds
 */
int CuTimer::timeout() const
{
    return m_timeout;
}

/*!
 * \brief returns true if the single shot mode is enabled.
 * @return true: the timer is running once
 * @return false: the timer is run repeatedly
 */
bool CuTimer::isSingleShot() const
{
    return m_singleShot;
}

/*!
 * \brief pause the timer is paused
 *
 * A timeout of ULONG_MAX (limits.h) is set on the timer
 */
void CuTimer::pause()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    pbblue("CuTimer.pause");
    m_pause = true;
    m_terminate.notify_one();
}

/*! \brief the timer is resumed if paused, started if not running
 *
 * The timer is resumed if the timer thread is still running,
 * started otherwise
 */
void CuTimer::resume()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    pbblue("CuTimer.resume");
    m_pause = false;
    if(!m_quit) /* thread still running */
        m_terminate.notify_one();
    else  /* thread loop is over */
        start(m_timeout);
}

/*! \brief start the timer with the given interval in milliseconds
 *
 * @param millis the desired timeout
 *
 * If the timer is still running, CuTimer waits for it to finish before starting
 * another one
 */
void CuTimer::start(int millis)
{
    m_quit = m_pause = false;
    if(m_thread)
        m_thread->join();
    if(m_thread)
        delete m_thread;
    m_timeout = millis;
    m_thread = new std::thread(&CuTimer::run, this);
}

/*! \brief stops the timer, if active
 *
 * stops the timer, joins the timer thread and deletes it
 */
void CuTimer::stop()
{
    if(m_exited)
        return; /* already quit */
    pbblue("CuTimer.stop!!!! for this %p before lock", this);
    {
        auto locked = std::unique_lock<std::mutex>(m_mutex);
        m_quit = true;
        m_listener = NULL;
    }
    m_terminate.notify_one();
    if(m_thread->joinable())
    {
        m_thread->join();
        pbblue("CuTimer.stop: JOINETH!");
    }
    else
        pbblue("CuTimer.stop: NOT JOINABLE!!!");
    m_exited = true;
    delete m_thread;
    m_thread = NULL;
}

/*! \brief the timer loop
 *
 * \note internally used by the library
 *
 * The timer loop waits for the timeout to expire before quitting (if single shot)
 * or waiting again
 */
void CuTimer::run()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    unsigned long timeout = m_timeout;
    while (!m_quit)
    {
        std::chrono::milliseconds ms{timeout};
        std::cv_status status = m_terminate.wait_for(lock, ms);
//        cuprintf("CuTimer.run pause is %d status is %d timeout %d\n", m_pause, (int) status, m_timeout);
        //        if(status == std::cv_status::no_timeout)
        m_pause ?  timeout = ULONG_MAX : timeout = m_timeout;
        if(status == std::cv_status::timeout && m_listener)
        {
//            pbblue("CuTimer:run: this: %p triggering timeout in pthread 0x%lx (CuTimer's) m_listener %p m_exit %d CURRENT TIMEOUT is %lu m_pause %d", this,
//                   pthread_self(), m_listener, m_quit, timeout, m_pause);
            if(m_listener && !m_quit && !m_pause) /* if m_exit: m_listener must be NULL */
            {
                m_listener->onTimeout(this);
            }
        }
        /* check status: don't exit for a timeout change! Don't exit if we are in pause either */
        if(m_singleShot && (status == std::cv_status::timeout) & !m_pause)
            m_quit = true;
    }
//    pbblue("CuTimer:run: exiting!");
}

