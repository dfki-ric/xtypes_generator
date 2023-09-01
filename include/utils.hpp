#pragma once

#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <exception>

#include "CRC.h"

namespace nl = nlohmann;

namespace xtypes {
  /// Parses JSON and provides some error messages on failure
  static nl::json parseJson(const std::string& string, std::string info ="") {//, const std::source_location location = std::source_location::current()) { // C++20
    try {
      return nl::json::parse(string);
    } catch (...) {
      std::exception_ptr p = std::current_exception();
      std::cerr<<"Couldn't load the following string as json:"<<std::endl
               <<string<<std::endl;
      if (!info.empty())
        std::cerr<<"The following info was provided: "<<info<<std::endl;
      // std::cerr<<"Location: "<<location.file<<":"<<location.line<<std::endl; // C++20
      std::rethrow_exception(p);
    }
  }

  /// Converts the URI to a UUID
  static std::size_t uri_to_uuid(const std::string& uri)
  {
    // NOTE: We use CRC32 here to have a platform INDEPENDENT, FAST, LOW-COLLISION hashing value
    // If this is not enough, the CRC.h file also supports ESOTERIC CRC64
    // For a nice comparison of some hash functions, see
    // https://softwareengineering.stackexchange.com/questions/49550/which-hashing-algorithm-is-best-for-uniqueness-and-speed
    return CRC::Calculate(uri.c_str(), uri.length(), CRC::CRC_32());
  }
}
