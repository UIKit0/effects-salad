#include "mesh.h"
#include "init.h"

Mesh::Mesh() :
    vertexCount(0),
    indexCount(0),
    vao(0) {
    /* nothing */
}


Mesh::Mesh(int componentCount, const FloatList& verts) :
    vertexCount(verts.size()) {
    
    vao = ::InitVao(componentCount, verts);
}

Mesh::Mesh(int componentCount, 
            const FloatList& verts, 
            const IndexList& indices) :
        vertexCount(verts.size()),
        indexCount(indices.size()) {
    
    vao = ::InitVao(componentCount, verts, indices);
}

void 
Mesh::AddVertexAttribute(GLuint attrib, 
                            int componentCount, 
                            const FloatList& values) {
    if (!vao) {
        std::cerr << "Array object was not initialized" << std::endl;
    }
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 
                 sizeof(values[0]) * values.size(), 
                 &values[0], 
                 GL_STATIC_DRAW);

    glVertexAttribPointer(attrib, componentCount, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(attrib);
    
    pezCheck(glGetError() == GL_NO_ERROR, "Failed to add vertex attribute");
}

void 
Mesh::Bind() {
    glBindVertexArray(vao);
    pezCheck(vao != 0, "Invalid VAO in mesh.bind");
    pezCheck(glGetError() == GL_NO_ERROR, "mesh.bind failed");
}

