#include <pch.h>

void TexShader::SetupRender(Scene * pScene, Actor * pActor)
{
	this->Use();
	ICamera* pCam = pScene->GetCurrentCamera();

	// ----- Transform Matricies ------
	
	mat4 globalTransform = pActor->VGetGlobalTransform();
	SetUniformMatrix("Model", glm::value_ptr(globalTransform));
	mat4 MVP = pCam->GetVPMatrix()*globalTransform;
	SetUniformMatrix("MVP", glm::value_ptr(MVP));

	// ----- Lighting ------
	const DirectionLight& dirLight = pScene->GetDirLight();
	SetUniform("gLight.La", dirLight.La);
	SetUniform("gLight.Ld", dirLight.Ld);
	SetUniform("gLight.Ls", dirLight.Ls);
	SetUniform("gLight.direction", dirLight.direction);

	

}

void TexShader::LinkShader()
{
	glBindAttribLocation(m_iProgramID, SHADER_POSITION_ATTRIBUTE, "position");
	glBindAttribLocation(m_iProgramID, SHADER_NORMAL_ATTRIBUTE, "normal");
	glBindAttribLocation(m_iProgramID, SHADER_TEXCOORD_ATTRIBUTE, "uv");
	Shader::LinkShader();
}
