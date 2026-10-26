#include "Shader.hpp"

std::string Shader::read_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filepath << "\n";
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

unsigned int Shader::compile_shader(unsigned int type, const std::string& source) {
    // Allocate and compile the shader
    unsigned int id = glCreateShader(type);
    const char* src_ptr = source.c_str();
    glShaderSource(id, 1, &src_ptr, nullptr);
    glCompileShader(id);

    // Check for compilation errors
    int success = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        int length = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);

        std::vector<char> info_log(length);
        glGetShaderInfoLog(id, length, &length, info_log.data());

        std::cerr << "Shader compilation error ("
                  << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT") << "):\n"
                  << info_log.data() << "\n";
        glDeleteShader(id);
        return 0;
    }

    return id;
}

Shader::Shader(const std::string& vertex_path, const std::string& fragment_path) {
    std::string vert_source = read_file(vertex_path);
    std::string frag_source = read_file(fragment_path);

    unsigned int vert_shader = compile_shader(GL_VERTEX_SHADER, vert_source);
    unsigned int frag_shader = compile_shader(GL_FRAGMENT_SHADER, frag_source);

    // Link both shaders into a single pipeline Program
    program_id = glCreateProgram();
    glAttachShader(program_id, vert_shader);
    glAttachShader(program_id, frag_shader);
    glLinkProgram(program_id);

    // Check linking errors
    int success = 0;
    glGetProgramiv(program_id, GL_LINK_STATUS, &success);
    if (!success) {
        int length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &length);

        std::vector<char> info_log(length);
        glGetProgramInfoLog(program_id, length, &length, info_log.data());

        std::cerr << "Shader program linking error:\n" << info_log.data() << "\n";
    }

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
}

Shader::~Shader() {
    if (program_id != 0) {
        glDeleteProgram(program_id);
    }
}

void Shader::use() const {
    glUseProgram(program_id);
}

void Shader::set_float(const std::string& name, float value) const {
    int location = glGetUniformLocation(program_id, name.c_str());
    glUniform1f(location, value);
}

void Shader::set_vec2(const std::string& name, float x, float y) const {
    int location = glGetUniformLocation(program_id, name.c_str());
    glUniform2f(location, x, y);
}

void Shader::set_vec3(const std::string& name, float x, float y, float z) const {
    int location = glGetUniformLocation(program_id, name.c_str());
    glUniform3f(location, x, y, z);
}

void Shader::set_int(const std::string& name, int value) const {
    int location = glGetUniformLocation(program_id, name.c_str());
    glUniform1i(location, value);
}