#pragma once
#include <cstdint>
#include <memory>
#include <O2/Net/Packet.h>
#include <O2/Net/ThreadSafeQueue.h>

namespace o2 {
	namespace net {
		class Server {
		protected:
			// Deferring data members to backend implementation
			struct Impl;
			std::unique_ptr<Impl> impl;

		public:
			Server(uint16_t port);
			virtual ~Server();

			bool start();
			void stop();
			void waitForClientConnection();
			bool isAlive(ConnectionId client) const;
			void messageClient(ConnectionId client, const Packet& packet);
			void messageAllClients(const Packet& packet, ConnectionId ignored = 0);
			void tick(size_t maxPackets = -1, bool wait = false);

		protected:
			virtual bool onClientConnect(const std::string& clientAddress);
			virtual void onClientDisconnect(ConnectionId client);
			virtual void onMessage(ConnectionId client, Packet& packet);
		};
	} // namespace net
} // namespace o2