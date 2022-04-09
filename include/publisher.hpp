#ifndef RMW_NDN__PUBLISHER_HPP
#define RMW_NDN__PUBLISHER_HPP


#include "rmw/rmw.h"

rmw_publisher_t *
create_publisher(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_publisher_options_t * publisher_options);
