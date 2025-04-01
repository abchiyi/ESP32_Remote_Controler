#ifndef RW_LOCK_H
#define RW_LOCK_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// 读写锁
typedef struct
{
    SemaphoreHandle_t mutex;  // 保护内部计数器
    SemaphoreHandle_t w_lock; // 写锁
    int r_count;              // 读计数器
} rwlock_t;

// 初始化读写锁
void rwlock_init(rwlock_t *rwlock)
{
    rwlock->mutex = xSemaphoreCreateMutex();
    rwlock->w_lock = xSemaphoreCreateMutex();
    rwlock->r_count = 0;
}

void rwlock_read_lock(rwlock_t *rwlock)
{
    xSemaphoreTake(rwlock->mutex, portMAX_DELAY);
    rwlock->r_count++;
    if (rwlock->r_count == 1) // 第一个读操作获取写锁以阻塞写入
        xSemaphoreTake(rwlock->w_lock, portMAX_DELAY);
    xSemaphoreGive(rwlock->mutex);
}

void rwlock_read_unlock(rwlock_t *rwlock)
{
    xSemaphoreTake(rwlock->mutex, portMAX_DELAY);
    rwlock->r_count--;
    if (rwlock->r_count == 0)
        xSemaphoreGive(rwlock->w_lock);
    xSemaphoreGive(rwlock->mutex);
}

void rwlock_write_lock(rwlock_t *rwlock)
{
    xSemaphoreTake(rwlock->w_lock, portMAX_DELAY);
}

void rwlock_write_unlock(rwlock_t *rwlock)
{
    xSemaphoreGive(rwlock->w_lock);
}

#endif
