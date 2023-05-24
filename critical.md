VAOs do not hold state.
I though that when a VAO is bound it serves a macro for opengl function calls.

I lost 30 or so minutes because I thought when I bind a VAO then bind a VBO,
the VAO "records" this action so that when I switch to the VAO the VBO gets
set to the current ARRAY_BUFFER. This is wrong, horribly wrong.

The VAO just stores attributes...
