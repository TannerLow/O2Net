/////////////////////////////////
// ASIO Network backend for O2Net
/////////////////////////////////
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
#define ASIO_STANDALONE
#include <asio.hpp>
#include <O2/Net/Server.h>
#include <cstdint>
#include <memory>
#include <thread>
#include <iostream>
#include <unordered_set>
#include <O2/Net/_internal/Connection.h>


using asio::ip::tcp;


namespace o2 {
	namespace net {
			struct Server::Impl {
				ThreadSafeQueue<OwnedPacket> incoming;
				std::unordered_map<ConnectionId, std::shared_ptr<Connection>> connections;
				asio::io_context asioContext;
				std::thread networkThread;
				tcp::acceptor asioAcceptor;
				uint32_t idCounter = 10000;

				Impl(uint16_t port) : asioAcceptor(asioContext, tcp::endpoint(tcp::v4(), port)) {}

				std::shared_ptr<Connection> getConnection(ConnectionId client) {
					std::shared_ptr<Connection> connection;
					auto it = connections.find(client);
					if (it != connections.end()) {
						return it->second;
					}
					return connection;
				}
			};

			bool Server::isAlive(ConnectionId client) const {
				return impl->connections.count(client);
			}

			std::shared_ptr<Connection> getConnection(
				ConnectionId client, 
				std::unordered_map<ConnectionId, std::shared_ptr<Connection>>& connections
			) {
				std::shared_ptr<Connection> connection;
				auto it = connections.find(client);
				if (it != connections.end()) {
					return it->second;
				}
				return connection;
			}

			Server::Server(uint16_t port) {
				impl = std::make_unique<Impl>(port);
			}

			Server::~Server() {
				stop();
			}

			bool Server::start() {
				try {
					waitForClientConnection();

					impl->networkThread = std::thread([this]() { impl->asioContext.run(); });
				}
				catch (std::exception& e) {
					std::cerr << "[SERVER] Exception: " << e.what() << '\n';
					return false;
				}

				std::cout << "[SERVER] Started!\n";
				return true;
			}

			void Server::stop() {
				impl->asioContext.stop();

				if (impl->networkThread.joinable()) {
					impl->networkThread.join();
				}

				std::cout << "[SERVER] Stopped!\n";
			}

			// ASYNC
			void Server::waitForClientConnection() {
				impl->asioAcceptor.async_accept(
					[this](std::error_code ec, tcp::socket socket) {
						if (!ec) {
							std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << '\n';

							std::shared_ptr<Connection> newConnection = std::make_shared<Connection>(
								Connection::Owner::Server,
								impl->asioContext,
								std::move(socket),
								impl->incoming
							);

							if (onClientConnect(newConnection->getRemoteAddress())) {
								uint32_t id = impl->idCounter++;
								impl->connections.emplace(id, (std::move(newConnection)));
								impl->connections[id]->connectToClient(id);
								std::cout << '[' << impl->connections[id]->getId() << "] Connection Approved\n";
							}
							else {
								std::cout << "[-----] Connection Denied\n";
							}
						}
						else {
							std::cout << "[SERVER] New Connection Error: " << ec.message() << '\n';
						}

						waitForClientConnection();
					}
				);
			}

			void Server::messageClient(ConnectionId client, const Packet& packet) {
				if (isAlive(client)) {
					std::shared_ptr<Connection> connection = impl->getConnection(client);
					if (connection and connection->isConnected()) {
						connection->send(packet);
					}
					else {
						onClientDisconnect(client);
						impl->connections.erase(client);
					}
				}
			}

			void Server::messageAllClients(const Packet& packet, ConnectionId ignored) {
				bool invalidClientExists = false;

				auto& connections = impl->connections;

				for (auto& client : connections) {
					if (client.second and client.second->isConnected()) {
						if (client.first != ignored) {
							client.second->send(packet);
						}
					}
					else {
						onClientDisconnect(client.first);
						client.second.reset();
						invalidClientExists = true;
					}
				}

				if (invalidClientExists) {
					for (auto it = connections.begin(); it != connections.end(); ) {
						if (it->second == nullptr) {
							it = connections.erase(it);
						}
						else {
							++it;
						}
					}
				}
			}

			void Server::tick(size_t maxPackets, bool wait) {
				if (wait) {
					impl->incoming.wait();
				}

				size_t packetCount = 0;
				while (packetCount < maxPackets and !impl->incoming.empty()) {
					auto packet = impl->incoming.pop();

					onMessage(packet.remote, packet.packet);

					packetCount++;
				}
			}

			bool Server::onClientConnect(const std::string& clientAddress) {
				return false;
			}

			void Server::onClientDisconnect(ConnectionId client) {

			}

			void Server::onMessage(ConnectionId client, Packet& packet) {

			}
	}
}