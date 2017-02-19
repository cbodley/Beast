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

    // VFALCO What about alignment?
    char buf_[sizeof(typename std::conditional<isRequest,
        req_impl<
            typename detail::parser_base::Body_exemplar,
            typename detail::parser_base::Fields_exemplar>,
        res_impl<
            typename detail::parser_base::Body_exemplar,
            typename detail::parser_base::Fields_exemplar>
                >::type) + 4 * sizeof(void*)];
    impl_type* impl_;
    std::unique_ptr<impl_type> p_;

public:
    /// `true` if this parser parses requests, `false` for responses.
    static bool constexpr is_request = isRequest;

    /// Destructor
    ~parser();

    /** Move constructor.

        After the move, the only valid operation
        on the moved-from object is destruction.
    */
    parser(parser&& other);

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

    /// Copy constructor (disallowed)
    parser(parser const&) = delete;

    /// Copy assignment (disallowed)
    parser& operator=(parser const&) = delete;

private:
    friend class basic_parser<isRequest, parser>;

    template<class T, class Arg>
    void
    alloc(Arg& arg, std::true_type)
    {
        // type-pun
        impl_ = new(buf_) T{arg};
    }

    template<class T, class Arg>
    void
    alloc(Arg& arg, std::false_type)
    {
        p_.reset(new T{arg});
        impl_ = p_.get();
    }

    template<class T, class Arg>
    void
    alloc(Arg& arg)
    {
        alloc<T>(arg,
            std::integral_constant<bool,
                sizeof(T) <= sizeof(buf_)>{});
    }

    template<class Fields>
    void
    construct(header<true, Fields>& h)
    {
        this->split(true);
        alloc<req_h_impl<Fields>>(h);
    }

    template<class Fields>
    void
    construct(header<false, Fields>& h)
    {
        this->split(true);
        alloc<res_h_impl<Fields>>(h);
    }

    template<class Body, class Fields>
    void
    construct(message<true, Body, Fields>& m)
    {
        this->split(false);
        alloc<req_impl<Body, Fields>>(m);
    }

    template<class Body, class Fields>
    void
    construct(message<false, Body, Fields>& m)
    {
        this->split(false);
        alloc<res_impl<Body, Fields>>(m);
    }

    void
    on_request(boost::string_ref const& method,
        boost::string_ref const& path,
            int version, error_code&)
    {
        impl_->on_req(method, path, version);
    }

    void
    on_response(int status,
        boost::string_ref const& reason,
            int version, error_code&)
    {
        impl_->on_res(status, reason, version);
    }

    void
    on_field(boost::string_ref const& name,
        boost::string_ref const& value,
            error_code&)
    {
        impl_->on_field(name, value);
    }

    void
    on_header(error_code& ec)
    {
        impl_->on_header(ec);
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
        impl_->on_body(data, ec);
    }

    void
    on_done(error_code& ec)
    {
        impl_->on_done(ec);
    }
};

} // http
} // beast

#include <beast/http/impl/parser.ipp>

#endif
