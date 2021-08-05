/*
Copyright 2021 Tomas Prerovsky (cepsdev@hotmail.com).

Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef INC_CEPSWEBSOCKET_HPP
#define INC_CEPSWEBSOCKET_HPP

#include <string>
#include <vector>
#include <tuple>

namespace ceps{
    namespace websocket{
        struct websocket_frame{
            std::vector<unsigned char> payload;
            bool fin = false;
            bool rsv1 = false;
            bool rsv2 = false;
            bool rsv3 = false;
            std::uint8_t opcode = 0;
        };
        std::pair<bool,websocket_frame> read_websocket_frame(int sck);
    }
    namespace http{
        using http_header_t = std::vector<std::pair<std::string,std::string>>;
        std::pair<bool,std::string> get_http_attribute_content(std::string attr, http_header_t const & http_header);
        bool field_with_content(std::string attr, std::string value,http_header_t const & http_header);
        std::tuple<bool,std::string,http_header_t> read_http_request(int sck,std::string& unconsumed_data);
    }
    namespace net{
        std::string encode_base64(void * mem, size_t len);
        std::string encode_base64(std::string s);
    }
    namespace crypto{
        std::string sha1(std::string s);
    }
}

#endif