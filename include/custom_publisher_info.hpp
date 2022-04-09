#ifndef RMW_NDN__CUSTOM_PUBLISHER_INFO_HPP_
#define RMW_NDN__CUSTOM_PUBLISHER_INFO_HPP_

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <set>


#include "rmw/rmw.h"


class PubListener;

typedef struct CustomPublisherInfo
{
  virtual ~CustomPublisherInfo() = default;

  ndn::Name topic;
  PubListener * listener_{nullptr};
  rmw_gid_t publisher_gid{};
  const char * typesupport_identifier_{nullptr};

} CustomPublisherInfo;

#endif
