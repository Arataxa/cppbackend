#pragma once

#include <boost/asio.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Windows.h>
#endif

#include <memory>
#include <iostream>
#include <string>

#include "audio.h"

using namespace std::literals;
namespace net = boost::asio;
using net::ip::tcp;

const int port = 12345;

enum EntryType{
	CLIENT, SERVER
};

class ApplicationBehavior {
public:
	virtual void Run() = 0;

	virtual ~ApplicationBehavior() = default;
};

class ClientBehavior : public ApplicationBehavior {
public:
	void Run() override {
		if (!ConnectToServer()) {
			return;
		}

		SendData(Serialize(RecordMessage()));
	}
private:
	bool ConnectToServer() {
		std::string ip;
		std::cout << "Enter the server IP" << std::endl;
		std::cin >> ip;

		boost::system::error_code ec;
		auto endpoint = tcp::endpoint(net::ip::make_address(ip, ec), port);
		net::io_context io_context;

		socket_ = std::make_unique<tcp::socket>(io_context);
		socket_->connect(endpoint, ec);

		if (ec) {
			std::cout << "Can't connect to server" << ec.what() << std::endl;
			return false;
		}

		return true;
	}

	Recorder::RecordingResult RecordMessage() const {
		Recorder recorder(ma_format_u8, 1);
		std::string str;

		std::cout << "Press Enter to record message..." << std::endl;
		std::getline(std::cin, str);

		auto rec_result = recorder.Record(65000, 1.5s);
		std::cout << "Recording done" << std::endl;

		return rec_result;
	}

	std::vector<char> Serialize(Recorder::RecordingResult rec_result) const {
		std::vector<char> buffer;

		// Сериализация размера данных
		size_t data_size = rec_result.data.size();
		buffer.insert(buffer.end(), reinterpret_cast<const char*>(&data_size), reinterpret_cast<const char*>(&data_size) + sizeof(data_size));

		// Сериализация самих данных
		buffer.insert(buffer.end(), rec_result.data.begin(), rec_result.data.end());

		// Сериализация количества кадров
		buffer.insert(buffer.end(), reinterpret_cast<const char*>(&rec_result.frames), reinterpret_cast<const char*>(&rec_result.frames) + sizeof(rec_result.frames));

		buffer.insert(buffer.end(), '\n');

		return buffer;
	}

	void SendData(std::vector<char> buffer) const {
		if (!socket_) {
			std::cout << "Error sending message: connection not found" << std::endl;
			return;
		}

		boost::system::error_code ec;
		boost::asio::write(*socket_, boost::asio::buffer(buffer), ec);

		if (ec) {
			std::cerr << "Failed to send data: " << ec.message() << std::endl;
		}
		else {
			std::cout << "Data sent successfully" << std::endl;
		}
	}

	std::unique_ptr<tcp::socket> socket_;
};

class ServerBehavior : public ApplicationBehavior {
public:
	void Run() override {
		AcceptConnection();

		net::streambuf stream_buf;
		ReadMessage(stream_buf);
		
		ProcessData(Deserialize(stream_buf));
	}

private:
	void ProcessData(const Recorder::RecordingResult& recording_result) {
		Player player(ma_format_u8, 1);

		player.PlayBuffer(recording_result.data.data(), recording_result.frames, 1.5s);
	}

	Recorder::RecordingResult Deserialize(net::streambuf& stream_buf) {
		std::istream is(&stream_buf);
		std::vector<char> buffer((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());

		size_t offset = 0;

		// Десериализация размера данных
		size_t data_size;
		std::memcpy(&data_size, buffer.data() + offset, sizeof(size_t));
		offset += sizeof(size_t);

		// Десериализация самих данных
		std::vector<char> audio_data(buffer.begin() + offset, buffer.begin() + offset + data_size);
		offset += data_size;

		// Десериализация количества кадров
		size_t frames;
		std::memcpy(&frames, buffer.data() + offset, sizeof(size_t));

		std::cout << "Получено " << data_size << " байт данных аудио и " << frames << " кадров." << std::endl;

		return Recorder::RecordingResult{ audio_data, frames };
	}

	void ReadMessage(net::streambuf& stream_buf) const {
		boost::system::error_code ec;

		net::read_until(*socket_, stream_buf, '\n', ec);

		if (ec) {
			std::cerr << "Error reading from socket: " << ec.message() << std::endl;
			return;
		}
	}

	void AcceptConnection() {
		net::io_context io_context;
		tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
		std::cout << "Waiting for connection..."sv << std::endl;

		boost::system::error_code ec;
		socket_ = std::make_unique<tcp::socket>(io_context);
		acceptor.accept(*socket_, ec);

		if (ec) {
			std::cout << "Can't accept connection"sv << std::endl;
			return;
		}
	}

	std::unique_ptr<tcp::socket> socket_;
};

class Application {
public:
	void Run() {
		std::cout << "Enter entry type: client or server." << std::endl;
		std::string login_type;
		std::cin >> login_type;

		if (login_type == "client") {
			behavior_ = std::make_unique<ClientBehavior>();
		}
		else if (login_type == "server") {
			behavior_ = std::make_unique<ServerBehavior>();
		}
		else {
			std::cout << "Error" << std::endl;
		}

		behavior_->Run();
	}

private:
	std::unique_ptr<ApplicationBehavior> behavior_;
};