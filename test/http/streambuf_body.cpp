//
// Copyright (c) 2013-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Test that header file is self-contained.
#include <beast/http/streambuf_body.hpp>

#include <beast/core/to_string.hpp>
#include <beast/http/fields.hpp>
#include <beast/http/message_parser.hpp>
#include <beast/http/read.hpp>
#include <beast/http/write.hpp>
#include <beast/test/string_istream.hpp>
#include <beast/unit_test/suite.hpp>
#include <boost/lexical_cast.hpp>

namespace beast {
namespace http {

class streambuf_body_test : public beast::unit_test::suite
{
    boost::asio::io_service ios_;

public:
    void run() override
    {
        std::string const s =
            "HTTP/1.1 200 OK\r\n"
            "Server: test\r\n"
            "Content-Length: 3\r\n"
            "\r\n"
            "xyz";
        test::string_istream ss(ios_, s);
        message_parser<false, streambuf_body, fields> p;
        streambuf sb;
        read(ss, sb, p);
        auto const& m = p.get();
        BEAST_EXPECT(to_string(m.body.data()) == "xyz");
        BEAST_EXPECT(boost::lexical_cast<std::string>(m) == s);
    }
};

BEAST_DEFINE_TESTSUITE(streambuf_body,http,beast);

} // http
} // beast
