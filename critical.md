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

....

Andom M. Coleman points out that "GL_ARRAY_BUFFER" is *not* one of the states that VAOs track,
but this binding is used by commands such as glVertexAttribPointer(...)

he says that "glVertexAttribPointer establishes a pointer that is relative to memory owned by whatever
is bound to GL_ARRAY_BUFFER. Also that this is a bad design

... Digestion ...

VAO stores offsets to the attributes of the currently bound buffer.
This glVertexAttributePointer(...) sets an offset from the bound VBO.
Which means that if a VAO "calls" glVertexAttributePointer, than
the GL_ARRAY_BUFFER needs to be bound before binding the VAO.

VBO SHOULD BE BOUND BEFORE VAO

... Digestion 2 ... (https://stackoverflow.com/questions/40652905/render-one-vao-containing-two-vbos)

vao and vbo should be a one to one relation.
we can change the buffer a vao uses, however this requires settings up the vao again
with glVertexAttribPointer to make the offsets w/ respect to the new bound vbo.

---

for loop always execute once
for ( i = 2; i < 2; i++ ) {
  std::cout << " i will print " << std::endl;
}

---

the dot product returns the relative angle between vectors, what about a directed angle counterclockwise from v1 to v2?
https://stackoverflow.com/questions/14066933/direct-way-of-computing-the-clockwise-angle-between-two-vectors/16544330#16544330

-- sound

https://homepage.ntu.edu.tw/~karchung/phonetics%20II%20page%20eight.htm
https://musiclab.chromeexperiments.com/Spectrogram/
http://salfordacoustics.co.uk/sound-waves/standing-waves

---

a great resource on pitch shifting

https://www.guitarpitchshifter.com/
