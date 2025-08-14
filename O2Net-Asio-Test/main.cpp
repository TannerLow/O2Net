#include <O2/Net/Client.h>
#include <iostream>

enum class PacketType : uint32_t {
	PingRequest,
	PingResponse
};

class CustomClient : public o2::net::Client {
public:
	void pingServer() {
		o2::net::Packet packet;
		packet.header.type = (uint32_t)PacketType::PingRequest;
		packet.header.size = 0;

		std::cout << "Sending ping request" << std::endl;
		send(packet);
	}
};

int main() {

	CustomClient client;

	client.connect("localhost", 42069);

	while (!client.isConnected()) {}

	if (client.isConnected()) {
		std::cout << "Connected to the server" << std::endl;
		client.pingServer();
	}

	while (true) {
		if (!client.incoming().empty()) {
			o2::net::OwnedPacket packet = client.incoming().front();
			client.incoming().pop();
			if (packet.packet.header.type == (uint32_t)PacketType::PingResponse) {
				std::cout << "Received ping response" << std::endl;
			}
		}
	}

	return 0;
}