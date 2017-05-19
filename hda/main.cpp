// CRT
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// HE
#include <HAPI/HAPI.h>

// PRMAN
#include <ri.hpp>
#include <rx.h>

#include <RixRIB.h>

// DEF
#ifdef LINUX

#define DLLEXPORT

#else

#define DLLEXPORT __declspec(dllexport)


#endif // LINUX

// SERVICES

string get_string(HAPI_StringHandle string_handle)
{
	// A string handle of 0 means an invalid string handle -- similar to
	// a null pointer.  Since we can't return NULL, though, return an empty
	// string.
	if (string_handle == 0) return "";
	int buffer_length;
	HAPI_GetStringBufLength(nullptr, string_handle, &buffer_length);
	char * buf = new char[buffer_length];
	HAPI_GetString(nullptr, string_handle, buf, buffer_length);
	string result(buf);
	delete[] buf;
	return result;
};

extern "C" {


// SUBD2
RtVoid DLLEXPORT Subdivide2(RtContextHandle _ctx, RtFloat detail, RtInt n, RtToken tk[], RtPointer vl[])
{
	cerr << "<<SUBDV" << endl;
	bool init = false;

	HAPI_Session session;
	HAPI_CookOptions cook_options = HAPI_CookOptions_Create();

	HAPI_NodeId node_id;

	for(int i=0;i<n;i++)
	{
		if (!strstr(tk[i], "hda")) continue;

		const char* otl = *(RtString*)vl[i];

		HAPI_CreateInProcessSession(&session);

		// INITIALIZE
		HAPI_Bool 	use_cooking_thread = false;
		int 	cooking_thread_stack_size = -1;

		HAPI_Initialize(&session, &cook_options, use_cooking_thread, cooking_thread_stack_size, NULL, NULL, NULL, NULL, NULL);

		// LOAD ASSET
		HAPI_AssetLibraryId 	library_id;
		HAPI_LoadAssetLibraryFromFile(&session, otl, false, &library_id);

		HAPI_StringHandle asset_name_sh;
		HAPI_GetAvailableAssets(&session, library_id, &asset_name_sh, 1);

		string asset_name = get_string(asset_name_sh);

		HAPI_Result res = HAPI_CreateNode(&session, -1, asset_name.c_str(), NULL, false, &node_id);

		if (res != HAPI_RESULT_SUCCESS)
		{
			break;
		}


		init = true;

		break;
	};

	//RixContext* ctx = (RixContext*)_ctx; // RxGetRixContext();
	RixContext* ctx = RxGetRixContext();

	if (!init)
	{
		cout << "NOT INIT!" << endl;
		HAPI_Cleanup(&session);
		return;
	};

	// SETUP PARAMS

	vector<string> TK;
	vector<int> VLI;

	for(int i = 0;i < n;i++)
	{
		if (strstr(tk[i], "hda")) continue;

		string clz = tk[i];

		TK.push_back(clz);
		VLI.push_back(i);
	};

	RixInterface* rixi = ctx->GetRixInterface(k_RixRenderState);

	RixRenderState* rxstate = reinterpret_cast<RixRenderState*>(rixi);
	RixRenderState::FrameInfo fi;
	rxstate->GetFrameInfo(&fi);
		
	int frame = fi.frame;

	HAPI_NodeInfo node_info;
	HAPI_GetNodeInfo(&session, node_id, &node_info);

	float shutter[2] = { frame + fi.shutter[0],frame + fi.shutter[1] };

	cout << "SHUTTER " << shutter[0] << ":" << shutter[1] << endl;

	for (int i = 0;i < node_info.parmCount;i++)
	{
		HAPI_ParmInfo pi;
		HAPI_GetParameters(&session, node_id, &pi,i,1);

		string name = get_string(pi.nameSH);

		bool found = false;
		int j;
		for(j = 0; j < TK.size(); j++)
		{
			vector<string> CLZ;

			string _TK(TK[j].c_str());

			char* pch = strtok((char*)_TK.c_str(), " ");

			while (pch != NULL)
			{
				CLZ.push_back(string(pch));
				pch = strtok(NULL, " ");
			};

			if (CLZ.size()!=2) continue;

			found = CLZ[1] == name;

			if (found) break;
		};

		if (!found) continue;

		if (pi.size != 1)
		{
			cout << "SKIP LARGE" << endl;
		};

			
		switch (pi.type)
		{
		case HAPI_PARMTYPE_INT:
		case HAPI_PARMTYPE_TOGGLE:
			{
				int VL = *(RtInt*)vl[VLI[j]];
				HAPI_SetParmIntValue(&session, node_id, name.c_str(), 0, VL);
			}
			break;

		case HAPI_PARMTYPE_FLOAT:
			{
				float VL = *(RtFloat*)vl[VLI[j]];
				HAPI_SetParmFloatValue(&session, node_id, name.c_str(), 0, VL);
			}
		//case HAPI_PARMTYPE_COLOR:
			break;

		case HAPI_PARMTYPE_STRING:
		case HAPI_PARMTYPE_PATH_FILE:
		case HAPI_PARMTYPE_PATH_FILE_GEO:
		case HAPI_PARMTYPE_PATH_FILE_IMAGE:
			{
				string VL = *(RtString*)vl[VLI[j]];
				HAPI_SetParmStringValue(&session, node_id, VL.c_str(), pi.id, 0);
			}				
			break;
		default:
			break;
		};

	};

	// ...ASSIGN FRAME
	bool motion = (shutter[0] != shutter[1]);

	if (motion) RiMotionBegin(2, fi.shutter[0], fi.shutter[1]);

	// COOK
	cook_options.splitGeosByGroup = false;
	cook_options.refineCurveToLinear = true;

	int last = (motion ? 1 : 0);

	for (int T = 0;T <= last;T++)
	{
		HAPI_SetTime(&session, shutter[T]);

		cout << "TIME : " << shutter[T] << endl;

		HAPI_CookNode(&session, node_id, &cook_options);

		// QUERY
		HAPI_GeoInfo geo_info;
		HAPI_GetGeoInfo(&session, node_id, &geo_info);

		// EMIT
		for (int i = 0;i < min(1,geo_info.partCount);i++)
		{
			HAPI_PartInfo part_info;
			HAPI_GetPartInfo(&session, node_id, i, &part_info);

			if (part_info.type != HAPI_PARTTYPE_CURVE) continue;

			HAPI_CurveInfo curve_info;
			HAPI_GetCurveInfo(&session, node_id, i, &curve_info);
			if (curve_info.curveType == HAPI_CURVETYPE_LINEAR) cout << "curve mesh type = Linear" << endl;
			else if (curve_info.curveType == HAPI_CURVETYPE_BEZIER) cout << "curve mesh type = Bezier" << endl;
			else if (curve_info.curveType == HAPI_CURVETYPE_NURBS) cout << "curve mesh type = Nurbs" << endl;
			else cout << "curve mesh type = Unknown" << endl;

			cout << "curve count: " << curve_info.curveCount << endl;

			int* nvertices = new int[curve_info.curveCount];

			HAPI_GetCurveCounts(&session, node_id, part_info.id, nvertices, 0, curve_info.curveCount);

			int pcnt = 0;

			for (int j = 0;j < curve_info.curveCount;j++) pcnt += nvertices[j];

			// P
			HAPI_AttributeInfo attr_info_p;
			HAPI_GetAttributeInfo(&session, node_id, part_info.id, "P", HAPI_ATTROWNER_POINT, &attr_info_p);
			vector<float> p_array(attr_info_p.count * attr_info_p.tupleSize);
			HAPI_GetAttributeFloatData(&session, node_id, part_info.id, "P", &attr_info_p, 0, &p_array.front(), 0, attr_info_p.count);

			float* P = p_array.data();

			// width
			HAPI_AttributeInfo attr_info_w;
			HAPI_GetAttributeInfo(&session, node_id, part_info.id, "width", HAPI_ATTROWNER_POINT, &attr_info_w);
			vector<float> w_array(attr_info_w.count);
			HAPI_GetAttributeFloatData(&session, node_id, part_info.id, "width", &attr_info_w, 0, &w_array.front(), 0, attr_info_w.count);

			float* width = w_array.data();

			RtToken nms[2] = { "vertex point P","vertex float width" };
			RtPointer vals[2] = { P, width };

			RiCurvesV("cubic", curve_info.curveCount, nvertices, "nonperiodic", 2, nms, vals);

			delete[] nvertices;;
		};

	};

	if (motion) RiMotionEnd();

	// CLEANUP
	HAPI_Cleanup(&session);

	cerr << "SUBDV>>" << endl;
};

// BOUND
RtVoid DLLEXPORT Bound(RtInt n, RtToken const tk[], RtPointer const vl[], RtBound result[2])
{
	cerr << "IN BOUND!!! " << endl;

	//result[0][0] = -10.0f; result[0][1] = -10.0f; result[0][2] = -10.0f;
	//result[0][3] = 10.0f; result[0][4] = 10.0f; result[0][5] = 10.0f;

	//result[1][0] = -10.0f; result[1][1] = -10.0f; result[1][2] = -10.0f;
	//result[1][3] = 10.0f; result[1][4] = 10.0f; result[1][5] = 10.0f;

	bool init = false;

	HAPI_Session session;
	HAPI_CookOptions cook_options = HAPI_CookOptions_Create();

	HAPI_NodeId node_id;

	for (int i = 0;i<n;i++)
	{
		if (!strstr(tk[i], "hda")) continue;

		const char* otl = *(RtString*)vl[i];

		HAPI_CreateInProcessSession(&session);

		// INITIALIZE
		HAPI_Bool 	use_cooking_thread = false;
		int 	cooking_thread_stack_size = -1;

		HAPI_Initialize(&session, &cook_options, use_cooking_thread, cooking_thread_stack_size, NULL, NULL, NULL, NULL, NULL);

		// LOAD ASSET
		HAPI_AssetLibraryId 	library_id;
		HAPI_LoadAssetLibraryFromFile(&session, otl, false, &library_id);

		HAPI_StringHandle asset_name_sh;
		HAPI_GetAvailableAssets(&session, library_id, &asset_name_sh, 1);

		string asset_name = get_string(asset_name_sh);

		if (HAPI_CreateNode(&session, -1, asset_name.c_str(), NULL, false, &node_id) != HAPI_RESULT_SUCCESS) break;


		init = true;

		break;
	};

	RixContext* ctx = RxGetRixContext();

	result[0][0] = 0.0f; result[0][1] = 0.0f; result[0][2] = 0.0f;
	result[0][3] = 0.0f; result[0][4] = 0.0f; result[0][5] = 0.0f;

	result[1][0] = 0.0f; result[1][1] = 0.0f; result[1][2] = 0.0f;
	result[1][3] = 0.0f; result[1][4] = 0.0f; result[1][5] = 0.0f;

	if (!init)
	{
		cout << "NOT INIT!" << endl;
		HAPI_Cleanup(&session);
		return;
	};


	// SETUP PARAMS

	vector<string> TK;
	vector<int> VLI;

	for (int i = 0;i < n;i++)
	{
		if (strstr(tk[i], "hda")) continue;

		string clz = tk[i];

		TK.push_back(clz);
		VLI.push_back(i);
	};

	RixInterface* rixi = ctx->GetRixInterface(k_RixRenderState);

	RixRenderState* rxstate = reinterpret_cast<RixRenderState*>(rixi);
	RixRenderState::FrameInfo fi;
	rxstate->GetFrameInfo(&fi);

	int frame = fi.frame;

	float shutter[2] = { frame+fi.shutter[0],frame+fi.shutter[1] };

	cout << "SHUTTER " << shutter[0] << ":" << shutter[1] << endl;

	HAPI_NodeInfo node_info;
	HAPI_GetNodeInfo(&session, node_id, &node_info);

	for (int i = 0;i < node_info.parmCount;i++)
	{
		HAPI_ParmInfo pi;
		HAPI_GetParameters(&session, node_id, &pi, i, 1);

		string name = get_string(pi.nameSH);

		bool found = false;
		int j;
		for (j = 0; j < TK.size(); j++)
		{
			vector<string> CLZ;

			string _TK(TK[j].c_str());

			char* pch = strtok((char*)_TK.c_str(), " ");

			while (pch != NULL)
			{
				CLZ.push_back(string(pch));
				pch = strtok(NULL, " ");
			};

			if (CLZ.size() != 2) continue;

			found = CLZ[1] == name;

			if (found) break;
		};

		if (!found) continue;

		if (pi.size != 1)
		{
			cout << "SKIP LARGE" << endl;
		};

		switch (pi.type)
		{
		case HAPI_PARMTYPE_INT:
		case HAPI_PARMTYPE_TOGGLE:
		{
			int VL = *(RtInt*)vl[VLI[j]];
			HAPI_SetParmIntValue(&session, node_id, name.c_str(), 0, VL);
		}
		break;

		case HAPI_PARMTYPE_FLOAT:
		{
			float VL = *(RtFloat*)vl[VLI[j]];
			HAPI_SetParmFloatValue(&session, node_id, name.c_str(), 0, VL);
		}
		//case HAPI_PARMTYPE_COLOR:
		break;

		case HAPI_PARMTYPE_STRING:
		case HAPI_PARMTYPE_PATH_FILE:
		case HAPI_PARMTYPE_PATH_FILE_GEO:
		case HAPI_PARMTYPE_PATH_FILE_IMAGE:
		{
			string VL = *(RtString*)vl[VLI[j]];
			HAPI_SetParmStringValue(&session, node_id, VL.c_str(), pi.id, 0);
		}
		break;
		default:
			break;
		};

	};

	// ASSIGN BOUND 
	bool assigned_bound = false;
	for (int i = 0;i < node_info.parmCount;i++)
	{
		HAPI_ParmInfo pi;
		HAPI_GetParameters(&session, node_id, &pi, i, 1);

		string name = get_string(pi.nameSH);

		if (name != "bound") continue;

		if (pi.type != HAPI_PARMTYPE_TOGGLE)
		{
			cout << "BOUND PARAM SHOULD BE A TOGGLE!" << endl;
			break;
		};

		HAPI_SetParmIntValue(&session, node_id, "bound", 0, 1);
		assigned_bound = true;
	};

	if (!assigned_bound)
	{
		cout << "LACKS BOUND-QUERYING TOGGLE!" << endl;
		HAPI_Cleanup(&session);
		return;
	};

#define BIG 1.0e20

	result[0][0] = BIG; result[0][1] = -BIG; result[0][2] = BIG;
	result[0][3] = -BIG; result[0][4] = BIG; result[0][5] = -BIG;

	result[1][0] = BIG; result[1][1] = -BIG; result[1][2] = BIG;
	result[1][3] = -BIG; result[1][4] = BIG; result[1][5] = -BIG;

	bool motion = (shutter[0] != shutter[1]);

	int last = (motion ? 1 : 0);

	for(int T = 0; T<=last; T++)
	{
		HAPI_SetTime(&session, shutter[T]);

		// COOK
		HAPI_CookNode(&session, node_id, &cook_options);

		// QUERY
		HAPI_GeoInfo geo_info;
		HAPI_GetGeoInfo(&session, node_id, &geo_info);

		// EMIT
		for (int i = 0;i < geo_info.partCount;i++)
		{
			HAPI_PartInfo part_info;
			HAPI_GetPartInfo(&session, node_id, i, &part_info);

			if (part_info.type != HAPI_PARTTYPE_MESH) continue;

			// P
			HAPI_AttributeInfo attr_info_p;
			HAPI_GetAttributeInfo(&session, node_id, part_info.id, "P", HAPI_ATTROWNER_POINT, &attr_info_p);
			vector<float> p_array(attr_info_p.count * attr_info_p.tupleSize);
			HAPI_GetAttributeFloatData(&session, node_id, part_info.id, "P", &attr_info_p, 0, &p_array.front(), 0, attr_info_p.count);

			for (int j=0;j < p_array.size() / 3;j++)
			{
				if (result[T][0] > p_array[j * 3]) result[T][0] = p_array[j * 3]; // +
				if (result[T][1] < p_array[j * 3]) result[T][1] = p_array[j * 3];
				if (result[T][2] > p_array[j * 3+1]) result[T][2] = p_array[j * 3+1];
				if (result[T][3] < p_array[j * 3+1]) result[T][3] = p_array[j * 3+1];
				if (result[T][4] > p_array[j * 3 + 2]) result[T][4] = p_array[j * 3 + 2];
				if (result[T][5] < p_array[j * 3 + 2]) result[T][5] = p_array[j * 3 + 2];
			};

		};

		if (!motion)
		{
			result[1][0] = result[0][0];result[1][1] = result[0][1];result[1][2] = result[0][2];
			result[1][3] = result[0][3];result[1][4] = result[0][4];result[1][5] = result[0][5];
		};

	};

	// CLEANUP
	HAPI_Cleanup(&session);

};

}