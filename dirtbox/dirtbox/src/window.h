/**
 
 * @author Hunter Borlik
 * @brief 
 * @version 0.1
 * @date 2021-01-20
 * 
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_WINDOW_H_HEADER_GUARD
#define DIRTBOX_WINDOW_H_HEADER_GUARD

#include <string>
#include <memory>

namespace dirtbox {

class Event;

/**
 * @brief main thread run forever event processing loop
 * 
 */
int run(int argc, char *args[]);

// functions for messaging from other threads
void destroyWindow();
void setWindowPos(int32_t _x, int32_t _y);
void setWindowSize(uint32_t _width, uint32_t _height);
void setWindowTitle(const std::string& _title);
void toggleFullscreen();
void setMouseLock(bool _lock);

std::unique_ptr<Event> poll();
bool windowEventsEmpty();

}

#endif // DIRTBOX_WINDOW_H_HEADER_GUARD