#include <common.h>


wxBitmapBundle GetAsset(wxString asset_name, wxSize asset_size) {
	return wxBitmapBundle::FromSVGFile(ASSETS_PATH + asset_name, asset_size);
}