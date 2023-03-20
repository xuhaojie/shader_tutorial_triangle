// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

GLuint load_bmp_texture(const char * imagepath){

	// Data read from the header of the BMP file
	unsigned char header[54];
	unsigned int dataPos;
	unsigned int imageSize;
	unsigned int width, height;
	// Actual RGB data
	unsigned char * data;

	// Open the file
	FILE * file = fopen(imagepath,"rb");
	if (!file){
		printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", imagepath);
		getchar();
		return 0;
	}

	// Read the header, i.e. the 54 first bytes

	// If less than 54 bytes are read, problem
	if ( fread(header, 1, 54, file)!=54 ){ 
		printf("Not a correct BMP file\n");
		fclose(file);
		return 0;
	}

	// A BMP files always begins with "BM"
	if ( header[0]!='B' || header[1]!='M' ){
		printf("Not a correct BMP file\n");
		fclose(file);
		return 0;
	}

	// Make sure this is a 24bpp file
	if ( *(int*)&(header[0x1E])!=0  || *(int*)&(header[0x1C])!=24 ) 
	{
		printf("Not a correct BMP file\n");
		fclose(file);
		return 0;
	}

	// Read the information about the image
	dataPos    = *(int*)&(header[0x0A]);
	imageSize  = *(int*)&(header[0x22]);
	width      = *(int*)&(header[0x12]);
	height     = *(int*)&(header[0x16]);

	// Some BMP files are misformatted, guess missing information
	if (imageSize==0)    imageSize=width*height*3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos==0)      dataPos=54; // The BMP header is done that way

	// Create a buffer
	data = new unsigned char [imageSize];

	// Read the actual data from the file into the buffer
	fread(data,1,imageSize,file);

	// Everything is in memory now, the file can be closed.
	fclose (file);

	// Create one OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);
	
	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

	// OpenGL has now copied the data. Free our own version
	delete [] data;

	// Poor filtering, or ...
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 

	// ... nice trilinear filtering ...
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	// ... which requires mipmaps. Generate them automatically.
	glGenerateMipmap(GL_TEXTURE_2D);

	// Return the ID of the texture we just created
	return textureID;
}

GLuint load_shader(GLuint type, const char * file_path){

	// Read the Vertex Shader code from the file
	FILE* file = fopen(file_path, "rb");
	if(NULL == file){
		printf("Impossible to open %s.\n", file_path);
		return 0;
	}
	fseek(file, 0 , SEEK_END);
	const int file_size = ftell(file);
	printf("shader file %s size: %d\n", file_path, file_size);
	fseek(file, 0 , SEEK_SET);
	char* shader_code = (char* )malloc(file_size+1);
	if(NULL == shader_code){
		printf("Impossible to alloc memory for shader coed .\n");
		fclose(file);
		return 0;
	}
	const size_t read_size = fread(shader_code, file_size, 1, file);
	shader_code[file_size] = '\0';
	
	// Create the shaders
	GLuint shaderID = glCreateShader(type);
	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader: %s\n", file_path);
	char const * sourcePointer = shader_code;
	glShaderSource(shaderID, 1, &sourcePointer , NULL);
	glCompileShader(shaderID);
	free(shader_code);
	// Check Vertex Shader
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		char* error_message = (char* )malloc(InfoLogLength+1);
		if(NULL == error_message){
			printf("Impossible to alloc memory for shader coed .\n");
			return 0;
		}		
		glGetShaderInfoLog(shaderID, InfoLogLength, NULL, error_message);
		printf("%s\n", error_message);
		free(error_message);
	}
	return shaderID;
}

GLFWwindow* create_window(int width, int height, const char* title) {
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	return window;
}

struct SceneContext {
	GLuint vbo;
	GLuint vao;
	GLuint program;
};

static const GLfloat g_vertex_buffer_data[] = {
	-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	 0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
	 0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f,
};

int setup_scene(SceneContext* scene_context, const char* vertexShader, const char* fragmentShader) {

	// 创建VBO
	glGenBuffers(1, &scene_context->vbo);

	// 绑定VBO
	glBindBuffer(GL_ARRAY_BUFFER, scene_context->vbo);

	// 传输数据到VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	// 创建VAO
	glGenVertexArrays(1, &scene_context->vao);

	// 绑定VBO
	glBindVertexArray(scene_context->vao);
	
	printf("Reading vertex shader file %s\n", vertexShader);
	GLuint vertexShaderId = load_shader(GL_VERTEX_SHADER, vertexShader);

	printf("Reading fragment shader file %s\n", fragmentShader);
	GLuint fragmentShaderId = load_shader(GL_FRAGMENT_SHADER, fragmentShader);

	// Link the program
	printf("Linking program\n");
	scene_context->program =  glCreateProgram();

	glAttachShader(scene_context->program, vertexShaderId);
	glAttachShader(scene_context->program, fragmentShaderId);
	glLinkProgram(scene_context->program);

	// Check the program
	GLint Result = GL_FALSE;
	int InfoLogLength;
	glGetProgramiv(scene_context->program, GL_LINK_STATUS, &Result);
	glGetProgramiv(scene_context->program, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		char* error_message = (char* )malloc(InfoLogLength+1);
		if(NULL == error_message){
			printf("Impossible to alloc memory for shader coed .\n");
			return 0;
		}				
		glGetProgramInfoLog(scene_context->program, InfoLogLength, NULL, error_message);
		printf("%s\n", error_message);
	}

	glDetachShader(scene_context->program, vertexShaderId);
	glDetachShader(scene_context->program, fragmentShaderId);
	glDeleteShader(fragmentShaderId);
	glDeleteShader(fragmentShaderId);

	glUseProgram(scene_context->program);

	// 设置顶点位置属性
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		8 * sizeof(GL_FLOAT),                  // stride
		(void*)0            // array buffer offset
	);

	// 设置顶点颜色属性
	glVertexAttribPointer(
		1,                  // attribute 0. No particular reason for 1, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		8 * sizeof(GL_FLOAT),                  // stride
		(void*)(3 * sizeof(float))            // array buffer offset
	);

	// 设置顶点UV属性
	glVertexAttribPointer(
		2,                  // attribute 0. No particular reason for 1, but must match the layout in the shader.
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		8 * sizeof(GL_FLOAT),                  // stride
		(void*)(6 * sizeof(float))            // array buffer offset
	);

	// 启用顶点属性，在glVertexAttribPointer之前之后调用都可以
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	// Load Texture
	const char* image_path = "texture.bmp";
	printf("Reading image %s\n", image_path);
	GLuint textureID = load_bmp_texture(image_path);
	glBindTexture(GL_TEXTURE_2D, textureID);
	return 0;
}

int destroy_scene(SceneContext* scene_context) {

	// 禁用顶点属性
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	// Cleanup VBO
	glDeleteBuffers(1, &scene_context->vbo);
	// Cleanup VAO
	glDeleteVertexArrays(1, &scene_context->vao);
	// Cleanup Program
	glDeleteProgram(scene_context->program);

	return 0;
}

void update_vbo(GLuint vbo) {
	// 绑定VBO,如果已经绑定则无需重复绑定
	//glBindBuffer(GL_ARRAY_BUFFER_ARB, vbo);

	// 映射VBO
	float* ptr = (float*)glMapBuffer(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);

	// 如果指针为空（映射后的），更新VBO
	if (ptr)
	{
		// 更新位置信息
		float timeValue = glfwGetTime();

		float color_delta = 0.25*cos(timeValue*3.14) + 0.75;

		for (int t = 0; t < 3; t++) {
			int index = t * 8 + 3;
			ptr[index++] = g_vertex_buffer_data[index] * color_delta;
			ptr[index++] = g_vertex_buffer_data[index] * color_delta;
			ptr[index] = g_vertex_buffer_data[index] * color_delta;
		}		
		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB); // 使用之后解除映射
	}
}

void update_uniform(GLuint program) {

	// Update uniform
	float timeValue = glfwGetTime();
	int uniform_location = glGetUniformLocation(program, "time_value");
	glUniform1f(uniform_location, timeValue);
}

int render_scene(SceneContext* scene_context) {
	// 更新_vbo
	update_vbo(scene_context->vbo);

	// 更新uniform
	update_uniform(scene_context->program);
	
	// Draw the triangles!
	glDrawArrays(GL_TRIANGLES, 0, 3); // 3 indices starting at 0 -> 1 triangle

	return 0;
}

int main( void )
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

	GLFWwindow* window = create_window(1024, 768, "Shader Tutorial --- Triangle");
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		getchar();
		glfwTerminate();
		return -1;
	}
	// 执行这句后，OpenGL环境才就绪
	glfwMakeContextCurrent(window);

	int nrAttributes;
	// 获取系统最大支持的顶点属性的数量，通常为16
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
	printf("Maximum nr of vertex attributes supported: %d\n", nrAttributes);

	// 初始化GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// 确保后续可以检测到ESC键被按下, Windows系统好像不需要?
	//glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// 设置背景清除颜色为深蓝
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	SceneContext scene_context;

	// 创建场景
	if (setup_scene(&scene_context, "vertex_shader.txt", "fragment_shader.txt") < 0) {
		fprintf(stderr, "Failed to setup scene\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// 渲染循环
	do{
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT);

		// 渲染场景
		render_scene(&scene_context);

		// Swap buffers
		glfwSwapBuffers(window);

		// 查询事件
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 );
	
	// 销毁场景
	destroy_scene(&scene_context);

	// 销毁窗口
	glfwDestroyWindow(window);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

