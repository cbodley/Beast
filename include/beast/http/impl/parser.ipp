//
// Copyright (c) 2013-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BEAST_HTTP_IMPL_PARSER_IPP
#define BEAST_HTTP_IMPL_PARSER_IPP

namespace beast {
namespace http {

template<bool isRequest>
parser<isRequest>::
~parser()
{
    if(! p_)
        impl_->~impl_type();
}

template<bool isRequest>
parser<isRequest>::
parser(parser&& other)
{
    if(! other.p_)
    {
        other.impl_->move_to(buf_);
        // type-pun
        impl_ = reinterpret_cast<
            impl_type*>(buf_);
    }
    else
    {
        p_ = std::move(other.p_);
        impl_ = p_.get();
    }
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
