#include "Lux.h"
#include <maya/MFnMesh.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MMatrix.h>
#include <maya/MIntArray.h>
#include <maya/MFloatArray.h>


#include "../mtlu_common/mtlu_mayaScene.h"
#include "../mtlu_common/mtlu_mayaObject.h"
#include "LuxUtils.h"
#include "utilities/tools.h"

#include "utilities/logging.h"
static Logging logger;

void LuxRenderer::defineTriangleMesh(mtlu_MayaObject *obj)
{
	MObject meshObject = obj->mobject;
	MStatus stat = MStatus::kSuccess;
	MFnMesh meshFn(meshObject, &stat);
	
	CHECK_MSTATUS(stat);
	MItMeshPolygon faceIt(meshObject, &stat);
	CHECK_MSTATUS(stat);

	MPointArray points;
	meshFn.getPoints(points);
    MFloatVectorArray normals;
    meshFn.getNormals( normals, MSpace::kWorld );
	MFloatArray uArray, vArray;
	meshFn.getUVs(uArray, vArray);

	logger.debug(MString("Translating mesh object ") + meshFn.name().asChar());
	MString meshFullName = obj->fullNiceName;


	MIntArray trianglesPerFace, triVertices;
	meshFn.getTriangles(trianglesPerFace, triVertices);
	int numTriangles = 0;
	for( size_t i = 0; i < trianglesPerFace.length(); i++)
		numTriangles += trianglesPerFace[i];

	// lux render does not have a per vertex per face normal definition, here we can use one normal and uv per vertex only
	// So I create the triangles with unique vertices, normals and uvs. Of course this way vertices etc. cannot be shared.
	int numPTFloats = numTriangles * 3 * 3;
	logger.debug(MString("Num Triangles: ") + numTriangles + " num tri floats " + numPTFloats);

	float *floatPointArray = new float[numPTFloats];
	float *floatNormalArray = new float[numPTFloats];
	float *floatUvArray = new float[numTriangles * 3 * 2];
	
	logger.debug(MString("Allocated ") + numPTFloats + " floats for point and normals");

	MIntArray triangelVtxIdListA;
	MFloatArray floatPointArrayA;

	MPointArray triPoints;
	MIntArray triVtxIds;
	MIntArray faceVtxIds;
	MIntArray faceNormalIds;
	
	int *triangelVtxIdList = new int[numTriangles * 3];

	for( uint sgId = 0; sgId < obj->shadingGroups.length(); sgId++)
	{
		MString slotName = MString("slot_") + sgId;
	}
	
	int triCount = 0;
	int vtxCount = 0;

	for(faceIt.reset(); !faceIt.isDone(); faceIt.next())
	{
		int faceId = faceIt.index();
		int numTris;
		faceIt.numTriangles(numTris);
		faceIt.getVertices(faceVtxIds);

		MIntArray faceUVIndices;

		faceNormalIds.clear();
		for( uint vtxId = 0; vtxId < faceVtxIds.length(); vtxId++)
		{
			faceNormalIds.append(faceIt.normalIndex(vtxId));
			int uvIndex;
			faceIt.getUVIndex(vtxId, uvIndex);
			faceUVIndices.append(uvIndex);
		}

		int perFaceShadingGroup = 0;
		if( obj->perFaceAssignments.length() > 0)
			perFaceShadingGroup = obj->perFaceAssignments[faceId];
		//logger.info(MString("Face ") + faceId + " will receive SG " +  perFaceShadingGroup);

		for( int triId = 0; triId < numTris; triId++)
		{
			int faceRelIds[3];
			faceIt.getTriangle(triId, triPoints, triVtxIds);

			for( uint triVtxId = 0; triVtxId < 3; triVtxId++)
			{
				for(uint faceVtxId = 0; faceVtxId < faceVtxIds.length(); faceVtxId++)
				{
					if( faceVtxIds[faceVtxId] == triVtxIds[triVtxId])
					{
						faceRelIds[triVtxId] = faceVtxId;
					}
				}
			}

			
			uint vtxId0 = faceVtxIds[faceRelIds[0]];
			uint vtxId1 = faceVtxIds[faceRelIds[1]];
			uint vtxId2 = faceVtxIds[faceRelIds[2]];
			uint normalId0 = faceNormalIds[faceRelIds[0]];
			uint normalId1 = faceNormalIds[faceRelIds[1]];
			uint normalId2 = faceNormalIds[faceRelIds[2]];
			uint uvId0 = faceUVIndices[faceRelIds[0]];
			uint uvId1 = faceUVIndices[faceRelIds[1]];
			uint uvId2 = faceUVIndices[faceRelIds[2]];
			
			floatPointArray[vtxCount * 3] = points[vtxId0].x;
			floatPointArray[vtxCount * 3 + 1] = points[vtxId0].y;
			floatPointArray[vtxCount * 3 + 2] = points[vtxId0].z;

			//floatNormalArray[vtxCount * 3] = normals[normalId0].x;
			//floatNormalArray[vtxCount * 3 + 1] = normals[normalId0].y;
			//floatNormalArray[vtxCount * 3 + 2] = normals[normalId0].z;

			//floatUvArray[vtxCount * 3] = uArray[uvId0];
			//floatUvArray[vtxCount * 3 + 1] = uArray[uvId0];

			vtxCount++;

			floatPointArray[vtxCount * 3] = points[vtxId1].x;
			floatPointArray[vtxCount * 3 + 1] = points[vtxId1].y;
			floatPointArray[vtxCount * 3 + 2] = points[vtxId1].z;

			//floatNormalArray[vtxCount * 3] = normals[normalId1].x;
			//floatNormalArray[vtxCount * 3 + 1] = normals[normalId1].y;
			//floatNormalArray[vtxCount * 3 + 2] = normals[normalId1].z;

			//floatUvArray[vtxCount * 3] = uArray[uvId1];
			//floatUvArray[vtxCount * 3 + 1] = uArray[uvId1];

			vtxCount++;

			floatPointArray[vtxCount * 3] = points[vtxId2].x;
			floatPointArray[vtxCount * 3 + 1] = points[vtxId2].y;
			floatPointArray[vtxCount * 3 + 2] = points[vtxId2].z;

			//floatNormalArray[vtxCount * 3] = normals[normalId2].x;
			//floatNormalArray[vtxCount * 3 + 1] = normals[normalId2].y;
			//floatNormalArray[vtxCount * 3 + 2] = normals[normalId2].z;

			//floatUvArray[vtxCount * 2] = uArray[uvId2];
			//floatUvArray[vtxCount * 2 + 1] = uArray[uvId2];

			vtxCount++;
			
			logger.debug(MString("Vertex count: ") + vtxCount + " maxId " + ((vtxCount - 1) * 3 + 2) + " ptArrayLen " + (numTriangles * 3 * 3));

			triangelVtxIdList[triCount * 3] = triCount * 3;
			triangelVtxIdList[triCount * 3 + 1] = triCount * 3 + 1;
			triangelVtxIdList[triCount * 3 + 2] = triCount * 3 + 2;

			triCount++;
		}		
	}

	return;

	ParamSet triParams = CreateParamSet();
	int numPointValues = numTriangles * 3 * 3;
	int numUvValues = numTriangles * 3 * 2;
	triParams->AddInt("indices", triangelVtxIdList, numTriangles * 3);
	triParams->AddPoint("P", floatPointArray, numPointValues);

	//triParams->AddVector("N", floatNormalArray, numPointValues);
	//triParams->AddFloat("uv",  floatUvArray, numUvValues);

	this->luxFile << "indices:\n";
	for( int i = 0; i < (numTriangles * 3); i++)
	{
		logger.debug(MString("id ") + i + " " + triangelVtxIdList[i]);
		int v = triangelVtxIdList[i];
		this->luxFile << "index " << i << " value " << v << "\n";
	}
	luxFile.flush();
	this->luxFile << "P:\n";
	for( int i = 0; i < (numPointValues); i++)
	{
		logger.debug(MString("p ") + i + " " + floatPointArray[i]);
		float v = floatPointArray[i];
		this->luxFile << "index " << i << " value " << v << "\n";
	}
	luxFile.flush();
	this->luxFile << "N:\n";
	for( int i = 0; i < (numPointValues); i++)
	{
		logger.debug(MString("n ") + i + " " + floatNormalArray[i]);
		float v = floatNormalArray[i];
		this->luxFile << "index " << i << " value " << v << "\n";
	}
	luxFile.flush();

	this->lux->objectBegin(meshFullName.asChar());
	this->lux->shape("trianglemesh", triParams.get());
	this->lux->objectEnd();

	return;

}

void LuxRenderer::defineGeometry()
{
	for( size_t i = 0; i < this->mtlu_scene->objectList.size(); i++)
	{
		mtlu_MayaObject *obj = (mtlu_MayaObject *)this->mtlu_scene->objectList[i];
		if( obj->visible )
		{
			if( obj->mobject.hasFn(MFn::kMesh))
			{
				this->defineTriangleMesh(obj);

				MMatrix tm = obj->dagPath.inclusiveMatrix();
				float fm[16];
				setZUp(tm, fm);

				this->lux->transformBegin();
				this->lux->transform(fm);
				//this->lux->objectInstance(obj->fullNiceName.asChar());
				this->lux->transformEnd();
			}
		}
	}
}
