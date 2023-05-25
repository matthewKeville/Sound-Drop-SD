VAOs do not hold state.
I though that when a VAO is bound it serves a macro for opengl function calls.

I lost 30 or so minutes because I thought when I bind a VAO then bind a VBO,
the VAO "records" this action so that when I switch to the VAO the VBO gets
set to the current ARRAY_BUFFER. This is wrong, horribly wrong.

The VAO just stores attributes...

---

Massive HEADACHE with VAOs

https://stackoverflow.com/questions/21652546/what-is-the-role-of-glbindvertexarrays-vs-glbindbuffer-and-what-is-their-relatio

VAOs do not store data such as the current GL_ARRAY_BUFFER (set by glBindBuffer() ),
they can store the bound EBO (for instanced drawing however).

With that being said a GL_ARRAY_BUFFER has to be bound to
to use glVertexAttribPointer ?

Before I made sure this was the case, my code was bugged, and binding a vbo 
before glVertexAttribPointer and after glBindVertex Arrays resolved this error
