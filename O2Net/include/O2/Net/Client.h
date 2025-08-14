#pragma once
#define _WIN32_WINNT 0x0A00
#include "ThreadSafeQueue.h"
#include <memory>

namespace o2 {
	namespace net {
		class Client {
		protected:
			// Deferring data members to backend implementation
			struct Impl;
			std::unique_ptr<Impl> impl;

		public:
			Client();
			virtual ~Client();

			bool connect(const std::string& host, const uint16_t port);
			void disconnect();
			void send(const Packet& packet);
			bool isConnected();
			ThreadSafeQueue<OwnedPacket>& incoming();
		};
	} // namespace net
} // namespace o2