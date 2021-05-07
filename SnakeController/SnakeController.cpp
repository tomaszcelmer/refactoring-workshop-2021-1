#include "SnakeController.hpp"

#include <algorithm>
#include <sstream>

#include "EventT.hpp"
#include "IPort.hpp"

namespace Snake
{
ConfigurationError::ConfigurationError()
    : std::logic_error("Bad configuration of Snake::Controller.")
{}

UnexpectedEventException::UnexpectedEventException()
    : std::runtime_error("Unexpected event received!")
{}

Controller::Controller(IPort& p_display_port, IPort& p_food_port, IPort& p_score_ort, std::string const& p_config)
    : m_display_port(p_display_port),
      m_food_port(p_food_port),
      m_score_port(p_score_ort)
{
    std::istringstream istr(p_config);
    char w, f, s, d;

    int width, height, length;
    int food_x, food_y;
    istr >> w >> width >> height >> f >> food_x >> food_y >> s;

    if (w == 'W' and f == 'F' and s == 'S') {
        m_map_dimension = std::make_pair(width, height);
        m_food_position = std::make_pair(food_x, food_y);

        istr >> d;
        switch (d) {
            case 'U':
                m_current_direction = Direction_UP;
                break;
            case 'D':
                m_current_direction = Direction_DOWN;
                break;
            case 'L':
                m_current_direction = Direction_LEFT;
                break;
            case 'R':
                m_current_direction = Direction_RIGHT;
                break;
            default:
                throw ConfigurationError();
        }
        istr >> length;

        while (length) {
            Segment seg;
            istr >> seg.x >> seg.y;
            seg.ttl = length--;

            m_segments.push_back(seg);
        }
    } else {
        throw ConfigurationError();
    }
}

void Controller::receive(std::unique_ptr<Event> e)
{
    try {
        auto const& timer_event = *dynamic_cast<EventT<TimeoutInd> const&>(*e);

        Segment const& current_head = m_segments.front();

        Segment new_head;
        new_head.x = current_head.x + ((m_current_direction & 0b01) ? (m_current_direction & 0b10) ? 1 : -1 : 0);
        new_head.y = current_head.y + (not (m_current_direction & 0b01) ? (m_current_direction & 0b10) ? 1 : -1 : 0);
        new_head.ttl = current_head.ttl;

        bool lost = false;

        for (auto segment : m_segments) {
            if (segment.x == new_head.x and segment.y == new_head.y) {
                m_score_port.send(std::make_unique<EventT<LooseInd>>());
                lost = true;
                break;
            }
        }

        if (not lost) {
            if (std::make_pair(new_head.x, new_head.y) == m_food_position) {
                m_score_port.send(std::make_unique<EventT<ScoreInd>>());
                m_food_port.send(std::make_unique<EventT<FoodReq>>());
            } else if (new_head.x < 0 or new_head.y < 0 or
                       new_head.x >= m_map_dimension.first or
                       new_head.y >= m_map_dimension.second) {
                m_score_port.send(std::make_unique<EventT<LooseInd>>());
                lost = true;
            } else {
                for (auto &segment : m_segments) {
                    if (not --segment.ttl) {
                        DisplayInd l_evt;
                        l_evt.x = segment.x;
                        l_evt.y = segment.y;
                        l_evt.value = Cell_FREE;

                        m_display_port.send(std::make_unique<EventT<DisplayInd>>(l_evt));
                    }
                }
            }
        }

        if (not lost) {
            m_segments.push_front(new_head);
            DisplayInd place_new_head;
            place_new_head.x = new_head.x;
            place_new_head.y = new_head.y;
            place_new_head.value = Cell_SNAKE;

            m_display_port.send(std::make_unique<EventT<DisplayInd>>(place_new_head));

            m_segments.erase(
                std::remove_if(
                    m_segments.begin(),
                    m_segments.end(),
                    [](auto const& segment){ return not (segment.ttl > 0); }),
                m_segments.end());
        }
    } catch (std::bad_cast&) {
        try {
            auto direction = dynamic_cast<EventT<DirectionInd> const&>(*e)->direction;

            if ((m_current_direction & 0b01) != (direction & 0b01)) {
                m_current_direction = direction;
            }
        } catch (std::bad_cast&) {
            try {
                auto received_food = *dynamic_cast<EventT<FoodInd> const&>(*e);

                bool requested_food_collided_with_snake = false;
                for (auto const& segment : m_segments) {
                    if (segment.x == received_food.x and segment.y == received_food.y) {
                        requested_food_collided_with_snake = true;
                        break;
                    }
                }

                if (requested_food_collided_with_snake) {
                    m_food_port.send(std::make_unique<EventT<FoodReq>>());
                } else {
                    DisplayInd clear_old_food;
                    clear_old_food.x = m_food_position.first;
                    clear_old_food.y = m_food_position.second;
                    clear_old_food.value = Cell_FREE;
                    m_display_port.send(std::make_unique<EventT<DisplayInd>>(clear_old_food));

                    DisplayInd place_new_food;
                    place_new_food.x = received_food.x;
                    place_new_food.y = received_food.y;
                    place_new_food.value = Cell_FOOD;
                    m_display_port.send(std::make_unique<EventT<DisplayInd>>(place_new_food));
                }

                m_food_position = std::make_pair(received_food.x, received_food.y);

            } catch (std::bad_cast&) {
                try {
                    auto requested_food = *dynamic_cast<EventT<FoodResp> const&>(*e);

                    bool requested_food_collided_with_snake = false;
                    for (auto const& segment : m_segments) {
                        if (segment.x == requested_food.x and segment.y == requested_food.y) {
                            requested_food_collided_with_snake = true;
                            break;
                        }
                    }

                    if (requested_food_collided_with_snake) {
                        m_food_port.send(std::make_unique<EventT<FoodReq>>());
                    } else {
                        DisplayInd place_new_food;
                        place_new_food.x = requested_food.x;
                        place_new_food.y = requested_food.y;
                        place_new_food.value = Cell_FOOD;
                        m_display_port.send(std::make_unique<EventT<DisplayInd>>(place_new_food));
                    }

                    m_food_position = std::make_pair(requested_food.x, requested_food.y);
                } catch (std::bad_cast&) {
                    throw UnexpectedEventException();
                }
            }
        }
    }
}

} // namespace Snake
