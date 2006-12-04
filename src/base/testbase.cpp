//
//  libavg - Media Playback Engine. 
//  Copyright (C) 2003-2006 Ulrich von Zadow
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  Current versions can be found at www.libavg.de
//

#include "Queue.h"
#include "Command.h"
#include "WorkerThread.h"

#include "../base/TestSuite.h"
#include "../base/TimeSource.h"

#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace avg;
using namespace boost;
using namespace std;

class QueueTest: public Test {
public:
    QueueTest()
        : Test("QueueTest", 2)
    {
    }

    void runTests() 
    {
        runSingleThreadTests();
        runMultiThreadTests();
    }

private:
    void runSingleThreadTests()
    {
        Queue<int> q;
        TEST(q.empty());
        q.push(1);
        TEST(q.size() == 1);
        TEST(!q.empty());
        q.push(2);
        q.push(3);
        TEST(q.size() == 3);
        TEST(q.pop() == 1);
        TEST(q.pop() == 2);
        q.push(4);
        TEST(q.pop() == 3);
        TEST(q.pop() == 4);
        TEST(q.empty());
        bool bExceptionThrown = false;
        try {
            q.pop(false);
        } catch (Exception& ex) {
            if (ex.GetCode() == AVG_ERR_QUEUE_EMPTY) {
                bExceptionThrown = true;
            }
        }
        TEST(bExceptionThrown);
    }

    void runMultiThreadTests()
    {
        Queue<int> q(10);
        thread pusher(bind(&pushThread, &q));
        thread popper(bind(&popThread, &q));
        pusher.join();
        popper.join();
        TEST(q.empty());
    }

    static void pushThread(Queue<int>* pq)
    {
        for (int i=0; i<100; ++i) {
            pq->push(i);
            TimeSource::get()->msleep(1);
        }
    }

    static void popThread(Queue<int>* pq)
    {
        for (int i=0; i<100; ++i) {
            pq->pop();
            TimeSource::get()->msleep(3);
        }
    }
};

class TestWorkerThread: public WorkerThread<TestWorkerThread> {
public:
    TestWorkerThread(CmdQueue& CmdQ, int* pNumFuncCalls)
        : WorkerThread<TestWorkerThread>(CmdQ),
          m_pNumFuncCalls(pNumFuncCalls)
    {
    }

    bool init() 
    {
        (*m_pNumFuncCalls)++;
        return true;
    }

    bool work()
    {
        (*m_pNumFuncCalls)++;
        return true;
    }

    void deinit()
    {
        (*m_pNumFuncCalls)++;
    }

private:
    int * m_pNumFuncCalls;
};

class WorkerThreadTest: public Test {
public:
    WorkerThreadTest()
        : Test("WorkerThreadTest", 2)
    {
    }

    void runTests() 
    {
        TestWorkerThread::CmdQueue CmdQ;
        boost::thread* pTestThread;
        int NumFuncCalls = 0;
        CmdQ.push(Command<TestWorkerThread>(boost::bind(&TestWorkerThread::stop, _1)));
        pTestThread = new boost::thread(TestWorkerThread(CmdQ, &NumFuncCalls));
        pTestThread->join();
        delete pTestThread;
        TEST(NumFuncCalls == 3);
    }
};


class BaseTestSuite: public TestSuite {
public:
    BaseTestSuite() 
        : TestSuite("BaseTestSuite")
    {
        addTest(TestPtr(new QueueTest));
        addTest(TestPtr(new WorkerThreadTest));
    }
};

int main(int nargs, char** args)
{
    BaseTestSuite Suite;
    Suite.runTests();
    bool bOK = Suite.isOk();

    if (bOK) {
        return 0;
    } else {
        return 1;
    }
}

