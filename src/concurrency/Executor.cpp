#include <afina/concurrency/Executor.h>

namespace Afina {
    namespace Concurrency {

        Executor::Executor(unsigned int low_watermark,
                           unsigned int high_watermark,
                           unsigned int max_queue_size,
                           size_t idle_time) {
            _low_watermark = low_watermark;
            _high_watermark = high_watermark;
            _max_queue_size = max_queue_size;
            _idle_time = idle_time;

            state = State::kRun;

            _n_free_workers = 0;

            std::unique_lock<std::mutex> lock(mutex);
            lock.lock();

            for (unsigned int worker_id = 0; worker_id < _low_watermark; ++worker_id) {
                std::thread tmp(perform, this);
                tmp.detach();
                _n_existing_workers++;
            }
        }

        void Executor::Stop(bool await) {
            std::unique_lock<std::mutex> lock(mutex);
            lock.lock();
            state = State::kStopping;
            empty_condition.notify_all();
            if (await) {
                stop_condition.wait(lock);
            }
        }

        void perform(Afina::Concurrency::Executor *executor) {
            while (executor->state == Executor::State::kRun) {
                std::function<void()> task;
                auto terminate_time =
                        std::chrono::system_clock::now() + std::chrono::milliseconds(executor->_idle_time);
                {
                    std::unique_lock<std::mutex> lock(executor->mutex);
                    lock.lock();
                    while (executor->state == Executor::State::kRun && executor->tasks.empty()) {
                        executor->_n_free_workers++;
                        if (executor->empty_condition.wait_until(lock, terminate_time) == std::cv_status::timeout) {
                            if (executor->_n_existing_workers > executor->_low_watermark) {
                                executor->_n_free_workers--;
                                executor->_n_existing_workers--;
                                return;
                            } else {
                                executor->empty_condition.wait(lock);
                            }
                        }
                        executor->_n_free_workers--;
                    }
                    task = executor->tasks.front();
                    executor->tasks.pop_front();
                }
                task();
            }
            std::unique_lock<std::mutex> lock(executor->mutex);
            lock.lock();
            executor->_n_existing_workers--;
            if (executor->_n_existing_workers == 0) {
                executor->stop_condition.notify_one();
                executor->state = Executor::State::kStopped;
            }

        }
    }
} // namespace Afina
