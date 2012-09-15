uniform mat4 mvp_matrix;
attribute vec4 a_position;
attribute vec4 a_color;
uniform float a_point_size;
varying vec4 v_color;

void main()
{
    v_color = a_color;
    gl_PointSize = a_point_size;
    gl_Position = mvp_matrix * a_position;
}