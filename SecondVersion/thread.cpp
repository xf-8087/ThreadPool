#include "thread.h"

const int TASK_MAX_THRESHHOLD = INT32_MAX;
const int THREAD_MAX_THRESHHOLD = 1024;
const int THREAD_MAX_IDLE_TIME = 60;	//��λ����
int Thread::m_generateId = 0;

/*
=========Threadʵ��=============
*/
Thread::Thread(ThreadFunc func)
	:m_func(func)
	, m_threadId(m_generateId++)
{
}

void Thread::start()
{
	//����һ���߳���ִ��һ���̺߳���
	std::thread t(m_func, m_threadId);
	t.detach();	//�����ػ��߳�
}

int Thread::getId()const
{
	return m_threadId;
}


/*
========ThreadPoolʵ��==========
*/
ThreadPool::ThreadPool()
	:m_initThreadSize(0)
	, m_taskSize(0)
	, m_idleThreadSize(0)
	, m_curThreadSize(0)
	, m_taskqueMaxThresHold(TASK_MAX_THRESHHOLD)
	, m_threadSizeThreshHold(THREAD_MAX_THRESHHOLD)
	, m_poolMode(PoolMode::MODE_FIXED)
	, m_isPoolRunning(false)
{
}

ThreadPool::~ThreadPool()
{
	m_isPoolRunning = false;

	//�ȴ��̳߳��������е��̷߳��� �߳�������״̬ ����������ִ����
	std::unique_lock<std::mutex> lock(m_taskQueMtx);
	m_notEmpty.notify_all();
	m_exitCond.wait(lock, [&]()->bool {return m_threads.size() == 0; });
}

void ThreadPool::setMode(PoolMode mode)
{
	if (checkRunningState())return;
	m_poolMode = mode;
}

void ThreadPool::setTaskQueMaxThrshHold(int threshhold)
{
	if (checkRunningState()) return;
	m_taskqueMaxThresHold = threshhold;
}

void ThreadPool::setThreadSizeThreshHold(int threshHold)
{
	if (checkRunningState()) return;
	if (m_poolMode == PoolMode::MODE_CACHED)
		m_threadSizeThreshHold = threshHold;
}

void ThreadPool::threadFunc(int threadId)
{
	auto lastTime = std::chrono::high_resolution_clock().now();

	//�����������ִ����ɣ��̳߳ز��ܻ��������߳���Դ
	while (true)
	{
		Task task;
		{
			//��ȡ��
			std::unique_lock<std::mutex> lock(m_taskQueMtx);
			std::cout << "tid:" << std::this_thread::get_id() << "���ڳ��Ի�ȡ����" << std::endl;
			// cachedģʽ�£��п����Ѿ������˺ܶ���̣߳����ǿ���ʱ�䳬��60s��Ӧ�ðѶ�����߳�
			// �������յ�������initThreadSize_�������߳�Ҫ���л��գ�
			// ��ǰʱ�� - ��һ���߳�ִ�е�ʱ�� > 60s

			// ÿһ���з���һ��   ��ô���֣���ʱ���أ������������ִ�з���
			// �� + ˫���ж�
			while (m_taskque.size() == 0)
			{
				//�̳߳�Ҫ���� �����߳���Դ
				if (!m_isPoolRunning)
				{
					m_threads.erase(threadId);
					std::cout << "thread id: " << std::this_thread::get_id << " exit" << std::endl;
					m_exitCond.notify_all();
					return;//�̺߳������� 
				}

				if (m_poolMode == PoolMode::MODE_CACHED)
				{
					if (std::cv_status::timeout ==
						m_notEmpty.wait_for(lock, std::chrono::seconds(1)))
					{
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
						if (dur.count() >= THREAD_MAX_IDLE_TIME
							&& m_curThreadSize > m_initThreadSize)
						{
							// ��ʼ���յ�ǰ�߳�
							// ��¼�߳���������ر�����ֵ�޸�
							// ���̶߳�����߳��б�������ɾ��   û�а취 threadFunc��=��thread����
							// threadid => thread���� => ɾ��
							m_threads.erase(threadId); // std::this_thread::getid()
							m_curThreadSize--;
							m_idleThreadSize--;

							std::cout << "threadid:" << std::this_thread::get_id() << " exit!"
								<< std::endl;
							return;
						}
					}
				}
				else
				{
					//�ȴ�empty����
					m_notEmpty.wait(lock);
				}
			}
			m_idleThreadSize--;
			std::cout << "tid:" << std::this_thread::get_id() << "��ȡ����ɹ�" << std::endl;

			//�����������ȡ������
			task = m_taskque.front();
			m_taskque.pop();
			m_taskSize--;

			//�����Ȼ������ ��֪ͨ�����߳�
			if (m_taskque.size() > 0)
			{
				m_notEmpty.notify_all();
			}
			//ȡ��һ���������֪ͨ ֪ͨ���Լ����ύ����
			m_notFull.notify_all();
		}//�����ŵ�
		
		//��ǰ�̸߳���ִ���������
		if (task != nullptr)
		{
			task();
		}
		m_idleThreadSize++;
		lastTime = std::chrono::high_resolution_clock().now();
	}
}

void ThreadPool::start(int initThreadSize)
{
	//�����̳߳�����״̬
	m_isPoolRunning = true;
	
	//��¼��ʼ�̳߳صĸ���
	m_curThreadSize = initThreadSize;
	m_initThreadSize = initThreadSize;

	//�����̶߳���
	for (int i = 0; i < m_initThreadSize; i++)
	{
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int	threadId = ptr->getId();
		m_threads.emplace(threadId, std::move(ptr));
	}

	for (int i = 0; i < m_initThreadSize; i++)
	{
		m_threads[i]->start();
		m_idleThreadSize++;
	}
}


bool ThreadPool::checkRunningState()const
{
	return m_isPoolRunning;
}