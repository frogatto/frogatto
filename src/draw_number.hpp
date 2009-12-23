#ifndef DRAW_NUMBER_HPP_INCLUDED
#define DRAW_NUMBER_HPP_INCLUDED

namespace graphics {
class blit_queue;
}

void queue_draw_number(graphics::blit_queue& q, int number, int places, int xpos, int ypos);
void draw_number(int number, int places, int xpos, int ypos);

#endif
