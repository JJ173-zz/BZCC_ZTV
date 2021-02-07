--[[ BZCC DLL Utils Helper Written by General BlackDragon --]]
local _DLLUtils = {}

function GetCheckedNetworkSvar(svar, listType)
	local svarStr = "network.session.svar" .. svar;
	local pContents = GetVarItemStr(svarStr);

	-- Error finding? Bail, asap
	if (pContents == nil or pContents == "") then
		return nil;
	end

	local count = GetNetworkListCount(listType);

	for i = 0, count do
		local pItem = GetNetworkListItem(listType, i);

		if (pItem ~= nil) then
			if (pContents == pItem) then
				-- Got a good match. Return it
				return pContents;
			end
		end
	end

	-- No match found. Report error
	return nil;
end

function IsRecycler(h)
	local ObjClass = GetClassLabel(h);

	if (ObjClass == nil) then
		return false;
	end

	return (ObjClass == "CLASS_RECYCLERVEHICLE") or (ObjClass == "CLASS_RECYCLER") or (ObjClass == "CLASS_RECYCLERVEHICLEH");
end

return _DLLUtils;