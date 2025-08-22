#pragma once
#include <mutex>
#include <queue>
#include "Packet.h"

namespace o2 {
	namespace net {
		template<class MessageType>
		class ThreadSafeQueue {
		protected:
			std::mutex queuetex;
			std::queue<MessageType> queue;

			std::condition_variable waiter;
			std::mutex waitLock;

		public:
			ThreadSafeQueue() = default;
			ThreadSafeQueue(const ThreadSafeQueue&) = delete;

			const MessageType& front() {
				std::scoped_lock lock(queuetex);
				return queue.front();
			}

			const MessageType& back() {
				std::scoped_lock lock(queuetex);
				return queue.back();
			}

			void push(const MessageType& item) {
				std::scoped_lock queueLock(queuetex);
				queue.emplace(std::move(item));

				std::unique_lock<std::mutex> lock(waitLock);
				waiter.notify_one();
			}

			bool empty() {
				std::scoped_lock lock(queuetex);
				return queue.empty();
			}

			size_t size() const {
				std::scoped_lock lock(queuetex);
				return queue.size();
			}

			void clear() {
				std::scoped_lock lock(queuetex);
				queue = {};
			}

			MessageType pop() {
				std::scoped_lock lock(queuetex);
				auto t = std::move(queue.front());
				queue.pop();
				return t;
			}

			void wait() {
				while (empty()) {
					std::unique_lock<std::mutex> lock(waitLock);
					waiter.wait(lock);
				}
			}
		};
	} // namespace net
} // namespace o2