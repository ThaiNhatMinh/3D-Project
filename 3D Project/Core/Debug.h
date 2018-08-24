#pragma once

class DebugData
{
public:
	glm::vec3 pos;
	glm::vec3 color;
	
};
class CameraComponent;
class Debug: public ISubSystem
{
private:
	mat4 m_View;
	mat4 m_Proj;
	mat4 VP;
	Shader* pShader;
	VertexArray			VAO;
	BufferObject		VBO;
	std::vector<DebugData> m_Lists;
public:
	Debug(Context* c);
	void Update();
	~Debug();

	void DrawLine(const vec3& from, const vec3& to, const vec3& color, const mat4& m = mat4());
	void DrawLineBox(vec3 min, vec3 max, vec3 color = vec3(0.5f), const mat4& m = mat4());
	void DrawCoord(const mat4& m);
	void Render(Scene* pScene);
};
