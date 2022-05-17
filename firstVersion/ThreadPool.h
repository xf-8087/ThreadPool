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

//����һ��Any���� ���Խ����κ���������
//C++17���ṩ��Any���� ���Ա����κ���������
class Any
{
public:
	Any() = default;
	~Any() = default;
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

	//����ɺ��� ���Խ����κ���������
	template<typename T>
	Any(T data) :m_base(std::make_unique<Derive<T>>(data))
	{
	}

	template<typename T>
	T cast()		//������ȡ���������
	{
		//�Ƚ�����Ļ���ת��Ϊ����
		Derive<T>* dp = dynamic_cast<Derive<T>*>(m_base.get());
		if (dp == nullptr)
		{
			throw "type is wrong";
		}
		return dp->m_data;
	}

private:
	class Base		//��������
	{
	public:
		virtual ~Base() = default;
	};

	template<typename T>
	class Derive :public Base		//����������
	{
	public:
		Derive(T data) :m_data(data)
		{
		}
		T m_data;	//���Ա���������������
	};

private:
	//����һ������ָ��
	std::unique_ptr<Base> m_base;
};


//����һ���ź��� Semaphore��C++20���Ѿ��ṩ
class Semaphore
{
public:
	//���캯��
	Semaphore(int limit = 0)
		:m_resLimit(limit)
		, m_isExit(false)
	{

	}

	~Semaphore()
	{
		m_isExit = true;
	}

	//��ȡһ���ź�����Դ
	void wait()
	{
		if (m_isExit)
			return;
		std::unique_lock<std::mutex> lock(m_mtx);
		//�ȴ��ź�������Դ��û����Դ�Ļ� ������ǰ�߳�
		m_cond.wait(lock, [&]()->bool {return m_resLimit > 0; });
		m_resLimit--;
	}

	//����һ���ź�����Դ
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
//����������
class Task
{
public:
	Task();
	~Task() = default;

	void setResult(Result* res);
	void exec();
	//�û������Զ����κ��������ͣ���T��ʵ�ʼ̳У���дrun������ʵ���Զ������� ����
	virtual Any run() = 0;

private:
	Result* m_result;
};

//�ύ�����̳߳���ɺ�ķ���ֵ����
class Result
{
public:
	Result(std::shared_ptr<Task> task, bool isValid = true);

	//setVal() ��ȡ����ִ����ķ���ֵ
	void SetVal(Any any);

	//get() �û��������������ȡTask�ķ���ֵ
	Any get();

	~Result() = default;

private:
	std::shared_ptr<Task> m_task;	//ָ���Ӧ���񷵻�ֵ���������
	Any m_any;						//�洢����ķ���ֵ
	Semaphore m_sem;				//�߳�ͨ���ź���
	std::atomic_bool m_isValid;		//����ֵ�Ƿ���Ч
};

//�̳߳�֧�ֵ�ģʽ
enum class PoolMode
{
	MODE_FIXED,		//�̶��������߳�
	MODE_CACHED,	//�߳��������Զ�̬����
};

//�߳�����
class Thread
{
public:
	//�̺߳�����������
	using ThreadFunc = std::function<void(int)>;
	// �̹߳���
	Thread(ThreadFunc func);
	// �߳�����
	~Thread();
	// �����߳�
	void start();

	//��ȡ�߳�ID
	int getId()const;
private:
	ThreadFunc m_func;
	static int m_generateId;
	int m_threadId;		//�����߳�id
};

//�̳߳�����
class ThreadPool
{
public:
	//�̳߳ع���
	ThreadPool();
	//�̳߳�����
	~ThreadPool();

	//�����̳߳صĹ���ģʽ
	void setMode(PoolMode mode);

	//����task����������ߵ���ֵ
	void setTaskQueMaxThrshHold(int threshhold);
	//���̳߳��ύ����
	Result submitTask(std::shared_ptr<Task> sp);

	//�����̳߳�cachedģʽ���߳���ֵ
	void setThreadSizeThreshHold(int threshHold);

	//�����̳߳�
	void start(int initThreadSize = std::thread::hardware_concurrency());

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

private:
	//�����̺߳���
	void threadFunc(int threadId);

	//���pool������״̬
	bool checkRunningState() const;
private:
	//std::vector<std::unique_ptr<Thread>> m_threads;	//�߳��б�
	std::unordered_map <int, std::unique_ptr<Thread>> m_threads;
	//��ʼ���߳�����
	int m_initThreadSize;

	//��¼��ǰ�̳߳������̵߳�������
	std::atomic_int m_curThreadSize;

	//�߳�����������ֵ
	int m_threadSizeThreshHold;

	//��¼�����̵߳�����
	std::atomic_int m_idleThreadSize;

	//�������
	std::queue<std::shared_ptr<Task>> m_taskque;

	//���������
	std::atomic_int m_taskSize;
	//��������������޵���ֵ
	int m_taskqueMaxThresHold;

	//��װ������е��̰߳�ȫ
	std::mutex m_taskQueMtx;

	//��ʾ������в���
	std::condition_variable m_notFull;
	//��ʾ������в���
	std::condition_variable m_notEmpty;

	//��ʾ�ȴ��߳���Դȫ������
	std::condition_variable m_exitCond;

	//��ǰ�̳߳صĹ���ģʽ
	PoolMode m_poolMode;

	//��ʾ��ǰ�̳߳ص�����״̬
	std::atomic_bool m_isPoolRunning;
};

#endif // !THREADPOOL_H_
