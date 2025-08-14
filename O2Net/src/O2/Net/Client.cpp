#pragma once

/////////////////////////////////
// ASIO Network backend for O2Net
/////////////////////////////////

#include <O2/Net/Client.h>
#include <asio.hpp>
#include <iostream>
#include <O2/Net/_internal/Connection.h>

using asio::ip::tcp;

namespace o2 {
	namespace net {
		struct Client::Impl {
			ThreadSafeQueue<OwnedPacket> incomingPackets;
			asio::io_context asioContext;
			std::thread networkThread;
			std::unique_ptr<Connection> connection;
		};

		Client::Client() {
			impl = std::make_unique<Impl>();
		}

		Client::~Client() {
			disconnect();
		}

		bool Client::connect(const std::string& host, const uint16_t port) {
			try {
				tcp::resolver resolver(impl->asioContext);
				auto endpoints = resolver.resolve(host, std::to_string(port));

				impl->connection = std::make_unique<Connection>(
					Connection::Owner::Client,
					impl->asioContext,
					tcp::socket(impl->asioContext),
					impl->incomingPackets
				);

				impl->connection->connectToServer(endpoints);

				impl->networkThread = std::thread([this]() { impl->asioContext.run(); });
			}
			catch (std::exception& e) {
				std::cerr << "Client Exception: " << e.what() << '\n';
				return false;
			}

			return true;
		}

		void Client::disconnect() {
			if (isConnected()) {
				impl->connection->disconnect();
			}

			impl->asioContext.stop();
			if (impl->networkThread.joinable()) {
				impl->networkThread.join();
			}

			impl->connection.release();
		}

		void Client::send(const Packet& packet) {
			if (isConnected()) {
				impl->connection->send(packet);
			}
		}

		bool Client::isConnected() {
			return impl->connection != nullptr and impl->connection->isConnected();
		}

		ThreadSafeQueue<OwnedPacket>& Client::incoming() {
			return impl->incomingPackets;
		}
	} // namespace net
} // namespace o2