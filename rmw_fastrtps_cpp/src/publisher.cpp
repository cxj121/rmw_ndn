// Copyright 2019 Open Source Robotics Foundation, Inc.
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

#include <string>
#include <iostream>
#include "fastrtps/Domain.h"
#include "fastrtps/participant/Participant.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/validate_full_topic_name.h"

#include "rcpputils/scope_exit.hpp"

#include "rmw_fastrtps_shared_cpp/create_rmw_gid.hpp"
#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_publisher_info.hpp"
#include "rmw_fastrtps_shared_cpp/names.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"
#include "rmw_fastrtps_cpp/publisher.hpp"

#include "type_support_common.hpp"

using Domain = eprosima::fastrtps::Domain;
using Participant = eprosima::fastrtps::Participant;
using TopicDataType = eprosima::fastrtps::TopicDataType;

rmw_publisher_t *
rmw_fastrtps_cpp::create_publisher(
  const CustomParticipantInfo * participant_info,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_publisher_options_t * publisher_options,
  bool keyed,
  bool create_publisher_listener)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(participant_info, nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(type_supports, nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(topic_name, nullptr);
  if (0 == strlen(topic_name)) {
    RMW_SET_ERROR_MSG("topic_name argument is an empty string");
    return nullptr;
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(qos_policies, nullptr);
  if (!qos_policies->avoid_ros_namespace_conventions) {
    int validation_result = RMW_TOPIC_VALID;
    rmw_ret_t ret = rmw_validate_full_topic_name(topic_name, &validation_result, nullptr);
    if (RMW_RET_OK != ret) {
      return nullptr;
    }
    if (RMW_TOPIC_VALID != validation_result) {
      const char * reason = rmw_full_topic_name_validation_result_string(validation_result);
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("invalid topic name: %s", reason);
      return nullptr;
    }
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher_options, nullptr);
  std::cout<<"begin create fastrtps publisher"<<std::endl;
  Participant * participant = participant_info->participant;
  RMW_CHECK_ARGUMENT_FOR_NULL(participant, nullptr);

  const rosidl_message_type_support_t * type_support = get_message_typesupport_handle(
    type_supports, RMW_FASTRTPS_CPP_TYPESUPPORT_C);
  if (!type_support) {
    type_support = get_message_typesupport_handle(
      type_supports, RMW_FASTRTPS_CPP_TYPESUPPORT_CPP);
    if (!type_support) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return nullptr;
    }
  }

  CustomPublisherInfo * info = nullptr;
  rmw_publisher_t * rmw_publisher = nullptr;
  eprosima::fastrtps::PublisherAttributes publisherParam;
  if (!is_valid_qos(*qos_policies)) {
    return nullptr;
  }

  // Load default XML profile.
  Domain::getDefaultPublisherAttributes(publisherParam);

  info = new (std::nothrow) CustomPublisherInfo();
  if (!info) {
    RMW_SET_ERROR_MSG("failed to allocate CustomPublisherInfo");
    return nullptr;
  }
  auto cleanup_info = rcpputils::make_scope_exit(
    [info, participant]() {
      if (info->type_support_) {
        _unregister_type(participant, info->type_support_);
      }
      delete info->listener_;
      delete info;
    });
  info->typesupport_identifier_ = type_support->typesupport_identifier;
  info->type_support_impl_ = type_support->data;
  auto callbacks = static_cast<const message_type_support_callbacks_t *>(type_support->data);
  std::cout<<"add callbacks to message type support"<<std::endl;
  std::string type_name = _create_type_name(callbacks);
  if (
    !Domain::getRegisteredType(
      participant, type_name.c_str(),
      reinterpret_cast<TopicDataType **>(&info->type_support_)))
  {
    info->type_support_ = new (std::nothrow) MessageTypeSupport_cpp(callbacks);
    if (!info->type_support_) {
      RMW_SET_ERROR_MSG("failed to allocate MessageTypeSupport");
      return nullptr;
    }
    _register_type(participant, info->type_support_);
  }

  if (!participant_info->leave_middleware_default_qos) {
    publisherParam.qos.m_publishMode.kind = eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE;
    publisherParam.historyMemoryPolicy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  }
  publisherParam.topic.topicKind =
    keyed ? eprosima::fastrtps::rtps::WITH_KEY : eprosima::fastrtps::rtps::NO_KEY;
  publisherParam.topic.topicDataType = type_name;
  publisherParam.topic.topicName = _create_topic_name(qos_policies, ros_topic_prefix, topic_name);
  std::cout<<"begin get datawriter qos"<<std::endl;
  if (!get_datawriter_qos(*qos_policies, publisherParam)) {
    return nullptr;
  }

  info->listener_ = nullptr;
  if (create_publisher_listener) {
    info->listener_ = new (std::nothrow) PubListener(info);
    if (!info->listener_) {
      RMW_SET_ERROR_MSG("create_publisher() could not create publisher listener");
      return nullptr;
    }
  }
    info->publisher_ = Domain::createPublisher(
    participant,
    publisherParam,
    info->listener_);
    
   if (!info->publisher_) {
    RMW_SET_ERROR_MSG("create_publisher() could not create publisher");
    return nullptr;
  }

  info->publisher_gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
    eprosima_fastrtps_identifier, info->publisher_->getGuid());
  
  rmw_publisher = rmw_publisher_allocate();
  if (!rmw_publisher) {
    RMW_SET_ERROR_MSG("failed to allocate publisher");
    return nullptr;
  }
  auto cleanup_publisher = rcpputils::make_scope_exit(
    [rmw_publisher]() {
      rmw_free(const_cast<char *>(rmw_publisher->topic_name));
      rmw_publisher_free(rmw_publisher);
    });
  rmw_publisher->implementation_identifier = eprosima_fastrtps_identifier;
  rmw_publisher->data = info;
  
  rmw_publisher->topic_name = static_cast<char *>(rmw_allocate(strlen(topic_name) + 1));
 
  if (!rmw_publisher->topic_name) {
    RMW_SET_ERROR_MSG("failed to allocate memory for publisher topic name");
    return nullptr;
  }
  memcpy(const_cast<char *>(rmw_publisher->topic_name), topic_name, strlen(topic_name) + 1);

  rmw_publisher->options = *publisher_options;
  printf("rmw publisher is areadly\n");

  cleanup_publisher.cancel();
  cleanup_info.cancel();
  printf("cleanup  publisher seccess\n");
  return rmw_publisher;
}
