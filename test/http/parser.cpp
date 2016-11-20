//
// Copyright (c) 2013-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Test that header file is self-contained.
#include <beast/http/parser.hpp>

#include "fail_parser.hpp"

#include <beast/unit_test/suite.hpp>
#include <beast/test/string_istream.hpp>
#include <beast/test/string_ostream.hpp>
#include <beast/test/yield_to.hpp>
#include <beast/core/flat_streambuf.hpp>
#include <beast/core/streambuf.hpp>
#include <beast/http/read.hpp>
#include <beast/http/string_body.hpp>

namespace beast {
namespace http {

struct str_body
{
    using value_type = std::string;

    class reader
    {
        std::size_t len_ = 0;
        value_type& body_;

    public:
        using mutable_buffers_type =
            boost::asio::mutable_buffers_1;

        template<bool isRequest, class Fields>
        explicit
        reader(message<isRequest, str_body, Fields>& msg)
            : body_(msg.body)
        {
        }

        void
        init(boost::optional<
            std::uint64_t> const& content_length,
                error_code& ec)
        {
            if(content_length)
            {
                if(*content_length >
                        (std::numeric_limits<std::size_t>::max)())
                    throw std::domain_error{"Content-Length overflow"};
                body_.reserve(*content_length);
            }
        }

        mutable_buffers_type
        prepare(std::size_t n, error_code& ec)
        {
            body_.resize(len_ + n);
            return {&body_[len_], n};
        }

        void
        commit(std::size_t n, error_code& ec)
        {
            if(body_.size() > len_ + n)
                body_.resize(len_ + n);
            len_ = body_.size();
        }

        void
        finish(error_code& ec)
        {
            body_.resize(len_);
        }
    };
};

//------------------------------------------------------------------------------

class parser_test
    : public beast::unit_test::suite
    , public beast::test::enable_yield_to
{
public:
    template<bool isRequest, class Pred>
    void
    testMatrix(std::string const& s, Pred const& pred)
    {
        beast::test::string_istream ss{get_io_service(), s};
        error_code ec;
    #if 0
        streambuf dynabuf;
    #else
        flat_streambuf dynabuf;
        dynabuf.reserve(1024);
    #endif
        message<isRequest, string_body, fields> m;
        read(ss, dynabuf, m, ec);
        if(! BEAST_EXPECTS(! ec, ec.message()))
            return;
        pred(m);
    }

    void
    testRead()
    {
        testMatrix<false>(
            "HTTP/1.0 200 OK\r\n"
            "Server: test\r\n"
            "\r\n"
            "*******",
            [&](message<false, string_body, fields> const& m)
            {
                BEAST_EXPECTS(m.body == "*******",
                    "body='" + m.body + "'");
            }
        );
        testMatrix<false>(
            "HTTP/1.0 200 OK\r\n"
            "Server: test\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "5\r\n"
            "*****\r\n"
            "2;a;b=1;c=\"2\"\r\n"
            "--\r\n"
            "0;d;e=3;f=\"4\"\r\n"
            "Expires: never\r\n"
            "MD5-Fingerprint: -\r\n"
            "\r\n",
            [&](message<false, string_body, fields> const& m)
            {
                BEAST_EXPECT(m.body == "*****--");
            }
        );
        testMatrix<false>(
            "HTTP/1.0 200 OK\r\n"
            "Server: test\r\n"
            "Content-Length: 5\r\n"
            "\r\n"
            "*****",
            [&](message<false, string_body, fields> const& m)
            {
                BEAST_EXPECT(m.body == "*****");
            }
        );
        testMatrix<true>(
            "GET / HTTP/1.1\r\n"
            "User-Agent: test\r\n"
            "\r\n",
            [&](message<true, string_body, fields> const& m)
            {
            }
        );
        testMatrix<true>(
            "GET / HTTP/1.1\r\n"
            "User-Agent: test\r\n"
            "X: \t x \t \r\n"
            "\r\n",
            [&](message<true, string_body, fields> const& m)
            {
                BEAST_EXPECT(m.fields["X"] == "x");
            }
        );
    }

    void
    testParse()
    {
        using boost::asio::buffer;
        {
            error_code ec;
            message<true, string_body, fields> m;
            parser<true> p{m};
            std::string const s =
                "GET / HTTP/1.1\r\n"
                "User-Agent: test\r\n"
                "Content-Length: 1\r\n"
                "\r\n"
                "*";
            p.write(buffer(s), ec);
            BEAST_EXPECTS(! ec, ec.message());
            BEAST_EXPECT(p.is_done());
            BEAST_EXPECT(m.method == "GET");
            BEAST_EXPECT(m.url == "/");
            BEAST_EXPECT(m.version == 11);
            BEAST_EXPECT(m.fields["User-Agent"] == "test");
            BEAST_EXPECT(m.body == "*");
        }
        {
            error_code ec;
            message<false, string_body, fields> m;
            parser<false> p{m};
            std::string const s =
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Content-Length: 1\r\n"
                "\r\n"
                "*";
            p.write(buffer(s), ec);
            BEAST_EXPECTS(! ec, ec.message());
            BEAST_EXPECT(p.is_done());
            BEAST_EXPECT(m.status == 200);
            BEAST_EXPECT(m.reason == "OK");
            BEAST_EXPECT(m.version == 11);
            BEAST_EXPECT(m.fields["Server"] == "test");
            BEAST_EXPECT(m.body == "*");
        }
        // skip body
        {
            error_code ec;
            message<false, string_body, fields> m;
            parser<false> p{m};
            std::string const s =
                "HTTP/1.1 200 Connection Established\r\n"
                "Proxy-Agent: Zscaler/5.1\r\n"
                "\r\n";
            p.set_option(skip_body{true});
            p.write(buffer(s), ec);
            BEAST_EXPECTS(! ec, ec.message());
            BEAST_EXPECT(p.is_done());
        }
    }

    void
    run() override
    {
        testRead();
        testParse();
    }
};

BEAST_DEFINE_TESTSUITE(parser,http,beast);

} // http
} // beast

