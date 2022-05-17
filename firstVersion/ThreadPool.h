#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>

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


class Result;
//任务抽象基类
class Task
{
public:
	Task();
	~Task() = default;

	void setResult(Result* res);
	void exec();
	//用户可以自定义任何任务类型，从T按实际继承，重写run方法，实现自定义任务 处理
	virtual Any run() = 0;

private:
	Result* m_result;
};

//提交任务到线程池完成后的返回值类型
class Result
{
public:
	Result(std::shared_ptr<Task> task, bool isValid = true);

	//setVal() 获取任务执行完的返回值
	void SetVal(Any any);

	//get() 用户调用这个方法获取Task的返回值
	Any get();

	~Result() = default;

private:
	std::shared_ptr<Task> m_task;	//指向对应任务返回值的任务对象
	Any m_any;						//存储任务的返回值
	Semaphore m_sem;				//线程通信信号量
	std::atomic_bool m_isValid;		//返回值是否有效
};

//线程池支持的模式
enum class PoolMode
{
	MODE_FIXED,		//固定数量的线程
	MODE_CACHED,	//线程数量可以动态增长
};

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

#endif // !THREADPOOL_H_
