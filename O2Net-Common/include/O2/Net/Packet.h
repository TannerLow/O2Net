#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <O2/Net/TypesAliases.h>

namespace o2 {
	namespace net {
		struct PacketHeader {
			uint32_t type;
			uint32_t size = 0;
		};

		struct Packet {
			PacketHeader header;
			std::vector<std::byte> body;

			size_t size() const {
				return sizeof(PacketHeader) + body.size();
			}
		};

		struct OwnedPacket {
			ConnectionId remote = 0;
			Packet packet;
		};
	} // namespace net
} // namespace o2