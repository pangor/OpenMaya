#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>

#include "mayatoCorona.h"
#include "mtco_common/mtco_renderGlobalsNode.h"

#define VENDOR "haggis vfx & animation"
#define VERSION "0.2"

MStatus initializePlugin( MObject obj )
{
	const MString	UserClassify( "shader/surface" );

	MGlobal::displayInfo(MString("Loading plugin MayaToCorona version: ") + MString(VERSION));
	MStatus   status;
	MFnPlugin plugin( obj, VENDOR, VERSION, "Any");

	status = plugin.registerCommand(MAYATOCMDNAME, MayaToCorona::creator, MayaToCorona::newSyntax );
	if (!status) {
		status.perror("cannot register command: mayatoCorona");
		return status;
	}

	MString command( "if( `window -exists createRenderNodeWindow` ) {refreshCreateRenderNodeWindow(\"" );
	command += UserClassify;
	command += "\");}\n";
	MGlobal::executeCommand( command );

	status = plugin.registerNode(MayaToCoronaGlobalsName, MayaToCoronaGlobals::id, MayaToCoronaGlobals::creator, MayaToCoronaGlobals::initialize );
	if (!status) {
		status.perror("cannot register node: MayaToCoronaGlobals");
		return status;
	}

	MString cmd = MString("import mtco_initialize as minit; minit.initRenderer()");
	MGlobal::displayInfo("try to register...");
	status = MGlobal::executePythonCommand(cmd, true, false);
	if(!status)
	{
		status.perror("Problem executing cmd: mtco_initialize.initRenderer()");
		return status;
	}

	return status;
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus   status;
	MFnPlugin plugin( obj );

	const MString UserClassify( "shader/surface" );
	
	std::cout << "deregister mtap cmd\n";
	status = plugin.deregisterCommand( MAYATOCMDNAME );
	if (!status) {
		status.perror("cannot deregister command: MayaToCoronaCmd");
		return status;
	}

	std::cout << "deregister mtap globals\n";
	status = plugin.deregisterNode( MayaToCoronaGlobals::id );
	if (!status) {
		status.perror("cannot deregister node: MayaToCoronaGlobals");
		return status;
	}
   
	//std::cout << "deregister mtCorona shader\n";
	//status = plugin.deregisterNode( mtco_surfaceShader::id);
	//if (!status) {
	//	status.perror("cannot deregister node: mtco_surfaceShader");
	//	return status;
	//}

	std::cout << "update mtco shader ui\n";
	MString command( "if( `window -exists createRenderNodeWindow` ) {refreshCreateRenderNodeWindow(\"" );
	command += UserClassify;
	command += "\");}\n";
	CHECK_MSTATUS( MGlobal::executeCommand( command ) );

	std::cout << "minit.unregister()\n";
	MString cmd = MString("import mtco_initialize as minit; minit.unregister()");
	status = MGlobal::executePythonCommand(cmd);
	if(!status)
	{
		status.perror("Problem executing cmd: mtco_initialize.unregister()");
		return status;
	}

	return status;
}