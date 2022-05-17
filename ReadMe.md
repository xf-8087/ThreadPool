# 线程池介绍 

作为五大池之一（内存池、连接池、线程池、进程池、协程池），线程池的应用非常广泛，不管事客户端程序还是后台服务端，都是提高业务处理能力的必备模块。

线程池有很多的开源实现，虽然在接口使用上优点区别，但是其核心实现原理基本相同。

# 线程池知识背景

- C++面向对象的标准
  - 组合和继承、多态、STL容器、智能指针、函数对象、绑定器、可变参数模板等
- C++11多线程编程
  - thread、mutex、atomic、condition_variable、unique_lock等
- C++17和C++20标准
  - C++17中的any类型可以接受任何类型的参数、C++20 的semaphore信号量
- 多线程理论
  - 多线程基本知识、线程互斥、线程同步、原子操作等。

# 并发和并行

- 并发：单核上，多个线程占用不同的CPU时间片，物理上还是串行执行的，但是由于每个线程占用的CPU时间片非常短（比如10ms），看起来就像是多个线程都在共同执行一样，这样的场景称作并发（concurrent）。

- 并行：在多核或者多CPU上，多个线程是在真正的同时执行，这样的场景称作并行（parallel）。

# 线程池的优势

> 多线程程序一定就好吗？不一定，要看具体的应用场景：

- 多线程处理可以同时运⾏多个线程。由于多线程应⽤程序将程序划分成多个独⽴的任务,因此可以在以下⽅⾯显著提⾼性能:
  (1)多线程技术使程序的响应速度更快 ,因为⽤户界⾯可以在进⾏其它⼯作的同时⼀直处于活动状态;
  (2)当前没有进⾏处理的任务时可以将处理器时间让给其它任务;
  (3)占⽤⼤量处理时间的任务可以定期将处理器时间让给其它任务;
  (4)可以随时停⽌任务;
  (5)可以分别设置各个任务的优先级以优化性能。
- 多线程开发的缺点：
  同样的 ,多线程也存在许多缺点 ,在考虑多线程时需要进⾏充分的考虑。多线程的主要缺点包括: 
  (1)等候使⽤共享资源时造成程序的运⾏速度变慢。这些共享资源主要是独占性的资源 ,如打印机等。
  (2)对线程进⾏管理要求额外的 CPU开销。线程的使⽤会给系统带来上下⽂切换的额外负担。当这种负担超过⼀定程度时,多线程的特点主要
  表现在其缺点上,⽐如⽤独⽴的线程来更新数组内每个元素。
  (3)线程的死锁。即较长时间的等待或资源竞争以及死锁等多线程症状。
  (4)对公有变量的同时读或写。当多个线程需要对公有变量进⾏写操作时,后⼀个线程往往会修改掉前⼀个线程存放的数据,从⽽使前⼀个线程
  的参数被修改;另外 ,当公⽤变量的读写操作是⾮原⼦性时,在不同的机器上,中断时间的不确定性,会导致数据在⼀个线程内的操作产⽣错误,从⽽产⽣莫名其妙的错误,⽽这种错误是程序员⽆法预知的。

- fixed模式线程池
  线程池里面的线程个数是固定不变的，一般是ThreadPool创建时根据当前机器的CPU核心数量进行指
  定。
  cached模式线程池
  线程池里面的线程个数是可动态增长的，根据任务的数量动态的增加线程的数量，但是会设置一个线程
  数量的阈值（线程过多的坏处上面已经讲过了），任务处理完成，如果动态增长的线程空闲了60s还没
  有处理其它任务，那么关闭线程，保持池中最初数量的线程即可。fixed模式线程池：
  线程池里面的线程个数是固定不变的，一般是ThreadPool创建时根据当前机器的CPU核心数量进行指
  定。
- cached模式线程池：
  线程池里面的线程个数是可动态增长的，根据任务的数量动态的增加线程的数量，但是会设置一个线程
  数量的阈值（线程过多的坏处上面已经讲过了），任务处理完成，如果动态增长的线程空闲了60s还没
  有处理其它任务，那么关闭线程，保持池中最初数量的线程即可。

> 两个版本的线程池，第一个版本自己定义的类型较多；第二个版本多采用C++新特性提供的类型

# 第一个版本

**线程池两个模式：**

```c++
//线程池支持的模式
enum class PoolMode
{
	MODE_FIXED,		//固定数量的线程
	MODE_CACHED,	//线程数量可以动态增长
};
```

**C++17提供了Any类型 可以接受任何参数，我们第一个版本手写一个Any类型用于接受任何参数：**

```c++
//定义一个Any类型 可以接受任何数据类型
//C++17中提供了Any类型 可以保存任何数据类型
class Any
{
public:
	Any() = default;
	~Any() = default;
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

	//构造成函数 可以接受任何数据类型
	template<typename T>
	Any(T data) :m_base(std::make_unique<Derive<T>>(data))
	{
	}

	template<typename T>
	T cast()		//用来提取保存的数据
	{
		//先将保存的基类转化为子类
		Derive<T>* dp = dynamic_cast<Derive<T>*>(m_base.get());
		if (dp == nullptr)
		{
			throw "type is wrong";
		}
		return dp->m_data;
	}

private:
	class Base		//基类类型
	{
	public:
		virtual ~Base() = default;
	};

	template<typename T>
	class Derive :public Base		//派生类类型
	{
	public:
		Derive(T data) :m_data(data)
		{
		}
		T m_data;	//可以保存所有数据类型
	};

private:
	//定义一个基类指针
	std::unique_ptr<Base> m_base;
};
```

**C++20提供了semaphore信号量 但是之前的版本没有提供 下面我们手写一个信号量：**

```c++
//定义一个信号量 Semaphore在C++20中已经提供
class Semaphore
{
public:
	//构造函数
	Semaphore(int limit = 0)
		:m_resLimit(limit)
		, m_isExit(false)
	{

	}

	~Semaphore()
	{
		m_isExit = true;
	}

	//获取一个信号量资源
	void wait()
	{
		if (m_isExit)
			return;
		std::unique_lock<std::mutex> lock(m_mtx);
		//等待信号量有资源，没有资源的话 阻塞当前线程
		m_cond.wait(lock, [&]()->bool {return m_resLimit > 0; });
		m_resLimit--;
	}

	//增加一个信号量资源
	void post()
	{
		if (m_isExit)
			return;
		std::unique_lock<std::mutex> lock(m_mtx);
		m_resLimit++;
		m_cond.notify_all();
	}

private:
	std::atomic_bool m_isExit;
	int m_resLimit;
	std::mutex m_mtx;
	std::condition_variable m_cond;
};
```

**线程类型类：**

```c++
//线程类型
class Thread
{
public:
	//线程函数对象类型
	using ThreadFunc = std::function<void(int)>;
	// 线程构造
	Thread(ThreadFunc func);
	// 线程析构
	~Thread();
	// 启动线程
	void start();

	//获取线程ID
	int getId()const;
private:
	ThreadFunc m_func;
	static int m_generateId;
	int m_threadId;		//保存线程id
};
```

**线程池类型：**

```c++
//线程池类型
class ThreadPool
{
public:
	//线程池构造
	ThreadPool();
	//线程池析构
	~ThreadPool();

	//设置线程池的工作模式
	void setMode(PoolMode mode);

	//设置task任务队列上线的阈值
	void setTaskQueMaxThrshHold(int threshhold);
	//给线程池提交任务
	Result submitTask(std::shared_ptr<Task> sp);

	//设置线程池cached模式下线程阈值
	void setThreadSizeThreshHold(int threshHold);

	//开启线程池
	void start(int initThreadSize = std::thread::hardware_concurrency());

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

private:
	//定义线程函数
	void threadFunc(int threadId);

	//检查pool的运行状态
	bool checkRunningState() const;
private:
	//std::vector<std::unique_ptr<Thread>> m_threads;	//线程列表
	std::unordered_map <int, std::unique_ptr<Thread>> m_threads;
	//初始的线程数量
	int m_initThreadSize;

	//记录当前线程池里面线程的总数量
	std::atomic_int m_curThreadSize;

	//线程数量上限阈值
	int m_threadSizeThreshHold;

	//记录空闲线程的数量
	std::atomic_int m_idleThreadSize;

	//任务队列
	std::queue<std::shared_ptr<Task>> m_taskque;

	//任务的数量
	std::atomic_int m_taskSize;
	//任务队列数量上限的阈值
	int m_taskqueMaxThresHold;

	//包装任务队列的线程安全
	std::mutex m_taskQueMtx;

	//表示任务队列不满
	std::condition_variable m_notFull;
	//表示任务队列不空
	std::condition_variable m_notEmpty;

	//表示等待线程资源全部回收
	std::condition_variable m_exitCond;

	//当前线程池的工作模式
	PoolMode m_poolMode;

	//表示当前线程池的启动状态
	std::atomic_bool m_isPoolRunning;
};
```

测试类型(main.cpp)：

```c++
#include "ThreadPool.h"

#include <thread>
#include <chrono>
#include <iostream>

using ulong = unsigned long long;

class MyTask:public Task
{
public:
	MyTask(int begin, int end)
		:m_begin(begin),
		m_end(end)
	{
	}
	Any run()
	{
		std::cout << "tid:" << std::this_thread::get_id()
			<< "begin..." << std::endl;
		//std::this_thread::sleep_for(std::chrono::seconds(2));
		ulong sum = 0;
		for (int i = m_begin; i <= m_end; i++)
		{
			sum += i;
		}
		std::cout << "tid:" << std::this_thread::get_id()
			<< "end..." << std::endl;
		return sum;
	}
private:
	int m_begin;
	int m_end;
};

int main()
{
	{
		ThreadPool pool;
		pool.setMode(PoolMode::MODE_CACHED);
		// 开始启动线程池
		pool.start(2);

		// linux上，这些Result对象也是局部对象，要析构的！！！
		Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
		Result res2 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
		pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
		pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
		pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));

		ulong sum1 = res1.get().cast<ulong>();
		std::cout << sum1 << std::endl; 
	} // 这里Result对象也要析构!!! 在vs下，条件变量析构会释放相应资源的

	std::cout << "main over!" << std::endl;
	getchar();
}
```

结果：

![](image/1.png)

# 第二个版本

**线程池支持的模式:**

```c++
enum class PoolMode
{
	MODE_FIXED,		//固定数量的线程
	MODE_CACHED,	//线程数量可以动态增长
};
```

**线程类型:**

```c++
class Thread
{
public:
	//线程函数对象类型
	using ThreadFunc = std::function<void(int)>;
	// 线程构造
	Thread(ThreadFunc func);
	// 线程析构
	~Thread() = default;
	// 启动线程
	void start();

	//获取线程ID
	int getId()const;
private:
	ThreadFunc m_func;
	static int m_generateId;
	int m_threadId;		//保存线程id
};
```

**测试类型(main.cpp)：**

```c++
#include "thread.h"

#include <future>
#include <functional>
#include <thread>
#include <chrono>

int sum1(int a, int b)
{
	std::this_thread::sleep_for(std::chrono::seconds(1));
	return a + b;
}

int sum2(int a, int b, int c)
{
	std::this_thread::sleep_for(std::chrono::seconds(1));
	return a + b + c;
}

int main()
{
	ThreadPool pool;
	pool.start(4);

	std::future<int> res1 = pool.submit(sum1, 100, 300);
	std::future<int> res2 = pool.submit(sum2, 1, 2, 3);

	std::cout << res1.get() << std::endl;
	std::cout << res2.get() << std::endl;
}
```

**测试结果：**

![](image/2.png)