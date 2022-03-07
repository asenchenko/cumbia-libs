#include "cutimer.h"
#include "cuevent.h"
#include "cumacros.h"
#include "cutimerlistener.h"
#include <limits.h>


/*! \brief create the timer and install the listener
 *
 * @param loos a pointer to a CuEventLoopService
 *
 * \note if loos is not null, listeners will be notified in the CuEventLoopService thread
 * of execution.
 *
 * CuThread is a CuTimerListener
 *
 * By default, the timeout is set to 1000 milliseconds, and the single shot
 * property is true.
 */
CuTimer::CuTimer(CuEventLoopService *loos) {
    d = new CuTimerPrivate();
}

/*! \brief class destructor
 *
 * If still running, CuTimer::stop is called to interrupt the timer
 * and join the thread
 */
CuTimer::~CuTimer()
{
    pdelete("CuTimer %p d->m_quit %d", this, d->m_quit);
    if(!d->m_quit)
        stop();
    delete d;
}

void CuTimer::setName(const std::string &name) {
    d->m_name = name;
}

std::string CuTimer::name() const {
    return d->m_name;
}

int CuTimer::id() const {
    return d->m_id;
}

/*!
 * \brief change the timeout
 * \param millis the new timeout
 */
void CuTimer::setTimeout(int millis)
{
    d->m_timeout = millis;
    d->m_wait.notify_one();
}

/*!
 * \brief return the timeout in milliseconds
 * \return the timeout in milliseconds
 */
int CuTimer::timeout() const
{
    return d->m_timeout;
}

/*!
 * \brief cancels current scheduled timeout.
 *
 * \note
 * start must be called after reset if you want to schedule
 * the next timeout
 *
 * \note
 * Not lock guarded
 */
void CuTimer::reset() {
    d->m_skip = true;
    d->m_wait.notify_one();
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
    std::unique_lock<std::mutex> lock(d->m_mutex);
    d->m_quit = d->m_pause = false;
    d->m_timeout = millis;
    if(!d->m_thread) { // first time start is called or after stop
        d->m_thread = new std::thread(&CuTimer::run, this);
    }
    else {
        if(d->m_pending > 0) {
            reset();
//            d->m_last_start_pt = std::chrono::steady_clock::now();
        }
        else {
//            d->m_first_start_pt = d->m_last_start_pt = std::chrono::steady_clock::now();
        }
        d->m_pending++;
        d->m_wait.notify_one();
    }
}

/*! \brief stops the timer, if active
 *
 * stops the timer, joins the timer thread and deletes it
 */
void CuTimer::stop()
{
    if(d->m_exited)
        return; /* already quit */
    {
        std::unique_lock<std::mutex> lock(d->m_mutex);
        d->m_quit = true;
        d->m_lis_map.clear();
        d->m_wait.notify_one();
    }
    if(d->m_thread->joinable()) {
        d->m_thread->join();
    }
    d->m_exited = true;
    delete d->m_thread;
    d->m_thread = nullptr;
}

void CuTimer::m_notify() {
    for(std::map<CuTimerListener *, CuEventLoopService *>::const_iterator it = d->m_lis_map.begin(); it != d->m_lis_map.end(); ++it) {
        it->first->onTimeout(this);
    }
}

/*! \brief implements CuEventLoopListener interface.
 *
 */
void CuTimer::onEvent(CuEventI *e) {
    m_notify();
}

/*!
 * \brief CuTimer::addListener adds a listener to the timer
 * \param l the new listener
 *
 * This method can be accessed from several different threads
 */
void CuTimer::addListener(CuTimerListener *l, CuEventLoopService *ls) {
    std::unique_lock<std::mutex> lock(d->m_mutex);
    d->m_lis_map[l] = ls;
    if(ls)
        ls->addCuEventLoopListener(this); // inserts into set
}

/*!
 * \brief CuTimer::removeListener removes a listener from the timer
 * \param l the listener to remove
 *
 * This method can be accessed from several different threads
 */
void CuTimer::removeListener(CuTimerListener *l) {
    std::unique_lock<std::mutex> lock(d->m_mutex);
    d->m_lis_map.erase(l);
}

/*!
 * \brief CuTimer::listeners returns the list of registered listeners
 *
 * @return the list of registered listeners
 *
 * This method can be accessed from several different threads
 */
std::map<CuTimerListener *, CuEventLoopService *> CuTimer::listenersMap()  {
    std::unique_lock<std::mutex> lock(d->m_mutex);
    return d->m_lis_map;
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
    std::unique_lock<std::mutex> lock(d->m_mutex);
    unsigned long timeout = d->m_timeout;
    while (!d->m_quit) {
        timeout = d->m_timeout;
        std::cv_status status;
        std::chrono::milliseconds ms{timeout};
        status = d->m_wait.wait_for(lock, ms);
        if(d->m_skip && !d->m_quit) {
            d->m_skip = false;
        }
        else {
            d->m_pause ?  timeout = ULONG_MAX : timeout = d->m_timeout;
            // issues with wasm: erratically expected timeout status is no_timeout
            (void ) status;
            if(/*status == std::cv_status::timeout && */!d->m_quit && !d->m_pause) {
                for(std::map<CuTimerListener *, CuEventLoopService *>::const_iterator it = d->m_lis_map.begin(); it != d->m_lis_map.end(); ++it) {
                    it->second != nullptr ?
                                it->second->postEvent(this, new CuTimerEvent())
                              : it->first->onTimeout(this);
                }
            }
            d->m_pending = 0;
            if(!d->m_quit) { // wait for next start()
                d->m_wait.wait(lock);
            }
        } // !d->m_skip
    }
}

