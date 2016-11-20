//
// Copyright (c) 2013-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BEAST_HTTP_PARSER_HPP
#define BEAST_HTTP_PARSER_HPP

#include <beast/http/message.hpp>
#include <beast/http/basic_parser.hpp>
#include <beast/http/detail/parser.hpp>
#include <array>
#include <type_traits>
#include <utility>

namespace beast {
namespace http {

/** A parser for producing HTTP/1 messages.

    This class uses the basic HTTP/1 wire format parser to convert
    a series of octets into a @ref header or @ref message.

    @note A new instance of the parser is required for each message.
*/
template<bool isRequest>
class parser
    : public basic_parser<isRequest,
        parser<isRequest>>
    , private detail::parser_base
{
    using impl_type = typename std::conditional<
        isRequest, req_impl_base, res_impl_base>::type;

    std::unique_ptr<impl_type> p_;

public:
    /// `true` if this parser parses requests, `false` for responses.
    static bool constexpr is_request = isRequest;

    /// Destructor
    ~parser();

    /** Construct a parser to process a header.

        This constructor creates a new parser which attempts
        to parse a complete header from the input sequence.
        If the semantics of the message indicate that there
        is no body, or the caller sets the @ref skip_body
        option, the message is considered complete.

        After the parse is completed, if a message body is
        indicated the parser is left in a state ready to
        continue parsing the body.
    */
    template<class Fields>
    parser(header<isRequest, Fields>& m);

    /** Construct a parser to process a message.

        This constructor creates a new parser which attempts
        to parse a complete message from the input sequence.
        up to the header of the input sequence.
    */
    template<class Body, class Fields>
    parser(message<isRequest, Body, Fields>& m);

    /** Move constructor.

        After the move, the only valid operation
        on the moved-from object is destruction.
    */
    parser(parser&& other) = default;

    /// Copy constructor (disallowed)
    parser(parser const&) = delete;

    /// Copy assignment (disallowed)
    parser& operator=(parser const&) = delete;

private:
    friend class basic_parser<isRequest, parser>;

    impl_type&
    impl()
    {
        return *p_.get();
    }

    template<class Fields>
    void
    construct(header<true, Fields>& h)
    {
        this->split(true);
        using type = req_h_impl<Fields>;
        p_.reset(new type{h});
    }

    template<class Fields>
    void
    construct(header<false, Fields>& h)
    {
        this->split(true);
        using type = res_h_impl<Fields>;
        p_.reset(new type{h});
    }

    template<class Body, class Fields>
    void
    construct(message<true, Body, Fields>& m)
    {
        this->split(false);
        using type = req_impl<Body, Fields>;
        p_.reset(new type{m});
    }

    template<class Body, class Fields>
    void
    construct(message<false, Body, Fields>& m)
    {
        this->split(false);
        using type = res_impl<Body, Fields>;
        p_.reset(new type{m});
    }

    void
    on_request(boost::string_ref const& method,
        boost::string_ref const& path,
            int version, error_code&)
    {
        impl().on_req(method, path, version);
    }

    void
    on_response(int status,
        boost::string_ref const& reason,
            int version, error_code&)
    {
        impl().on_res(status, reason, version);
    }

    void
    on_field(boost::string_ref const& name,
        boost::string_ref const& value,
            error_code&)
    {
        impl().on_field(name, value);
    }

    void
    on_header(error_code& ec)
    {
        impl().on_header(ec);
    }

    void
    on_chunk(std::uint64_t length,
        boost::string_ref const& ext,
            error_code&)
    {
    }

    void
    on_body(boost::string_ref const& data,
        error_code& ec)
    {
        impl().on_body(data, ec);
    }

    void
    on_done(error_code& ec)
    {
        impl().on_done(ec);
    }
};

template<bool isRequest>
parser<isRequest>::
~parser()
{
    impl().~impl_type();
}

template<bool isRequest>
template<class Fields>
parser<isRequest>::
parser(header<isRequest, Fields>& m)
{
    construct(m);
}

template<bool isRequest>
template<class Body, class Fields>
parser<isRequest>::
parser(message<isRequest, Body, Fields>& m)
{
    construct(m);
}

} // http
} // beast

#endif
