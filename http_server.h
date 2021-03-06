#pragma once

#include <boost/asio.hpp>
#include <stdint.h>
#include <atomic>

#include "http_connection.h"
#include "datetime.h"
#include "logging.h"

namespace crow
{
    using namespace boost;
    using tcp = asio::ip::tcp;
    template <typename Handler>
    class Server
    {
    public:
        Server(Handler* handler, uint16_t port, uint16_t concurrency = 1)
            : acceptor_(io_service_, tcp::endpoint(asio::ip::address(), port)), 
            socket_(io_service_), 
            signals_(io_service_, SIGINT, SIGTERM),
            handler_(handler), 
            concurrency_(concurrency),
            port_(port)
        {
            do_accept();
        }

        void run()
        {
            std::vector<std::future<void>> v;
            for(uint16_t i = 0; i < concurrency_; i ++)
                v.push_back(
                        std::async(std::launch::async, [this]{io_service_.run();})
                        );

            CROW_LOG_INFO << server_name_ << " server is running, local port " << port_;

            signals_.async_wait(
                [&](const boost::system::error_code& error, int signal_number){
                    io_service_.stop();
                });
            
        }

        void stop()
        {
            io_service_.stop();
        }

    private:
        void do_accept()
        {
            acceptor_.async_accept(socket_, 
                [this](boost::system::error_code ec)
                {
                    if (!ec)
                        (new Connection<Handler>(std::move(socket_), handler_, server_name_))->start();
                    do_accept();
                });
        }

    private:
        asio::io_service io_service_;
        tcp::acceptor acceptor_;
        tcp::socket socket_;
        boost::asio::signal_set signals_;

        Handler* handler_;

        uint16_t concurrency_ = {1};
        std::string server_name_ = "Crow/0.1";
        uint16_t port_;
    };
}
