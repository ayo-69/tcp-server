#define BOOST_ASIO_HEADER_ONLY
#define BOOST_SYSTEM_NO_DEPRECATED
#include <boost/asio.hpp>
#include <iostream>
#include <deque>
#include <memory>
#include <set>
#include <string>

using boost::asio::ip::tcp;

// Forward declaration
class chat_session;

// A message queue for a single participant
using chat_message_queue = std::deque<std::string>;

// Interface for a chat participant
class chat_participant {
public:
    virtual ~chat_participant() {}
    virtual void deliver(const std::string& msg) = 0;
};

using chat_participant_ptr = std::shared_ptr<chat_participant>;

// The room manages all participants
class chat_room {
public:
    void join(chat_participant_ptr participant) {
        participants_.insert(participant);
    }

    void leave(chat_participant_ptr participant) {
        participants_.erase(participant);
    }

    void deliver(const std::string& msg, chat_participant_ptr sender) {
        for (auto participant : participants_) {
            if (participant != sender) {
                participant->deliver(msg);
            }
        }
    }

    void broadcast(const std::string& msg) {
        for (auto participant : participants_) {
            participant->deliver(msg);
        }
    }

private:
    std::set<chat_participant_ptr> participants_;
};

// Represents a single client connection
class chat_session : public chat_participant,
                     public std::enable_shared_from_this<chat_session> {
public:
    chat_session(tcp::socket socket, chat_room& room)
        : socket_(std::move(socket)), room_(room) {
        
        boost::system::error_code ec;
        auto remote_ep = socket_.remote_endpoint(ec);
        if (!ec) {
            client_endpoint_str_ = remote_ep.address().to_string() + ":" + std::to_string(remote_ep.port());
        } else {
            client_endpoint_str_ = "unknown";
        }
    }

    void start() {
        room_.join(shared_from_this());

        std::string welcome_msg = "Welcome! You are connected from " + client_endpoint_str_ + "\n";
        deliver(welcome_msg);

        std::string join_msg = "Client " + client_endpoint_str_ + " has joined the chat.\n";
        std::cout << join_msg;
        room_.deliver(join_msg, shared_from_this());

        do_read();
    }

    void deliver(const std::string& msg) override {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(msg);
        if (!write_in_progress) {
            do_write();
        }
    }

private:
    void do_read() {
        auto self(shared_from_this());
        boost::asio::async_read_until(socket_, read_buffer_, "\n",
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    std::istream is(&read_buffer_);
                    std::string line;
                    std::getline(is, line);
                    
                    std::string msg_to_broadcast = "Client " + client_endpoint_str_ + ": " + line + "\n";
                    std::cout << msg_to_broadcast;
                    room_.deliver(msg_to_broadcast, shared_from_this());
                    do_read();
                } else {
                    // Don't broadcast leave message if socket is closed due to server shutdown
                    if (ec != boost::asio::error::operation_aborted) {
                        std::string leave_msg = "Client " + client_endpoint_str_ + " has left the chat.\n";
                        std::cout << leave_msg;
                        room_.deliver(leave_msg, shared_from_this());
                    }
                    room_.leave(shared_from_this());
                }
            });
    }

    void do_write() {
        auto self(shared_from_this());
        boost::asio::async_write(socket_,
            boost::asio::buffer(write_msgs_.front()),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    write_msgs_.pop_front();
                    if (!write_msgs_.empty()) {
                        do_write();
                    }
                } else {
                     if (ec != boost::asio::error::operation_aborted) {
                        room_.leave(shared_from_this());
                     }
                }
            });
    }

    tcp::socket socket_;
    chat_room& room_;
    boost::asio::streambuf read_buffer_;
    chat_message_queue write_msgs_;
    std::string client_endpoint_str_;
};

// The server accepts incoming connections and creates sessions
class chat_server {
public:
    chat_server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint)
        : acceptor_(io_context, endpoint) {
        do_accept();
    }

    void stop() {
        acceptor_.close();
        room_.broadcast("Server is shutting down. Goodbye!\n");
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<chat_session>(std::move(socket), room_)->start();
                }
                // If the operation was not aborted, continue accepting
                if (ec != boost::asio::error::operation_aborted) {
                    do_accept();
                }
            });
    }

    tcp::acceptor acceptor_;
    chat_room room_;
};

int main(int argc, char* argv[]) {
    try {
        unsigned short port;
        if (argc == 1) {
            port = 8080; // Default port
            std::cout << "No port specified. Using default port 8080.\n";
        } else if (argc == 2) {
            port = std::atoi(argv[1]);
        } else {
            std::cerr << "Usage: server [port]\n";
            return 1;
        }

        boost::asio::io_context io_context;
        tcp::endpoint endpoint(tcp::v4(), port);
        
        chat_server server(io_context, endpoint);

        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait(
            [&](const boost::system::error_code& /*error*/, int /*signal_number*/) {
                std::cout << "\nShutdown signal received." << std::endl;
                server.stop();
                io_context.stop();
            });


        std::cout << "Server is listening on port " << port << "...\n";
        
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
