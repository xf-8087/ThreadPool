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
#include <thread>
#include <future>
#include <iostream>

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
	~Thread() = default;
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

	//�����̳߳�cachedģʽ���߳���ֵ
	void setThreadSizeThreshHold(int threshHold);

	//���̳߳��ύ����
	//ʹ�ÿɱ��ģ���� ��submitTask���Խ������������������������Ĳ���
	//Result submitTask(std::shared_ptr<Task> sp);
	template<typename Func, typename... Args>
	auto submit(Func&& func, Args&&... args)->std::future<decltype(func(args...))>
	{
		//������� �����������
		using RType = decltype(func(args...));
		auto task = std::make_shared<std::packaged_task<RType()>>(std::bind(std::forward<Func>(func),
			std::forward<Args>(args)...));

		std::future<RType> result = task->get_future();

		//��ȡ��
		std::unique_lock<std::mutex> lock(m_taskQueMtx);
		//�û��ύ��������ɳ���һ�룬�������ύ����ʧ��
		if (!m_notFull.wait_for(lock, std::chrono::seconds(1),
			[&]()->bool {return m_taskque.size() < (size_t)m_taskqueMaxThresHold; }))
		{
			//��ʾ�ȴ�һ�� ������Ȼû������
			std::cerr << "task queue is full, submit task failed." << std::endl;
			auto task = std::make_shared<std::packaged_task<RType()>>(
				[]()->RType {return RType(); });
			(*task)();
			return task->get_future();
		}

		//����п��࣬ ������������������
		m_taskque.emplace([task]() {(*task)(); });
		m_taskSize++;

		//��Ϊ�·������� ����������п϶���Ϊ���ˣ���m_notEmpty�������֪ͨ���Ͽ�����߳�ִ��ִ����Դ
		m_notEmpty.notify_all();

		// cachedģʽ ������ȽϽ��� ������С��������� 
		//��Ҫ�������������Ϳ����̵߳��������ж��Ƿ���Ҫ�����µ��̳߳���
		if (m_poolMode == PoolMode::MODE_CACHED && m_taskSize > m_idleThreadSize
			&& m_curThreadSize < m_threadSizeThreshHold)
		{
			std::cout << ">>> create new thread..." << std::endl;
			//�����µ��̶߳���
			auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
			int threadId = ptr->getId();
			m_threads.emplace(threadId, std::move(ptr));
			//�����߳�
			m_threads[threadId]->start();
			//�޸��̸߳�����صı���
			m_curThreadSize++;
			m_idleThreadSize++;
		}
		return result;
	}
	
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

	//Task���� ��������
	using Task = std::function<void()>;

	//�������
	std::queue<Task> m_taskque;

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
