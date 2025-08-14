#pragma once
#include <memory>
#include <O2/Net/Packet.h>
#include <O2/Net/ThreadSafeQueue.h>
#include <asio.hpp>
#include <iostream>

using asio::ip::tcp;

namespace o2 {
	namespace net {
		class Connection : public std::enable_shared_from_this<Connection> {
		public:
			enum class Owner {
				Server,
				Client
			};

		protected:
			tcp::socket socket;
			asio::io_context& asioContext;
			ThreadSafeQueue<Packet> outgoing;
			ThreadSafeQueue<OwnedPacket>& incoming;
			Owner owner = Owner::Server;
			uint32_t id = 0;
			bool connectionEstablished = false;

			Packet inboundPacket;

		public:
			Connection(
				Owner owner,
				asio::io_context& context,
				tcp::socket socket,
				ThreadSafeQueue<OwnedPacket>& incoming
			) : asioContext(context), socket(std::move(socket)), incoming(incoming) {
				this->owner = owner;
			}

			virtual ~Connection() {}

			void connectToServer(const tcp::resolver::results_type& endpoints) {
				if (owner == Owner::Client) {
					asio::async_connect(socket, endpoints,
						[this](std::error_code ec, tcp::endpoint endpoint) {
							if (!ec) {
								connectionEstablished = true;
								readHeader();
							}
							else {
								std::cout << '[' << id << "] Failed to connect to server: (" << ec.value() << ") " << ec.message() << "\n";
							}
						}
					);
				}
			}

			void connectToClient(uint32_t uid = 0) {
				if (owner == Owner::Server) {
					if (socket.is_open()) {
						id = uid;
						readHeader();
					}
				}
			}

			void disconnect() {
				if (isConnected()) {
					asio::post(asioContext, [this]() { socket.close(); });
				}
			}

			bool isConnected() const {
				if (owner == Owner::Client) {
					return socket.is_open() and connectionEstablished;
				}
				return socket.is_open();
			}

			uint32_t getId() const {
				return id;
			}

			void send(const Packet& packet) {
				asio::post(asioContext,
					[this, packet]() {
						bool isSending = !outgoing.empty();
						outgoing.push(packet);
						if (!isSending) {
							writeHeader();
						}
					}
				);
			}

		private:
			void readHeader() {
				asio::async_read(socket, asio::buffer(&inboundPacket.header, sizeof(PacketHeader)),
					[this](std::error_code ec, size_t length) {
						if (!ec) {
							if (inboundPacket.header.size > 0) {
								inboundPacket.body.resize(inboundPacket.header.size);
								readBody();
							}
							else {
								addToIncoming();
							}
						}
						else {
							std::cout << '[' << id << "] Read Header Fail.\n";
							socket.close();
						}
					}
				);
			}

			void readBody() {
				asio::async_read(socket, asio::buffer(inboundPacket.body.data(), inboundPacket.body.size()),
					[this](std::error_code ec, size_t length) {
						if (!ec) {
							addToIncoming();
						}
						else {
							std::cout << '[' << id << "] Read Body Fail.\n";
							socket.close();
						}
					}
				);
			}

			void writeHeader() {
				asio::async_write(socket, asio::buffer(&outgoing.front().header, sizeof(PacketHeader)),
					[this](std::error_code ec, size_t length) {
						if (!ec) {
							if (outgoing.front().body.size() > 0) {
								writeBody();
							}
							else {
								outgoing.pop();

								if (!outgoing.empty()) {
									writeHeader();
								}
							}
						}
						else {
							std::cout << '[' << id << "] Write Header Fail: (" << ec.value() << ") " << ec.message() << "\n";
							socket.close();
						}
					}
				);
			}

			void writeBody() {
				asio::async_write(socket, asio::buffer(outgoing.front().body.data(), outgoing.front().body.size()),
					[this](std::error_code ec, size_t length) {
						if (!ec) {
							outgoing.pop();

							if (!outgoing.empty()) {
								writeHeader();
							}
						}
						else {
							std::cout << '[' << id << "] Write Header Fail.\n";
							socket.close();
						}
					}
				);
			}

			void addToIncoming() {
				if (owner == Owner::Server) {
					incoming.push({ this->shared_from_this(), inboundPacket });
				}
				else {
					incoming.push({ nullptr, inboundPacket });
				}

				readHeader();
			}
		};
	}
}