#include "Material.h"

Material::Material(const std::string& configFilename)
{

}

bool Material::SetFloat(const std::string& name, std::uint32_t offset, std::uint32_t count, const float* data)
{
	auto it = mParameters.find(name);
	if (it == mParameters.end())
	{
		auto tp = mParameters.emplace(name, MaterialParameter());
		it		= tp.first;
	}

	//it->second.Data = k
	
	

	/*const MaterialParameter& param = it->second;
	param.bindingObject->Buffer.buffer->SetSubData(param.offset + offset, count * sizeof(float), data);*/

	return true;
}

bool Material::SetTexture(const std::string& name, TexturePtr texture)
{
	mTextures[name] = texture;
	return true;
}