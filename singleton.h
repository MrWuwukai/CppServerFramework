#ifndef _SINGLETON_H_
#define _SINGLETON_H_

namespace Framework {

    // ģ����Singleton������ʵ�ֵ���ģʽ
    // T��Ҫʵ�ֵ����������ͣ�X��Ĭ��ģ��������ͣ�һ�㲻�ã�����Ĭ��Ϊvoid����N����һ��Ĭ��ģ�������һ�㲻�ã�����Ĭ��Ϊ0��
    template<class T, class X = void, int N = 0>
    class Singleton {
    public:
        // ��̬��Ա���������ڻ�ȡ���������ָ��
        static T* GetInstance() {
            // ����һ����̬��T���Ͷ���v����֤�ڳ�������������ֻ��һ��ʵ��
            static T v;
            // ���ظõ��������ָ��
            return &v;
        }
    };

    // ģ����SingletonPtr������ʵ�ִ�����ָ��ĵ���ģʽ
    // T��Ҫʵ�ֵ����������ͣ�X��Ĭ��ģ��������ͣ�һ�㲻�ã�����Ĭ��Ϊvoid����N����һ��Ĭ��ģ�������һ�㲻�ã�����Ĭ��Ϊ0��
    template<class T, class X = void, int N = 0>
    class SingletonPtr {
    public:
        // ��̬��Ա���������ڻ�ȡ�������������ָ��
        static std::shared_ptr<T> GetInstance() {
            // ����һ����̬��std::shared_ptr<T>���Ͷ���v��ʹ��new����T���͵�ʵ������������������
            static std::shared_ptr<T> v(new T);
            // ���ظõ������������ָ��
            return v;
        }
    };

}

#endif