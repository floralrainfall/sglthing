out vec4 FragColor;
  
in vec2 texcoord;

uniform sampler2D image;

void main()
{
    FragColor = texture(image, texcoord);
}