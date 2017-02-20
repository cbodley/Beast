//
// Copyright (c) 2013-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BEAST_HTTP_TEST_FAIL_PARSER_HPP
#define BEAST_HTTP_TEST_FAIL_PARSER_HPP

#include <beast/http/basic_parser.hpp>
#include <beast/test/fail_counter.hpp>

namespace beast {
namespace http {

template<bool isRequest>
class fail_parser
    : public basic_parser<
        isRequest, fail_parser<isRequest>>
{
    test::fail_counter& fc_;

public:
    std::string body;

    template<class... Args>
    explicit
    fail_parser(test::fail_counter& fc,
            Args&&... args)
        : fc_(fc)
    {
    }

    void
    on_begin_request(boost::string_ref const&,
        boost::string_ref const&,
            int, error_code& ec)
    {
        fc_.fail(ec);
    }

    void
    on_begin_response(
        int, boost::string_ref const&,
            int, error_code& ec)
    {
        fc_.fail(ec);
    }

    void
    on_field(boost::string_ref const&,
        boost::string_ref const&,
            error_code& ec)
    {
        fc_.fail(ec);
    }

    void
    on_end_header(error_code& ec)
    {
        fc_.fail(ec);
    }

    void
    on_begin_body(error_code& ec)
    {
        fc_.fail(ec);
    }

    void
    on_chunk(std::uint64_t,
        boost::string_ref const&,
            error_code& ec)
    {
        fc_.fail(ec);
    }

    void
    on_body(boost::string_ref const& s,
        error_code& ec)
    {
        if(fc_.fail(ec))
            return;
        body.append(s.data(), s.size());
    }

    void
    on_end_body(error_code& ec)
    {
        if(fc_.fail(ec))
            return;
    }

    void
    on_end_message(error_code& ec)
    {
        if(fc_.fail(ec))
            return;
    }
};

} // http
} // beast

#endif
