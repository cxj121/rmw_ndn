// Copyright 2016-2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef RMW_FASTRTPS_SHARED_CPP__CUSTOM_SUBSCRIBER_INFO_HPP_
#define RMW_FASTRTPS_SHARED_CPP__CUSTOM_SUBSCRIBER_INFO_HPP_

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <set>
#include <utility>

#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/subscriber/SubscriberListener.h"

#include "rcpputils/thread_safety_annotations.hpp"

#include "rmw/impl/cpp/macros.hpp"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"
#include "rmw_fastrtps_shared_cpp/custom_event_info.hpp"

#include "ndn-cxx/face.hpp"
#include "ndn-cxx/name.hpp"
#include <ndn-cxx/util/random.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/segment-fetcher.hpp>
#include <ndn-cxx/util/time.hpp>
#include "ndn-cxx/interest.hpp"

class SubListener;

class Subscriber;//add ndn subscribe api

struct CustomSubscriberInfo : public CustomEventInfo
{
  virtual ~CustomSubscriberInfo() = default;


  ndn::Name topic;//add topic to subscribe to publihser
  eprosima::fastrtps::Subscriber * subscriber_{nullptr};
  SubListener * listener_{nullptr};
  rmw_fastrtps_shared_cpp::TypeSupport * type_support_{nullptr};
  const void * type_support_impl_{nullptr};
  rmw_gid_t subscription_gid_{};
  const char * typesupport_identifier_{nullptr};

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  EventListenerInterface *
  getListener() const final;
};

class SubListener : public EventListenerInterface, public eprosima::fastrtps::SubscriberListener
{
public:
  explicit SubListener(CustomSubscriberInfo * info)
  : data_(0),
    deadline_changes_(false),
    liveliness_changes_(false),
    conditionMutex_(nullptr),
    conditionVariable_(nullptr)
  {
    // Field is not used right now
    (void)info;
  }

  // SubscriberListener implementation
  void
  onSubscriptionMatched(
    eprosima::fastrtps::Subscriber * sub, eprosima::fastrtps::rtps::MatchingInfo & info) final
  {
    {
      std::lock_guard<std::mutex> lock(internalMutex_);
      if (eprosima::fastrtps::rtps::MATCHED_MATCHING == info.status) {
        publishers_.insert(info.remoteEndpointGuid);
      } else if (eprosima::fastrtps::rtps::REMOVED_MATCHING == info.status) {
        publishers_.erase(info.remoteEndpointGuid);
      }
    }
    data_taken(sub);
  }

  void
  onNewDataMessage(eprosima::fastrtps::Subscriber * sub) final
  {
    data_taken(sub);
  }

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void
  on_requested_deadline_missed(
    eprosima::fastrtps::Subscriber *,
    const eprosima::fastrtps::RequestedDeadlineMissedStatus &) final;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void
  on_liveliness_changed(
    eprosima::fastrtps::Subscriber *,
    const eprosima::fastrtps::LivelinessChangedStatus &) final;

  // EventListenerInterface implementation
  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  bool
  hasEvent(rmw_event_type_t event_type) const final;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  bool
  takeNextEvent(rmw_event_type_t event_type, void * event_info) final;

  // SubListener API
  void
  attachCondition(std::mutex * conditionMutex, std::condition_variable * conditionVariable)
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = conditionMutex;
    conditionVariable_ = conditionVariable;
  }

  void
  detachCondition()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = nullptr;
    conditionVariable_ = nullptr;
  }

  bool
  hasData() const
  {
    return data_.load(std::memory_order_relaxed) > 0;
  }

  void
  data_taken(eprosima::fastrtps::Subscriber * sub)
  {
    // Make sure to call into Fast-RTPS before taking the lock to avoid an
    // ABBA deadlock between internalMutex_ and mutexes inside of Fast-RTPS.
#if FASTRTPS_VERSION_MAJOR == 1 && FASTRTPS_VERSION_MINOR < 9
    uint64_t unread_count = sub->getUnreadCount();
#else
    uint64_t unread_count = sub->get_unread_count();
#endif

    std::lock_guard<std::mutex> lock(internalMutex_);
    ConditionalScopedLock clock(conditionMutex_, conditionVariable_);
    data_.store(unread_count, std::memory_order_relaxed);
  }

  size_t publisherCount()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    return publishers_.size();
  }

private:
  mutable std::mutex internalMutex_;

  std::atomic_size_t data_;

  std::atomic_bool deadline_changes_;
  eprosima::fastrtps::RequestedDeadlineMissedStatus requested_deadline_missed_status_
    RCPPUTILS_TSA_GUARDED_BY(internalMutex_);

  std::atomic_bool liveliness_changes_;
  eprosima::fastrtps::LivelinessChangedStatus liveliness_changed_status_
    RCPPUTILS_TSA_GUARDED_BY(internalMutex_);

  std::mutex * conditionMutex_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
  std::condition_variable * conditionVariable_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);

  std::set<eprosima::fastrtps::rtps::GUID_t> publishers_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
};

class Subscriber
{
	public:
		Subscriber(ndn::Interest &interest) : m_interest(interest)
		{

		}
		void subscribe_to_topic(rmw_serialized_message_t* serialized_message,ndn::Interest &interest)
		{
			interest.setMustBeFresh(true);
			m_face.expressInterest(interest,
                           std::bind(&Subscriber::onData, this,  _1, _2,serialized_message),
                           std::bind(&Subscriber::onNack, this, _1, _2),
                           std::bind(&Subscriber::onTimeout, this, _1));
			m_face.processEvents();
		}
		void onData(const ndn::Interest&,const ndn::Data& data,rmw_serialized_message_t * serialized_message)
		{
	           //get data content and copy it to serialized_message->buffer
		   
		   const ndn::Block block=data.getContent();
		   const uint8_t * buffer=block.value();
		   size_t length=block.value_size();
		   if(length>serialized_message->buffer_capacity)
		     rmw_serialized_message_resize(serialized_message,length);
		   serialized_message->buffer_length=length;
		   memcpy(serialized_message->buffer,buffer,serialized_message->buffer_length);
		}
		void onNack(const ndn::Interest&,const ndn::lp::Nack &nack)
		{
			 std::cout << "received Nack with reason " << nack.getReason() << " for interest " << std::endl;
		}
		void onTimeout(const ndn::Interest&)
		{
			std::cout << "Timeout for " << std::endl;
		}
        private:
		ndn::Face m_face;
		ndn::Interest m_interest;
};
#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_SUBSCRIBER_INFO_HPP_
