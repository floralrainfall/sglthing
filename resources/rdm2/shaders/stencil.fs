uniform vec4 stencil_color;

out vec4 FragColor;
out vec4 BrightColor;

void main()
{
    FragColor = stencil_color;
    BrightColor = stencil_color;
}
