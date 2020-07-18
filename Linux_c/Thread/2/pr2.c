#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
 
#define N 5   //哲学家数量 
 
sem_t chopsticks[N];//设置5种信号量，有5种不同类型的资源，每一种有1个,这样便于理解，因为每个哲学家需要的资源不同
 
pthread_mutex_t mutex;//定义互斥锁
 
int philosophers[N] = {0, 1, 2, 3, 4};//代表5个哲学家的编号
 
//延迟判断
void delay (int len) 
{
	int i = rand() % len;
	int x;
	while (i > 0) 
	{
		x = rand() % len;
		while (x > 0) 
		{
			x--;
		}
		i--;
	}
}
 
void *philosopher (void* arg) 
{
	int i = *(int *)arg;
	int left = i;//左筷子的编号和哲学家的编号相同
	int right = (i + 1) % N;//右筷子的编号为哲学家编号+1
	while (1)
	 {
		printf("哲学家%d正在思考问题\n", i);
		delay(60000);
		
		printf("哲学家%d饿了\n", i);
 
		pthread_mutex_lock(&mutex);//加锁，进入临界区 
 
		sem_wait(&chopsticks[left]);//此时这个哲学家左筷子的信号量-1之后>=0时，表示能继续执行。去拿左边筷子 
		printf("哲学家%d拿起了%d号筷子,现在只有一支筷子,不能进餐\n", i, left);
		sem_wait(&chopsticks[right]);//去拿右边筷子 
		printf("哲学家%d拿起了%d号筷子\n", i, right);
 
		pthread_mutex_unlock(&mutex);//解锁，退出临界区 
 
		printf("哲学家%d现在有两支筷子,开始进餐\n", i);
		delay(60000);
		sem_post(&chopsticks[left]);//放下左边筷子 
		printf("哲学家%d放下了%d号筷子\n", i, left);
		sem_post(&chopsticks[right]);//放下右边筷子 
		printf("哲学家%d放下了%d号筷子\n", i, right);
	}
}
 
int main (int argc, char **argv) 
{
	    int i;
        srand(time(NULL));
	    pthread_t philo[N];

        //信号量初始化
        for(i=0;i<N;i++)
        {
            sem_init(&chopsticks[i],0,1);
        }				
        pthread_mutex_init(&mutex,NULL);
        //创建线程
	for(i=0;i<N;i++)
        {
            pthread_create(&philo[i],NULL,philosopher,&philosophers[i]);
        }
	//挂起线程		
        for(i=0;i<N;i++)
        {
            pthread_join(philo[i],NULL);
        }
        //销毁信号量
        for(i=0;i<N;i++)
        {
            sem_destroy(&chopsticks[i]);
        }
        pthread_mutex_destroy(&mutex);
        return 0;		
}  	
