//yinru ndn-cxx


#ifndef RMW_NDN__COMMON_HPP
#define RMW_NDN__COMMON_HPP

#include "ndn-cxx/face.hpp"
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/security/validator.hpp>
#include <ndn-cxx/face.hpp>
#include <iostream>


namespace ndn{
	using gid = ndn::Name;
	using seqNo = unit64_t;

	using ndn::security::ValidationError;

        using DataValidatedCallback = function<void(const Data&)>;
        using DataValidationErrorCallback = function<void(const Data&, const ValidationError& error)> ;
}
