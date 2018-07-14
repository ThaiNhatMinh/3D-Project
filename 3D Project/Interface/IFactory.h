#pragma once

namespace Light
{
	class IFactory: public ISubSystem
	{
	public:
		virtual ~IFactory() = default;

		virtual bool			VRegisterComponentFactory(string name, std::function<IComponent*()>) = 0;
		virtual bool			VRegisterActorFactory(const string& name, std::function<IActor*(int id)>) = 0;
		virtual IActor*			VCreateActor(const char* filePath, bool isCreateChild) = 0;
		//virtual IShader*		VCreateShader(const char* type, const char* vs, const char* fs) = 0;

	};
}