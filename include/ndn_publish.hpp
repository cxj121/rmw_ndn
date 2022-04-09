#ifndef RMW_NDN__NDN_PUBLISH_HPP
#define RMW_NDN__NDN_PUBLISH_HPP


#include "common.hpp"
#include "rmw/rmw.h"

class Publish : noncopyable
{
   public:
	   Publish(ndn::Name& name,
	           ndn::Data& data,
		   ndn::Face& face,
		   const updatecallback& updatecallback);
	   virtual ~SVSPubSub() = default;
//publish a data packet
	   bool ndn_publish(const ndn::Data& data,const ndn:: Name& name);
   private:
	   const ndn::Name m_name;
	   const ndn::Data m_data;
	   const ndn::Face m_face;
	   const updatecallback m_updatecallback;

}


#endif
