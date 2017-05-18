// CRT
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// HE
#include <HAPI/HAPI.h>

// DEFINES

#define ENSURE_SUCCESS( result ) \
if ( (result) != HAPI_RESULT_SUCCESS ) \
{ \
    cout << "failure at " << __FILE__ << ":" << __LINE__ << endl; \
    cout << get_last_error() << endl; \
    exit( 1 ); \
}
#define ENSURE_COOK_SUCCESS( result ) \
if ( (result) != HAPI_STATE_READY ) \
{ \
    cout << "failure at " << __FILE__ << ":" << __LINE__ << endl; \
    cout << get_last_cook_error() << endl; \
    exit( 1 ); \
}

// SERVICES

string get_last_error()
{
	int buffer_length;
	HAPI_GetStatusStringBufLength(
		nullptr,
		HAPI_STATUS_CALL_RESULT, HAPI_STATUSVERBOSITY_ERRORS, &buffer_length);
	char * buf = new char[buffer_length];
	HAPI_GetStatusString(
		nullptr, HAPI_STATUS_CALL_RESULT, buf, buffer_length);
	string result(buf);
	delete[] buf;
	return result;
};

string get_last_cook_error()
{
	int buffer_length;
	HAPI_GetStatusStringBufLength(
		nullptr,
		HAPI_STATUS_COOK_RESULT, HAPI_STATUSVERBOSITY_ERRORS, &buffer_length);
	char * buf = new char[buffer_length];
	HAPI_GetStatusString(
		nullptr, HAPI_STATUS_CALL_RESULT, buf, buffer_length);
	string result(buf);
	delete[] buf;
	return result;
}

void wait_for_cook()
{
	int status;
	do
	{
		HAPI_GetStatus(nullptr, HAPI_STATUS_COOK_STATE, &status);
	}
	while (status > HAPI_STATE_MAX_READY_STATE);
	ENSURE_COOK_SUCCESS(status);
};

//////////////////////////////////////////////////////////////////////////

/*void dump_part_curves(HAPI_NodeId asset_id, HAPI_PartId part_id)
{
	cout << "part " << part_id << endl;
	HAPI_PartInfo part_info;
	ENSURE_SUCCESS(HAPI_GetPartInfo(nullptr,asset_id, part_id, &part_info));
	if (part_info.type == HAPI_PARTTYPE_CURVE)
	{
		HAPI_CurveInfo curve_info;
		ENSURE_SUCCESS(HAPI_GetCurveInfo(nullptr, asset_id, part_id, &curve_info));
		if (curve_info.curveType == HAPI_CURVETYPE_LINEAR) cout << "curve mesh type = Linear" << endl;
		else if (curve_info.curveType == HAPI_CURVETYPE_BEZIER) cout << "curve mesh type = Bezier" << endl;
		else if (curve_info.curveType == HAPI_CURVETYPE_NURBS) cout << "curve mesh type = Nurbs" << endl;
		else cout << "curve mesh type = Unknown" << endl;

		cout << "curve count: " << curve_info.curveCount << endl;
		int vertex_offset = 0;
		int knot_offset = 0;
		for (int i = 0; i < curve_info.curveCount; i++)
		{
			cout << "curve " << i + 1 << " of " << curve_info.curveCount << ":" << endl;
			// Number of CVs
			int num_vertices;
			HAPI_GetCurveCounts(nullptr,asset_id, part_id, &num_vertices, i, 1);
			cout << "num vertices: " << num_vertices << endl;
			// Order of this particular curve
			int order;
			if (curve_info.order != HAPI_CURVE_ORDER_VARYING
				&& curve_info.order != HAPI_CURVE_ORDER_INVALID)
				order = curve_info.order;
			else
				HAPI_GetCurveOrders(nullptr, asset_id, part_id,	&order, i, 1);
			cout << "curve order: " << order << endl;
			// If there's not enough vertices, then don't try to
			// create the curve.
			if (num_vertices < order)
			{
				cout << "not enought vertices on curve " << i << " of "
					<< curve_info.curveCount << ": skipping" << endl;
				// The curve at i will have numVertices vertices, and may have
				// some knots. The knot count will be numVertices + order for
				// nurbs curves.
				vertex_offset += num_vertices * 4;
				knot_offset += num_vertices + order;
				continue;
			}
			HAPI_AttributeInfo attr_info_p;
			HAPI_GetAttributeInfo(nullptr,asset_id, part_id,"P", HAPI_ATTROWNER_POINT, &attr_info_p);
			std::vector<float> p_array(attr_info_p.count * attr_info_p.tupleSize);
			HAPI_GetAttributeFloatData(nullptr,asset_id, part_id,"P", &attr_info_p,0,&p_array.front(), 0, attr_info_p.count);
			HAPI_AttributeInfo attr_info_pw;
			HAPI_GetAttributeInfo(nullptr,asset_id, part_id,"Pw", HAPI_ATTROWNER_POINT, &attr_info_pw);
			std::vector<float> pw_array(
				attr_info_pw.count * attr_info_pw.tupleSize);
			HAPI_GetAttributeFloatData(nullptr,asset_id,  part_id,"Pw", &attr_info_pw,0,&pw_array.front(), 0, attr_info_pw.count);
			for (int j = 0; j < num_vertices; j++)
			{
				cout << "cv " << j << ": " << p_array[j * 3 + 0] << ","
					<< p_array[j * 3 + 1] << ","
					<< p_array[j * 3 + 2] << ","
					<< pw_array[j] << endl;
			};
			if (curve_info.hasKnots)
			{
				std::vector< float > knots;
				knots.resize(num_vertices + order);
				HAPI_GetCurveKnots(nullptr,asset_id, part_id, &knots.front(),knot_offset, num_vertices + order);
				for (int j = 0; j < num_vertices + order; j++)
				{
					cout<< "knot " << j	<< ": " << knots[j] << endl;
				}
			}
			// NOTE: Periodicity is always constant, so periodic and
			// non-periodic curve meshes will have different parts.
			// The curve at i will have numVertices vertices, and may have
			// some knots. The knot count will be numVertices + order for
			// nurbs curves.
			vertex_offset += num_vertices * 4;
			knot_offset += num_vertices + order;
		}
	}
}*/

string get_string(HAPI_StringHandle string_handle)
{
	// A string handle of 0 means an invalid string handle -- similar to
	// a null pointer.  Since we can't return NULL, though, return an empty
	// string.
	if (string_handle == 0) return "";
	int buffer_length;
	ENSURE_SUCCESS(HAPI_GetStringBufLength(nullptr, string_handle, &buffer_length));
	char * buf = new char[buffer_length];
	ENSURE_SUCCESS(HAPI_GetString(nullptr, string_handle, buf, buffer_length));
	string result(buf);
	delete[] buf;
	return result;
};
/*void print_curves_info(HAPI_NodeId asset_id, const HAPI_AssetInfo &asset_info)
{
	HAPI_ObjectInfo * object_infos = new HAPI_ObjectInfo[asset_info.objectCount];
	ENSURE_SUCCESS(HAPI_GetComposedObjectList(nullptr, asset_id, object_infos, 0, asset_info.objectCount));
	for (int object_index = 0; object_index < asset_info.objectCount;++object_index)
	{
		HAPI_ObjectInfo &object_info = object_infos[object_index];
		for (int geo_index = 0; geo_index < object_info.geoCount;++geo_index)
		{
			HAPI_GeoInfo geo_info;
			ENSURE_SUCCESS(HAPI_GetGeoInfo(nullptr, asset_id, &geo_info));
			for (int part_index = 0; part_index < geo_info.partCount;++part_index)
			{
				dump_part_curves(asset_id, part_index);
			};
		};
	}
	delete[] object_infos;
};*/

/*int main(int argc, char ** argv)
{
	const char * otl_file = "C:/Users/joe/Documents/houdini16.0/otls/joeLib.hda";
	HAPI_CookOptions cook_options = HAPI_CookOptions_Create();
	ENSURE_SUCCESS(HAPI_Initialize(nullptr,&cook_options,true,-1,nullptr,nullptr,nullptr,nullptr,nullptr));
	int library_id;
	int asset_id;
	if (HAPI_LoadAssetLibraryFromFile(nullptr,otl_file,true,&library_id) != HAPI_RESULT_SUCCESS)
	{
		std::cout << "Could not load " << otl_file << std::endl;
		exit(1);
	}
	HAPI_StringHandle asset_name_sh;
	ENSURE_SUCCESS(HAPI_GetAvailableAssets(nullptr, library_id, &asset_name_sh, 1));
	std::string asset_name = get_string(asset_name_sh);

	HAPI_NodeId parent = -1; // ..TODO

	if (HAPI_CreateNode(nullptr,parent,asset_name.c_str(),"Object/dso",true,&asset_id) != HAPI_RESULT_SUCCESS)
	{
		std::cout << "Could not instantiate asset " << asset_name << std::endl;
		exit(1);
	}
	wait_for_cook();
	// Retrieve information about the asset.
	HAPI_AssetInfo asset_info;
	ENSURE_SUCCESS(HAPI_GetAssetInfo(nullptr, asset_id, &asset_info));
	// Print information about the curves contained inside the asset.
	print_curves_info(asset_id, asset_info);
	ENSURE_SUCCESS(HAPI_Cleanup(nullptr));
	return 0;
};*/

//////////////////////////////////////////////////////////////////////////////

/*int main(int argc, char ** argv)
{
	HAPI_CookOptions cook_options = HAPI_CookOptions_Create();
	ENSURE_SUCCESS(HAPI_Initialize(nullptr, &cook_options,true,-1,nullptr,nullptr,nullptr,nullptr,nullptr));

	HAPI_NodeId geoCreatorId;
	ENSURE_SUCCESS(HAPI_CreateInputNode(nullptr,&geoCreatorId,nullptr));

	ENSURE_SUCCESS(HAPI_CookNode(nullptr,geoCreatorId,nullptr));
	wait_for_cook();

	HAPI_PartInfo newPart = HAPI_PartInfo_Create();
	newPart.type = HAPI_PARTTYPE_MESH;
	newPart.vertexCount = 3;
	newPart.pointCount = 3;
	newPart.faceCount = 1;
	ENSURE_SUCCESS(HAPI_SetPartInfo(nullptr, geoCreatorId, 0, &newPart));
	HAPI_AttributeInfo pointInfo = HAPI_AttributeInfo_Create();
	pointInfo.count = 3; // 3 points
	pointInfo.tupleSize = 3; // 3 floats per point (x, y, z)
	pointInfo.exists = true;
	pointInfo.owner = HAPI_ATTROWNER_POINT;
	pointInfo.storage = HAPI_STORAGETYPE_FLOAT;
	ENSURE_SUCCESS(HAPI_AddAttribute(nullptr, geoCreatorId, 0, "P", &pointInfo));
	float positions[9] =
	{ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
	ENSURE_SUCCESS(HAPI_SetAttributeFloatData(nullptr, geoCreatorId, 0, "P", &pointInfo, positions, 0, 3));
	int vertices[3] = { 0, 1, 2 };
	ENSURE_SUCCESS(HAPI_SetVertexList(nullptr, geoCreatorId, 0, vertices, 0, 3));
	int face_counts[1] = { 3 }; // 3 edges for the first face (the only face)
	ENSURE_SUCCESS(HAPI_SetFaceCounts(nullptr, geoCreatorId, 0, face_counts, 0, 1));

	char ** strs = new char *[3];
	strs[0] = _strdup("str1");
	strs[1] = _strdup("str2");
	strs[2] = _strdup("str3");

	pointInfo.count = 3; // 3 points
	pointInfo.tupleSize = 1;
	pointInfo.exists = true;
	pointInfo.owner = HAPI_ATTROWNER_POINT;
	pointInfo.storage = HAPI_STORAGETYPE_STRING;
	ENSURE_SUCCESS(HAPI_AddAttribute(nullptr, geoCreatorId, 0, "strData", &pointInfo));
	HAPI_AttributeInfo attributeInfo;
	attributeInfo.exists = true;
	attributeInfo.owner = HAPI_ATTROWNER_POINT;
	attributeInfo.storage = HAPI_STORAGETYPE_STRING;
	attributeInfo.count = 3;
	attributeInfo.tupleSize = 1;
	ENSURE_SUCCESS(HAPI_SetAttributeStringData(nullptr, geoCreatorId, 0, "strData", &attributeInfo,(const char **)strs, 0, 3));
	ENSURE_SUCCESS(HAPI_CommitGeo(nullptr, geoCreatorId));
	ENSURE_SUCCESS(HAPI_SaveHIPFile(nullptr, "testoutput.hip", false));
	return 0;
}*/

/*Marshalling Point Clouds

For documentation on marshalling point clouds into Houdini, see Marshalling Point Clouds.

The following sample showscases marshalling of a point cloud into Houdini Engine :


int
main(int argc, char ** argv)
{
	HAPI_CookOptions cook_options = HAPI_CookOptions_Create();
	cook_options.maxVerticesPerPrimitive = 4;
	ENSURE_SUCCESS(HAPI_Initialize(
		nullptr, // session
		&cook_options,
		true, // use_cooking_thread
		-1, // cooking_thread_stack_size
		nullptr, // otl_search_path
		nullptr, // dso_search_path
		nullptr, // image_dso_search_path
		nullptr // audio_dso_search_path
	));

	HAPI_AssetId geoCreatorId;
	ENSURE_SUCCESS(HAPI_CreateInputAsset(nullptr, &geoCreatorId, nullptr));
	ENSURE_SUCCESS(HAPI_CookAsset(nullptr, geoCreatorId, nullptr));
	wait_for_cook();
	HAPI_PartInfo newPart = HAPI_PartInfo_Create();
	newPart.type = HAPI_PARTTYPE_MESH;
	newPart.vertexCount = 0;
	newPart.pointCount = 8;
	newPart.faceCount = 0;
	ENSURE_SUCCESS(HAPI_SetPartInfo(
		nullptr, geoCreatorId, 0, 0, &newPart));
	HAPI_AttributeInfo pointInfo = HAPI_AttributeInfo_Create();
	pointInfo.count = 8; // 8 points
	pointInfo.tupleSize = 3; // 3 floats per point (x, y, z)
	pointInfo.exists = true;
	pointInfo.owner = HAPI_ATTROWNER_POINT;
	pointInfo.storage = HAPI_STORAGETYPE_FLOAT;
	ENSURE_SUCCESS(HAPI_AddAttribute(
		nullptr, geoCreatorId, 0, 0, "P", &pointInfo));
	float positions[24] = {
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f
	};
	ENSURE_SUCCESS(HAPI_SetAttributeFloatData(
		nullptr, geoCreatorId, 0, 0, "P", &pointInfo, positions, 0, 8));
	ENSURE_SUCCESS(HAPI_CommitGeo(nullptr, geoCreatorId, 0, 0));

	ENSURE_SUCCESS(HAPI_SaveHIPFile(
		nullptr, "C:\\test\\testoutput.hip", false));
	return 0;
}
*/


/*Connecting Assets

For documentation on connecting assets, see Connecting Assets.

The sample below marshals a cube into Houdini Engine, then proceeds to connect that cube to the subdivde node in Houdini.Note that the subdivide node is a standard Houdini node, we did not need to first load its definition from file with HAPI_LoadAssetLibraryFromFile().The result is then dumped to a file so it can be viewed in Houdini :

int
main(int argc, char ** argv)
{
	HAPI_CookOptions cook_options = HAPI_CookOptions_Create();
	cook_options.maxVerticesPerPrimitive = 4;
	ENSURE_SUCCESS(HAPI_Initialize(
		nullptr, // session
		&cook_options,
		true, // use_cooking_thread
		-1, // cooking_thread_stack_size
		nullptr, // otl_search_path
		nullptr, // dso_search_path
		nullptr, // image_dso_search_path
		nullptr // audio_dso_search_path
	));

	HAPI_AssetId geoCreatorId;
	ENSURE_SUCCESS(HAPI_CreateInputAsset(nullptr, &geoCreatorId, NULL));
	ENSURE_SUCCESS(HAPI_CookAsset(nullptr, geoCreatorId, NULL));
	wait_for_cook();
	HAPI_PartInfo newPart = HAPI_PartInfo_Create();
	newPart.type = HAPI_PARTTYPE_MESH;
	newPart.vertexCount = 24;
	newPart.pointCount = 8;
	newPart.faceCount = 6;
	ENSURE_SUCCESS(HAPI_SetPartInfo(
		nullptr, geoCreatorId, 0, 0, &newPart));
	HAPI_AttributeInfo pointInfo = HAPI_AttributeInfo_Create();
	pointInfo.count = 8; // 8 points
	pointInfo.tupleSize = 3; // 3 floats per point (x, y, z)
	pointInfo.exists = true;
	pointInfo.owner = HAPI_ATTROWNER_POINT;
	pointInfo.storage = HAPI_STORAGETYPE_FLOAT;
	ENSURE_SUCCESS(HAPI_AddAttribute(
		nullptr, geoCreatorId, 0, 0, "P", &pointInfo));
	float positions[24] = {
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f
	};
	ENSURE_SUCCESS(HAPI_SetAttributeFloatData(
		nullptr, geoCreatorId, 0, 0, "P", &pointInfo, positions, 0, 8));
	int vertices[24] = {
		0, 2, 6, 4,
		2, 3, 7, 6,
		2, 0, 1, 3,
		1, 5, 7, 3,
		5, 4, 6, 7,
		0, 4, 5, 1,
	};
	ENSURE_SUCCESS(HAPI_SetVertexList(
		nullptr, geoCreatorId, 0, 0, vertices, 0, 24));
	int face_counts[6] = { 4, 4, 4, 4, 4, 4 }; // 4 verts for each face (quads)
	ENSURE_SUCCESS(HAPI_SetFaceCounts(
		nullptr, geoCreatorId, 0, 0, face_counts, 0, 6));
	ENSURE_SUCCESS(HAPI_CommitGeo(nullptr, geoCreatorId, 0, 0));
	HAPI_AssetId subdivideAssetId = -1;

	ENSURE_SUCCESS(HAPI_InstantiateAsset(
		nullptr, "Sop/subdivide", true, &subdivideAssetId));
	ENSURE_SUCCESS(HAPI_ConnectAssetGeometry(
		nullptr, geoCreatorId, 0, subdivideAssetId, 0));
	ENSURE_SUCCESS(HAPI_SaveHIPFile(
		nullptr, "C:\\test\\testoutput.hip", false));
	return 0;
}
*/


/*// DEF

#define NULL 0L


// CRT

#include <iostream>

using namespace std;

// HE

#include "HAPI/HAPI.h"*/

int main(int argc, void* argv)
{
	int res = 0;

	do
	{
		// CREATE SESSION

		HAPI_Session	session;
		HAPI_CreateInProcessSession(&session);

		// INITIALIZE

		HAPI_CookOptions cook_options = HAPI_CookOptions_Create();
		HAPI_Bool 	use_cooking_thread = false;
		int 	cooking_thread_stack_size = -1;

		HAPI_Initialize(&session, &cook_options, use_cooking_thread, cooking_thread_stack_size, NULL, NULL, NULL, NULL, NULL);

		// LOAD ASSET
		const char * 	file_path = "C:/Users/joe/Documents/houdini16.0/otls/jH1.hda";
		HAPI_AssetLibraryId 	library_id;

		HAPI_LoadAssetLibraryFromFile(&session, file_path, false, &library_id);

		HAPI_StringHandle asset_name_sh;
		HAPI_GetAvailableAssets(&session, library_id, &asset_name_sh, 1);

		string asset_name = get_string(asset_name_sh);
	
		HAPI_NodeId node_id;
		if (HAPI_CreateNode(&session, -1, asset_name.c_str(), NULL, false, &node_id) != HAPI_RESULT_SUCCESS)
		{
			res = -1;
			break;
		}



		// ...SET PARAMS
		//int value = 5;
		//HAPI_SetParmIntValue(&session, node_id, "amt", 0, value);

		//value = 1;
		//HAPI_SetParmIntValue(&session, node_id, "input", 0, value);

		HAPI_ParmInfo parm_info;
		HAPI_GetParmInfoFromName(&session, node_id, "file", &parm_info);

		const char* strvalue = "C:/Users/joe/Documents/geo/meta.bgeo";
		HAPI_SetParmStringValue(&session, node_id, strvalue, parm_info.id, 0);

		// COOK
		HAPI_CookNode(&session,node_id,	&cook_options);

		// QUERY

		HAPI_GeoInfo asset_info;
		HAPI_GetGeoInfo(&session, node_id, &asset_info);


		for (int i = 0;i < asset_info.partCount;i++)
		{
			HAPI_PartInfo part_info;
			HAPI_GetPartInfo(&session, node_id, i, &part_info);

			if (part_info.type == HAPI_PARTTYPE_CURVE)
			{
				HAPI_CurveInfo curve_info;
				HAPI_GetCurveInfo(&session, node_id, i, &curve_info);
				if (curve_info.curveType == HAPI_CURVETYPE_LINEAR) cout << "curve mesh type = Linear" << endl;
				else if (curve_info.curveType == HAPI_CURVETYPE_BEZIER) cout << "curve mesh type = Bezier" << endl;
				else if (curve_info.curveType == HAPI_CURVETYPE_NURBS) cout << "curve mesh type = Nurbs" << endl;
				else cout << "curve mesh type = Unknown" << endl;

				cout << "curve count: " << curve_info.curveCount << endl;

				continue;

				int vertex_offset = 0;
				int knot_offset = 0;
				for (int j = 0; j<curve_info.curveCount; j++)
				{
					//cout << "curve " << j+1 << " of " << curve_info.curveCount << ":" << endl;
					// Number of CVs
					int num_vertices;
					HAPI_GetCurveCounts(&session, node_id, i, &num_vertices, j, 1);
					//cout << "num vertices: " << num_vertices << endl;

					// Order of this particular curve
					int order;
					if (curve_info.order != HAPI_CURVE_ORDER_VARYING && curve_info.order != HAPI_CURVE_ORDER_INVALID)
						order = curve_info.order;
					else
						HAPI_GetCurveOrders(&session, node_id, i, &order, j, 1);
					//cout << "curve order: " << order << endl;
					// If there's not enough vertices, then don't try to
					// create the curve.
					if (num_vertices < order)
					{
						cout << "not enought vertices on curve " << j << " of "
							<< curve_info.curveCount << ": skipping" << endl;
						// The curve at i will have numVertices vertices, and may have
						// some knots. The knot count will be numVertices + order for
						// nurbs curves.
						vertex_offset += num_vertices * 4;
						knot_offset += num_vertices + order;
						continue;
					};
					HAPI_AttributeInfo attr_info_p;
					HAPI_GetAttributeInfo(&session, node_id, i, "P", HAPI_ATTROWNER_POINT, &attr_info_p);
					vector<float> p_array(attr_info_p.count * attr_info_p.tupleSize);
					HAPI_GetAttributeFloatData(&session, node_id, i, "P", &attr_info_p, 0, &p_array.front(), 0, attr_info_p.count);
					//HAPI_AttributeInfo attr_info_pw;
					//HAPI_GetAttributeInfo(&session, node_id, i, "Pw", HAPI_ATTROWNER_POINT, &attr_info_pw);
					//vector<float> pw_array(attr_info_pw.count * attr_info_pw.tupleSize);
					//HAPI_GetAttributeFloatData(&session, node_id, i, "Pw", &attr_info_pw, 0, &pw_array.front(), 0, attr_info_pw.count);
					for (int k = 0; k < num_vertices; k++)
					{
						//cout << "cv " << k << ": " << p_array[k * 3 + 0] << ","
						//	<< p_array[k * 3 + 1] << ","
						//	<< p_array[k * 3 + 2] << endl; // ","
						//	//<< pw_array[k] << endl;
					};
					if (curve_info.hasKnots)
					{
						vector< float > knots;
						knots.resize(num_vertices + order);
						HAPI_GetCurveKnots(&session, node_id, i, &knots.front(), knot_offset, num_vertices + order);
						for (int k = 0; k < num_vertices + order; k++)
						{
							//cout << "knot " << k << ": " << knots[k] << endl;
						};
					};
					// NOTE: Periodicity is always constant, so periodic and
					// non-periodic curve meshes will have different parts.
					// The curve at i will have numVertices vertices, and may have
					// some knots. The knot count will be numVertices + order for
					// nurbs curves.
					vertex_offset += num_vertices * 4;
					knot_offset += num_vertices + order;
				};
			};
		};


		// CLEANUP
		HAPI_Cleanup(&session);

	}
	while (false);

	return res;
};

//