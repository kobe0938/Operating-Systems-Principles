
#include <unistd.h>

#include "stack.hh"
#include "thread.hh"
#include "timer.hh"
#include <iostream>

Thread *Thread::initial_thread = new Thread(nullptr);

Thread *Thread::curThread = initial_thread;

std::queue<Thread *> Thread::readyQueue;

Thread *Thread::tempThread = initial_thread;

// Create a placeholder Thread for the program's initial thread (which
// already has a stack, so doesn't need one allocated).
Thread::Thread(std::nullptr_t)
{
    // You have to implement this; depending on the contents of your
    // Thread structure, an empty function body may be sufficient.
    sp = nullptr;
}

Thread::~Thread()
{
    Thread *currrThread;

    int index = -1;

    if (this == curThread) {
        tempThread = std::move(this);
    } else {
        for (int i = 0; i < (int)readyQueue.size(); i++) {
            currrThread = readyQueue.front();
            readyQueue.pop();
            if (currrThread == this) {
                index = i;
                break;
            }
            readyQueue.push(currrThread);
        }

        for (int i = 0; i < index; i++) {
            currrThread = readyQueue.front();
            if (i == index - 1) {
                readyQueue.pop();
                break;
            }
            readyQueue.pop();
            readyQueue.push(currrThread);
        }
    }
}

void
Thread::start() {
    intr_enable(true);
    Thread::current()->curMain();
    Thread::exit();
}

Thread::Thread(std::function<void()> main, size_t stack_size) {
    IntrGuard guard;
    curMain = main;
    stack = (Bytes) new char[stack_size];
    sp = stack_init(stack.get(), stack_size, start);

    this->schedule();
}

void
Thread::create(std::function<void()> main, size_t stack_size)
{
    new Thread(main, stack_size);
}

Thread *
Thread::current()
{
    return curThread;
}

void
Thread::schedule()
{
    IntrGuard guard;
    Thread * first = readyQueue.front();

    if (first != nullptr){
        if (first == this) {
        readyQueue.push(this);
        return;
    }
}


    std::queue<Thread *> copyQueue(readyQueue);

    while(!copyQueue.empty()) {
        if (copyQueue.front() != this) {
            copyQueue.pop();
        } else {
            return;
        }
    }

    readyQueue.push(this);
}

void
Thread::swtch()
{
    IntrGuard guard;
    Thread *prev = curThread;

    if (readyQueue.size() == 0) {
        ::exit(0);
    } else if (readyQueue.size() == 1) {
        // do nothing
    } else {
        if (readyQueue.front() != curThread) {
            curThread = readyQueue.front();
        } else {
            readyQueue.pop();
            curThread = readyQueue.front();
        }
    }

    stack_switch(&prev->sp, &curThread->sp);
}

void
Thread::yield()
{
    IntrGuard guard;
    curThread->schedule();
    Thread::swtch();
}

void
Thread::exit()
{    
    if(curThread != initial_thread) {
        delete curThread;
    }

    Thread::swtch();
    std::abort();  // Leave this line--control should never reach here
}

void
Thread::preempt_init(std::uint64_t usec)
{
    timer_init(usec, yield);
    // You have to implement this
}
