#pragma once

#include <glad/glad.h>
#include <iostream>

class FeedbackBuffer {
public:
    FeedbackBuffer(int width, int height);
    ~FeedbackBuffer();

    // no copy or assignment allowed
    FeedbackBuffer(const FeedbackBuffer&) = delete;
    FeedbackBuffer& operator=(const FeedbackBuffer&) = delete;

    void resize(int new_width, int new_height);
    void swap();
    unsigned int get_current_texture() const;

    // Bind current render target to offscreen framebuffer
    void bind_write();

    // Bind previous frame texture for the shader to read as an input uniform
    void bind_read_texture(unsigned int texture_unit = 0);

private:
    int m_width;
    int m_height;

    unsigned int m_fbo[2] = {0, 0};
    unsigned int m_tex[2] = {0, 0};

    int m_read_index  = 0;
    int m_write_index = 1;

    void setup_buffers();
    void destroy_buffers();
};