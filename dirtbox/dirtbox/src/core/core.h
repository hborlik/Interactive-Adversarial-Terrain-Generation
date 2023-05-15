/**
 * @brief 
 * @date 2021-05-14
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_CORE_H
#define DIRTBOX_CORE_H

#include <core/event.h>
#include <core/task_manager.h>

namespace dirtbox {

class Core {
public:
    TaskManager& getTaskManager() {return tm;}

    ThreadEvent<uint32_t> FrameEvent;

    static Core& Get() {
        static Core core{};
        return core;
    }

private:
    TaskManager tm;
};
    
}

#endif // DIRTBOX_CORE_H