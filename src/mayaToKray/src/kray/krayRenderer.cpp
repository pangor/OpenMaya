//


#include "threads/renderQueueWorker.h"
#include "../mtkr_common/mtkr_mayaObject.h"
#include "../mtkr_common/mtkr_mayaScene.h"
#include "../mtkr_common/mtkr_renderGlobals.h"
#include "utilities/tools.h"
#include "utilities/attrTools.h"
#include "utilities/pystring.h"

#include <maya/MEulerRotation.h>


//#include "kraysdk/proto/direct.h"
//#include "kraysdk/SharedSources/libraryDir.h"
//#include "kraysdk/kray.h"
//
#include "krayRenderer.h"
#include "krayUtils.h"

#include "krayTestScene.h"
#include "utilities/logging.h"

static Logging logger;

namespace krayRender
{
	KrayRenderer::KrayRenderer()
	{
		this->is_good = false;
		//TODO: path more common
		const char *libpath = "C:/Users/haggi/coding/OpenMaya/src/mayaToKray/mtkr_devmodule/bin/";
		this->kli = new Kray::Library(libpath);				// here we start with KrayLib object
		this->kin = this->kli->createInstanceWithListner(&listener);
		if( this->kli && this->kin)
			this->is_good = true;

		this->pro = NULL;
	}

	KrayRenderer::~KrayRenderer()
	{
		logger.debug("deleting kin");
		if( this->kin )
			delete this->kin;

		logger.debug("deleting kli");
		this->kli->release();
		if( this->kli )
			delete this->kli;
	}
	bool KrayRenderer::good()
	{
		return this->is_good;
	}

	void KrayRenderer::definePrototyper()
	{
		if (this->good()) // check if instance was created, if not, Kray library wasn't found
		{				
			if( this->mtkr_renderGlobals->exportSceneFile)
			{
				const char *SCRIPTFILENAME = "C:/daten/3dprojects/kray/scene.kray";
				this->outStream = std::ofstream(SCRIPTFILENAME);
				//this->pro = new krayRender::OstreamPrototyper(this->outStream,false);
				this->pro = new krayRender::OstreamPlusDirectPrototyper(this->outStream, *this->kin, false);
				this->pro->parseMode(1);	// turns on, turbo parsing mode
			}else
				this->pro = new Kray::DirectPrototyper(this->kin);		// prototyper gives access to Kray Script command prototypes
		}else{
			logger.error("Unable to create kray renderer instance");
		}
	}

	void KrayRenderer::updateTransform(mtkr_MayaObject *obj)
	{
		logger.debug(MString("KrayRenderer::updateTransform ") + obj->shortName);
	}

	void KrayRenderer::updateDeform(mtkr_MayaObject *obj)
	{
		logger.debug(MString("KrayRenderer::updateDeform ") + obj->shortName);
		this->defineGeometry(obj);
	}

	void KrayRenderer::defineSampling()
	{
		// dof only works with full screen AA, so turn it off if fsaa is turned off
		if(!this->mtkr_renderGlobals->fullScreenAA)
		{
			this->mtkr_renderGlobals->doDof = false;
		}

		switch( this->mtkr_renderGlobals->samplingType)
		{
		case 0: // none
			this->pro->imageSampler_none();
			break;
		case 1: // grid
			if(!this->mtkr_renderGlobals->fullScreenAA)
			{
				if( this->mtkr_renderGlobals->rotateGrid)
					this->pro->imageSampler_rotUniform(this->mtkr_renderGlobals->gridSize);
				else
					this->pro->imageSampler_uniform(this->mtkr_renderGlobals->gridSize);
			}else{
				if( this->mtkr_renderGlobals->rotateGrid)
					this->pro->imageSampler_totalRotUniform(this->mtkr_renderGlobals->gridSize);
				else
					this->pro->imageSampler_totalUniform(this->mtkr_renderGlobals->gridSize);
			}
			break;
		case 2: // quasi random 
			if(!this->mtkr_renderGlobals->fullScreenAA)
			{
				this->pro->imageSampler_qmcAdaptive(this->mtkr_renderGlobals->aa_minRays, this->mtkr_renderGlobals->aa_maxRays, this->mtkr_renderGlobals->aa_threshold); 
			}else{
				if( (this->mtkr_renderGlobals->aa_minRays >= this->mtkr_renderGlobals->aa_maxRays) || 
					 this->mtkr_renderGlobals->aa_threshold == 0.0f || 
					 this->mtkr_renderGlobals->doDof || 
					 this->mtkr_renderGlobals->doMb )
				{
					this->pro->imageSampler_totalQmc(this->mtkr_renderGlobals->aa_minRays);
				}else{
					this->pro->imageSampler_totalQmcAdaptive(this->mtkr_renderGlobals->aa_minRays, this->mtkr_renderGlobals->aa_maxRays, this->mtkr_renderGlobals->aa_threshold); 
				}
			}
			break;
		case 3: // random full screen AA
			if(!this->mtkr_renderGlobals->fullScreenAA)
			{
			}else{
				if( this->mtkr_renderGlobals->doMb )
				{
					this->pro->imageSampler_totalMbRandom(this->mtkr_renderGlobals->aa_rays, this->mtkr_renderGlobals->mb_subframes);
				}else{
					this->pro->imageSampler_totalRandom(this->mtkr_renderGlobals->aa_rays);
				}
			}
			break;
		}

		this->pro->edgeDetectorGlobals(this->mtkr_renderGlobals->aa_thickness, this->mtkr_renderGlobals->aa_upsample);
		this->pro->edgeDetectorSlot(0, this->mtkr_renderGlobals->aa_edgeAbsolute, this->mtkr_renderGlobals->aa_relative, 
			this->mtkr_renderGlobals->aa_normal, this->mtkr_renderGlobals->aa_z, this->mtkr_renderGlobals->aa_overburn);
	}

	void KrayRenderer::defineCamera()
	{
		// define image size
		int width = this->mtkr_renderGlobals->imgWidth;
		int height = this->mtkr_renderGlobals->imgHeight;
		//this->pro->frameSize(width, height);

		// define camera
		//MVector rot;
		//MPoint pos;
		//posRotFromMatrix(this->mtkr_scene->camList[0]->transformMatrices[0], pos, rot);
		MMatrix matrix = this->mtkr_scene->camList[0]->transformMatrices[0];
		matrix *= this->mtkr_renderGlobals->sceneScaleMatrix;

		MFnDependencyNode camFn(this->mtkr_scene->camList[0]->mobject);
		
		float horizontalFilmAperture = 24.892f;
		float verticalFilmAperture = 18.669f;
		float imageAspect = (float)this->mtkr_renderGlobals->imgHeight / (float)mtkr_renderGlobals->imgWidth;
		bool dof = false;
		float mtap_cameraType = 0;
		int mtap_diaphragm_blades = 0;
		float mtap_diaphragm_tilt_angle = 0.0;
		float focusDistance = 0.0;
		float fStop = 0.0;
		float focusRegionScale = 0.1f;
		float focalLength = 35.0f;
		getFloat(MString("horizontalFilmAperture"), camFn, horizontalFilmAperture);
		getFloat(MString("verticalFilmAperture"), camFn, verticalFilmAperture);
		getFloat(MString("focalLength"), camFn, focalLength);
		getBool(MString("depthOfField"), camFn, dof);
		getFloat(MString("focusDistance"), camFn, focusDistance);
		getFloat(MString("fStop"), camFn, fStop);
		getFloat(MString("focusRegionScale"), camFn, focusRegionScale);
		
		// in kray the focal distance is defined in pixels.
		// So... if you know how big is your virtual sensor in Kray. 
		// Lets say its standars 35mm film you can find out how big is pixel of Kray's virtual sensor. 
		// Its pixel_size_in_mm=35mm/image_size . Than you can compute focal_distance_in_pixels=focal_distance_in_mm/pixel_size_in_mm

		// in maya the sensor size is the horizontal film aperture expressed in inces
		float horizontalImageSizeMM = horizontalFilmAperture * 25.4f;
		float pixelSizeInMM = horizontalImageSizeMM / this->mtkr_renderGlobals->imgWidth;
		float focalDistance = focalLength / pixelSizeInMM;

		this->pro->previewSize(width, height);

		MMatrix posMatrix = matrix;// * this->mtkr_renderGlobals->sceneRotMatrix;
		MMatrix rotMatrix = matrix;

		MVector rot;
		MPoint pos;
		posRotFromMatrix( rotMatrix, pos, rot);
		//logger.debug(MString("rot: ") + rot.x + " " + rot.y + " " + rot.z);
		rotMatrix.setToIdentity();
		MTransformationMatrix tm(rotMatrix);
		// add 180 degree to bank
		MEulerRotation euler(rot.x + M_PI, rot.y, rot.z, MEulerRotation::kXYZ);
		tm.rotateBy(euler, MSpace::kWorld);
		rotMatrix = tm.asMatrix();

		Kray::Matrix4x4 camPosMatrix;
		Kray::Matrix4x4 camRotMatrix;
		
		//posMatrix[3][0] = -posMatrix[3][0];
		//posMatrix[3][1] = -posMatrix[3][1];
		MMatrixToAMatrix(posMatrix, camPosMatrix);
		MMatrixToAMatrix(rotMatrix, camRotMatrix);

		Kray::Vector camPos(camPosMatrix);
		Kray::AxesHpb camRot(camRotMatrix);

		logger.debug(MString("Define camera: img size ") + width + " " + height + " " + focalDistance);

		if( this->mtkr_renderGlobals->doDof)
		{
			this->pro->camera_lens1(width, height, focalDistance, focusDistance, focusRegionScale, camPos, camRot);
		}else{
			this->pro->camera_picture(width, height, focalDistance, camPos, camRot);
		}
	}

	void KrayRenderer::writeImageFile(MString imageName)
	{
		if( this->mtkr_renderGlobals->bitdepth == 0) 
		{
			logger.debug(MString("Writing 8-bit output file: ") + imageName);
			if(pystring::endswith(imageName.asChar(), ".tif"))
				this->pro->outputSave_tifa(imageName.asChar());
			if(pystring::endswith(imageName.asChar(), ".hdr"))
				this->pro->outputSave_hdr(imageName.asChar());
			if(pystring::endswith(imageName.asChar(), ".tga"))
				this->pro->outputSave_tgaa(imageName.asChar());
			if(pystring::endswith(imageName.asChar(), ".jpg"))
				this->pro->outputSave_jpg(imageName.asChar(), this->mtkr_renderGlobals->jpgQuality);
			if(pystring::endswith(imageName.asChar(), ".png"))
				this->pro->outputSave_pnga(imageName.asChar());
			if(pystring::endswith(imageName.asChar(), ".bmp"))
				this->pro->outputSave_bmpa(imageName.asChar());
		}		
		// 16 bit
		if( this->mtkr_renderGlobals->bitdepth == 1) 
		{
			logger.debug(MString("Writing 16-bit output file: ") + imageName);
			if(pystring::endswith(imageName.asChar(), ".tif"))
				this->pro->outputSave_tifa16(imageName.asChar());
			if(pystring::endswith(imageName.asChar(), ".png"))
				this->pro->outputSave_pnga16(imageName.asChar());
		}
	}
	void KrayRenderer::definePixelOrder()
	{
		// define pixel order
		switch(this->mtkr_renderGlobals->pixelOrder)
		{
		case 0:
			this->pro->pixelOrder_scanLine();
			break;
		case 1:
			this->pro->pixelOrder_scanRow();
			break;
		case 2:
			this->pro->pixelOrder_random();
			break;
		case 3:
			this->pro->pixelOrder_progressive();
			break;
		case 4:
			this->pro->pixelOrder_worm();		
			break;
		case 5:
			this->pro->pixelOrder_frost();		
			break;
		default:
			this->pro->pixelOrder_worm();		
			break;
		};
	}

	void KrayRenderer::defineFilter()
	{
		double filterSize = this->mtkr_renderGlobals->filterSize;
		// define pixel order
		switch(this->mtkr_renderGlobals->filterType)
		{
		case 0: // box
			this->pro->pixelFilter_box(filterSize);
			break;
		case 1: // cone
			this->pro->pixelFilter_cone(filterSize);
			break;
		case 2: // cubic
			this->pro->pixelFilter_cubic(filterSize);
			break;
		case 3: // lanczos
			this->pro->pixelFilter_lanczos(filterSize);
			break;
		case 4: // mitchell
			this->pro->pixelFilter_mitchell();
			break;
		case 5: // spline
			this->pro->pixelFilter_spline();
			break;
		case 6: // catmull
			this->pro->pixelFilter_catmull();
			break;
		case 7: // quadric
			this->pro->pixelFilter_quadric(filterSize);
			break;
		default: // catmull
			this->pro->pixelFilter_catmull();
			break;
		};
	}

	void KrayRenderer::render()
	{	
		//EventListener listener;
		//const char *libpath = "C:/Users/haggi/coding/OpenMaya/src/mayaToKray/mtkr_devmodule/bin/";
		//this->kli = new Kray::Library(libpath);				// here we start with KrayLib object
		
		//Kray::Library klib(libpath);				// here we start with KrayLib object
		////Kray::Library klib(KRAY_DIRECTORY);				// here we start with KrayLib object
		////Kray::Instance* kinst=klib.createInstance();	// then we create Kray instance, each instance have its own scene and settings
		////klib.createInstanceWithHandler()
		//Kray::Instance* kinst = this->kli->createInstanceWithListner(&listener);

		if (this->kin) // check if instance was created, if not, Kray library wasn't found
		{	
			if( this->pro )
			{			
				if( this->mtkr_renderGlobals->camSingleSided )
					this->pro->camSingleSide(true);
				this->defineLigths();
				this->defineSampling(); // before defineCamera because ggf. dof will be turned off
				this->defineCamera();
				this->defineFilter();
				this->definePixelOrder();
				this->defineBackground();
				this->defineEnvironment();
				this->pro->echo("Rendering....");
				
				//this->pro->groupSelect("Testgroup");
				//this->pro->instanceAdd("name", vec, ax);
				this->pro->showInfo_all();
				//this->pro->groupSelect()
				//this->pro->animOb_typeMeshInstance("animobj", mesh);
				this->pro->render();
				this->writeImageFile(this->mtkr_renderGlobals->imageOutputFile);
				this->pro->reset();
			}			
		}else{
		}
	
		EventQueue::Event e;
		e.data = NULL;
		e.type = EventQueue::Event::FRAMEDONE;
		theRenderEventQueue()->push(e);
		
		//if( kinst )
		//	delete kinst;
	}

} // namespace kray