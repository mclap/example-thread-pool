#pragma once

#include <functional>
#include <queue>
#include <memory>
#include <optional>
#include <mutex>
#include <thread>
#include <condition_variable>

using TaskId = std::size_t;
using Task = std::function<void()>;

struct Dispatcher
{
    virtual ~Dispatcher() = default;

    virtual std::optional<Task> getNext() = 0;
};

struct Thread
{
    Dispatcher *m_dispatcher;
    std::thread m_thread;

    Thread(Dispatcher *in_dispatcher)
        : m_dispatcher(in_dispatcher)
    {
    }

    void start()
    {
        std::thread t(&Thread::work, this);
        m_thread.swap(t);
        m_thread.detach();
    }

    void stop()
    {
        if (m_thread.joinable())
            m_thread.join();
    }

    void work()
    {
        std::optional<Task> taskOpt;
        do
        {
            taskOpt = m_dispatcher->getNext();
            if (taskOpt.has_value())
            {
                taskOpt.value()();
            }
        } while(taskOpt);
    }
};

struct ThreadPool : public Dispatcher
{
    ThreadPool(size_t numThreads)
    {
        while(numThreads-- > 0)
        {
            m_threads.push_back(std::make_unique<Thread>(this));
        }
    }

    void start()
    {
        for(auto& elm: m_threads)
        {
            elm->start();
        }
    }
    void stop()
    {
        {
            std::queue<Task> empty;
            std::scoped_lock guard(m_tasksLock);
            m_tasks.swap(empty);
            m_cv.notify_all();
        }

        for(auto& elm: m_threads)
        {
            elm->stop();
        }
    }

    TaskId addTask(Task task)
    {
        std::scoped_lock guard(m_tasksLock);
        TaskId result = ++m_taskId;

        m_tasks.push(task);
        m_cv.notify_one();

        return result;
    }

protected:
    // Dispatcher interface
    std::optional<Task> getNext() override
    {
        std::optional<Task> result;
        std::unique_lock guard(m_tasksLock);

        if (m_tasks.empty())
        {
            m_cv.wait(guard);
        }

        if (!m_tasks.empty())
        {
            result = m_tasks.front();
            m_tasks.pop();
        }

        return result;
    }

protected:
    std::vector<std::unique_ptr<Thread>> m_threads;

    TaskId m_taskId{0};
    std::mutex m_tasksLock;
    std::condition_variable m_cv;
    std::queue<Task> m_tasks;
};
