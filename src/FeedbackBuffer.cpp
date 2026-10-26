#include "FeedbackBuffer.hpp"

FeedbackBuffer::FeedbackBuffer(int width, int height)
    : m_width(width), m_height(height) {
    setup_buffers();
}

FeedbackBuffer::~FeedbackBuffer() {
    destroy_buffers();
}

void FeedbackBuffer::destroy_buffers() {
    if (m_fbo[0] != 0) {
        glDeleteFramebuffers(2, m_fbo);
        glDeleteTextures(2, m_tex);
        m_fbo[0] = m_fbo[1] = 0;
        m_tex[0] = m_tex[1] = 0;
    }
}

void FeedbackBuffer::setup_buffers() {
    glGenFramebuffers(2, m_fbo);
    glGenTextures(2, m_tex);

    // Set 2 buffers
    for (int i = 0; i < 2; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo[i]);

        glBindTexture(GL_TEXTURE_2D, m_tex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex[i], 0);

        // Render
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void FeedbackBuffer::resize(int new_width, int new_height) {
    if (new_width <= 0 || new_height <= 0) return;
    if (new_width == m_width && new_height == m_height) return;

    m_width = new_width;
    m_height = new_height;

    // Destroy existing buffers and reallocate with new dimensions
    destroy_buffers();
    setup_buffers();
}

void FeedbackBuffer::bind_write() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo[m_write_index]);
    glViewport(0, 0, m_width, m_height);
}

void FeedbackBuffer::bind_read_texture(unsigned int texture_unit) {
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(GL_TEXTURE_2D, m_tex[m_read_index]);
}

void FeedbackBuffer::swap() {
    std::swap(m_read_index, m_write_index);
}

unsigned int FeedbackBuffer::get_current_texture() const {
    return m_tex[m_write_index];
}