/**
 * @file WaitSet.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @author Jose Luis Hidalgo <joseluis.hidalgo@gmail.com> - Mac OS X port
 * @brief Wait set.
 */

#pragma once

#include <vector>
#include <sstream>
#include <cerrno>
#include <cstdint>

#include <utki/config.hpp>
#include <utki/debug.hpp>
#include <utki/Exc.hpp>
#include <utki/Buf.hpp>


#if M_OS == M_OS_WINDOWS
#	include <utki/windows.hpp>

#elif M_OS == M_OS_LINUX
#	include <sys/epoll.h>
#	include <unistd.h>

#elif M_OS == M_OS_MACOSX
#	include <sys/types.h>
#	include <sys/event.h>
#	include <unistd.h>

#else
#	error "Unsupported OS"
#endif


//disable warning about throw specification is ignored.
#if M_COMPILER == M_COMPILER_MSVC
#	pragma warning(push) //push warnings state
#	pragma warning( disable : 4290)
#endif


#include "Waitable.hpp"


namespace wt{

/**
 * @brief Set of Waitable objects to wait for.
 */
class WaitSet{
	const unsigned size_var;
	unsigned numWaitables_var = 0;//number of Waitables added

#if M_OS == M_OS_WINDOWS
	std::vector<Waitable*> waitables;
	std::vector<HANDLE> handles; //used to pass array of HANDLEs to WaitForMultipleObjectsEx()

#elif M_OS == M_OS_LINUX
	int epollSet;

	std::vector<epoll_event> revents;//used for getting the result from epoll_wait()
#elif M_OS == M_OS_MACOSX
	int queue; // kqueue
	
	std::vector<struct kevent> revents;//used for getting the result
#else
#	error "Unsupported OS"
#endif

public:

	/**
	 * @brief WaitSet related exception class.
	 */
	class Exc : public utki::Exc{
	public:
		Exc(const std::string& message = std::string()) :
				utki::Exc(message)
		{}
	};
	
	/**
	 * @brief Constructor.
	 * @param maxSize - maximum number of Waitable objects can be added to this wait set.
	 */
	WaitSet(unsigned maxSize) :
			size_var(maxSize)
#if M_OS == M_OS_WINDOWS
			,waitables(maxSize)
			,handles(maxSize)
	{
		ASSERT_INFO(maxSize <= MAXIMUM_WAIT_OBJECTS, "maxSize should be less than " << MAXIMUM_WAIT_OBJECTS)
		if(maxSize > MAXIMUM_WAIT_OBJECTS){
			throw Exc("WaitSet::WaitSet(): requested WaitSet size is too big");
		}
	}

#elif M_OS == M_OS_LINUX
			,revents(maxSize)
	{
		ASSERT(int(maxSize) > 0)
		this->epollSet = epoll_create(int(maxSize));
		if(this->epollSet < 0){
			throw Exc("WaitSet::WaitSet(): epoll_create() failed");
		}
	}
#elif M_OS == M_OS_MACOSX
			,revents(maxSize * 2)
	{
		this->queue = kqueue();
		if(this->queue == -1){
			throw Exc("WaitSet::WaitSet(): kqueue creation failed");
		}
	}
#else
#	error "Unsupported OS"
#endif



	/**
	 * @brief Destructor.
	 * Note, that destructor will check if the wait set is empty. If it is not, then an assert
	 * will be triggered.
	 * It is user's responsibility to remove any waitable objects from the waitset
	 * before the wait set object is destroyed.
	 */
	~WaitSet()noexcept{
		//assert the wait set is empty
		ASSERT_INFO(this->numWaitables_var == 0, "attempt to destroy WaitSet containig Waitables")
#if M_OS == M_OS_WINDOWS
		//do nothing
#elif M_OS == M_OS_LINUX
		close(this->epollSet);
#elif M_OS == M_OS_MACOSX
		close(this->queue);
#else
#	error "Unsupported OS"
#endif
	}



	/**
	 * @brief Get maximum size of the wait set.
	 * @return maximum number of Waitables this WaitSet can hold.
	 */
	unsigned size()const noexcept{
		return this->size_var;
	}

	/**
	 * @brief Get number of Waitables already added to this WaitSet.
	 * @return number of Waitables added to this WaitSet.
	 */
	unsigned numWaitables()const noexcept{
		return this->numWaitables_var;
	}


	/**
	 * @brief Add Waitable object to the wait set.
	 * @param w - Waitable object to add to the WaitSet.
	 * @param flagsToWaitFor - determine events waiting for which we are interested.
	 * @throw ting::WaitSet::Exc - in case the wait set is full or other error occurs.
	 */
	void add(Waitable& w, Waitable::EReadinessFlags flagsToWaitFor);



	/**
	 * @brief Change wait flags for a given Waitable.
	 * Changes wait flags for a given waitable, which is in this WaitSet.
	 * @param w - Waitable for which the changing of wait flags is needed.
	 * @param flagsToWaitFor - new wait flags to be set for the given Waitable.
	 * @throw ting::WaitSet::Exc - in case the given Waitable object is not added to this wait set or
	 *                    other error occurs.
	 */
	void change(Waitable& w, Waitable::EReadinessFlags flagsToWaitFor);



	/**
	 * @brief Remove Waitable from wait set.
	 * @param w - Waitable object to be removed from the WaitSet.
	 * @throw ting::WaitSet::Exc - in case the given Waitable is not added to this wait set or
	 *                    other error occurs.
	 */
	void remove(Waitable& w)noexcept;



	/**
	 * @brief wait for event.
	 * This function blocks calling thread execution until one of the Waitable objects in the WaitSet
	 * triggers. Upon return from the function, pointers to triggered objects are placed in the
	 * 'out_events' buffer and the return value from the function indicates number of these objects
	 * which have triggered.
	 * Note, that it does not change the readiness state of non-triggered objects.
	 * @param out_events - pointer to buffer where to put pointers to triggered Waitable objects.
	 *                     The buffer will not be initialized to 0's by this function.
	 *                     The buffer shall be large enough to hold maxmimum number of Waitables
	 *                     this WaitSet can hold.
	 *                     It is valid to pass 0 pointer, in that case this argument will not be used.
	 * @return number of objects triggered.
	 *         NOTE: for some reason, on Windows it can return 0 objects triggered.
	 * @throw ting::WaitSet::Exc - in case of errors.
	 */
	unsigned wait(utki::Buf<Waitable*> out_events){
		return this->wait(true, 0, &out_events);
	}
	
	/**
	 * @brief wait for event.
	 * Same as Wait(const Buffer<Waitable*>& out_events) but does not return out_events.
     * @return number of objects triggered.
     */
	unsigned wait(){
		return this->wait(true, 0, 0);
	}


	/**
	 * @brief wait for event with timeout.
	 * The same as Wait() function, but takes wait timeout as parameter. Thus,
	 * this function will wait for any event or timeout. Note, that it guarantees that
	 * it will wait AT LEAST for specified number of milliseconds, or more. This is because of
	 * implementation for linux, if wait is interrupted by signal it will start waiting again,
	 * and so on.
	 * @param timeout - maximum time in milliseconds to wait for event.
	 * @param out_events - buffer where to put pointers to triggered Waitable objects.
	 *                     The buffer size must be equal or greater than the number of waitables
	 *                     currently added to the wait set.
	 * @return number of objects triggered. If 0 then timeout was hit.
	 *         NOTE: for some reason, on Windows it can return 0 before timeout was hit.
	 * @throw ting::WaitSet::Exc - in case of errors.
	 */
	unsigned waitWithTimeout(std::uint32_t timeout, utki::Buf<Waitable*> out_events){
		return this->wait(false, timeout, &out_events);
	}
	
	/**
	 * @brief wait for event with timeout.
	 * Same as WaitWithTimeout(std::uint32_t timeout, const Buffer<Waitable*>& out_events) but
	 * does not return out_events.
     * @param timeout - maximum time in milliseconds to wait for event.
     * @return number of objects triggered. If 0 then timeout was hit.
	 *         NOTE: for some reason, on Windows it can return 0 before timeout was hit.
     */
	unsigned waitWithTimeout(std::uint32_t timeout){
		return this->wait(false, timeout, 0);
	}



private:
	unsigned wait(bool waitInfinitly, std::uint32_t timeout, utki::Buf<Waitable*>* out_events);
	
	
#if M_OS == M_OS_MACOSX
	void AddFilter(Waitable& w, int16_t filter);
	void RemoveFilter(Waitable& w, int16_t filter);
#endif

};



}


//restore warnings state
#if M_COMPILER == M_COMPILER_MSVC
#	pragma warning(pop) //pop warnings state
#endif
