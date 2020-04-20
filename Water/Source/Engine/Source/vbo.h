#pragma once


class VBO
{
public:
	VBO(const void* data, unsigned int size, GLenum drawmode = GL_STATIC_DRAW);
	~VBO();

	void Bind() const;
	void Unbind() const;

	GLuint GetID() { return rendererID_; }
private:
	GLuint rendererID_;
};