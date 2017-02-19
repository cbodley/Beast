//
// Copyright (c) 2013-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BEAST_HTTP_DETAIL_PARSER_HPP
#define BEAST_HTTP_DETAIL_PARSER_HPP

#include <beast/http/message.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/utility/string_ref.hpp>
#include <utility>

namespace beast {
namespace http {
namespace detail {

class parser_base
{
protected:
    // type-erasure helpers

    struct impl_base
    {
        virtual
        ~impl_base() = default;

        virtual
        void
        move_to(void* p) = 0;

        virtual
        void
        on_field(
            boost::string_ref const&,
            boost::string_ref const&) = 0;

        virtual
        void
        on_header(boost::optional<
            std::uint64_t> const&,
                error_code&)
        {
        }

        virtual
        void
        on_body(boost::string_ref const&,
            error_code& ec)
        {
        }
        
        virtual
        void
        on_done(error_code& ec)
        {
        }
    };

    struct req_impl_base : impl_base
    {
        virtual
        void
        on_req(
            boost::string_ref const&,
            boost::string_ref const&,
            int) = 0;
    };

    struct res_impl_base : impl_base
    {
        virtual
        void
        on_res(
            int,
            boost::string_ref const&,
            int) = 0;
    };

    template<class Fields>
    class req_h_impl : public req_impl_base
    {
        header<true, Fields>& h_;

    public:
        req_h_impl(req_h_impl&&) = default;

        req_h_impl(header<true, Fields>& h)
            : h_(h)
        {
        }

        void
        move_to(void* p) override
        {
            new(p) req_h_impl{std::move(*this)};
        }

        void
        on_req(
            boost::string_ref const& method,
            boost::string_ref const& path,
            int version) override
        {
            h_.version = version;
            h_.url = std::string(
                path.data(), path.size());
            h_.method = std::string(
                method.data(), method.size());
        }

        void
        on_field(
            boost::string_ref const& name,
            boost::string_ref const& value) override
        {
            h_.fields.insert(name, value);
        }
    };

    template<class Fields>
    class res_h_impl : public res_impl_base
    {
        header<false, Fields>& h_;

    public:
        res_h_impl(res_h_impl&&) = default;

        res_h_impl(header<false, Fields>& h)
            : h_(h)
        {
        }

        void
        move_to(void* p) override
        {
            new(p) res_h_impl{std::move(*this)};
        }

        void
        on_res(
            int status,
            boost::string_ref const& reason,
            int version) override
        {
            h_.status = status;
            h_.version = version;
            h_.reason = std::string(
                reason.data(), reason.size());
        }

        void
        on_field(
            boost::string_ref const& name,
            boost::string_ref const& value) override
        {
            h_.fields.insert(name, value);
        }
    };

    template<class Body, class Fields>
    class req_impl : public req_impl_base
    {
        using reader_type =
            typename Body::reader;

        message<true, Body, Fields>& m_;
        boost::optional<reader_type> r_;

    public:
        req_impl(req_impl&&) = default;

        req_impl(message<true, Body, Fields>& m)
            : m_(m)
        {
        }

        void
        move_to(void* p) override
        {
            new(p) req_impl{std::move(*this)};
        }

        void
        on_req(
            boost::string_ref const& method,
            boost::string_ref const& path,
            int version) override
        {
            m_.version = version;
            m_.url = std::string(
                path.data(), path.size());
            m_.method = std::string(
                method.data(), method.size());
        }

        void
        on_field(
            boost::string_ref const& name,
            boost::string_ref const& value) override
        {
            m_.fields.insert(name, value);
        }

        void
        on_header(boost::optional<
            std::uint64_t> const& content_length,
                error_code& ec) override
        {
            r_.emplace(m_);
            r_->init(content_length, ec);
        }

        void
        on_body(boost::string_ref const& s,
            error_code& ec) override
        {
            using boost::asio::buffer;
            using boost::asio::buffer_copy;
            boost::optional<typename
                reader_type::mutable_buffers_type> mb;
            mb = r_->prepare(s.size(), ec);
            if(ec)
                return;
            r_->commit(buffer_copy(*mb,
                buffer(s.data(), s.size())), ec);
            if(ec)
                return;
        }

        void
        on_done(error_code&) override
        {
        }
    };

    template<class Body, class Fields>
    class res_impl : public res_impl_base
    {
        using reader_type =
            typename Body::reader;

        message<false, Body, Fields>& m_;
        boost::optional<
            typename Body::reader> r_;

    public:
        res_impl(res_impl&&) = default;

        res_impl(message<false, Body, Fields>& m)
            : m_(m)
        {
        }

        void
        move_to(void* p) override
        {
            new(p) res_impl{std::move(*this)};
        }

        void
        on_res(
            int status,
            boost::string_ref const& reason,
            int version) override
        {
            m_.status = status;
            m_.version = version;
            m_.reason = std::string(
                reason.data(), reason.size());
        }

        void
        on_field(
            boost::string_ref const& name,
            boost::string_ref const& value) override
        {
            m_.fields.insert(name, value);
        }

        void
        on_header(boost::optional<
            std::uint64_t> const& content_length,
                error_code& ec) override
        {
            r_.emplace(m_);
            r_->init(content_length, ec);
        }

        void
        on_body(boost::string_ref const& s,
            error_code& ec) override
        {
            using boost::asio::buffer;
            using boost::asio::buffer_copy;
            boost::optional<typename
                reader_type::mutable_buffers_type> mb;
            mb = r_->prepare(s.size(), ec);
            if(ec)
                return;
            r_->commit(buffer_copy(*mb,
                buffer(s.data(), s.size())), ec);
            if(ec)
                return;
        }

        void
        on_done(error_code&) override
        {
        }
    };

    struct Fields_exemplar
    {
        void
        insert(boost::string_ref const&,
            boost::string_ref const&);
    };

    struct Body_exemplar
    {
        using value_type = int;

        struct reader
        {
            using mutable_buffers_type =
                boost::asio::mutable_buffers_1;

            message<true,
                Body_exemplar,
                Fields_exemplar>& m;

            template<bool isRequest,
                class Body, class Fields>
            reader(message<
                isRequest, Body, Fields>&);

            void
            init(boost::optional<
                std::uint64_t> const&, error_code&);

            boost::optional<mutable_buffers_type>
            prepare(std::size_t, error_code&);

            void
            commit(std::size_t, error_code&);

            void
            finish(error_code&);
        };
    };
};

} // detail
} // http
} // beast

#endif
