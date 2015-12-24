/*
 * @file	singleton.h
 * @brief	单间模式的实现
 *
 * @author	yangliang
 * @version 1.0
 * @date	2014-3-24
*/
#ifndef _SINGLETON_H_
#define _SINGLETON_H_

#include <assert.h>
#include <memory>
using namespace std;

template <typename T>
class Singleton
{
public:
	//初始化函数
	Singleton(void)
	{
	}
	~Singleton(void)
	{

	}

	//返回实例
	static T& Instance(void)
	{
		if (NULL == _pInstance)
		{
			_pInstance = new T;
		}
		return (*_pInstance);
	}

	//返回实例指针
	static T* InstancePtr(void)
	{
		if (NULL == _pInstance)
		{
			_pInstance = new T;
		}
		return _pInstance;
	}

	static void DeleteObj()
	{
		if (NULL != _pInstance)
		{
			delete _pInstance;
			_pInstance = NULL;
		}
	}
protected:
	//用于保存实例指针
	static T* _pInstance;
};

template <typename T> T* Singleton<T>::_pInstance = NULL;
#endif