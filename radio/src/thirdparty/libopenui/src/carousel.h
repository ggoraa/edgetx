/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   libopenui - https://github.com/opentx/libopenui
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#pragma once

#include <vector>
#include "button.h"

class CarouselItem
{
  public:
    CarouselItem(Window * front, Window * back):
      front(front),
      back(back)
    {
    }

    void setSelectHandler(std::function<void()> handler)
    {
      selectHandler = std::move(handler);
    }

    virtual ~CarouselItem() = default;

  public:
    Window * front;
    Window * back;
    std::function<void()> selectHandler;
};

class CarouselWindow: public Window
{
  friend class Carousel;

  public:
    CarouselWindow(Window * parent, const rect_t & rect, uint8_t count):
      Window(parent, rect, NO_SCROLLBAR),
      count(count)
    {
    }

    void deleteLater(bool detach = true, bool trash = true) override // NOLINT(google-default-arguments)
    {
      if (_deleted)
        return;

      clear();
      Window::deleteLater(detach, trash);
    }

    void addItem(CarouselItem * item)
    {
      items.push_back(item);
      update();
    }

    void clear()
    {
      // children will be deleted later (front and back)
      children.clear();

      for (auto & item: items) {
        item->front->deleteLater();
        item->back->deleteLater();
        delete item;
      }
      items.clear();
    }

    void select(int index, bool scroll = true)
    {
      selection = index;
      if (selection >= 0) {
        auto item = items[selection];
        if (item->selectHandler) {
          item->selectHandler();
        }
      }
      update();
    }

  protected:
    std::vector<CarouselItem *> items;
    int selection = 0;
    unsigned count;
    void update();
};

class Carousel: public Window
{
  public:
    Carousel(Window * parent, const rect_t & rect):
      Window(parent, rect, NO_SCROLLBAR)
    {
    }

#if defined(DEBUG_WINDOWS)
    std::string getName() const override
    {
      return "Carousel";
    }
#endif

    void addItem(CarouselItem * item)
    {
      body->addItem(item);
    }

    void clear()
    {
      body->clear();
    }

    void select(int index)
    {
      body->select(index);
      previousButton->enable(index > 0);
      nextButton->enable(index < (int)body->items.size() - 1);
    }

    int getSelection() const
    {
      return body->selection;
    }

#if defined(HARDWARE_KEYS)
    void onEvent(event_t event) override;
#endif

  protected:
    Button * previousButton = nullptr;
    Button * nextButton = nullptr;
    CarouselWindow * body = nullptr;
};

