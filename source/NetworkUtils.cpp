#include "NetworkUtils.h"
/*
static void updateEntity(Entity entity, BinaryInput inBuffer) {
	NetworkUtils::NetworkUpdateType type = (NetworkUtils::NetworkUpdateType)inBuffer.readUInt8();
}*/

void NetworkUtils::updateEntity(shared_ptr<Entity> entity, BinaryInput &inBuffer)
{
	NetworkUtils::NetworkUpdateType type = (NetworkUtils::NetworkUpdateType)inBuffer.readUInt8();
	if (type == NOOP) {
		return;
	}
	else if (type == NetworkUpdateType::REPLACE_FRAME) {
		CoordinateFrame frame;
		frame.deserialize(inBuffer);
		entity->setFrame(frame);
	}
}

void NetworkUtils::createFrameUpdate(GUniqueID id, shared_ptr<Entity> entity, BinaryOutput &outBuffer)
{
	outBuffer.writeBytes(&id, sizeof(id));
	outBuffer.writeUInt8(NetworkUpdateType::REPLACE_FRAME);
	entity->frame().serialize(outBuffer);
}
