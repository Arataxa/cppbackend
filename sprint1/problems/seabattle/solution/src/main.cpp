#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "seabattle.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;

void PrintFieldPair(const SeabattleField& left, const SeabattleField& right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket& socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket& socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()), ec);

    return !ec;
}

class SeabattleAgent {
public:
    SeabattleAgent(const SeabattleField& field)
        : my_field_(field) {
    }

    void StartGame(tcp::socket& socket, bool my_initiative) {      
        while(!IsGameEnded()) {
            PrintFields();

            if (my_initiative) {
                while (true) {
                    try {
                        my_initiative = MakeMove(socket);

                        break;
                    }
                    catch (const std::exception& e) {
                        std::cout << "An error occurred: " << e.what() << std::endl;
                        std::cout << "Would you like to try making the move again? Enter Y or N." << std::endl;

                        std::string answer;
                        std::cin >> answer;

                        if (answer == "Y" || answer == "y") {
                            continue;
                        }
                        else {
                            return;
                        }
                    }
                }
            }
            else {
                my_initiative = ProcessTurn(socket);
            }
        }

        if (my_field_.IsLoser()) {
            std::cout << "You are lose." << std::endl;
        }
        else {
            std::cout << "Your are win." << std::endl;
        }
    }

private:
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

        if (p1 < 0 || p1 > 8) return std::nullopt;
        if (p2 < 0 || p2 > 8) return std::nullopt;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        char buff[] = {static_cast<char>(move.first) + 'A', static_cast<char>(move.second) + '1'};
        return {buff, 2};
    }

    void PrintFields() const {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    bool MakeMove(tcp::socket& socket) {
        std::cout << "Your turn." << std::endl;
        
        std::string str;

        while (true) {
            std::cin >> str;

            if (str.size() != 2) {
                std::cerr << "Coordinates must be in 2 characters." << std::endl;
            }
            else {
                break;
            }
        }

        auto raw_coordinates = ParseMove(str);

        if (!raw_coordinates) {
            throw std::invalid_argument("Invalid coordinates format.");
        }

        SendMove(socket, raw_coordinates.value());

        auto shoot_result = GetMoveResult(socket);

        switch (shoot_result) {
        case SeabattleField::ShotResult::MISS:
            std::cout << "Miss!" << std::endl;
            other_field_.MarkMiss(raw_coordinates.value().second, raw_coordinates.value().first);
            return false;
        case SeabattleField::ShotResult::HIT:
            other_field_.MarkHit(raw_coordinates.value().second, raw_coordinates.value().first);
            std::cout << "Hit!" << std::endl;
            return true;
        case SeabattleField::ShotResult::KILL:
            other_field_.MarkKill(raw_coordinates.value().second, raw_coordinates.value().first);
            std::cout << "Kill!" << std::endl;
            return true;
        }

        return false;
    }

    SeabattleField::ShotResult GetMoveResult(tcp::socket& socket) const {
        char result;

        boost::system::error_code ec;
        net::read(socket, net::buffer(&result, 1), ec);

        if (ec) {
            std::cerr << "Failed to get result: " << ec.what() << std::endl;
        }

        return static_cast<SeabattleField::ShotResult>(result);
    }

    void SendMoveResult(tcp::socket& socket, SeabattleField::ShotResult shoot_result) const {
        char shoot_result_byte = static_cast<char>(shoot_result);

        boost::system::error_code ec;
        net::write(socket, net::buffer(&shoot_result_byte, 1), ec);

        if (ec) {
            std::cerr << "Error sending shot result: " << ec.message() << std::endl;
            throw boost::system::system_error(ec);
        }
    }

    void SendMove(tcp::socket& socket, std::pair<int, int> move) const {
        std::array<uint8_t, 2> move_data = { static_cast<uint8_t>(move.first), static_cast<uint8_t>(move.second) };

        boost::system::error_code ec;
        net::write(socket, net::buffer(move_data), ec);

        if (ec) {
            std::cerr << "Error sending message" << ec.message() << std::endl;
            throw boost::system::system_error(ec);
        }
    }

    bool ProcessTurn(tcp::socket& socket) {
        std::cout << "Waiting for turn..." << std::endl;

        auto raw_coordinates = GetMove(socket);

        std::cout << "Shot to " << MoveToString(raw_coordinates) << std::endl;

        auto shoot_result = my_field_.Shoot(raw_coordinates.second, raw_coordinates.first);

        SendMoveResult(socket, shoot_result);

        switch (shoot_result) {
        case SeabattleField::ShotResult::MISS:
            return true;
        case SeabattleField::ShotResult::HIT:
            return false;
        case SeabattleField::ShotResult::KILL:
            return false;
        }

        return false;
    }

    std::pair<int, int> GetMove(tcp::socket& socket) const {
        uint8_t buffer[2];
        boost::system::error_code ec;

        net::read(socket, net::buffer(buffer, 2), ec);

        if (ec) {
            std::cerr << "Failed to read message: " << ec.what() << std::endl;
            throw boost::system::system_error(ec);
        }

        return std::make_pair(static_cast<int>(buffer[0]), static_cast<int>(buffer[1]));
    }


private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);

    std::cout << "Waiting for connection..." << std::endl;

    net::io_context io_context;
    boost::system::error_code ec;

    tcp::socket socket{ io_context };
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));

    acceptor.accept(socket);

    if (ec) {
        std::cout << "Can't accept connection"sv << std::endl;
        return;
    }

    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);

    boost::system::error_code ec;
    auto endpoint = tcp::endpoint(net::ip::make_address(ip_str, ec), port);

    if (ec) {
        std::cout << "Wrong IP format"sv << std::endl;
        return;
    }

    net::io_context io_context;
    tcp::socket socket{ io_context };
    socket.connect(endpoint, ec);

    if (ec) {
        std::cout << "Can't connect to server"sv << std::endl;
        return;
    }

    agent.StartGame(socket, true);
};

int main(int argc, const char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::cerr.rdbuf(std::cout.rdbuf());

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}
