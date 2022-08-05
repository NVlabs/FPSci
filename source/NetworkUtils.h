#pragma once
#include <G3D/G3D.h>

#define BATCH_UPDATE_COUNT_POSITION 1
class NetworkUtils
{
public:
	enum NetworkUpdateType {
		NOOP,
		REPLACE_FRAME,
	};
	static void updateEntity(shared_ptr<Entity> entity, BinaryInput &inBuffer);

	static void createFrameUpdate(GUniqueID id, shared_ptr<Entity> entity, BinaryOutput &outBuffer);
};

