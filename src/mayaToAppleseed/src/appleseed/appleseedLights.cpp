#include "appleseed.h"
#include <maya/MFnLight.h>
#include "../mtap_common/mtap_mayaScene.h"
#include "utilities/tools.h"
#include "utilities/attrTools.h"
#include "utilities/logging.h"

static Logging logger;

using namespace AppleRender;

void AppleseedRenderer::defineLights()
{
	// Create a color called "light_exitance" and insert it into the assembly.
	MStatus stat;

	for(int objId = 0; objId < this->mtap_scene->lightList.size(); objId++)
	{
		mtap_MayaObject *mlight = (mtap_MayaObject *)this->mtap_scene->lightList[objId];
		asf::Matrix4d appMatrix = asf::Matrix4d::identity();
		logger.debug(MString("Creating light: ") + mlight->shortName);
		MMatrix colMatrix = mlight->transformMatrices[0];
		this->MMatrixToAMatrix(colMatrix, appMatrix);

		MFnLight lightFn(mlight->mobject, &stat);
		MColor color(1.0f,1.0f,1.0f);
		float intensity = 1.0f;
		if( !stat )
		{
			logger.error(MString("Could not get light info from ") + mlight->shortName);
			return;
		}
		getColor(MString("color"), lightFn, color);
		getFloat(MString("intensity"), lightFn, intensity);
		
		float apColor[3];
		apColor[0] = color.r;
		apColor[1] = color.g;
		apColor[2] = color.b;
		asr::ColorValueArray aLightColor(3, apColor);
		MString lightColorName = mlight->shortName + "_exitance";
		//this->defineColor(lightColorName, color, NULL);

		intensity *= 30.0f; // 30 is the default value in the example
		this->masterAssembly->colors().insert(
			asr::ColorEntityFactory::create(
				lightColorName.asChar(),
				asr::ParamArray()
					.insert("color_space", "srgb")
					.insert("multiplier", (MString("")+intensity).asChar()),
					aLightColor));

		
		// Create a point light called "light" and insert it into the assembly.
		if( mlight->mobject.hasFn(MFn::kSpotLight))
		{
			//inner_angle
			//outer_angle
			logger.debug(MString("Creating spotLight: ") + lightFn.name());
			float coneAngle = 45.0f;
			float penumbraAngle = 3.0f;
			getFloat(MString("coneAngle"), lightFn, coneAngle);
			getFloat(MString("penumbraAngle"), lightFn, penumbraAngle);
			coneAngle = (float)RadToDeg(coneAngle);
			penumbraAngle = (float)RadToDeg(penumbraAngle);

			logger.debug(MString("ConeAngle: ") + coneAngle);
			logger.debug(MString("penumbraAngle: ") + penumbraAngle);
			float inner_angle = coneAngle;
			float outer_angle = coneAngle + penumbraAngle;
			
			// spot light is pointing in -z, appleseeds spot light is pointing in y, at least until next update...
			// I create a rotation matrix for this case.
			asf::Matrix4d rotMatrix = asf::Matrix4d::rotation(asf::Vector3d(1.0, 0.0, 0.0), asf::deg_to_rad(-90.0));
			asf::Matrix4d finalMatrix = appMatrix * rotMatrix;

			asf::auto_release_ptr<asr::Light> light(
				asr::SpotLightFactory().create(
					mlight->shortName.asChar(),
					asr::ParamArray()
						.insert("exitance", lightColorName.asChar())
						.insert("inner_angle", (MString("") + inner_angle).asChar())
						.insert("outer_angle", (MString("") + outer_angle).asChar())
						));
			light->set_transform(asf::Transformd(finalMatrix));
			this->masterAssembly->lights().insert(light);
			
		}else{
			asf::auto_release_ptr<asr::Light> light(
				asr::PointLightFactory().create(
					mlight->shortName.asChar(),
					asr::ParamArray()
						.insert("exitance", lightColorName.asChar())));
			light->set_transform(asf::Transformd(appMatrix));
			this->masterAssembly->lights().insert(light);
		}
	}
}


void AppleseedRenderer::defineLights(std::vector<MayaObject *>& lightList)
{
	this->defineLights();
	return;
	// Create a color called "light_exitance" and insert it into the assembly.
	MStatus stat;

	for(int objId = 0; objId < lightList.size(); objId++)
	{
		mtap_MayaObject *mlight = (mtap_MayaObject *)lightList[objId];
		asf::Matrix4d appMatrix = asf::Matrix4d::identity();
		logger.debug(MString("Creating light: ") + mlight->shortName);
		MMatrix colMatrix = mlight->transformMatrices[0];
		this->MMatrixToAMatrix(colMatrix, appMatrix);

		MFnLight lightFn(mlight->mobject, &stat);
		MColor color(1.0f,1.0f,1.0f);
		float intensity = 1.0f;
		if( !stat )
		{
			logger.error(MString("Could not get light info from ") + mlight->shortName);
			return;
		}
		getColor(MString("color"), lightFn, color);
		getFloat(MString("intensity"), lightFn, intensity);

		//asf::FloatArray lightColor(1.0f, 1.0f, 1.0f);
		
		float apColor[3];
		apColor[0] = color.r;
		apColor[1] = color.g;
		apColor[2] = color.b;
		asr::ColorValueArray aLightColor(3, apColor);
		MString lightColorName = mlight->shortName + "_exitance";
		intensity *= 30.0f; // 30 is the default value in the example
		this->masterAssembly->colors().insert(
			asr::ColorEntityFactory::create(
				lightColorName.asChar(),
				asr::ParamArray()
					.insert("color_space", "srgb")
					.insert("multiplier", (MString("")+intensity).asChar()),
					aLightColor));


		// Create a point light called "light" and insert it into the assembly.
		asf::auto_release_ptr<asr::Light> light(
			asr::PointLightFactory().create(
				mlight->shortName.asChar(),
				asr::ParamArray()
					.insert("exitance", lightColorName.asChar())));
		light->set_transform(asf::Transformd(appMatrix));
		this->masterAssembly->lights().insert(light);
	}
}

void AppleseedRenderer::defineDefaultLight()
{
	// default light will be created only if there are no other lights in the scene
	if( this->mtap_scene->lightList.size() > 0)
		return;
	if( !this->renderGlobals->createDefaultLight )
		return;
	
    // Create a color called "light_exitance" and insert it into the assembly.
    static const float LightExitance[] = { 1.0f, 1.0f, 1.0f };
	this->masterAssembly->colors().insert(
        asr::ColorEntityFactory::create(
            "light_exitance",
            asr::ParamArray()
                .insert("color_space", "srgb")
                .insert("multiplier", "30.0"),
            asr::ColorValueArray(3, LightExitance)));

    // Create a point light called "light" and insert it into the assembly.
    asf::auto_release_ptr<asr::Light> light(
        asr::PointLightFactory().create(
            "light",
            asr::ParamArray()
                .insert("exitance", "light_exitance")));
    light->set_transform(asf::Transformd(
        asf::Matrix4d::translation(asf::Vector3d(0.6, 2.0, 1.0))));
    this->masterAssembly->lights().insert(light);
}