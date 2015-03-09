#ifndef MAYATO_WORLD_H
#define MAYATO_WORLD_H

#include <vector>
#include <memory>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MImage.h>
#include <maya/MTimerMessage.h>
#include "utilities/tools.h"
#include "rendering/renderer.h"

class MayaScene;
class RenderGlobals;

namespace MayaTo{

	class MayaToWorld
	{
	public:
		MayaToWorld();
		~MayaToWorld();

		enum WorldRenderType{
			RTYPENONE = 0,
			SWATCHRENDER,
			UIRENDER,
			BATCHRENDER,
			IPRRENDER
		};
		enum WorldRenderState{
			RSTATENONE = 0,
			RSTATERENDERING,
			RSTATESTOPPED,
			RSTATEERROR,
			RSTATEDONE,
			RSTATETRANSLATING
		};
		WorldRenderType renderType;
		WorldRenderState renderState;

		std::shared_ptr<MayaScene> worldScenePtr;
		std::shared_ptr<Renderer> worldRendererPtr;
		std::shared_ptr<RenderGlobals> worldRenderGlobalsPtr;

		MStringArray objectNames;
		std::vector<void *> objectPtr;
		MImage previousRenderedImage;

		MStringArray shaderSearchPath;

		void *getObjPtr(MString name)
		{
			for (uint i = 0; i < objectNames.length(); i++)
				if (objectNames[i] == name)
					return objectPtr[i];
			return NULL;
		}

		void addObjectPtr(MString name, void *ptr)
		{
			objectNames.append(name);
			objectPtr.push_back(ptr);
		}

		void initialize();
		void initializeScene();
		void initializeRenderer();
		void initializeRenderGlobals();
		void initializeRenderEnvironment();
		
		void setRenderState(WorldRenderState state)
		{
			this->renderState = state;
		}

		WorldRenderState getRenderState()
		{
			return this->renderState;
		}		

		void cleanUp();
		void cleanUpAfterRender();
		void afterOpenScene();
		void afterNewScene();

		static void callAfterOpenCallback(void *);
		static void callAfterNewCallback(void *);
	};

	void *getObjPtr(MString name);
	static void addObjectPtr(MString name, void *ptr);

	void deleteWorld();
	void defineWorld();
	MayaToWorld *getWorldPtr();

	static MayaToWorld *worldPointer = NULL;

	struct CmdArgs{
		CmdArgs()
		{
			MFnDependencyNode defaultGlobals(objectFromName("defaultRenderGlobals"));
			width = defaultGlobals.findPlug("width").asInt();
			height = defaultGlobals.findPlug("height").asInt();
			renderType = MayaToWorld::WorldRenderType::UIRENDER;
		}
		~CmdArgs(){
			std::cout << "Cmd args are deleted\n";
		}
		int width;
		int height;
		MDagPath cameraDagPath;
		MayaToWorld::WorldRenderType renderType;
	};

};


#endif // !MAYATO_WORLD_H
