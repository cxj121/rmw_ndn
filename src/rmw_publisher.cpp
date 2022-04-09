#include <string>


#include "rmw/rmw.h"
#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"

#include "common.hpp"

//fini a allocation 

rmw_ret_t
rmw_init_publisher_allocation(
  const rosidl_message_type_support_t * type_support,
  const rosidl_runtime_c__Sequence__bound * message_bounds,
  rmw_publisher_allocation_t * allocation){

}

//create a publisher
rmw_publisher_t *
rmw_create_publisher(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_publisher_options_t * publisher_options)
{
	//check paramaters is valid
        RMW_CHECK_ARGUMENT_FOR_NULL(node, nullptr);
	//identifier match ndn
	RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    ndn_identifier,
    return nullptr);
        RMW_CHECK_ARGUMENT_FOR_NULL(type_supports, nullptr);
	RMW_CHECK_ARGUMENT_FOR_NULL(topic_name, nullptr);
	if (0 == strlen(topic_name)) {
    RMW_SET_ERROR_MSG("create_publisher() called with an empty topic_name argument");
    return nullptr;
  }
	RMW_CHECK_ARGUMENT_FOR_NULL(qos_policies, nullptr);

// get the rmw type handle
 
     const rosidl_message_type_support_t * type_support = get_message_typesupport_handle(
    type_supports, RMW_NDN_TYPESUPPORT_C);//xin jian ndn_support_c 
    

//create a topic,the topic name means ndn data packet name prefix
    ndn::Name topic(topic_name);
//create a costumpublisherinfo
    auto info=new (std::nothrow) CustomPublisherInfo();
    if (!info) {
    RMW_SET_ERROR_MSG("create_publisher() failed to allocate CustomPublisherInfo");
    return nullptr;
  }
    info->topic=topic;
    info->typesupport_identifier_=ndn_identifier;
    info->publisher_gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
    ndn_identifier, info->topic);//xiugai


// Allocate publisher
  rmw_publisher_t * rmw_publisher = rmw_publisher_allocate();
  if (!rmw_publisher) {
    RMW_SET_ERROR_MSG("create_publisher() failed to allocate rmw_publisher");
    return nullptr;
  }
   auto cleanup_rmw_publisher = rcpputils::make_scope_exit(
    [rmw_publisher]() {
      rmw_free(const_cast<char *>(rmw_publisher->topic_name));
      rmw_publisher_free(rmw_publisher);
    });
  //init rmw_publisher
   rmw_publisher-> rmw_publisher->implementation_identifier = ndn_identifier;
   rmw_publisher->can_loan_messages = TRUE;
   rmw_publisher->data=info;
   rmw_publisher->topic_name = static_cast<char *>(rmw_allocate(strlen(topic_name) + 1));

   if (!rmw_publisher->topic_name) {
    RMW_SET_ERROR_MSG("create_publisher() failed to allocate memory for rmw_publisher topic name");
    return nullptr;
  }
  memcpy(const_cast<char *>(rmw_publisher->topic_name), topic_name, strlen(topic_name) + 1);

  topic.should_be_deleted = false;
  cleanup_rmw_publisher.cancel();
  cleanup_info.cancel();

  TRACEPOINT(
    rmw_publisher_init,
    static_cast<const void *>(rmw_publisher),
    info->publisher_gid.data);

  //finally
  return rmw_publisher;
}

rmw_ret_t
rmw_publisher_count_matched_subscriptions(
  const rmw_publisher_t * publisher,
  size_t * subscription_count)
{
}


rmw_ret_t
rmw_destroy_publisher(rmw_node_t * node, rmw_publisher_t * publisher)
{
}
