#include "rmw_ndn/ndn_publish.hpp"
#include "rmw_ndn/common.hpp"

 Publish::Publish(ndn::Name& name,
                   ndn::Data& data,
                   ndn::Face& face,
                   const updatecallback& updatecallback)
	:m_name(name),
	m_data(data),
	m_face(face),
	m_updatecallback(updatecallback){}

bool ndn_publish(const ndn::Data& data,const ndn::Name& name)
{

}
