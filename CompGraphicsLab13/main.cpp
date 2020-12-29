#include <GL/glew.h>
#include <gl/GL.h>   // GL.h header file    
#include <gl/GLU.h> // GLU.h header file     
#include <gl/freeglut.h>
#include <gl/glaux.h>
#include <glm/trigonometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "glm/gtx/transform.hpp"
#include <iostream>
#include "GL/SOIL.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include "GLShader.h"
#include "Light.h"
#include "Material.h"

using namespace std;

GLShader current;
// Попиксельное вычисление освещения
// Один источник света
GLShader glShader_tex;
GLShader glShader_tex_tex;
GLShader glShader_col_tex;
// Два источника света
GLShader glShader_tex2;
GLShader glShader_tex_tex2;
GLShader glShader_col_tex2;

// Повершинное вычисление освещения
// Один источник света
GLShader glShader_tex_vert;
GLShader glShader_tex_tex_vert;
GLShader glShader_col_tex_vert;
// Два источника света
GLShader glShader_tex_vert2;
GLShader glShader_tex_tex_vert2;
GLShader glShader_col_tex_vert2;

GLint Unif_matrix;

glm::mat4 Matrix_projection;

GLuint VBO_position, VBO_texcoord, VBO_normal, EBO;

bool pointLight2On = false;
bool is_vertex_lightning = false;

enum class PaintType
{
	COLOR,
	TEXTURE,
	TEXTURE_TEXTURE,
	COLOR_TEXTURE
};

class Model
{
public:

	vector<glm::vec3> vertices_m = vector<glm::vec3>();
	vector<glm::vec2> tex_coords = vector<glm::vec2>();
	vector<glm::vec3> normals = vector<glm::vec3>();
	vector<GLint> indices = vector<GLint>();

	glm::vec3 translate_model = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 scale_model = glm::vec3(1.0f, 1.0f, 1.0f);
	float rotate_x = 0.0f;
	float rotate_y = 0.0f;
	float rotate_z = 0.0f;
	Material material = new_material(glm::vec4(0.2, 0.2, 0.2, 1.0), // ambient
		glm::vec4(0.7, 0.7, 0.7, 1.0), // diffuse
		glm::vec4(0.4, 0.4, 0.4, 1.0), // specular
		glm::vec4(0.1, 0.1, 0.1, 1.0), // emission
		0.1 * 128, // shininess
		glm::vec4(0.7, 0.0, 0.7, 1.0)); // color
	PaintType type_coloring = PaintType::COLOR;
	GLuint texture1;
	GLuint texture2;

	Model() {}

	Model(char* path)
	{
		loadModel(path);
	}

	// Загрузка модели
	void loadModel(string path)
	{
		Assimp::Importer import;
		const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			cout << "ERROR::ASSIMP::" << import.GetErrorString() << endl;
			return;
		}
		string directory = path.substr(0, path.find_last_of('/'));

		processNode(scene->mRootNode, scene);
	}

	// Обработка узлов
	void processNode(aiNode* node, const aiScene* scene)
	{
		// Обрабатываем все меши (если они есть) у выбранного узла
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			processMesh(mesh, scene);
		}
		// И проделываем то же самое для всех дочерних узлов
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene);
		}
	}

	void processMesh(aiMesh* mesh, const aiScene* scene)
	{
		// Цикл по всем вершинам меша
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			glm::vec3 vector; // объявляем промежуточный вектор, т.к. Assimp использует свой собственный векторный класс, который не преобразуется напрямую в тип glm::vec3, поэтому сначала мы передаем данные в этот промежуточный вектор типа glm::vec3

			// Координаты
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertices_m.push_back(vector);

			// Нормали
			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			normals.push_back(vector);

			// Текстурные координаты
			if (mesh->mTextureCoords[0]) // если меш содержит текстурные координаты
			{
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				tex_coords.push_back(vec);
			}
			else
				tex_coords.push_back(glm::vec2(0.0f, 0.0f));
		}

		GLint last_ind = indices.size();
		// Теперь проходимся по каждой грани меша (грань - это треугольник меша) и извлекаем соответствующие индексы вершин
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			// Получаем все индексы граней и сохраняем их в векторе indices
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j] + last_ind);
		}
	}
};

vector<GLuint> VBO_vertexes;
vector<GLuint> VBO_textures;
vector<GLuint> VBO_normales;
vector<GLuint> EBO_indexes;
vector<Model> models;

//! Проверка ошибок OpenGL, если есть то вывод в консоль тип ошибки 
void checkOpenGLerror(string str)
{
	GLenum errCode;
	if ((errCode = glGetError()) != GL_NO_ERROR)
		std::cout << "OpenGl error! - " << gluErrorString(errCode) << str << "\n";
}

//! Инициализация шейдеров 
void initShader()
{
	glShader_tex.loadFiles("shaders/pixels/vertex_light.c", "shaders/pixels/fragment_blinn_tex.c");
	glShader_tex_tex.loadFiles("shaders/pixels/vertex_light.c", "shaders/pixels/fragment_blinn_tex_tex.c");
	glShader_col_tex.loadFiles("shaders/pixels/vertex_light.c", "shaders/pixels/fragment_blinn_tex_color.c");
	
	glShader_tex2.loadFiles("shaders/pixels/vertex_light2.c", "shaders/pixels/fragment_blinn_tex2.c");
	glShader_tex_tex2.loadFiles("shaders/pixels/vertex_light2.c", "shaders/pixels/fragment_blinn_tex_tex2.c");
	glShader_col_tex2.loadFiles("shaders/pixels/vertex_light2.c", "shaders/pixels/fragment_blinn_tex_color2.c");
	
	glShader_tex_vert.loadFiles("shaders/vertex/vertex_light_vert.c", "shaders/vertex/fragment_blinn_tex_vert.c");
	glShader_tex_tex_vert.loadFiles("shaders/vertex/vertex_light_vert.c", "shaders/vertex/fragment_blinn_tex_tex_vert.c");
	glShader_col_tex_vert.loadFiles("shaders/vertex/vertex_light_vert.c", "shaders/vertex/fragment_blinn_tex_color_vert.c");
	
	glShader_tex_vert2.loadFiles("shaders/vertex/vertex_light2_vert.c", "shaders/vertex/fragment_blinn_tex_vert.c");
	glShader_tex_tex_vert2.loadFiles("shaders/vertex/vertex_light2_vert.c", "shaders/vertex/fragment_blinn_tex_tex_vert.c");
	glShader_col_tex_vert2.loadFiles("shaders/vertex/vertex_light2_vert.c", "shaders/vertex/fragment_blinn_tex_color_vert.c");

	checkOpenGLerror("initShader");
}

// Load and create a texture 
GLuint texture_cat;
GLuint texture_table_tv;
GLuint texture_sofa_chair;
GLuint texture_other;
GLuint texture_table;
GLuint texture_smile;

void load_texture(GLuint& texture, const char* filename)
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height;
	unsigned char* image = SOIL_load_image(filename, &width, &height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void loadTextures()
{
	// Текстура для кота
	load_texture(texture_cat, "img/cat_diff.png");

	// Текстура для столика под телевизор
	load_texture(texture_table_tv, "img/wood.jpg");

	// Текстура для дивана и кресла
	load_texture(texture_sofa_chair, "img/sofa.jpg");

	// Текстура для столика
	load_texture(texture_table, "img/table.jpg");

	// Текстура смайлика
	load_texture(texture_smile, "img/awesomeface.png");

	// Другая текстура
	load_texture(texture_other, "img/list.jpg");

}

void add_to_buffer(const Model& glModel)
{
	glGenBuffers(1, &VBO_position);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_position);
	glBufferData(GL_ARRAY_BUFFER, glModel.vertices_m.size() * sizeof(glm::vec3), &glModel.vertices_m[0], GL_STATIC_DRAW);
	VBO_vertexes.push_back(VBO_position);

	glGenBuffers(1, &VBO_texcoord);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_texcoord);
	glBufferData(GL_ARRAY_BUFFER, glModel.tex_coords.size() * sizeof(glm::vec2), &glModel.tex_coords[0], GL_STATIC_DRAW);
	VBO_textures.push_back(VBO_texcoord);

	glGenBuffers(1, &VBO_normal);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_normal);
	glBufferData(GL_ARRAY_BUFFER, glModel.normals.size() * sizeof(glm::vec3), &glModel.normals[0], GL_STATIC_DRAW);
	VBO_normales.push_back(VBO_normal);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, glModel.indices.size() * sizeof(GL_UNSIGNED_INT), &glModel.indices[0], GL_STATIC_DRAW);
	EBO_indexes.push_back(EBO);

	models.push_back(glModel);
}

void initTableForTV()
{
	Model glModel = Model("tablefortv.obj");
	glModel.scale_model = glm::vec3(0.05f, 0.05f, 0.05f);
	glModel.translate_model = glm::vec3(0.0f, 0.0f, 100.0f);
	glModel.rotate_x = 0.0f;
	glModel.rotate_y = -glm::radians(50.0f);
	glModel.rotate_z = 0.0f;
	glModel.texture1 = texture_table_tv;
	glModel.type_coloring = PaintType::TEXTURE;

	glModel.material = new_material(glm::vec4(0.2, 0.2, 0.2, 1.0), // ambient
		glm::vec4(1.0, 1.0, 1.0, 1.0), // diffuse
		glm::vec4(0.4, 0.4, 0.4, 1.0), // specular
		glm::vec4(0.1, 0.1, 0.1, 1.0), // emission
		0.1 * 128, // shininess
		glm::vec4(0.7, 0.0, 0.7, 1.0)); // color

	add_to_buffer(glModel);
}

void initChair()
{
	Model glModel = Model("chair.obj");
	glModel.scale_model = glm::vec3(0.005f, 0.005f, 0.005f);
	glModel.translate_model = glm::vec3(500.0f, 0.0f, 0.0f);
	glModel.rotate_x = 0.0f;
	glModel.rotate_y = 0.7f;
	glModel.rotate_z = 0.0f;
	glModel.texture1 = texture_smile;
	glModel.texture2 = texture_sofa_chair;
	glModel.type_coloring = PaintType::TEXTURE_TEXTURE;

	glModel.material = new_material(glm::vec4(0.2, 0.2, 0.2, 1.0), // ambient
		glm::vec4(1.0, 1.0, 1.0, 1.0), // diffuse
		glm::vec4(0.4, 0.4, 0.4, 1.0), // specular
		glm::vec4(0.1, 0.1, 0.1, 1.0), // emission
		0.1 * 128, // shininess
		glm::vec4(0.7, 0.0, 0.7, 1.0)); // color

	add_to_buffer(glModel);
}

void initSofa()
{
	Model glModel = Model("sofa.obj");
	glModel.scale_model = glm::vec3(0.005f, 0.005f, 0.005f);
	glModel.translate_model = glm::vec3(0.0f, 0.0f, 0.0f);
	glModel.rotate_x = 0.0f;
	glModel.rotate_y = -glm::radians(50.0f);
	glModel.rotate_z = 0.0f;
	glModel.texture1 = texture_sofa_chair;
	glModel.type_coloring = PaintType::TEXTURE;

	glModel.material = new_material(glm::vec4(0.2, 0.2, 0.2, 1.0), // ambient
		glm::vec4(1.0, 1.0, 1.0, 1.0), // diffuse
		glm::vec4(0.4, 0.4, 0.4, 1.0), // specular
		glm::vec4(0.1, 0.1, 0.1, 1.0), // emission
		0.1 * 128, // shininess
		glm::vec4(0.7, 0.0, 0.7, 1.0)); // color

	add_to_buffer(glModel);
}

void initTable()
{
	Model glModel = Model("table.obj");
	glModel.scale_model = glm::vec3(0.01f, 0.01f, 0.01f);
	glModel.translate_model = glm::vec3(100.0f, 0.0f, 0.0f);
	glModel.rotate_x = 0.0f;
	glModel.rotate_y = 0.0f;
	glModel.rotate_z = 0.0f;
	glModel.texture1 = texture_other;
	glModel.type_coloring = PaintType::COLOR_TEXTURE;

	glModel.material = new_material(glm::vec4(0.2, 0.2, 0.2, 1.0), // ambient
		glm::vec4(1.0, 1.0, 1.0, 1.0), // diffuse
		glm::vec4(0.4, 0.4, 0.4, 1.0), // specular
		glm::vec4(0.1, 0.1, 0.1, 1.0), // emission
		0.1 * 128, // shininess
		glm::vec4(0.0, 1.0, 0.0, 1.0)); // color

	add_to_buffer(glModel);
}

void initTV()
{
	Model glModel = Model("tv.obj");
	glModel.scale_model = glm::vec3(3.5f, 3.5f, 3.5f);
	glModel.translate_model = glm::vec3(0.0f, 0.35f, -1.2f);
	glModel.rotate_x = 0.0f;
	glModel.rotate_y = glm::radians(130.0f);
	glModel.rotate_z = 0.0f;
	glModel.texture1 = texture_other;
	glModel.texture2 = texture_sofa_chair;
	glModel.type_coloring = PaintType::TEXTURE;

	glModel.material = new_material(glm::vec4(0.2, 0.2, 0.2, 1.0), // ambient
		glm::vec4(1.0, 1.0, 1.0, 1.0), // diffuse
		glm::vec4(0.4, 0.4, 0.4, 1.0), // specular
		glm::vec4(0.1, 0.1, 0.1, 1.0), // emission
		0.1 * 128, // shininess
		glm::vec4(0.7, 0.0, 0.7, 1.0)); // color

	add_to_buffer(glModel);
}

void initCat()
{
	Model glModel = Model("cat.obj");
	glModel.scale_model = glm::vec3(0.09f, 0.09f, 0.09f);
	glModel.translate_model = glm::vec3(0.0f, 0.0f, 0.0f);
	glModel.rotate_x = 0.0f;
	glModel.rotate_y = 0.0f;//glm::radians(130.0f);
	glModel.rotate_z = 0.0f;
	glModel.texture1 = texture_cat;
	glModel.type_coloring = PaintType::TEXTURE;

	glModel.material = new_material(glm::vec4(0.2, 0.2, 0.2, 1.0), // ambient
		glm::vec4(1.0, 1.0, 1.0, 1.0), // diffuse
		glm::vec4(0.6, 0.6, 0.6, 1.0), // specular
		glm::vec4(0.1, 0.1, 0.1, 1.0), // emission
		0.5 * 128, // shininess
		glm::vec4(0.7, 0.0, 0.7, 1.0)); // color

	add_to_buffer(glModel);
}

void initLamp()
{
	Model glModel = Model("lamp2.obj");
	glModel.scale_model = glm::vec3(2.0f, 2.0f, 2.0f);
	glModel.translate_model = glm::vec3(-3.5f, 0.0f, -2.0f);
	glModel.rotate_x = 0.0f;
	glModel.rotate_y = 0.0f;//glm::radians(130.0f);
	glModel.rotate_z = 0.0f;
	glModel.texture1 = texture_other;
	glModel.type_coloring = PaintType::TEXTURE;

	glModel.material = new_material(glm::vec4(0.2, 0.2, 0.2, 1.0), // ambient
		glm::vec4(1.0, 1.0, 1.0, 1.0), // diffuse
		glm::vec4(0.4, 0.4, 0.4, 1.0), // specular
		glm::vec4(0.1, 0.1, 0.1, 1.0), // emission
		0.1 * 128, // shininess
		glm::vec4(0.7, 0.0, 0.7, 1.0)); // color

	add_to_buffer(glModel);
	/*
	glModel.translate_model = glm::vec3(3.0f, 0.0f, -2.0f);
	glModel.rotate_y = glm::radians(120.0f);
	add_to_buffer(glModel);*/

}

//! Инициализация VBO 
void initVBO()
{
	initTableForTV();
	initChair();
	initSofa();
	initTable();
	initTV();
	initCat();
	initLamp();

	checkOpenGLerror("initVBO");
}

//! Освобождение буфера
void freeVBO()
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &VBO_normal);
	glDeleteBuffers(1, &VBO_texcoord);
	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &VBO_position);
}

float angle_x = 0;
float angle_y = 0;
float angle_z = 0;

void resizeWindow(int width, int height)
{
	glViewport(0, 0, width, height);
}

PointLight light = new_point_light(glm::vec4(0, 5, 15, 1.0), // position
	glm::vec4(0.4, 0.4, 0.4, 1.0), // ambient
	glm::vec4(0.7, 0.7, 0.7, 1.0), // diffuse
	glm::vec4(1.0, 1.0, 1.0, 1.0), // specular
	glm::vec3(1.0, 0.1, 0.0)); // attenuation

PointLight light2 = new_point_light(glm::vec4(0, 5, 0, 1.0), // position
	glm::vec4(0.4, 0.4, 0.4, 1.0), // ambient
	glm::vec4(0.7, 0.7, 0.7, 1.0), // diffuse
	glm::vec4(1.0, 1.0, 1.0, 1.0), // specular
	glm::vec3(1.0, 0.1, 0.0)); // attenuation

void add_parametrs_shader(GLShader& shader, int i, glm::mat4 Model, glm::mat4 ViewProjection, glm::mat3 normalMatrix)
{
	//! Устанавливаем шейдерную программу текущей 
	shader.use();
	shader.setUniform(shader.getUniformLocation("transform.model"), Model);
	shader.setUniform(shader.getUniformLocation("transform.viewProjection"), ViewProjection);
	shader.setUniform(shader.getUniformLocation("transform.normal"), normalMatrix);
	shader.setUniform(shader.getUniformLocation("transform.viewPosition"), vec3(4, 3, 3));

	set_uniform_point_light(shader, light);
	if (pointLight2On)
	{
		shader.setUniform(shader.getUniformLocation("light2.position"), light2.position);
		shader.setUniform(shader.getUniformLocation("light2.ambient"), light2.ambient);
		shader.setUniform(shader.getUniformLocation("light2.diffuse"), light2.diffuse);
		shader.setUniform(shader.getUniformLocation("light2.specular"), light2.specular);
		shader.setUniform(shader.getUniformLocation("light2.attenuation"), light2.attenuation);
	}

	set_uniform_material(shader, models[i].material); // color

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_indexes[i]);

	glEnableVertexAttribArray(shader.getAttribLocation("position"));
	glBindBuffer(GL_ARRAY_BUFFER, VBO_vertexes[i]);
	glVertexAttribPointer(shader.getAttribLocation("position"), 3, GL_FLOAT, GL_FALSE, 0, 0);

	glEnableVertexAttribArray(shader.getAttribLocation("texcoord"));
	glBindBuffer(GL_ARRAY_BUFFER, VBO_textures[i]);
	glVertexAttribPointer(shader.getAttribLocation("texcoord"), 2, GL_FLOAT, GL_FALSE, 0, 0);

	glEnableVertexAttribArray(shader.getAttribLocation("normal"));
	glBindBuffer(GL_ARRAY_BUFFER, VBO_normales[i]);
	glVertexAttribPointer(shader.getAttribLocation("normal"), 3, GL_FLOAT, GL_FALSE, 0, 0);
}
void add_texture_to_shader(GLShader& shader, int i)
{
	// Bind Textures using texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, i==4&& is_vertex_lightning?models[i].texture2: models[i].texture1);
	shader.setUniform(shader.getUniformLocation("ourTexture"), 0);
}
void add_2_textures_to_shader(GLShader& shader, int i)
{
	// Bind Textures using texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, models[i].texture1);
	shader.setUniform(shader.getUniformLocation("ourTexture1"), 0);

	// Bind Textures using texture units
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, models[i].texture2);
	shader.setUniform(shader.getUniformLocation("ourTexture2"), 1);
}
//! Отрисовка 
void render()
{
	angle_y += 0.0007f;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 1000.0f);
	glm::mat4 View = glm::lookAt(glm::vec3(0, 20, 30), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	checkOpenGLerror("render");

	for (int i = 0; i < models.size(); i++)
	{
		glm::mat4 scale = glm::scale(models[i].scale_model);
		glm::mat4 rotate_x = glm::rotate(angle_x + models[i].rotate_x, vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 rotate_y = glm::rotate(angle_y + models[i].rotate_y, vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 rotate_z = glm::rotate(angle_z + models[i].rotate_z, vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 translate = glm::translate(models[i].translate_model);

		glm::mat4 Model = scale * rotate_x * rotate_y * rotate_z * translate;
		glm::mat4 ViewProjection = Projection * View;
		glm::mat3 normalMatrix = glm::transpose(glm::inverse(Model));
		
		if (is_vertex_lightning) // если повершинное вычисление цвета
		{
			if (models[i].type_coloring == PaintType::TEXTURE_TEXTURE)
				if (pointLight2On)
					current = glShader_tex_tex_vert2;
				else current = glShader_tex_tex_vert;
			else if (models[i].type_coloring == PaintType::COLOR_TEXTURE)
				if (pointLight2On)
					current = glShader_col_tex_vert2;
				else current = glShader_col_tex_vert;
			else
				if (pointLight2On)
					current = glShader_tex_vert2;
				else current = glShader_tex_vert;
		}
		else // если попиксельное вычисление
		{
			if (models[i].type_coloring == PaintType::TEXTURE_TEXTURE)
				if (pointLight2On)
					current = glShader_tex_tex2;
				else current = glShader_tex_tex;
			else if (models[i].type_coloring == PaintType::COLOR_TEXTURE)
				if (pointLight2On)
					current = glShader_col_tex2;
				else current = glShader_col_tex;
			else
				if (pointLight2On)
					current = glShader_tex2;
				else current = glShader_tex;
		}
		
		add_parametrs_shader(current, i, Model, ViewProjection, normalMatrix);
		
		// Добавление текстур в шейдер
		if (models[i].type_coloring == PaintType::TEXTURE_TEXTURE)
			add_2_textures_to_shader(current, i);
		else add_texture_to_shader(current, i);

		glDrawElements(GL_TRIANGLES, models[i].indices.size(), GL_UNSIGNED_INT, 0);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
	}

	glFlush();
	glutSwapBuffers();
}

void keyboardCallback(unsigned char key, int x, int y) {
	switch (key)
	{
	case '1':
		pointLight2On = !pointLight2On;
		break;
	case '2':
		is_vertex_lightning = !is_vertex_lightning;
		break;
	}

	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	setlocale(0, "");
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_RGBA | GLUT_ALPHA | GLUT_DOUBLE);
	glutInitWindowSize(1000, 800);
	glutCreateWindow("Simple shaders");
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	//! Обязательно перед инициализацией шейдеров 
	GLenum glew_status = glewInit();
	if (GLEW_OK != glew_status)
	{
		//! GLEW не проинициализировалась  	 	
		std::cout << "Error: " << glewGetErrorString(glew_status) << "\n";
		return 1;
	}

	//! Проверяем доступность OpenGL 2.0  	
	if (!GLEW_VERSION_2_0)
	{
		//! OpenGl 2.0 оказалась не доступна  	 	
		std::cout << "No support for OpenGL 2.0 found\n";
		return 1;
	}

	//! Инициализация  
	glClearColor(0.5, 0.5, 0.5, 0);

	loadTextures();
	initVBO();
	initShader();
	glutReshapeFunc(resizeWindow);
	glutIdleFunc(render);
	glutDisplayFunc(render);
	glutKeyboardFunc(keyboardCallback);
	glutMainLoop();

	//! Освобождение ресурсов  
	freeVBO();
}