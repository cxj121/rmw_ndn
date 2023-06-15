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

#include <iostream>
#include "rmw/serialized_message.h"

#include "fastcdr/Cdr.h"
#include "fastcdr/FastBuffer.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/impl/cpp/macros.hpp"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/custom_publisher_info.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"
#include "ndn-cxx/face.hpp"


namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_publish(
  const char * identifier,
  const rmw_publisher_t * publisher,
  const void * ros_message,
  rmw_publisher_allocation_t * allocation)
{
  (void) allocation;
  RMW_CHECK_FOR_NULL_WITH_MSG(
    publisher, "publisher handle is null",
    return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher, publisher->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_FOR_NULL_WITH_MSG(
    ros_message, "ros message handle is null",
    return RMW_RET_INVALID_ARGUMENT);
 std::cout <<"begin shared publish"<<std::endl;
  auto info = static_cast<CustomPublisherInfo *>(publisher->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "publisher info pointer is null", return RMW_RET_ERROR);
  //jia ru ndn shuju
  rmw_fastrtps_shared_cpp::SerializedData data;
  data.is_cdr_buffer = false;
  data.data = const_cast<void *>(ros_message);
  data.impl = info->type_support_impl_;
  if (!info->publisher_->write(&data)) {
    RMW_SET_ERROR_MSG("cannot publish data");
    return RMW_RET_ERROR;
  }
  return RMW_RET_OK;
}

rmw_ret_t
__rmw_publish_serialized_message(
  const char * identifier,
  const rmw_publisher_t * publisher,
  const rmw_serialized_message_t * serialized_message,
  rmw_publisher_allocation_t * allocation)
{
  (void) allocation;
  RMW_CHECK_FOR_NULL_WITH_MSG(
    publisher, "publisher handle is null",
    return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher, publisher->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_FOR_NULL_WITH_MSG(
    serialized_message, "serialized message handle is null",
    return RMW_RET_INVALID_ARGUMENT);
  std::cout <<"begin shared publish serialized"<<std::endl;

  auto info = static_cast<CustomPublisherInfo *>(publisher->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "publisher info pointer is null", return RMW_RET_ERROR);

 /* eprosima::fastcdr::FastBuffer buffer(
    reinterpret_cast<char *>(serialized_message->buffer), serialized_message->buffer_length);
  eprosima::fastcdr::Cdr ser(
    buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN, eprosima::fastcdr::Cdr::DDS_CDR); */
  
  ndn::Name topic(publisher->topic_name);
  auto data=std::make_shared<ndn::Data>(topic);
  std::cout << "secessfully create a data with topic"<<std::endl;
  auto block=ndn::encoding::makeBinaryBlock(ndn::tlv::Content,serialized_message->buffer,serialized_message->buffer_length);
  data->setContent(block);
  std::cout <<"secessfully set content"<<std::endl;
  data->setFreshnessPeriod(ndn::time::milliseconds(1000));
  std::cout<<"secessfully set freshnessperiod"<<std::endl;
  ndn::Face face;
  //sign
  ndn::KeyChain keychain;
  keychain.sign(*data);
  std::cout<<"secessfully sign data with default"<<std::endl;
  face.put(*data);
  std::cout<<"secessfully put data into face"<<std::endl;
  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
